# Common Misuses: How *Not* to Use Standard HOFs, Ranges, and Views

Every construct in the notes above has a companion way to get it wrong. Some fail loudly at compile time, some fail silently at runtime, and a few compile clean, run clean, and just give the wrong answer. This page catalogs the ones worth knowing on purpose, organized the same three ways as the notes above — the classical HOF/iterator-pair model, the constrained ranges algorithms and sentinels, then views — using the same aliases (`namespace rg = std::ranges;` `namespace rv = std::ranges::views;`). Each entry pairs the mistake with the fix; a quick-reference table sits at the end.

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

### 1.2 Handing an algorithm the wrong iterator category

```cpp
std::list<int> lst{5, 3, 9, 1};
std::sort(lst.begin(), lst.end());
```

`std::sort` requires random-access iterators; `std::list` only offers bidirectional ones. The call still compiles at the line you wrote — the failure surfaces deep inside `<algorithm>`'s implementation, in an error message full of names you never typed.

```cpp
lst.sort();   // std::list has its own member sort() for exactly this reason
```

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

### 1.4 A comparator that isn't a *strict* weak ordering

```cpp
std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};
std::sort(v.begin(), v.end(), [](int a, int b) { return a <= b; });
```

`sort` requires `comp(x, x) == false` for every `x`; `<=` fails that immediately. The result is undefined behaviour — usually a wrong or partially sorted output, occasionally a crash, and an assertion failure in a debug-iterator build.

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; });
```

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

## Quick reference

Most of these come from the same handful of blind spots: assuming a compile-time check exists where it doesn't (1.1, 2.1, 2.4), discarding a return value that was the entire point of the call (2.2), forgetting that a lazy view caches state across separate iterations (3.1, 3.2), and treating a documented contract — strict weak ordering, adjacency-only dedup, shortest-wins `zip`, subrange tokens — as a suggestion instead of a requirement (1.3, 1.4, 2.5, 3.3, 3.4, 3.6).

| # | Mistake | What actually happens | Fix |
|---|---------|------------------------|-----|
| 1.1 | Mixing iterators from two containers | UB, no compile error | Use one container's `begin`/`end` |
| 1.2 | `std::sort` on a `std::list` | Deep template compile error | `lst.sort()` |
| 1.3 | `accumulate` seeded with `0` on `double` data | Silent truncation to `int` | Seed with `0.0` |
| 1.4 | `<=` as a sort comparator | UB — violates strict weak ordering | Use `<` |
| 1.5 | `unique` without sorting first | Non-adjacent duplicates survive | Sort, then `unique` |
| 2.1 | `rg::sort` on a `std::list` | Same compile error, clearer message | `lst.sort()` |
| 2.2 | Ignoring `remove_if`'s return value | Container left unchanged | Capture it, `erase()` the subrange |
| 2.3 | Dereferencing a `dangling` result | Compile error, by design | Name the range first |
| 2.4 | Assuming `dangling` catches all lifetime bugs | UB, no compile error | Don't return iterators into locals |
| 2.5 | A sentinel that never matches | Reads past the end — UB | Make the sentinel's condition real |
| 3.1 | Changing filtered-membership, then reusing the view | UB | Don't change predicate membership mid-use |
| 3.2 | Passing a `filter_view` by `const&` | Compile error | Take views by value |
| 3.3 | Expecting `zip` to pad short ranges | Silent truncation to the shortest | Check lengths first |
| 3.4 | Passing a `split` token to `stoi` | Compile error | Build a `string_view`/`string` from it first |
| 3.5 | Iterating a single-pass `lazy_split` twice | UB on the second pass | Do everything in one pass |
| 3.6 | `chunk_by` on unsorted data, expecting `GROUP BY` | Many tiny groups | Sort by the key first |
| 3.7 | `rv::common` used out of habit | Unnecessary runtime overhead | Apply only at a real iterator-pair boundary |

---

# Things to remember

When working with **higher-order functions** in **C++**—such as STL algorithms like `std::transform`, `std::for_each`, `std::accumulate`, and more—it's important to keep several **key concepts** in mind to ensure efficient, correct, and safe usage. Below are the **key things to remember** when using these functions in C++.


### 1. **Destination Range Requirements**

* Many STL algorithms like `std::copy_if`, `std::transform`, `std::remove_if`, etc., **modify** or **populate** the destination range, but they do not **resize** the destination container.
* **Key Point**: Always ensure that the destination container is **large enough** to hold the results or use an **inserter** like `std::back_inserter` to automatically handle resizing.

  ```cpp
  std::vector<int> output;
  std::copy_if(input.begin(), input.end(), std::back_inserter(output), [](int x) { return x > 0; });
  ```



### 2. **Lambdas and Return Values**

* When using **lambdas** inside algorithms like `std::transform`, `std::accumulate`, or `std::for_each`, make sure that the lambda **returns a value** if you expect the transformation to happen.
* **Key Point**: Lambdas without a return statement will cause incorrect behavior.

  ```cpp
  std::transform(vec.begin(), vec.end(), vec.begin(), [](int x) { return x * 2; }); // Correct
  ```



### 3. **Initial Value in `std::accumulate`**

* The **initial value** provided to `std::accumulate` is important. Using an incorrect initial value can lead to **incorrect results**.
* **Key Point**:

  * Use `0` for **addition** (sum).
  * Use `1` for **multiplication** (product).
  * Example:

    ```cpp
    int sum = std::accumulate(nums.begin(), nums.end(), 0); // Correct for sum
    int product = std::accumulate(nums.begin(), nums.end(), 1, std::multiplies<int>()); // Correct for product
    ```



### 4. **Iterator Validity**

* Some algorithms like `std::remove` and `std::erase` **invalidate iterators** when elements are removed from the container, so be careful when working with iterators after modifying containers.
* **Key Point**: After calling algorithms that modify containers, **recalculate iterators** if necessary.

  ```cpp
  auto it = std::remove(vec.begin(), vec.end(), 5);
  vec.erase(it, vec.end()); // Correct
  ```



### 5. **Side Effects in Predicates or Functions**

* **Predicates** (functions passed to algorithms like `std::find_if` or `std::remove_if`) should be **side-effect-free** to avoid **unexpected results**.
* **Key Point**: Avoid modifying the container or global state within the predicate.

  ```cpp
  // Side-effect free predicate
  auto isEven = [](int x) { return x % 2 == 0; };
  ```



### 6. **Avoiding Uninitialized Destination Containers**

* Algorithms like `std::transform`, `std::copy_if`, and others will **not resize** the destination container. Always ensure that the destination container has **sufficient capacity**.
* **Key Point**: Use `std::back_inserter` for automatic resizing when the destination container is empty or has unknown size.

  ```cpp
  std::vector<int> result;
  std::transform(vec.begin(), vec.end(), std::back_inserter(result), [](int x) { return x * 2; });
  ```



### 7. **Range-Based Algorithms in C++20 (`std::ranges`)**

* **C++20** introduced **ranges** and **range adaptors** (`std::ranges::view`, `std::ranges::transform`, `std::ranges::filter`, etc.) that allow for more **elegant and efficient** manipulation of sequences.
* **Key Point**: You can use **lazy evaluation** with ranges, meaning computations are done only when needed, and no intermediate containers are created.

  ```cpp
  #include <ranges>
  auto result = nums | std::views::transform([](int x) { return x * 2; }) | std::views::filter([](int x) { return x > 10; });
  ```



### 8. **`std::for_each` and Side Effects**

* `std::for_each` applies a function to each element in a container, but **it doesn’t return a value**. It’s typically used for **side effects**, like modifying elements in-place or printing values.
* **Key Point**: Be mindful that `std::for_each` does **not modify** the container or return a modified version.

  ```cpp
  std::for_each(vec.begin(), vec.end(), [](int& x) { x += 5; }); // Modify elements in-place
  ```



### 9. **Use of `std::find_if` and `std::count_if`**

* Functions like `std::find_if` and `std::count_if` are useful for **searching** or **counting** elements that match a condition, but their **return types** can be tricky to understand.
* **Key Point**: `std::find_if` returns an **iterator** to the first matching element, while `std::count_if` returns the **count** of elements that satisfy the predicate.

  ```cpp
  auto it = std::find_if(vec.begin(), vec.end(), [](int x) { return x > 10; });
  int count = std::count_if(vec.begin(), vec.end(), [](int x) { return x % 2 == 0; });
  ```



### 10. **`std::remove_if` and Erase-Remove Idiom**

* The **Erase-Remove Idiom** is a common pattern in C++ where you **remove** elements from a container and then **erase** them.
* **Key Point**: `std::remove_if` moves the elements that **don’t match** to the front of the container, but it doesn’t change the container’s size. After calling `std::remove_if`, you should call `erase` to remove the unwanted elements.

  ```cpp
  auto it = std::remove_if(vec.begin(), vec.end(), [](int x) { return x < 0; });
  vec.erase(it, vec.end()); // Correctly erases elements after removal
  ```



### 11. **Execution Policies (C++17 and Beyond)**

* In **C++17**, you can use **execution policies** with certain algorithms (like `std::for_each`, `std::transform`) to enable **parallel execution** and optimize performance for large datasets.
* **Key Point**: Use `std::execution::par` or `std::execution::seq` to specify parallel or sequential execution.

  ```cpp
  #include <execution>
  std::for_each(std::execution::par, vec.begin(), vec.end(), [](int& x) { x *= 2; });
  ```



### 12. **Avoiding Redundant Operations**

* Many algorithms, especially ones like `std::transform`, `std::copy_if`, and `std::for_each`, can lead to **redundant operations** if called repeatedly on the same data.
* **Key Point**: Combine multiple operations when possible, especially when filtering and transforming data. For example, instead of transforming then filtering, you can filter and transform in one pass.

  ```cpp
  std::transform(vec.begin(), vec.end(), vec.begin(), [](int x) { return (x > 0) ? x * 2 : x; });
  ```



### 13. **Use of `std::any_of`, `std::all_of`, `std::none_of`**

* These functions check conditions on **all**, **any**, or **none** of the elements in a range. They're useful for making conditional decisions based on the contents of a container.
* **Key Point**: Use these for **short-circuiting** when you only need a true/false result based on the condition.

  ```cpp
  bool anyEven = std::any_of(vec.begin(), vec.end(), [](int x) { return x % 2 == 0; });
  bool allPositive = std::all_of(vec.begin(), vec.end(), [](int x) { return x > 0; });
  ```



### 14. **Control parameters**
The execution policy and the seed values act as control parameters that defines how and from what starting point the operation is executed.
Execution policy controls:
* Threading (Whether the algorithm runs on one core or many cores)
* Ordering (Whether the elements are processed in a predictable order)
* Vectorization (Whether the CPU combines multiple operations into one hardware SIMD instruction)
* Determinism (Whether results are strictly predictable or allowed to vary slightly for speed)

It decides:
“How aggressively and in what manner should this computation run?”

Seed controls:
* Initial state (The very first value used in the computation)
* Algebraic identity (The mathematical identity element of the operation, e.g., 0 for addition, 1 for multiplication)
* Result type (The data type of the final result, e.g., char, int, double, etc.)
* Precision (How accurate the final answer can be)
* State structure (What kind of object holds the intermediate state)

It decides:
“What is the starting point and what kind of result are we building?”

**Key Point**: By changing these 2 parameters, we can completely change:
* performance
* threading behavior
* ordering guarantees
* result type
* precision
* shape of the final result



### TL;DR: Key Things to Remember

* Use `std::back_inserter` for destination ranges in algorithms.
* Always ensure lambdas **return values** when used in transformation algorithms.
* Be mindful of iterator validity when modifying containers.
* Prefer **side-effect free** predicates.
* **Execution policies** allow parallel execution (Since C++17).
* Use **ranges** for elegant and efficient transformations (Since C++20).
* Use **Erase-Remove Idiom** for removing elements from containers.

By keeping these principles in mind, you can write more **efficient**, **robust**, and **maintainable code** using C++ standard higher-order functions.

Source: ChatGPT
