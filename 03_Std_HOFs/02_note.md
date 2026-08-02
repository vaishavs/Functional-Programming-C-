# Common Misuses: How *Not* to Use Standard HOFs, Ranges, and Views

Every construct in the notes above has a companion way to get it wrong. Some fail loudly at compile time, some fail silently at runtime, and a few compile clean, run clean, and just give the wrong answer. This page catalogs the ones worth knowing on purpose, organized the same three ways as the notes above — the classical HOF/iterator-pair model, the constrained ranges algorithms and sentinels, then views — using the same aliases (`namespace rg = std::ranges;` `namespace rv = std::ranges::views;`). Each entry pairs the mistake with the fix and an explanation of the mechanism behind that fix; a quick-reference table sits at the end.

## 1. Misusing classical HOFs & the iterator-pair model

### 1.1 Mixing iterators from two different containers

```cpp
std::vector<int> a{1, 2, 3};
std::vector<int> b{10, 20, 30};
auto it = std::find(a.begin(), b.end(), 20);   // undefined behaviour
```

Both iterators happen to share a type, so this compiles without complaint. `a.begin()` and `b.end()` don't delimit a real sequence, though — there's no rule connecting one container's start to a different container's end — so the standard leaves the result undefined. Nothing catches this at compile time; it may look fine on one run and corrupt memory on the next.

```cpp
auto it = std::find(b.begin(), b.end(), 20);   // one container, start to finish
```

**Why it works:** An iterator pair is not two independent iterators — it is a single object's promise that repeatedly incrementing `first` reaches `last` in a bounded number of steps. That promise only holds when both come from the same underlying sequence; nothing in an iterator's type encodes which container produced it, so there is no way for the type system to reject the mismatch. The discipline that actually prevents this bug is naming a single container per algorithm call and reading both arguments off the same object, rather than assembling the pair from separately-computed expressions.

### 1.2 Handing an algorithm the wrong iterator category

```cpp
std::list<int> lst{5, 3, 9, 1};
std::sort(lst.begin(), lst.end());
```

`std::sort` requires random-access iterators; `std::list` only offers bidirectional ones. The call still compiles at the line you wrote — the failure surfaces deep inside `<algorithm>`'s implementation, in an error message full of names you never typed.

```cpp
lst.sort();   // std::list has its own member sort() for exactly this reason
```

**Why it works:** `std::list<T>` stores nodes as a doubly linked list, so nothing about its memory layout ever allows an iterator to jump `k` positions in O(1) — the requirement `std::sort` depends on for its partitioning and swapping is structurally unmeetable, not merely unimplemented. The library's response is to give the container its own `sort()`, implemented by relinking nodes rather than swapping through random access, meeting the same O(n log n) guarantee without ever needing random access. The general lesson: when `<algorithm>` rejects a container, checking whether the container ships a member function of the same name is worth doing first — `list::merge`, `list::remove`, and `list::unique` exist for this same structural reason.

### 1.3 Seeding `accumulate` with the wrong type

Same algorithm as the Workflow example above, different data:

```cpp
std::vector<double> prices{19.99, 5.49, 3.33};
double total = std::accumulate(prices.begin(), prices.end(), 0);
// total is 27.0, not 28.81
```

`accumulate`'s return type is deduced from the seed, not from the container. `0` is an `int`, so the running total is computed in `int`: every intermediate `double` sum gets truncated back to `int` before the next addition happens. The final implicit conversion to `double` on assignment doesn't recover what was already lost — it just makes the wrong number look plausible.

```cpp
double total = std::accumulate(prices.begin(), prices.end(), 0.0);   // 28.81
```

**Why it works:** `accumulate`'s return type and the type of its running total are both deduced from the initial value's type, not from the range's `value_type`. Passing `0.0` makes every intermediate sum a `double`, so each addition happens at full precision before the next one is folded in. Passing `0` commits every intermediate sum to `int` arithmetic instead — each `double` element is truncated on the way in, and that loss is permanent by the time the final `int`-to-`double` conversion happens on return. The pattern generalizes beyond this one algorithm: whenever a seed's type differs from a range's element type, the seed's type is what the whole computation runs in.

### 1.4 A comparator that isn't a *strict* weak ordering

```cpp
std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};
std::sort(v.begin(), v.end(), [](int a, int b) { return a <= b; });
```

`sort` requires `comp(x, x) == false` for every `x`; `<=` fails that immediately. The result is undefined behaviour — usually a wrong or partially sorted output, occasionally a crash, and an assertion failure in a debug-iterator build.

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; });
```

**Why it works:** A strict weak ordering requires `comp(x, x)` to be `false` for every `x`, along with asymmetry and transitivity of both the ordering and its induced equivalence. `<=` violates the first of these outright, since `comp(x, x)` evaluates to `true`. Comparison-based algorithms like `sort` rely on that contract internally — partitioning schemes assume an element is never "less than" itself — so a violation does not produce a cleanly wrong-but-predictable answer; it produces undefined behavior, which in practice ranges from a subtly mis-ordered result to out-of-bounds reads in some implementations. Switching to `<` restores irreflexivity and the rest of the contract along with it.

### 1.5 Expecting `unique` to remove *all* duplicates

```cpp
std::vector<int> v{1, 3, 2, 3, 1, 4};
v.erase(std::unique(v.begin(), v.end()), v.end());
// v is still {1, 3, 2, 3, 1, 4} — nothing was adjacent
```

`unique` only collapses runs of *consecutive* equal elements. It has no way to know the first `3` and the second `3` are "the same duplicate" with a `2` sitting between them.

```cpp
std::sort(v.begin(), v.end());                       // {1, 1, 2, 3, 3, 4}
v.erase(std::unique(v.begin(), v.end()), v.end());    // {1, 2, 3, 4}
```

**Why it works:** `unique` is a single forward pass that compares each element only to its immediate predecessor — it has no memory of anything seen earlier than the previous position, so it cannot recognize two equal elements separated by something different as duplicates of each other. Sorting first moves every occurrence of a given value into one contiguous run, exactly the shape `unique` is built to collapse. This two-step pattern — sort to establish adjacency, then a single adjacency-based pass — recurs throughout the standard library wherever an algorithm is documented as looking only at neighbors.

## 2. Misusing ranges & sentinels

### 2.1 Assuming `std::ranges::` "fixes" an algorithm's requirements

```cpp
std::list<int> lst{5, 3, 9, 1};
rg::sort(lst);   // "I'll just switch to ranges syntax and this'll work now" — it doesn't
```

Still a compile error. `rg::sort` needs `random_access_range`, exactly like classical `sort` needed random-access iterators — ranges didn't relax the requirement, they only changed how the failure is reported. The message now names the unmet concept directly instead of dumping the sort implementation's internals, which is a real improvement in diagnostics, but it's a better error, not a fix.

```cpp
lst.sort();
```

**Why it works:** Constrained algorithms check the same structural requirements as their classical counterparts — `ranges::sort` still needs `random_access_range` for the identical reason `std::sort` needed random-access iterators. What changed is where the check happens and how it reports failure: a concept is evaluated directly against the argument type at the call site, so the compiler names the unmet concept (`random_access_range`) instead of failing deep inside the algorithm's template instantiation. The fix is the same fix as 1.2 for the same underlying reason; ranges syntax produces a better error message, not a workaround.

### 2.2 Throwing away what `remove_if` gives back

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6};
rg::remove_if(v, [](int x) { return x % 2 == 0; });
std::cout << v.size();   // 6 — nothing was removed
```

`remove_if` only has iterators to work with; it has no way to call `erase` on a container it was never handed. It shuffles the elements to keep toward the front and returns where the junk begins. Drop that return value and the shuffle happened for nothing.

```cpp
auto junk = rg::remove_if(v, [](int x) { return x % 2 == 0; });
v.erase(junk.begin(), junk.end());   // v is {1, 3, 5}
```

**Why it works:** `remove_if` only has an iterator pair to work with — it was never handed the container itself, so it has no way to call `erase` on anything. What it does is shift every element that should survive toward the front and leave the tail in an unspecified, but valid-to-erase, state, then return an iterator marking where that tail begins. That return value is not a diagnostic or a convenience — it is the only record of where the shuffled junk starts, so discarding it does not undo the shuffle; it just makes the result of the shuffle inaccessible. The erase-remove idiom is exactly this: capture the returned iterator, then call the container's own `erase` with it.

### 2.3 Trying to use the result of a dangling call

```cpp
auto make = [] { return std::vector<int>{4, 8, 15}; };
auto it = rg::find(make(), 8);
std::cout << *it;   // does not compile
```

This is the protection described above working exactly as intended, not a bug in the code. `make()` returns a temporary, `vector` isn't a borrowed range, so `it`'s type is `rg::dangling`, which has no `operator*`. The fix is the one from the notes above — give the range a name so it outlives the call.

```cpp
auto v = make();
auto it = rg::find(v, 8);
std::cout << *it;   // 8
```

**Why it works:** `find`'s range parameter accepts the argument as passed, and when the argument is an rvalue supplied directly — the return value of `make()` — the algorithm cannot guarantee the temporary survives past the end of the full expression. `vector` does not model `borrowed_range` because it owns its storage; there is no way to keep referring to its elements once the vector itself is destroyed. The result type becomes `ranges::dangling`, a type deliberately missing `operator*`, so the danger surfaces as a compile error rather than a runtime one. Naming the vector — `auto v = make();` — turns the temporary into a named object with its own lifetime, one the found iterator can safely outlive within.

### 2.4 Trusting `dangling` to catch every lifetime bug

```cpp
auto get_first_even() {
    std::vector<int> local{1, 2, 3, 4};
    return rg::find_if(local, [](int x) { return x % 2 == 0; });
    // `local` is a named lvalue, not a temporary, so the dangling
    // guard from 2.3 doesn't apply here at all. This compiles cleanly
    // and returns a completely ordinary iterator into a vector that's
    // one line away from being destroyed.
}

auto it = get_first_even();
std::cout << *it;   // undefined behaviour, and nothing warned about it
```

The guard in 2.3 only engages for one specific shape: an rvalue range passed directly as the call's own argument. A named local — even one about to go out of scope — deduces as an lvalue reference, and an lvalue reference always counts as "borrowed" for this particular check, so a perfectly ordinary iterator comes out. `dangling` is a narrow guard against one calling pattern, not a general lifetime checker.

```cpp
auto get_first_even() {
    std::vector<int> local{1, 2, 3, 4};
    auto it = rg::find_if(local, [](int x) { return x % 2 == 0; });
    return it != local.end() ? std::optional{*it} : std::nullopt;   // copy the value out, not an iterator to it
}
```

**Why it works:** The `dangling` guard is a single, narrow rule: it fires only when the range argument is an rvalue passed directly into the call. A named local variable, no matter how soon it's destroyed, is an lvalue by the language's grammar, and an lvalue reference always counts as "the caller's responsibility" for this check — the library has no way to see that the object is about to go out of scope in the caller's own function. Fixing this requires stepping outside what the type system can catch: not returning a reference or iterator into something local at all. Returning `std::optional<T>` copies the value out while it's still valid, so nothing depends on the container's lifetime after the function returns.

### 2.5 A sentinel that never says "stop"

```cpp
struct always_false_sentinel {
    friend bool operator==(auto const&, always_false_sentinel) { return false; }
};

std::vector<int> v{1, 2, 3};
auto bad = rg::subrange(v.begin(), always_false_sentinel{});
rg::for_each(bad, [](int x) { std::cout << x << ' '; });
// keeps incrementing past v.end() forever — reads out of bounds, UB
```

A sentinel's entire contract is answering "is this the end?" correctly. Get that one answer wrong and there is no other safety net — both the classical `first == last` check and the ranges sentinel check rely completely on it being honest.

```cpp
auto good = rg::subrange(v.begin(), v.end());   // compares against a real end
```

**Why it works:** Every loop bounded by a sentinel — range-based `for`, `ranges::for_each`, the classical `first != last` check — has exactly one way to know when to stop: evaluating `iterator == sentinel` and trusting the answer. There is no secondary bounds check anywhere in the machinery; the sentinel's `operator==` *is* the entire contract. A sentinel that always returns `false` for equality removes that one safeguard completely, so the loop keeps incrementing past the container's actual storage with nothing left to catch it. The fix isn't really about `subrange` specifically — a sentinel's equality operator must genuinely reflect "has the end been reached," every time, for every value it's compared against.

## 3. Misusing views

(The dangling-view mistake is already shown above under *Materialization* — returning a `filter` view built on a local vector. That rule generalizes to every adaptor, not just `filter`: a view must never outlive what it's a window into. The mistakes below are different ones.)

### 3.1 Mutating a `filter_view`'s source, then iterating it again

```cpp
std::vector<int> w{1, 2, 3, 4, 5, 6};
auto evens = w | rv::filter([](int n) { return n % 2 == 0; });

for (int& x : evens) x = 7;          // 2, 4, 6 all become 7
for (int x : evens) std::cout << x;  // undefined behaviour
```

The notes above already state the rule: changing whether an already-visited element satisfies the predicate is undefined behaviour, by the standard's explicit wording. The first loop does exactly that — none of the new `7`s satisfy `% 2 == 0` — so it's the second loop's re-traversal that breaks.

```cpp
for (int& x : evens) x = 100;   // still even: membership unchanged, safe to keep using `evens`
```

**Why it works:** `filter_view::begin()` caches the position of the first element satisfying the predicate the first time it's called, specifically so that repeated iteration over the same view doesn't re-scan from the start each time. That caching is only sound if predicate membership stays fixed after it's been evaluated — the standard states this as an explicit precondition, not an implementation detail. Mutating an already-visited element so it no longer satisfies the predicate leaves the cached position pointing at something the view no longer believes should be there, and nothing re-validates that cache before the next traversal. Mutating in a way that never changes which elements pass the predicate — as in `x = 100` for an evenness filter — leaves the cache consistent with reality, so it stays safe.

### 3.2 Passing a view by `const&`

```cpp
void print_all(auto const& r) {
    for (auto x : r) std::cout << x << ' ';   // fails to compile when r is a filter_view
}

auto evens = w | rv::filter([](int x) { return x % 2 == 0; });
print_all(evens);
```

`filter_view` caches the position of its first element on the first call to `begin()`, so repeated iteration stays cheap. Writing to that cache means `begin()` can't be `const` — so `filter_view` doesn't satisfy `range` when accessed through a `const` reference. The "just take everything by `const&`" habit from ordinary containers doesn't carry over to every view.

```cpp
void print_all(auto r) {                 // views are O(1) to copy — that's the whole point
    for (auto x : r) std::cout << x << ' ';
}
```

**Why it works:** `filter_view` needs to write to its cached-begin-position on first access, which means `begin()` cannot be a `const` member function — writing through a `const` object isn't allowed. Because `range` requires `begin()` to be callable on the object as given, a `filter_view` accessed through `T const&` doesn't satisfy `range` at all, and the function template fails to instantiate. Views are explicitly designed to be cheap to copy — that's the entire reason they don't own their data — so passing by value (or `auto` in a template) isn't a performance concession the way it would be for a container; it's the intended usage pattern.

### 3.3 Assuming `zip` pads instead of truncates

```cpp
std::vector<std::string> names{"alice", "bob", "carol"};
std::vector<int> scores{95, 87};   // one score short

for (auto [name, score] : rv::zip(names, scores))
    std::cout << name << ": " << score << '\n';
// prints alice and bob only — carol is silently dropped, no error
```

`zip` stops the moment its shortest input runs out. Nothing flags the length mismatch — not an exception, not even a smaller output that looks obviously incomplete.

```cpp
if (names.size() != scores.size())
    throw std::runtime_error("length mismatch");
```

**Why it works:** `zip_view`'s `size()` (and its `end()`) is defined as the minimum of every underlying range's size, precisely so it never has to invent or default-construct a value for a range that ran out early. That means truncation isn't a bug in `zip` — it's the documented behavior — which is exactly why nothing about it looks like an error: no exception, no partial-and-obviously-wrong output, just a shorter result than expected. Anywhere a length mismatch would be a logic error rather than a legitimate scenario, that check has to happen before the `zip`, since `zip` itself will never surface it.

### 3.4 Treating a `split` token like a string

```cpp
std::string_view csv = "10,20,30";
for (auto token : csv | rv::split(','))
    int n = std::stoi(token);   // does not compile
```

A token from `split` is a subrange over the source, not a `std::string` or `std::string_view` — `stoi` has no overload that accepts one.

```cpp
for (auto token : csv | rv::split(',')) {
    std::string_view sv(token.begin(), token.end());   // or std::string_view{token} in C++23
    int n = std::stoi(std::string(sv));
}
```

**Why it works:** A token from `split` (or `lazy_split`) is a `subrange` — an iterator/sentinel pair delimiting a piece of the source — not an owned or even necessarily contiguous sequence of characters. `stoi` requires a `std::string` or something convertible to `const char*`, and a `subrange` provides neither; there's no overload for it to bind to, hence the compile failure rather than a wrong runtime answer. Explicitly constructing a `string_view` from the token's `begin()`/`end()` — or, for `split`'s forward-range tokens in C++23, direct `string_view{token}` construction, since those tokens satisfy `contiguous_range` and `sized_range` — bridges from "a window into the source" to a type the rest of the standard library already knows how to consume.

### 3.5 Iterating a `lazy_split` result more than once

```cpp
auto tokens = single_pass_source | rv::lazy_split(',');
for (auto tok : tokens) { /* pass one */ }
for (auto tok : tokens) { /* pass two — nothing left to read */ }
```

The notes above call out that `lazy_split` targets input ranges that can't be re-traversed. That's a property of the *source*, not of `lazy_split` itself — if the underlying range is something like a stream that's already been consumed, a second pass has nothing to pull from. `split`'s tokens are forward ranges and don't carry this restriction; `lazy_split`'s can, depending on what's underneath.

```cpp
for (auto tok : tokens) { /* validate and process in the same pass */ }
```

**Why it works:** `lazy_split`'s restriction traces back to the underlying range's category, not to anything `lazy_split` does internally — it's built to work over `input_range`s, which by definition can only be read once, forward, with no way to return to an earlier position (a network stream or an `istream_iterator`-backed range, for instance). `split`'s tokens are forward ranges specifically because `split` requires at least a `forward_range` source to begin with, so multiple passes are safe there. The practical check is inspecting what's underneath the adaptor rather than the adaptor's name — a `lazy_split` over a `vector` is perfectly re-iterable, since a `vector` is a forward range regardless of which adaptor sits on top of it.

### 3.6 Expecting `chunk_by` to behave like `GROUP BY`

```cpp
std::vector<int> ids{1, 2, 1, 3, 2, 1};
auto groups = ids | rv::chunk_by(std::equal_to<>{});
// six groups of size 1 — {1} {2} {1} {3} {2} {1}
```

`chunk_by` only looks at *adjacent* pairs. It has no concept of "all the 1s," scattered as they are through the sequence.

```cpp
std::ranges::sort(ids);                                // {1, 1, 1, 2, 2, 3}
auto groups = ids | rv::chunk_by(std::equal_to<>{});   // {1,1,1} {2,2} {3}
```

**Why it works:** `chunk_by` evaluates its predicate only on consecutive pairs — element `i` against element `i+1` — with no broader notion of grouping by value across the whole sequence. Every time the predicate returns `false` for a pair, that's a chunk boundary, so scattered occurrences of the same value produce a new boundary every time something else happens to separate them. Sorting first guarantees that every occurrence of a given value becomes adjacent to every other occurrence of that value, the only shape `chunk_by`'s adjacency-only view can turn into a single group.

### 3.7 Sprinkling `rv::common` everywhere "just in case"

```cpp
auto r = data | rv::filter(pred) | rv::transform(f) | rv::common;
for (int x : r) { /* ... */ }              // range-for never needed rv::common
rg::fold_left(r, 0, std::plus<>{});        // neither did this
```

`rv::common` earns its keep only at an actual boundary with a legacy two-iterator API. Range-`for` and every `std::ranges::` algorithm accept a range whose `begin` and `end` have different types directly; tacking `rv::common` on out of habit adds a runtime branch to every increment for no benefit at all.

```cpp
auto r = data | rv::filter(pred) | rv::transform(f);
for (int x : r) { /* ... */ }
rg::fold_left(r, 0, std::plus<>{});

auto r2 = r | rv::common;
std::accumulate(r2.begin(), r2.end(), 0);   // classical accumulate needs matching iterator types — here it's earned
```

**Why it works:** `common_range` exists to bridge to APIs that specifically require `begin()` and `end()` to return the same type — classical two-iterator-typed function templates like `std::accumulate`, or any code taking a `Container::iterator` by name and comparing it directly. Range-`for` and every `std::ranges::` algorithm are written against the `range` concept, which only requires that `begin()` and `end()` be *comparable*, not identically typed — a sentinel-typed `end()` is already fully usable there. Wrapping in `rv::common` when nothing downstream needs matching types adds a runtime branch on every increment, to decide whether the current position equals the sentinel, for a guarantee nothing is asking for.
