# How Not to Use Standard Ranges and Views in C++

The C++20 ranges library replaces iterator pairs with composable pipelines, and nearly every sharp edge in it follows from a single fact:

> **A view is a lazy reference to data, not a container holding data.**

**Contents**

- Part I — Lifetime and ownership (§1–§5)
- Part II — Laziness and cost (§6–§9)
- Part III — Mutation (§10–§12)
- Part IV — Type-system and interop (§13–§17)
- Part V — Individual adaptors (§18–§23)
- Part VI — Toolchain availability
- Quick reference

---

## Part I — Lifetime and ownership

### 1. Treating a view as a snapshot

A pipeline stores no elements. It refers to the source and recomputes on every traversal, so a later change to the source changes what the view yields.

```cpp
std::vector<int> a{1, 2, 3};
auto doubled = a | rv::transform([](int x) { return x * 2; });

a[0] = 100;
for (int x : doubled) std::cout << x << " ";   // 200 4 6   — not 2 4 6
```

The variable is named as though it holds results, but nothing was computed where it was declared. This is the root misconception behind §2, §5, §9 and §11 as well.

**Solution — materialise when a snapshot is wanted.** C++23 spells this `std::ranges::to`; libstdc++ ships it in GCC 14, so on GCC 13 the portable form is an explicit copy:

```cpp
std::vector<int> snap;
std::ranges::copy(a | rv::transform([](int x) { return x * 2; }), std::back_inserter(snap));

// C++23, GCC 14+ / recent libc++:
// auto snap = a | rv::transform([](int x){ return x * 2; }) | std::ranges::to<std::vector>();
```

Keep the view when the result is consumed once, immediately, in the same scope. Materialise when it outlives the source, is traversed more than once, or must be stable.

### 2. Assuming every view over a temporary dangles

This warning is widely repeated and is *wrong* for the case most often cited. Piping an **rvalue container** moves it into an `owning_view`, which keeps it alive for as long as the pipeline lives:

```cpp
auto owning = std::vector<int>{1, 2, 3, 4} | rv::filter([](int x) { return x % 2 == 0; });
for (int x : owning) std::cout << x << " ";    // 2 4   — safe; the vector was moved in
```

The genuinely broken case is a view over a **named local**, which is captured by `ref_view` and extends nothing:

```cpp
auto make_bad() {
    std::vector<int> local{1, 2, 3, 4};
    return local | rv::filter([](int x) { return x % 2 == 0; });   // ref_view over `local`
}                                                                   // `local` dies here
auto v = make_bad();
for (int x : v) /* ... */ ;      // AddressSanitizer: stack-use-after-return
```

The distinction is not "temporary vs named" but **what the pipeline was handed**: an rvalue container is *owned*, a named lvalue is merely *referenced*.

**Solution — return an owning result, or move the container into the pipeline.**

```cpp
std::vector<int> make_good() {
    std::vector<int> local{1, 2, 3, 4};
    std::vector<int> out;
    std::ranges::copy(local | rv::filter([](int x) { return x % 2 == 0; }),
                      std::back_inserter(out));
    return out;                                        // owns its data
}

auto owning2 = std::move(local) | rv::filter(/* ... */);   // or keep it lazy, by moving
```

### 3. Ignoring `std::ranges::dangling`

Range algorithms given an rvalue range refuse to return an iterator into it. The result type becomes `std::ranges::dangling`, which fails at the point of use rather than dangling silently:

```cpp
std::vector<int> make() { return {1, 2, 3}; }

auto it = std::ranges::find(make(), 2);
std::cout << *it;
// error: no match for 'operator*' (operand type is 'std::ranges::dangling')
```

This is the library working as designed — a compile error instead of undefined behaviour. The mistake is reading it as a defect and casting it away.

**Solution — name the range so it outlives the iterator.**

```cpp
auto data = make();
auto it = std::ranges::find(data, 2);      // a real iterator into `data`
```

Types that are safe to return iterators from — `std::string_view`, `std::span`, `subrange` —
opt in via `enable_borrowed_range` and never produce `dangling`.

### 4. Capturing a local by reference in a pipeline's callable

A view stores its predicate or transform by value, but a *reference capture inside* that callable is not protected by anything. The view outliving the captured object is a dangling read on every element:

```cpp
auto make = [&data] {
    int threshold = 3;                                  // local to this lambda
    return data | rv::filter([&threshold](int x) { return x > threshold; });
};
auto piped = make();                                    // `threshold` is gone
for (int x : piped) /* ... */ ;                         // ASan: stack-use-after-return
```

The same applies to a transform whose function returns a reference to something it created:

```cpp
auto bad = v | rv::transform([](const std::string& s) -> const std::string& {
    std::string tmp = s + "!";
    return tmp;                    // warning: reference to local variable 'tmp' returned
});                                // ASan: SEGV on dereference
```

**Solution — capture by value in anything a view stores, and return by value from a transform.**

```cpp
return data | rv::filter([threshold](int x) { return x > threshold; });   // by value
auto good = v | rv::transform([](const std::string& s) { return s + "!"; });  // returns a value
```

### 5. Modifying the source while a view is alive

Views hold iterators or pointers into the source, and several adaptors additionally cache the position of their first element. Mutating the container underneath produces silently wrong
results:

```cpp
std::vector<int> v{1, 2, 3};
auto evens = v | rv::filter([](int x) { return x % 2 == 0; });

for (int x : evens) std::cout << x << " ";     // pass 1: 2
v.insert(v.begin(), 2);                         // v is now {2, 1, 2, 3}
for (int x : evens) std::cout << x << " ";     // pass 2: 1 2      <-- correct answer is 2 2
```

Pass 2 reports the odd number `1` from a filter that keeps only even values. `filter_view` cached "the first survivor is at offset 1"; the insert shifted everything right, so that offset now names a different element. Note this is not a memory error — ASan and UBSan report nothing — it is a wrong answer with no diagnostic anywhere.

Implementation detail worth knowing: libstdc++ caches an *offset* for random-access sources and an *iterator* otherwise, so on a `std::list` the same pattern can dangle outright rather than merely mislead.

**Solution — treat a live view as read-only with respect to its source.** Finish with the view, mutate, then rebuild:

```cpp
std::vector<int> out;
std::ranges::copy(v | rv::filter(is_even), std::back_inserter(out));   // done with the view
v.insert(v.begin(), 2);
auto evens2 = v | rv::filter(is_even);                                  // fresh view, fresh cache
```

---

## Part II — Laziness and cost

### 6. Ordering `transform` before `filter`

Composition order changes cost, sometimes dramatically. A `filter` downstream of a `transform` must evaluate the transform once to test the predicate and again to yield the element:

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
int calls = 0;
auto expensive = [&calls](int x) { ++calls; return x * x; };

for (int x : v | rv::transform(expensive)
                | rv::filter([](int y) { return y % 2 == 0; })) (void)x;
// 4 elements yielded, calls == 12
```

**Solution — filter on the cheap input first, when the predicate permits.** Here `x*x` is even exactly when `x` is even, so the predicate can test the original value:

```cpp
for (int x : v | rv::filter([](int y) { return y % 2 == 0; })
                | rv::transform(expensive)) (void)x;
// same 4 elements, calls == 4
```

Same answer, a third of the work. When the predicate genuinely needs the transformed value, materialise the transform once rather than filtering over it repeatedly.

### 7. Putting impure functions in a lazy pipeline

Because views recompute, a function with side effects or internal state produces different results depending on how many times each element is visited — and §6 shows that count is not obvious from reading the pipeline.

```cpp
int n = 0;
auto bad = v | rv::transform([&n](int x) { return x + n++; });   // depends on call count
```

Two traversals of `bad` yield different sequences, and an adaptor that probes an element twice corrupts the result outright. Counters, logging, caching, and lazy initialisation inside a `transform` or `filter` all fall into this trap.

**Solution — keep pipeline callables pure.** Where a side effect is genuinely required, put it in a terminal loop or `std::ranges::for_each`, never inside a view:

```cpp
std::ranges::for_each(v | rv::filter(pred), [&](int x) { log(x); });   // effect at the end
```

### 8. Expecting a view to remember its work

A view memoises nothing except, in a few adaptors, the position of `begin()`. Traversing twice re-runs every predicate and every transform:

```cpp
std::vector<int> v{1, 2, 3, 4};
int preds = 0;
auto big = v | rv::filter([&preds](int x) { ++preds; return x > 2; });

for (int x : big) (void)x;      // preds == 4   (predicate ran on all four elements)
for (int x : big) (void)x;      // preds == 6   (begin() cached; the rest re-ran)
```

Note also that the *first* `begin()` on a `filter_view` is **O(n)**: it must scan for the first element that satisfies the predicate. A pipeline whose `begin()` is taken repeatedly pays this each time.

**Solution — materialise anything traversed more than once**, exactly as in §1.

### 9. Rebuilding a pipeline inside a loop

A fresh pipeline has a fresh (empty) cache, so re-creating one per iteration discards all the work of the previous pass:

```cpp
std::vector<int> v(1000, 1); v[999] = 9;
int preds = 0;
auto pred = [&preds](int x) { ++preds; return x > 5; };

for (int k = 0; k < 3; ++k) { auto f = v | rv::filter(pred); (void)*f.begin(); }
// preds == 3000   — the O(n) scan for the first survivor runs three times

auto f = v | rv::filter(pred);
for (int k = 0; k < 3; ++k) (void)*f.begin();
// preds == 1000   — one scan, then the cached position
```

**Solution — hoist the pipeline out of the loop**, or materialise it once before looping. Building a view is cheap; *starting* one is not always cheap.

---

## Part III — Mutation

### 10. Writing through `filter_view` so the predicate stops holding

Elements can be modified through a `filter_view`, but writing a value that no longer satisfies the predicate is explicitly undefined behaviour. It is worth seeing, because it can look harmless:

```cpp
std::vector<int> v{1, 2, 3, 4};
auto evens = v | rv::filter([](int x) { return x % 2 == 0; });

for (int x : evens) std::cout << x << " ";     // 2 4
v[1] = 7;                                       // the cached first survivor stops being even
for (int x : evens) std::cout << x << " ";     // 7 4   <-- an odd number from an "evens" view
```

This is the same cached-position mechanism as §5, reached by writing rather than inserting.

**Solution — do not modify elements through, or underneath, a live `filter_view`.** A plain loop is clearer and has no hidden state:

```cpp
for (int& x : v) if (x % 2 == 0) x += 1;                   // no view, no cache
auto evens2 = v | rv::filter([](int x){ return x % 2 == 0; });   // rebuild afterwards
```

### 11. Binding `auto&` to a `transform_view` element

A `transform` that returns by value yields prvalues, so there is no lvalue to bind:

```cpp
for (auto& x : v | rv::transform([](int y) { return y * 2; })) x = 0;
// error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'
```

**Solution — use `auto&&` (or `auto`) when the element type is not known to be a reference.**

```cpp
for (auto&& x : v | rv::transform([](int y) { return y * 2; })) std::cout << x;
```

`auto&&` is the correct default for range-`for` over any pipeline: it binds to references where they exist and to prvalues where they do not. Writing *through* a transform requires the function itself to return a reference, which reintroduces the lifetime hazard of §4.

### 12. Expecting to write through a const source or `views::as_const`

Constness propagates through a pipeline, and the diagnostics arrive far from the cause:

```cpp
const std::vector<int> v{1, 2, 3};
for (auto& x : v | rv::filter([](int y) { return y % 2; })) x = 0;
// error: assignment of read-only reference 'x'

auto c = v | rv::as_const;                                   // C++23
std::ranges::fill(c, 0);
// error: no match for call to '(const std::ranges::__fill_fn)'
```

**Solution — decide deliberately whether a pipeline is a read or a write path.** Use a non-const source for mutation; use `as_const` precisely when a pipeline should be *prevented* from writing — that is the feature, not an obstacle.

---

## Part IV — Type-system and interop

### 13. Passing a view as `const&`

The caching in §5 and §10 is why `filter_view::begin()` is non-`const`, which makes the whole view non-const-iterable. A generic helper taking `const auto&` therefore fails to compile:

```cpp
void print(const auto& r) { for (auto x : r) std::cout << x; }

auto f = v | rv::filter([](int x) { return x % 2; });
print(f);
// error: passing 'const std::ranges::filter_view<...>' as 'this' argument discards qualifiers
```

`drop_while_view`, `split_view`, and several others behave identically.

**Solution — take views by value or by forwarding reference.** Views are cheap to copy by design,
so neither is a pessimisation:

```cpp
void print(std::ranges::input_range auto&& r) { for (auto&& x : r) std::cout << x; }
// or: template <std::ranges::view V> void print(V r) { ... }
```

### 14. Sorting a view that cannot be sorted

Two distinct failures share one symptom — a long error naming `__sort_fn`:

```cpp
auto f = v | rv::filter([](int x) { return x > 1; });
std::ranges::sort(f);        // error: no match for call to '(const std::ranges::__sort_fn)'
```

`filter_view` is at best **bidirectional** — locating the *n*th survivor requires testing every element in between — while `sort` requires random access.

```cpp
auto t = v | rv::transform([](int x) { return x * 2; });
std::ranges::sort(t);        // error: no match for call to '(const std::ranges::__sort_fn)'
```

`transform_view` yields computed prvalues, so there is nothing to write back into: readable, never writable (§11).

**Solution — sort the container, or materialise first.**

```cpp
std::ranges::sort(v);                                // sort the real storage

std::vector<int> tmp;                                 // or take a snapshot and sort that
std::ranges::copy(v | rv::transform(f), std::back_inserter(tmp));
std::ranges::sort(tmp);
```

### 15. Calling `size()` on a pipeline that has no size

A filtered range cannot report its length without running the predicate over everything, so it is deliberately not a `sized_range`:

```cpp
auto f = v | rv::filter([](int x) { return x % 2; });
std::cout << std::ranges::size(f);
// error: no match for call to '(const std::ranges::__cust_access::_Size)'
```

**Solution — use `std::ranges::distance` when an O(n) count is acceptable**, or materialise:

```cpp
auto n = std::ranges::distance(f);        // O(n), and says so at the call site
```

The same applies to `view_interface`'s `operator[]`, `back()`, and `empty()` in some cases: they exist only when the underlying iterators can support them.

### 16. Feeding a view to a pre-C++20 algorithm

Classic algorithms require `begin()` and `end()` to be the *same type*. Many views end with a sentinel instead, so the call does not match:

```cpp
auto tw = v | rv::take_while([](int x) { return x > 0; });
std::accumulate(tw.begin(), tw.end(), 0);
// error: no matching function for call to 'accumulate(__normal_iterator<...>, take_while_view<...>::_Sentinel<true>, int)'
```

**Solution — adapt with `rv::common`, or prefer the `std::ranges::` algorithm.**

```cpp
auto c = v | rv::take_while([](int x) { return x > 0; }) | rv::common;
std::accumulate(c.begin(), c.end(), 0);              // 10
```

Note what `common` does and does not do: it makes the two types agree, but it does **not** upgrade the iterator category. `std::sort` on a `common`-adapted `take_while_view` still fails, because the iterators remain non-random-access. Where a `std::ranges::` equivalent exists, it accepts the sentinel form directly and is the better choice.

### 17. Assuming an algorithm's result indexes the container

An algorithm run over a view returns an iterator **into the view**, whose position bears no relation to the position in the underlying container:

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6};
auto f = v | rv::filter([](int x) { return x % 2 == 0; });
auto it = std::ranges::find(f, 4);

std::ranges::distance(f.begin(), it);        // 1   — second element of the filtered view
std::ranges::distance(v.begin(), it.base()); // 3   — fourth element of the container
```

Using the first number to index `v` silently reads the wrong element.

**Solution — call `.base()` to recover the underlying iterator** before measuring against the container, as above. (`filter_view::iterator`, `transform_view::iterator`, and the other adaptor iterators all provide it.)

---

## Part V — Individual adaptors

### 18. Expecting `split` to produce strings

`rv::split` yields **subranges over the original characters**, not `std::string` objects. There is no `operator<<` for them and no implicit conversion:

```cpp
std::string text = "alpha,beta,gamma";
for (auto part : text | rv::split(',')) {
    std::string piece(part.begin(), part.end());     // explicit materialisation
    std::cout << "[" << piece << "]";                 // [alpha][beta][gamma]
}
```

**Solution — construct explicitly**, as above; use `std::string_view(part.begin(), part.end())` to avoid the copy for contiguous input, or `part | std::ranges::to<std::string>()` in C++23
(GCC 14+). Note also that C++20's `split_view` differs from `lazy_split_view`: the former requires a forward range and preserves it, the latter works on input ranges but yields non-common inner ranges.

### 19. Traversing an input range twice

Views over input streams are single-pass. A second traversal silently yields nothing, because the first consumed the stream:

```cpp
std::istringstream in("1 2 3");
auto nums = std::ranges::istream_view<int>(in);

for (int x : nums) { /* 3 elements */ }
for (int x : nums) { /* 0 elements */ }     // exhausted, no error, no diagnostic
```

**Solution — materialise once and reuse the result**, or re-create the stream for a second pass. Any pipeline built on an input range is consumable, not repeatable — and the compiler will not warn.

### 20. Building an unbounded pipeline without a bound

`rv::iota(0)` is infinite. Traversing it, sizing it, or materialising it does not terminate:

```cpp
for (int x : rv::iota(0)) std::cout << x;          // never returns
```

**Solution — bound it before consuming**, with `take`, `take_while`, or a two-argument `iota`:

```cpp
for (int x : rv::iota(0) | rv::take(5)) std::cout << x << " ";     // 0 1 2 3 4
for (int x : rv::iota(0, 5))            std::cout << x << " ";     // 0 1 2 3 4
```

Adaptors that must reach the end — `reverse`, `size`, sorting — can never apply to an unbounded range, so the bound belongs as early in the pipeline as the logic allows.

### 21. Confusing `take`/`drop` with `counted`

`take` and `drop` **clamp** to the available length, which is often assumed to be an error and is
not:

```cpp
std::vector<int> v{1, 2, 3};
std::ranges::distance(v | rv::take(99));    // 3   — clamped, not UB
std::ranges::distance(v | rv::drop(99));    // 0   — clamped, not UB
```

`views::counted(it, n)` is the dangerous sibling: it promises that `n` elements exist from `it` onward and performs no checking, so an over-long count is undefined behaviour.

**Solution — prefer `take`/`drop` on a range**; reserve `counted` for cases where the count is known correct, and derive it from the range rather than assuming it.

### 22. Assuming `zip` pads to the longest range (C++23)

`rv::zip` stops at the **shortest** input. Extra elements are dropped with no diagnostic:

```cpp
std::vector<int>  a{1, 2, 3, 4};
std::vector<char> b{'x', 'y'};

for (auto [i, c] : rv::zip(a, b)) std::cout << i << c << " ";   // 1x 2y
std::ranges::distance(rv::zip(a, b));                            // 2, not 4
```

**Solution — check the lengths explicitly** when the ranges are expected to match:

```cpp
assert(std::ranges::size(a) == std::ranges::size(b));
```

Silent truncation is the intended semantic, so a mismatch is a bug in the caller's data, not something `zip` will report.

### 23. Using `auto` with `zip` and `enumerate` element access (C++23)

These views yield tuples of *references*. Binding with plain `auto` copies, so writes are lost:

```cpp
std::vector<std::string> s{"a", "b"};

for (auto [i, val] : rv::enumerate(s)) val += "!";     // modifies copies; `s` is unchanged
for (auto&& [i, val] : rv::enumerate(s)) val += "!";   // modifies `s`: a! b!
```

**Solution — use `auto&&` for structured bindings over any pipeline**, for the same reason as §11.

---

## Part VI — Toolchain availability

Feature-test macros are the reliable way to check what a given library provides. Measured on **g++ 13.3 / libstdc++** with `-std=c++23`:

| Facility | Macro | g++ 13.3 |
|---|---|---|
| `views::zip` | `__cpp_lib_ranges_zip` | 202110 |
| `views::enumerate` | `__cpp_lib_ranges_enumerate` | 202302 |
| `views::chunk` | `__cpp_lib_ranges_chunk` | 202202 |
| `views::slide` | `__cpp_lib_ranges_slide` | 202202 |
| `views::stride` | `__cpp_lib_ranges_stride` | 202207 |
| `views::cartesian_product` | `__cpp_lib_ranges_cartesian_product` | 202207 |
| `views::as_const` | `__cpp_lib_ranges_as_const` | 202207 |
| `std::ranges::to` | `__cpp_lib_ranges_to_container` | **absent** (GCC 14+) |

The common trap is assuming `std::ranges::to` accompanies the C++23 views, since most of them landed a release earlier. Guard portable code:

```cpp
#ifdef __cpp_lib_ranges_to_container
    auto out = pipeline | std::ranges::to<std::vector>();
#else
    std::vector<int> out;
    std::ranges::copy(pipeline, std::back_inserter(out));
#endif
```

---

# Quick reference

| # | Mistake | Consequence | Fix |
|---|---|---|---|
| 1 | Storing a pipeline as if it were a result | Recomputes; tracks later source edits | Materialise (`ranges::copy` / `ranges::to`) |
| 2 | Returning a view over a named local | Dangling (`stack-use-after-return`) | Return a container, or `std::move` the source in |
| 3 | Working around `std::ranges::dangling` | Undefined behaviour | Name the range first |
| 4 | Reference capture in a stored callable | Dangling read per element | Capture by value; return by value |
| 5 | Mutating the source under a live view | Silently wrong results, no sanitizer hit | Finish with the view, then rebuild |
| 6 | `transform` before `filter` | Transform runs ~3× more often | Filter the cheap input first |
| 7 | Stateful callable in a view | Results depend on traversal count | Keep pipeline callables pure |
| 8 | Traversing a view repeatedly | Silent recomputation; O(n) first `begin()` | Materialise |
| 9 | Rebuilding a pipeline in a loop | Cache discarded each iteration (3000 vs 1000) | Hoist the pipeline out |
| 10 | Writing through `filter_view` off-predicate | UB; stale cached position | Plain loop; rebuild the view |
| 11 | `auto&` over a `transform_view` | Compile error (prvalue) | Use `auto&&` |
| 12 | Writing through a const source / `as_const` | Compile error | Choose read vs write path deliberately |
| 13 | Passing a view as `const&` | Compile error (non-const `begin()`) | Take by value or `auto&&` |
| 14 | `sort` on `filter_view` / `transform_view` | Compile error | Sort the container, or materialise |
| 15 | `size()` on a filtered pipeline | Compile error (not a `sized_range`) | `ranges::distance`, or materialise |
| 16 | View into a pre-C++20 algorithm | Compile error (sentinel ≠ iterator) | `rv::common`, or the `ranges::` algorithm |
| 17 | Using a view iterator to index the container | Reads the wrong element | `.base()` |
| 18 | Treating `split` pieces as strings | Compile error | Construct `string`/`string_view` explicitly |
| 19 | Traversing an input range twice | Second pass empty, no diagnostic | Materialise once |
| 20 | Unbounded range without a bound | Hangs | `take` / `take_while` / bounded `iota` |
| 21 | `counted` with an over-long count | Undefined behaviour | Prefer `take`/`drop`, which clamp |
| 22 | Assuming `zip` pads | Silent truncation to the shortest | Assert equal lengths |
| 23 | `auto` structured bindings over `zip`/`enumerate` | Writes lost to copies | `auto&&` |

---

### The single habit that prevents most of this

At every step of a pipeline, it is best to decide explicitly whether a **lazy reference** or an **owned result** is wanted. Views are excellent at the former and are not the latter, and the library issues no diagnostic when one is mistaken for the other — the failures show up as wrong answers, as sanitizer reports, or as template errors far from the cause.

Three practical rules cover the majority of cases:

1. Materialise at boundaries.
2. Keep pipeline callables pure and capture by value.
3. Use `auto&&` in range-`for` and take views by value in interfaces.
