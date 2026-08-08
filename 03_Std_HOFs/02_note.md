# C++ Ranges and Views
From C++98 through C++17, every standard algorithm took a *pair* of iterators delimiting a
half-open interval `[first, last)`:

```cpp
std::vector<int> v{5, 3, 9, 1};
std::sort(v.begin(), v.end());
auto it = std::find(v.begin(), v.end(), 9);
```

The half-open convention is a genuinely good design. It makes the number of elements equal
`last - first`, makes empty ranges representable without a special case (`first == last`),
and allows subranges to be expressed by moving the endpoints.

```
half-open interval [first, last)

  index:    0     1     2     3     4
          +-----+-----+-----+-----+-----+
   data:  |  5  |  3  |  9  |  1  |  7  |
          +-----+-----+-----+-----+-----+
          ^                             ^
          first                         last   (one past the end)

  size == last - first == 5
  empty when first == last
```
### Four problems with the iterator-pair model

1. **Verbosity at the call site**: The overwhelmingly common case is "the whole container", which must nonetheless be spelled with two expressions that repeat the container name.

2. **No composition**: Algorithms consume iterator pairs and write to output iterators. To express "the squares of the even elements", the classical options are a raw loop, or a sequence of algorithm calls each materialising a temporary container. Neither composes as an expression.

    ```cpp
    // classical: two passes and a temporary
    std::vector<int> evens;
    std::copy_if(v.begin(), v.end(), std::back_inserter(evens),
                 [](int x) { return x % 2 == 0; });
    std::vector<int> result;
    std::transform(evens.begin(), evens.end(), std::back_inserter(result),
                   [](int x) { return x * x; });
    ```

3. **No compile-time checking of iterator requirements**: Passing a `std::list` iterator to `std::sort` compiles the call and fails deep inside the implementation, producing the error messages that gave templates their reputation.

4. **Mismatched iterators are undefined behaviour, not a compile error**: Nothing prevents `std::find(a.begin(), b.end(), x)`.

## Ranges
With the introduction of Ranges and Views in C++20, working with algorithms became more expressive and easier to read. 
**A caveat stated early.** Ranges are not uniformly superior. They increase compile times, their diagnostics — though better than pre-concepts templates — are still long, debug-build performance can be noticeably worse, and some pipelines re-evaluate work in ways that surprise newcomers.

A *range* is a representation of a asequence of elements with a begin iterator and an end sentinel. It is anything that `rg::begin()` and `rg::end()` can be called on. That is the entire requirement:

```cpp
template <class T>
concept range = requires (T& t) {
    rg::begin(t);
    rg::end(t);
};
```

Consequently, the following are all ranges: every standard container, C arrays, `std::string_view`, `std::initializer_list`, and every view. Raw arrays work because `rg::begin` handles them:

```cpp
int arr[] = {5, 2, 8};
rg::sort(arr);                 // compiles; no arr + 3 needed
// arr is now {2, 5, 8}
static_assert(rg::contiguous_range<decltype(arr)>);
```

### Sentinel
In the classical model, `first` and `last` of a sequence have the *same type*. But in the ranges model, `end()` returns a *sentinel*, i.e., *any type* comparable with the iterator via `==`.

```
Classical:                        

  iterator ----> ... ----> iterator
     |                        |
   same type              same type
```
```
Ranges:

    iterator ----> ... ----> sentinel
    |                        |
    any type            "is this the end?" oracle
```
The sentinel does not have to *point* anywhere. It only has to answer the question "is this iterator at the end?". Three consequences follow.

#### 1. Ranges with cheap end-detection stop being awkward.
A null-terminated C string has no end iterator without first walking it. With a sentinel, the walk disappears:

```cpp
struct null_sentinel {
    friend bool operator==(char const* p, null_sentinel) { return *p == '\0'; }
};

char const* msg = "hello";
auto cstr = rg::subrange(msg, null_sentinel{});
std::cout << rg::distance(cstr);      // prints 5
```

#### 2. Predicate-delimited ranges become first-class.
"Predicate-delimited" means the range's end is defined by a condition on the elements — "up to the first negative number", "up to the first null byte" — rather than by a position or a count. An entity is first-class when it can be stored in a variable, passed as an argument, returned from a function, and composed with other things. Before sentinels, a situation like "the elements up to the first negative" was handled in a separate control flow; but after sentinels, it's just a value.

```cpp
struct stop_at_negative {
    friend bool operator==(std::vector<int>::const_iterator it, stop_at_negative) {
        return *it < 0;
    }
};

auto until_negative(std::vector<int> const& v) {   // returned from a function
    return rg::subrange(v.cbegin(), stop_at_negative{});
}

auto r = until_negative(data);                     // stored in a variable
rg::fold_left(r, 0, std::plus<>{});                // passed to algorithms  -> 8
*rg::max_element(r);                               //                       -> 4
for (int x : r | rv::transform(times10) | rv::reverse) { }   // composed with views
```
There are two payoffs because of this:
1. Algorithms and adaptors accept it just because it satisfies a range, and no special-casing is needed.
2. The end of a sequence is discovered *during* the traversal, not before it.

#### 3. Infinite ranges are representable.
For instance, the range `rv::iota(0)` has an `std::unreachable_sentinel` for its end; the comparison is always `false`, so the range never finishes. This is only usable in combination with something that stops it.

The cost of the generalisation is that `begin()` and `end()` may now have different types, so such a range cannot be handed to a legacy iterator-pair API. Restoring the uniform type is what `rv::common` is for.

### The concept hierarchy
The standard ranges library, defined in the `<ranges>` header, is an extension and generalization of the standard algorithms and iterator libraries, which makes it more powerful, flexible, composable, and less error-prone.

The ranges library includes:
* range algorithms (function objects that perform immediate computation and execution of ranges ([eager evaluation](https://share.google/aimode/UnfevoWfIYgsIVtjH)))
* range adaptors (function objects applied to views that perform computations at a later time when needed ([lazy evaluation](https://nixiz.github.io/yazilim-notlari/2023/09/10/lazy-evaluation-en))).

A `std::ranges` algorithm assumes begin to end by default when passing in a container. There are also variants available for more granular control over a container.

In C++20, `std::ranges` are built on concepts:

`std::ranges::range<T>`

where `T` must have `begin(t)` and `end(t)` that return valid iterators.

And they are refined by the iterator categories:
* `std::ranges::input_range` → forward‑only, single‑pass.
* `std::ranges::forward_range` → bidi‑capable, multiple passes.
* `std::ranges::bidirectional_range` → can traverse backwards
* `std::ranges::random_access_range` → $O(1)$ random element access
* `std::ranges::contiguous_range` → elements are laid out contiguously in memory (enables pointer arithmetic)
* `std::ranges::sized_range` → `size()` is available in $O(1)$
* `std::ranges::common_range` → `begin(t)` and `end(t)` return the same type (no separate sentinel).

Each concept refines the previous one, progressively requiring more capabilities:
```
range
├── input_range         (single-pass)
│   └── forward_range   (multi-pass)
│       └── bidirectional_range
│           └── random_access_range
│               └── contiguous_range
├── sized_range     (O(1) size())   [orthogonal]
├── common_range    (begin/end same type) [orthogonal]
├── borrowed_range  (iterators outlive range) [orthogonal]
└── view            (O(1) move/copy/destroy) [orthogonal]
```
These concepts are checked statically at compile time. 

### Three orthogonal properties

* **`sized_range`** — the size is obtainable in $O(1)$ (less time) via `rg::size`. For example, a `std::vector` is sized, but a a `std::forward_list` is not (it has no $O(1)$ `size()` by design), and neither is a predicate-delimited range.

* **`common_range`** — `begin()` and `end()` have the same type. All classical containers are common ranges.

* **`borrowed_range`** — iterators obtained from the range stay valid even after the range object itself is destroyed. This is the property that makes dangling detection possible. A `std::string_view` is borrowed because it never owned the characters in the first place; destroying the view does not touch them. A `std::vector` prvalue is not borrowed because its destructor frees the elements.

It can be said that:
* All containers and container adaptors  are ranges
* Non-owning or borrowed containers like `std::string_view`, `std::span`, etc., are borrowed ranges. A range is "borrowed" if iterating over it does not depend on the lifetime of the range object itself.

### The constrained algorithms
Every classical algorithm has a `std::ranges::` counterpart taking a range:

```cpp
std::sort(v.begin(), v.end());     // classical
rg::sort(v);                       // ranges
rg::sort(v.begin(), v.end());      // the iterator form still exists
```

The ranges versions live in `<algorithm>` and `<numeric>` — the same headers — and are function objects rather than function templates. That has a practical consequence: they cannot be found by argument-dependent lookup and cannot be accidentally hijacked by an overload in another namespace, but it also means they must be named explicitly and can be passed around as values.

Ranges algorithms return richer information than their classical counterparts.

#### Algorithms that trim return a subrange.
The `rg::remove_if` does not resize anything — it cannot, since it has no access to the container — but it returns the *junk tail* as a subrange, which pairs directly with `erase`:

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6};
auto junk = rg::remove_if(v, [](int x) { return x % 2; });
// v.size() is still 6; junk covers the 3 discarded slots
v.erase(junk.begin(), junk.end());
// v is now {2, 4, 6}
```

This is the classical erase-remove idiom with a friendlier spelling. Discarding the return value is a silent no-op and one of the most common ranges bugs.

#### Algorithms with two cursors return a struct.
The `rg::copy` returns `{in, out}`; `rg::minmax` returns `{min, max}`:

```cpp
auto res = rg::copy(src, dst.begin());   // res.in, res.out
auto [mn, mx] = rg::minmax(nums);        // structured binding
```

#### Algorithms returning iterators guard against dangling.
If the argument is an rvalue range that is *not* a `borrowed_range`, the returned iterator would dangle immediately — so the library returns the empty tag type `rg::dangling` instead:

```cpp
auto make = [] { return std::vector<int>{4, 8, 15}; };

auto d = rg::find(make(), 8);            // decltype(d) is rg::dangling
// *d;                                   // does not compile — by design

auto named = make();
auto ok = rg::find(named, 8);            // a real iterator
std::cout << *ok;                        // 8

auto sv = rg::find(std::string_view{"abc"}, 'b');   // fine: string_view is borrowed
```

This converts a class of use-after-free bugs into compile errors. The diagnostic reads roughly `no match for operator*(std::ranges::dangling)`, which is opaque on first encounter but unambiguous once recognised.

#### Folds
The `std::accumulate`, however, has no direct ranges counterpart; C++23 instead introduced a family of folds with clearer semantics:

```cpp
rg::fold_left(v, 0, std::plus<>{});                  // 55 for 1..10
rg::fold_left_first(v, std::plus<>{});               // std::optional — empty-range safe
rg::fold_right(v, 0, std::plus<>{});
rg::fold_left_with_iter(v, 0, std::plus<>{});        // also returns the end iterator
```

Folds take no projection parameter, so a projection is expressed by piping through
`rv::transform`:

```cpp
rg::fold_left(staff | rv::transform(&Employee::salary), 0, std::plus<>{});   // 470
```

| Category | Algorithms |
|---|---|
| Non-modifying | `all_of` `any_of` `none_of` `for_each` `for_each_n` `count` `count_if` `mismatch` `equal` `lexicographical_compare` `find` `find_if` `find_if_not` `find_end` `find_first_of` `adjacent_find` `search` `search_n` `contains`† `contains_subrange`† `find_last`† `find_last_if`† `starts_with`† `ends_with`† `fold_left`† `fold_right`† `fold_left_first`† |
| Modifying | `copy` `copy_if` `copy_n` `copy_backward` `move` `move_backward` `fill` `fill_n` `generate` `generate_n` `transform` `replace` `replace_if` `replace_copy` `swap_ranges` `reverse` `reverse_copy` `rotate` `rotate_copy` `shift_left`† `shift_right`† `sample` `shuffle` `unique` `unique_copy` `remove` `remove_if` `remove_copy` `remove_copy_if` `iota`† |
| Partitioning | `is_partitioned` `partition` `stable_partition` `partition_copy` `partition_point` |
| Sorting | `sort` `stable_sort` `partial_sort` `partial_sort_copy` `is_sorted` `is_sorted_until` `nth_element` |
| Binary search | `lower_bound` `upper_bound` `equal_range` `binary_search` |
| Set operations | `merge` `inplace_merge` `includes` `set_union` `set_intersection` `set_difference` `set_symmetric_difference` |
| Heap | `push_heap` `pop_heap` `make_heap` `sort_heap` `is_heap` `is_heap_until` |
| Min/max | `min` `max` `minmax` `min_element` `max_element` `minmax_element` `clamp` |
| Permutation | `next_permutation` `prev_permutation` `is_permutation` |

## Views
A *view* is a range that is cheap to copy, move, and destroy — formally, one whose copy and move operations are $O(1)$. Views do not own the elements they present (with one deliberate exception).

A view stores where the data lives and what to do with it, not the data.

```
        view                          underlying container
   +---------------+                 +---+---+---+---+---+
   | ptr to vector | --------------> | 1 | 2 | 3 | 4 | 5 |
   | predicate     |                 +---+---+---+---+---+
   +---------------+
    16 bytes                          the actual storage
```

The namespace alias `std::views` is provided as a shorthand for `std::ranges::views`. A view is a lightweight range that works on a container without making internal data copies, unlike a range. A view provides a "window" into an existing range via reference semantics, i.e., it is memory efficient and mutable. In other words, it does not copy the elements of the container, and modifications to the underlying container are reflected in the view and vice-versa.

The view adapters are defined under `std::views`, such as `std::views::filter`, `std::views::transform`, etc., and don’t immediately process data. Instead, they create a view — a lightweight object that does not own or copy data from the container it works on, but just defines how elements should be seen. This allows for lazy evaluation, where the operations are defined immediately but logic is only executed when we actually iterate over the final result. For more on lazy evaluation in C++, read "Functional Programming in C++" by Ivan Cukic or "Learning C++ Functional Programming" by Wisnu Anggoro.

Custom views can also be created by inheriting from `std::ranges::view_interface`.
```cpp
template<std::ranges::view V>
class my_view : public std::ranges::view_interface<my_view<V>> {
    V base_;
public:
    my_view() = default;
    my_view(V base) : base_(std::move(base)) {}

    auto begin() { return std::ranges::begin(base_); }
    auto end()   { return std::ranges::end(base_); }
};
```

A view must satisfy:
```cpp
template<typename V>
concept view = std::ranges::range<V>
            && std::movable<V>
            && std::ranges::enable_view<V>; // opt-in marker
```
And additionally, all these operations must be $O(1)$:
* Move construction
* Move assignment
* Destruction
* Copy construction (if supported)
* Copy assignment (if supported)
This $O(1)$ constraint is the essential rule: a view must **never** copy the underlying data. It only holds a reference/pointer/iterator to it.

Consider a traditional STL example that squares even numbers:
```cpp
std::vector<int> data = {1, 2, 3, 4};

auto is_even = [](int x) { return x % 2 == 0; };
auto square = [](int x) { return x * x; };

// STEP 1: Filter (Eagerly processes entire vector into memory)
std::vector<int> evens;
std::copy_if(data.begin(), data.end(), std::back_inserter(evens), is_even);

// STEP 2: Transform (Eagerly processes the new vector into memory)
std::vector<int> squares;
std::transform(evens.begin(), evens.end(), std::back_inserter(squares), square);

// STEP 3: Consumer
for (int x : squares) {
    std::cout << x << " "; 
}
```
This approach:
* Requires intermediate containers
* Separates steps instead of expressing a continuous flow
* Can cause memory and performance overhead

The above example would then look like:
```cpp
std::vector<int> data = {1, 2, 3, 4};

auto is_even = [](int x) { return x % 2 == 0; };
auto square = [](int x) { return x * x; };

// The Consumer pulls elements directly through the pipeline on the go
for (int x : (data | std::views::filter(is_even) | std::views::transform(square))) {
    std::cout << x << " "; 
}
```
This reads almost like an English sentence:
Take a list of numbers → keep only even numbers → square them.

The general pattern looks like this:
```
Data | View_Adapter | Action
```
* Data → The original source of elements (for example, a `std::vector` of users).
* View Adapter → Describes how the data should be processed.
* Action → The final step where the result is actually consumed (for example, printing or storing values).

This diagram is the one worth memorising:
```
   for (int x : (data | filter(is_even) | transform(square)))

   The consumer pulls. Each request travels UP the pipeline,
   and one element travels back DOWN.

        consumer (range-for)
              |  "next element, please"
              v
        +-------------+
        |  transform  |   asks its source for one element, squares it
        +-------------+
              |  "next element, please"
              v
        +-------------+
        |   filter    |   asks its source repeatedly until one passes
        +-------------+
              |  "next element, please"
              v
        +-------------+
        |    data     |   yields 1, then 2, then 3, ...
        +-------------+

   Trace for data = {1,2,3,4}:
     pull -> filter asks data: 1 (odd, reject), 2 (even, accept)
          -> transform squares 2  -> consumer receives 4
     pull -> filter asks data: 3 (odd, reject), 4 (even, accept)
          -> transform squares 4  -> consumer receives 16
     pull -> filter asks data: exhausted -> pipeline ends
```

Here,
* only the pipeline structure is built.
* at the time of using `result` (e.g., for printing each element of `result`), each view’s iterator advances the base and applies the filter/transform on‑the‑fly.

Nothing is buffered between stages. No intermediate container exists at any point. One big design win is that composition is natural and efficient.

This contract ensures that:
* Many views can be chained without performance collapse.
* Views can be passed cheaply (by value) into algorithms or functions

Instead of calling algorithms separately and passing iterator pairs each time, operations can now be built in a pipeline style, similar to how data flows through stages. This is called as a "pipeable" workflow.

The pipe operator `|` is used to chain range adaptors. It was chosen deliberately to evoke the Unix pipeline metaphor, where data flows through a series of transformations, each receiving the output of the previous one. The semantic contract of the pipe operator is that the right-hand side is always applied to the left-hand side, producing a new range without modifying the original. This is done using *proxy iterators*.

The first adaptor in the pipeline returns a range structure whose begin iterator will be a smart proxy iterator that points to the first element in the source collection that satisfies the given predicate. And the end iterator will be a proxy for the original collection’s end iterator. The only thing the proxy iterator needs to do differently than the iterator from the original collection is to point only at the elements that satisfy the given predicate.

In a nutshell, every time the proxy iterator is incremented, it needs to find the next element in the original collection that satisfies the predicate. With the proxy iterator, a new temporary collection need not be created. Only a new 'view' of the existing data is created. This new range is a lazy view — it wraps the original rather than copying it. This view, instead of showing original elements as they are, shows them processed. The next adaptor in the pipeline sees only this view.

Consider another example that extracts the names of first three females:
```cpp
std::vector<std::string> names = people | filter(is_female)
                                        | transform(name)
                                        | take(3);
```
The type of `names` is something like:
```
take_view
  → transform_view
     → filter_view
        → ref_view
            → vector
```
This is a nested type — a compile-time description of the computation.

This time, the results are stored instead of being comsumed directly, so the diagram now looks like this:
```
   ── At the assignment, NOTHING runs. ──────────────────────────────
   The pipeline itself is a lightweight object holding: a pointer to
   `people`, copies of `is_female` and `name`, and the limit (3).
   Zero elements are processed while building this recipe.

        +-------------------------------------------+
        |  pipeline = take_view{                    |
        |               transform_view{             |
        |                 filter_view{              |
        |                   ref_view{ &people },    |
        |                   is_female },            |
        |                 name },                   |
        |               3 }                         |
        +-------------------------------------------+
                     a recipe, not a result


   ── Later, when something iterates `names`: ──────────────────────
   The vector constructor pulls elements to fill itself. Each request
   travels UP the pipeline, and one name travels back DOWN.

        consumer  (std::vector construction)
              |  "next name, please"
              v
        +-------------+
        |   take(3)   |   checks count. If < 3, asks its source,
        |             |   otherwise signals the end.
        +-------------+
              |  "next name, please"
              v
        +-------------+
        | transform   |   asks its source for one Person,
        |   (name)    |   applies the name function to it.
        +-------------+
              |  "next Person, please"
              v
        +-------------+
        |   filter    |   asks its source repeatedly, testing is_female,
        | (is_female) |   until one passes.
        +-------------+
              |  "next Person, please"
              v
        +-------------+
        |   people    |   yields adam, then bea, then carl, ...
        +-------------+

   Trace for people = { adam(M), bea(F), carl(M), diana(F), evan(M), fiona(F), george(M) }:

     pull -> take(3) allows (count: 0)
          -> filter asks people: adam (M -> reject)
                                 bea  (F -> accept)
          -> transform calls name(bea)       -> vector receives "bea"

     pull -> take(3) allows (count: 1)
          -> filter asks people: carl (M -> reject)
                                 diana(F -> accept)
          -> transform calls name(diana)     -> vector receives "diana"

     pull -> take(3) allows (count: 2)
          -> filter asks people: evan (M -> reject)
                                 fiona(F -> accept)
          -> transform calls name(fiona)     -> vector receives "fiona"

     pull -> take(3) intercepts (count: 3 reached)
          -> signals exhausted without asking transform/filter
          -> pipeline ends. `george` is never even looked at!

   Verified call counts to extract exactly 3 names:
     is_female : 6   (stops instantly after fiona; george is skipped)
     name      : 3   (only the 3 accepted females are projected)
```

The flow is as follows:
1. When `people | filter(is_female)` is evaluated, nothing happens other than a new view being created. Not a single person is accessed from the `people` collection, except potentially to initialize the iterator to the source collection to point to the first item that satisfies the `is_female` predicate.
2. This view is passed to `| transform(name)`. The only thing that happens is that a new view is created. Again, neither a single person is accessed nor the `name` function is called on any of them.
3. Then, `| take(3)` is applied to that result. Again, it creates a new view and nothing else.
4. A vector of strings is constructed from the view which was obtained as the result of the `| take(3)` transformation. To create a vector, the values to put in must be known. This step goes through the view and accesses each of its elements. When the vector of names is to be constructed from the range, all the values in the range have to be evaluated. 

[![Screenshot-2026-04-14-at-7-51-49-AM.png](https://i.postimg.cc/pXY0yMb2/Screenshot-2026-04-14-at-7-51-49-AM.png)](https://postimg.cc/9r0PNSQS)
When accessing an element from the view, the view proxies the request to the next view in the composite transformation, or to the collection. Depending on the type of the view, it may
transform the result, skip elements, traverse them in a different order, and so on.

For each element added to the vector, the following things happen:
1. A dereference operator is called on the proxy iterator that belongs to the range view returned by take, i.e., `take_view::iterator::operator*()`. The proxy iterator created by take passes the request to the proxy iterator created by `transform`.
2. Which calls `transform_view::iterator::operator*()`. This iterator just passes on the request.
3. It calls `filter_view::iterator::operator*()`. The proxy iterator defined by the `filter` transformation is dereferenced. It goes through the source collection and finds and returns the first person that satisfies the `is_female` predicate. This is the first time any of the persons in the collection are accessed, and the first time the `is_female` function is called.
4. This iterator advances until predicate is satisfied. The person retrieved by dereferencing the `filter` proxy iterator is passed to the `name` function, and the result is returned to the `take` proxy iterator, which passes it on to be inserted into the `names` vector. When an element is inserted, it goes to the next one, and then the next one, until the end is reached. 
5. It finally calls `ref_view::iterator::operator*()`, which reads from the vector, where the final value flows back up through `transform`.

This is lazy evaluation at work. Even though the code is shorter and more generic than the equivalent handwritten for loop, it does exactly the same thing and has no performance penalties. Each element of the underlying container is processed on demand, one at a time, with the full pipeline fused together. The compiler typically inlines everything into a tight loop.

From the address‑space perspective:
* `names` owns a contiguous buffer.
* `filter_view` stores a pointer‑like view into this buffer (`m_base` ≈ `names.data()` and `names.size()`).
* `transform_view` stores only:
    * Another pointer‑like base iterator.
    * A small function‑object `fn` (likely just a `size_t`‑sized lambda).
* `take_view` stores:
    * A pointer‑like base iterator.
    * Two `size_t`s: `m_count`, `m_max`.

No extra storage is allocated for the intermediate sequences.

Instead, the machine code for
```cpp
for (int x : pipeline) { ... }
```
can be optimized into a loop that conceptually looks like:
```cpp
int count = 0;
for (auto it = v.begin(); it != v.end() && count < 3; ++it) {
    if (*it <= 0) continue;
    int x = *it * *it;
    // ... use x
    ++count;
}
```
The compiler inlines `filter_view::iterator::operator++()` and `transform_view::iterator::operator*()` across the adaptor boundaries, so the indirection cost is very low.

Even though the model is deep, the compiler:
* inlines `operator*()` and `operator++()` through the layers.
* recognize that `m_pred` and `m_fn` are simple functions and keep them inline.
* collapses the whole chain into a tight loop with no extra allocation per `|`.

This keeps the pipeline design lazy and composable, while giving owned storage where needed.

Views compute nothing when constructed. Work happens only when an iterator is advanced or dereferenced, and only for the elements actually reached. Elements are pulled through the pipeline **one at a time**, not stage by stage.

Chaining pipes creates a tree of wrapper objects rooted at the original range. Each layer adds a small constant amount of overhead per element access. The C++ optimizer can typically inline through all these layers and produce machine code that is nearly as efficient as a hand-written loop with all the logic inlined, particularly with modern compilers and optimization levels.

It must be kept in mind that the full type of a deeply composed pipeline can be extraordinarily long and complex. If a compilation error is generated in a pipeline expression, the error message will typically dump this full nested type, which can be hundreds or thousands of characters long. Learning to read these errors requires practice and a clear mental model of which adaptor corresponds to which layer. Working from the inside out makes these errors tractable. The `auto` keyword is essential when working with adapted ranges precisely because the types are so complex and unwriteable by hand. The `auto` deduced type will be the full nested (hidden) type, which is correct and efficient. Also, the lifetime of the original range must exceed the lifetime of the view.

### Materialization
A view is non‑owning, i.e., it does not store, copy, or manage the lifecycle of the actual data it processes. It merely acts as a "lens" or a reference to data in the range that already exists somewhere else in memory. Hence, it must be ensured that the underlying range outlives the view.
```cpp
auto make_view() {
    std::vector<int> local = {1, 2, 3};
    auto v = local | std::views::filter([](int x) { return x > 1; });
    return v;   // DANGER: local is destroyed when function returns
}

for (int x : make_view()) { ... } // Undefined behavior!
```
The code above demonstrates a "dangling view." When we try to use the result, the vector `v` is already gone, leading to undefined behavior.

To avoid this situation, either:
* Keep the base alive:
```cpp
std::vector<int> data = {1, 2, 3};
auto pipeline = data | std::views::filter(...);
for (int x : pipeline) { }   // OK
```
* Or store the result:
```cpp
std::vector<int> stored;
std::ranges::copy(pipeline, std::back_inserter(stored));
```

* Or materialize with `std::ranges::to` (Since C++23 and GCC v16)
```cpp
std::vector<int> vec;
// Works from C++23 and GCC v16 onwards
auto result_vec = std::ranges::to<std::vector<int>>(vec);
```
It deduces the element type, reserves capacity when the source is sized, and works recursively for nested containers. It is the correct default when available.

A view is not necessarily read-only. When the underlying reference type is a non-const lvalue reference, assignment through the view modifies the source:

```cpp
std::vector<int> w{1, 2, 3, 4, 5, 6};

for (int& x : w | rv::filter([](int n) { return n % 2 == 0; }))
    x = 0;
// w is now {1, 0, 3, 0, 5, 0}

rg::fill(w | rv::take(2), 7);
// w is now {7, 7, 3, 0, 5, 0}
```

Mutating *through* a `filter_view` in a way that changes whether elements satisfy the predicate is undefined behaviour — the standard explicitly forbids it. Writing `0` above is legal only because the code does not subsequently use that view.

#### `views::all`, `ref_view`, and `owning_view`

Piping a container into an adaptor first converts it to a view. That conversion is `rv::all`, and its result type is spelled `rg::views::all_t<R>`:

```
   lvalue container  ──rv::all──►  ref_view<C>      holds C*        (8 bytes, no copy)
   rvalue container  ──rv::all──►  owning_view<C>   holds C by move (takes ownership)
   already a view    ──rv::all──►  itself           (copied; O(1) by definition)
```
### The catalog of views
#### Factories — views built from nothing

Factories are called directly rather than piped into.

```cpp
rv::empty<int>             // an empty range of int
rv::single(42)             // exactly one element:            42
rv::iota(1, 6)             // half-open integer sequence:     1 2 3 4 5
rv::iota(1)                // UNBOUNDED: 1 2 3 4 ...
rv::repeat(7)              // C++23, unbounded:               7 7 7 ...
rv::repeat(7, 2)           // C++23, bounded:                 7 7
rv::istream<int>(stream)   // pulls ints from a stream until it fails
```

```
   iota(1, 6)          1 ─ 2 ─ 3 ─ 4 ─ 5 ─┤
   iota(1)             1 ─ 2 ─ 3 ─ 4 ─ 5 ─ 6 ─ ...  (no end; use take/take_while)
   repeat(7, 2)        7 ─ 7 ─┤
   single(42)          42 ─┤
   empty<int>          ┤
```

`rv::iota` works with any incrementable type, including iterators — `rv::iota(v.begin(),
v.end())` yields the iterators themselves, occasionally useful.

#### Selection — choosing which elements pass

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};

v | rv::take(3)                                       // 1 2 3
v | rv::take(99)                                      // 1 2 3 4 5 6 7 8   (clamps, no UB)
v | rv::drop(5)                                       // 6 7 8
v | rv::take_while([](int x) { return x < 5; })       // 1 2 3 4
v | rv::drop_while([](int x) { return x < 5; })       // 5 6 7 8
v | rv::filter([](int x) { return x % 2 == 0; })      // 2 4 6 8
v | rv::stride(3)                                     // 1 4 7          (C++23)
rv::counted(v.begin(), 3)                             // 1 2 3
```

```
   input        1  2  3  4  5  6  7  8

   take(3)      1  2  3
   drop(5)                        6  7  8
   take_while   1  2  3  4                  stops at the FIRST failure
   drop_while                  5  6  7  8   drops the leading run only
   filter          2     4     6     8      skips failures, keeps going
   stride(3)    1        4        7         every 3rd, always includes the first
```

The `take_while` / `filter` distinction repays attention. Given `{1, 2, 9, 3, 4}` and the
predicate `x < 5`, `take_while` yields `1 2` and stops at `9`; `filter` yields `1 2 3 4`.

#### Transformation — changing the elements

```cpp
v | rv::transform([](int x) { return x * x; })   // 1 4 9 16 25 36 49 64
v | rv::reverse                                  // 8 7 6 5 4 3 2 1

std::vector<std::string> words{"alpha", "beta", "gamma"};
words | rv::transform(&std::string::size)        // 5 4 5
```

`rv::transform` accepts anything invocable, including pointers-to-member, since it uses the
`std::invoke` protocol.

```
   transform(f)     x0      x1      x2
                     |       |       |
                    f(x0)   f(x1)   f(x2)      element count unchanged

   reverse          x0  x1  x2   ──►   x2  x1  x0   (requires bidirectional)
```

#### Flattening and splitting

```cpp
std::vector<std::vector<int>> nested{{1, 2}, {}, {3, 4, 5}};
nested | rv::join                                    // 1 2 3 4 5

std::vector<std::string> words{"alpha", "beta", "gamma"};
words | rv::join_with('-')       // C++23; yields CHARS: a l p h a - b e t a - g a m m a

std::string_view{"ab,cd,ef"} | rv::split(',')        // {ab} {cd} {ef}
std::string_view{"a::b::c"} | rv::split(std::string_view{"::"})   // {a} {b} {c}
std::string_view{"ab,cd"} | rv::lazy_split(',')      // {ab} {cd}
```

```
   join          [ [1,2] , [] , [3,4,5] ]  ──►  1 2 3 4 5
                        one level removed; empty inner ranges vanish

   join_with('-')  [ "ab", "cd" ]  ──►  a b - c d      (separator interleaved)

   split(',')    "ab,cd,ef"  ──►  ["ab"] ["cd"] ["ef"]
                 each token is a SUBRANGE, not a string or string_view
```

Two important details.

**Tokens are subranges, not strings.** Converting is a two-step incantation that appears in
essentially every real use:

```cpp
for (auto token : text | rv::split(',')) {
    std::string_view sv(token.begin(), token.end());   // works in C++20 and later
    // C++23 also allows: std::string_view sv(token);
}
```

**`split` versus `lazy_split`.** As originally standardised in C++20, `split_view` produced
tokens that were themselves lazy ranges — nearly unusable for the common case of splitting a
string. Paper P2210R2 fixed this *as a defect report*, retargeting `views::split` to produce
subranges over forward ranges, and renaming the original behaviour to `views::lazy_split`.
Use `split` by default; `lazy_split` is for input ranges that cannot be re-traversed.

#### Multi-range adaptors (C++23)

```cpp
std::vector<std::string> words{"alpha", "beta", "gamma"};
std::vector<int> scores{90, 80, 70};

rv::zip(words, scores)                        // alpha=90 beta=80 gamma=70
rv::zip_transform(std::plus<>{}, v, v)        // 2 4 6 8 10 12 14 16
words | rv::enumerate                         // 0:alpha 1:beta 2:gamma
v | rv::adjacent<2>                           // (1,2) (2,3) (3,4) (4,5) ...
v | rv::adjacent_transform<2>(std::plus<>{})  // 3 5 7 9 11 13 15
rv::cartesian_product(std::vector{1,2}, std::string_view{"xy"})   // 1x 1y 2x 2y
```

```
   zip           A:  a0  a1  a2                 length = min(lengths)
                 B:  b0  b1                     ──►  (a0,b0) (a1,b1)
                                                     a2 is dropped

   enumerate         x0     x1     x2
                 (0,x0) (1,x1) (2,x2)           index is a ptrdiff_t

   adjacent<2>   x0  x1  x2  x3
                 └──┘
                     └──┘
                         └──┘                   n-1 overlapping pairs

   cartesian_product({1,2}, {x,y})
                 1x  1y  2x  2y                 LAST range varies fastest
```

`rv::zip` stops at the shortest input, which silently discards trailing elements of longer
ranges — usually desirable, occasionally a bug. `rv::pairwise` is a synonym for
`rv::adjacent<2>`, and `rv::pairwise_transform` for `rv::adjacent_transform<2>`.

#### Grouping (C++23)

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
v | rv::chunk(3)                              // {123} {456} {78}
v | rv::slide(3)                              // {123} {234} {345} {456} {567} {678}

std::vector<int> runs{1, 1, 2, 2, 2, 3};
runs | rv::chunk_by(std::equal_to<>{})        // {11} {222} {3}

std::vector<int> asc{1, 2, 5, 3, 4, 9};
asc | rv::chunk_by(std::less_equal<>{})       // {125} {349}   (ascending runs)
```

```
   chunk(3)      1 2 3 | 4 5 6 | 7 8          disjoint; last may be short
   slide(3)      1 2 3
                   2 3 4
                     3 4 5 ...                overlapping windows, all full size

   chunk_by(p)   a new group starts wherever p(prev, curr) is FALSE
                 {1,1,2,2,2,3} with equal_to  ──►  {1,1} {2,2,2} {3}
```

`chunk_by` groups only *adjacent* elements. To group by a key the way a database `GROUP BY`
does, sort by that key first — see the recipe in section 9.4.

#### Tuple-like element access

```cpp
std::map<std::string, int> m{{"a", 1}, {"b", 2}};
m | rv::keys              // a b
m | rv::values            // 1 2

std::vector<std::pair<int, char>> ps{{1, 'x'}, {2, 'y'}};
ps | rv::elements<1>      // x y
```

`rv::keys` and `rv::values` are aliases for `rv::elements<0>` and `rv::elements<1>`.

#### Conversion adaptors

```cpp
auto tw = v | rv::take_while([](int x) { return x < 4; });
// rg::common_range<decltype(tw)>              is false
// rg::common_range<decltype(tw | rv::common)> is true

v | rv::as_const     // C++23; reference type becomes int const&
v | rv::as_rvalue    // C++23; reference type becomes int&& — for moving elements out
```

`rv::common` exists solely to adapt a range for a legacy API requiring `begin()` and `end()`
of the same type. It costs a runtime branch on some ranges, so it should be applied at the
boundary rather than habitually.

Sources:
* https://www.youtube.com/watch?v=HYENjkZvsrM
* https://www.youtube.com/watch?v=Rbl3h0RJuuY
* https://www.youtube.com/watch?v=Q434UHWRzI0
* https://www.youtube.com/watch?v=5iXUCcFP6H4
* Functional Programming in C++ by Ivan Cukic
