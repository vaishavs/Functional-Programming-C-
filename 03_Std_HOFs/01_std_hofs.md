# Higher-Order Functions in the C++ Standard Library

A large part of the C++ standard library is built on a single idea: a function can be given to another function as data. Once that is possible, a routine such as "walk this sequence and keep the elements matching a rule" need not know what the rule is, so one implementation of `find_if` serves every rule anyone will ever write. Functions that work this way are called **higher-order functions**, and they account for most of `<algorithm>`, all of `<numeric>`, the ordering behaviour of the associative containers, the whole of `std::ranges`, and a scattering of utilities elsewhere.

A function is higher-order when it does at least one of two things: it **takes** one or more functions as arguments, or it **returns** a function. A function that does neither is called first-order, and the great majority of ordinary code consists of first-order functions.

The word "function" is used loosely above, because C++ has several distinct things that can be called. A higher-order function in this library accepts all of them, and the reason is worth understanding before the catalogue begins.

## Components of standard HOFs
The implementation of standard higher order functions in C++ is based on the following components:
### Callable entities
These are the function types such as function pointers, functors, lambdas, etc., that are passed around (see [03_HOF.md](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/03_HOF.md)). They are used to provide specialized functional logic, such as:
* Predicate: A function that takes one or two arguments and returns a bool.
* Generator: A function that takes no arguments and returns values to fill a range.
* Operator: A function that produces specific results based on the inputs provided.
* Execution policy: A function that makes use of multi-core processors. (Since C++17)
### Execution Contexts
These are the containers or ranges on which various operations are performed. They are:
* Containers → `std::vector`, `std::list`, `std::map`, etc.
* Iterators → Mechanism to access elements of a container/range, e.g., `std::iterator`, etc.
* Ranges → More expressive composition of HOFs that eliminate the need to manually pass begin/end iterators. (Since C++20)
* Seed values → Initial values needed as starting point for certain callables, e.g., for reductions (`std::accumulate` needs a starting value).
### Algorithms/wrappers
Algorithms are standard higher-order functions that accept callables and apply them on containers/ranges. These are the wrappers that define the logic for how an operation should be executed. They apply the callable on the data. They can be categorized in the following manner:
| Category |	Typical Logic Type|	Purpose	|Example Algorithm| Signature Expectation |
|--------- | ------------------ | ------- | --------------- | --------------------- |
| Non-Modifying |	Predicate(Unary/Binary) |	Searching & Validation |	`find_if`, `all_of` | `(T) -> bool` |
| Modifying |	Unary/Binary Op |	Transformation |	`transform`, `generate` | `(T) -> U` |
| Sorting |	Comparator (Binary predicate) |	Custom Ordering |	`sort`, `stable_sort` | `(Acc, T) -> Acc` |
| Numeric |	Binary Op |	Accumulation/Reduction |	`accumulate`, `reduce` | `(T, T) -> bool` |
| Parallel |	Execution Policy |	Threading/Concurrency | `seq`, `par`, `par_unseq` |	`() -> T` |

To illustrate diagrammatically:

[![Copilot-20260214-125707.png](https://i.postimg.cc/RVWj6PPY/Copilot-20260214-125707.png)](https://postimg.cc/D8KCRc3Q)

### Workflow
A standard HOF typically follows this structure:
```
algorithm(execution_policy?, range_begin, range_end, seed?, callable);
```
For example:
```cpp
int sum = std::accumulate(
    vec.begin(),
    vec.end(),
    0,
    [](int a, int b) { return a + b; }
);
```
Here:
* Execution context → `vec.begin()`, `vec.end()`
* Seed value → `0`
* Callable (operator) → `[](int a, int b) { return a + b; }`
* Algorithm (HOF) → `std::accumulate`

Standard HOFs in C++ are built on:
* Behavior (Callable entities): Defines what operation should be performed.
* Data (Containers, iterators, ranges): Defines where the operation is applied.
* Control (Execution policy & seed values): Defines how and from what starting point the operation is executed (see [Control parameters](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/03_Std_HOFs/02_note.md#14-control-parameters)).

### What can be passed

The standard defines a single operation, named INVOKE, that describes how to call any callable. Every higher-order facility in the library is specified in terms of INVOKE rather than in terms of ordinary call syntax, which is why the same algorithm accepts all of the following without any overloading on the library's part.

| Kind of callable | Example | Carries state? |
|---|---|---|
| Function pointer | `bool is_even(int);` passed as `is_even` | No |
| Lambda without captures | `[](int x){ return x % 2 == 0; }` | No |
| Lambda with captures | `[k](int x){ return x % k == 0; }` | Yes |
| Function object, or functor | `struct Div { int k; bool operator()(int) const; };` | Yes |
| Standard function object | `std::less<>{}`, `std::plus<>{}` | No |
| Pointer to member function | `&Widget::scale` | No, the object is passed separately |
| Pointer to data member | `&Person::age` | No, and the call is a read |
| Type-erased wrapper | `std::function<bool(int)>` | Yes, and it owns the target |

The last two rows deserve a note. A pointer to a member cannot be called with plain `f(args)` syntax at all, yet algorithms accept it, because INVOKE supplies the rule that the object comes first. A pointer to a *data* member is not a function in any ordinary sense, and INVOKE nonetheless treats reading it as a call, which is what allows `&Person::age` to be handed to a ranges algorithm as a key extractor.

## Types of standard HOFs
```
                        HIGHER-ORDER FUNCTIONS
                                  │
             ┌────────────────────┴────────────────────┐
             │                                         │
      TAKES a callable                          RETURNS a callable
             │                                         │
   ┌─────────┴─────────┐                     ┌─────────┴─────────┐
   │                   │                     │                   │
std::find_if      std::sort            std::bind_front    std::views::filter
std::transform    std::visit           std::not_fn        std::mem_fn
std::accumulate   std::thread          std::bind          (adaptor closures)
   │                                                             │
   └──────────────────────► BOTH ◄──────────────────────────────┘
                              │
                    std::views::transform(f)
```
This diagram sorts C++'s higher-order functions according to the role a callable plays in a function's signature: whether it is passed in as an argument, returned as a result, or both.

#### Takes a callable
This group invokes the callable passed to it, typically while iterating over data. For instance, `std::find_if` calls its predicate once per element until one returns true, `std::transform` applies a unary or binary operation to elements as they are written to an output range, `std::accumulate` folds a binary operation across a range to produce a single value, `std::sort` calls a comparator to establish ordering, `std::visit` calls a visitor with whichever alternative is currently active in a `std::variant`, and `std::thread`'s constructor takes a callable and runs it on a new thread. In each case, the callable is executed directly; what comes back is a value, an iterator, or a thread handle — never a new function.

#### Returns a callable
This group does not execute anything directly; instead, it produces new callables for later use. For instance, `std::bind` and `std::bind_front` take a callable plus some arguments and return a new callable with those arguments already bound in, the `std::not_fn` takes a predicate and returns one that negates it, `std::mem_fn` takes a pointer to a member function or member variable — neither directly invocable with `()` on its own — and wraps it into an ordinary callable object, and `std::views::filter`, given a predicate, returns a *range adaptor closure*: a callable that has not yet been applied to any data. Each of these acts as a factory; the role is to package a callable, not to invoke it.

#### Both
The `std::views::transform` is an interesting case that takes as well as returns a callable. It involves two related calls. The outer call — `std::views::transform(f)` — takes a callable, `f`, as its argument, matching the defining trait of the "takes a callable" group. Rather than invoking `f`, however, it returns a closure object, matching the defining trait of the "returns a callable" group. That closure is itself callable. Because the outer call both accepts a callable and produces one, `std::views::transform` satisfies both categories at once.

The underlying pattern reflects standard functional-programming vocabulary applied to C++: the "takes" group consists of ordinary higher-order algorithms; the "returns" group consists of combinators for partial application and adaptation (`bind_front`, `not_fn`) or for wrapping non-callable-looking constructs into callables (`mem_fn`); the range adaptors build on that combinator machinery so they can compose using `|`.

## Category one — facilities that consume a callable

This is by far the largest group. The callable arrives as an ordinary function argument, and the algorithm calls it once per element or once per pair of elements.

### Predicates: algorithms that ask a yes-or-no question

A predicate is a callable returning something usable as `bool`. These algorithms differ only in what they do with the answer.

```cpp
std::vector<int> v{5, 2, 8, 1, 9, 4};
auto is_even = [](int x){ return x % 2 == 0; };

*std::find_if(v.begin(), v.end(), is_even);                 // 2, the first match
std::count_if(v.begin(), v.end(), is_even);                 // 3
std::all_of (v.begin(), v.end(), is_even);                  // false
std::any_of (v.begin(), v.end(), is_even);                  // true
std::none_of(v.begin(), v.end(), is_even);                  // false

std::vector<int> evens;
std::copy_if(v.begin(), v.end(), std::back_inserter(evens), is_even);   // 2 8 4

auto mid = std::partition(v.begin(), v.end(), is_even);      // 4 2 8 | 1 9 5
```

`std::partition` is worth singling out because it returns an iterator to the boundary between the elements that satisfied the predicate and those that did not, which makes it the basis of several other operations including the erase-remove idiom discussed among the pitfalls.

### Transformations: algorithms that map values

```cpp
std::vector<int> squares;
std::transform(v.begin(), v.end(), std::back_inserter(squares),
               [](int x){ return x * x; });                  // 25 4 64 1 81 16

std::vector<int> a{1,2,3}, b{10,20,30}, sums(3);
std::transform(a.begin(), a.end(), b.begin(), sums.begin(), std::plus<>{});   // 11 22 33

std::vector<int> gen(4);
int seed = 0;
std::generate(gen.begin(), gen.end(), [&seed]{ return seed += 2; });          // 2 4 6 8
```

The two-range form of `std::transform` takes a *binary* callable, and `std::generate` takes a callable of *no* arguments. Matching the arity of the callable to the algorithm is the single most common compile error in this area, and the pitfall section returns to it.

### Comparators: algorithms that impose an order

A comparator is a binary predicate answering whether its first argument must come before its second.

```cpp
std::sort(v.begin(), v.end(), std::greater<>{});             // 9 8 5 4 2 1

std::vector<std::string> words{"pear", "fig", "banana"};
std::sort(words.begin(), words.end(),
          [](const auto& x, const auto& y){ return x.size() < y.size(); });   // fig pear banana

*std::max_element(v.begin(), v.end());                        // 9
```

Every ordering algorithm in the library takes an optional comparator, including `std::stable_sort`, `std::nth_element`, `std::lower_bound`, `std::merge`, the heap operations, and the set operations. When the comparator is omitted, `operator<` is used directly.

### Folds: algorithms that reduce a sequence to one value

```cpp
std::accumulate(v.begin(), v.end(), 0);                                   // 29
std::accumulate(v.begin(), v.end(), 1, std::multiplies<>{});              // 2880
std::reduce(v.begin(), v.end(), 0);                                       // 29, may reorder
std::transform_reduce(v.begin(), v.end(), 0, std::plus<>{},
                      [](int x){ return x * x; });                        // 191
std::inner_product(a.begin(), a.end(), b.begin(), 0);                     // 140

std::vector<int> running(v.size());
std::partial_sum(v.begin(), v.end(), running.begin());                    // 5 7 15 16 25 29
```

`std::transform_reduce` deserves attention as the clearest example of a facility taking *two* callables at once: a unary operation applied to each element, and a binary operation folding the results. It also fuses the two passes that a `transform` followed by an `accumulate` would require, so no intermediate container is allocated.

The difference between `std::accumulate` and `std::reduce` is that the latter does not promise left-to-right order and may therefore be parallelised, which means the binary operation given to `reduce` must be associative and commutative while the one given to `accumulate` need not be.

### Traversal

```cpp
std::for_each(v.begin(), v.end(), [](int x){ std::cout << x << " "; });
std::for_each_n(v.begin(), 3, [](int x){ std::cout << x << " "; });
```

`std::for_each` has one property no other algorithm shares: it *returns* the callable it was given, by value, after the traversal. That is what allows state accumulated inside a functor to be recovered, as the pitfall section demonstrates.

### Containers that take a callable as a template argument

The associative containers are higher-order in a different way. The callable is not passed as a function argument but supplied as a **template argument**, which means the container's very type records how it orders or hashes its elements.

```
   ALGORITHM                            CONTAINER
   ─────────                            ─────────
   std::sort(first, last, cmp)          std::set<Key, Compare>
                        ▲                              ▲
                        │                              │
        a VALUE parameter, chosen               a TEMPLATE parameter, fixed
        at each call site                       for the container's lifetime
```

```cpp
std::set<int, std::greater<int>> descending(v.begin(), v.end());       // 9 8 5 4 2 1
std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap; // top() is the smallest

struct CaseInsensitive {
    bool operator()(const std::string& x, const std::string& y) const;
};
std::map<std::string, int, CaseInsensitive> ci{{"Apple", 1}};
ci.find("APPLE") != ci.end();                                           // true

struct PairHash {
    std::size_t operator()(const std::pair<int,int>& p) const {
        return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
    }
};
std::unordered_map<std::pair<int,int>, int, PairHash> um{{{1,2}, 3}};
um.at({1,2});                                                            // 3
```

The unordered containers take *two* callables in this way, a hash function and an equality predicate, because both are needed to place and find an element. A consequence of the template-argument form is that two `std::set<int>` objects with different comparator types are different types and cannot be assigned to one another, which is occasionally surprising and is the price of the ordering being known at compile time.

### Dispatch on a run-time type

`std::visit` takes a callable and applies it to whichever alternative a `std::variant` currently holds, choosing at run time and checking at compile time that every alternative is handled.

```cpp
std::variant<int, std::string, double> var = std::string("text");

std::string kind = std::visit([](const auto& x) -> std::string {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<T, int>)              return "int";
    else if constexpr (std::is_same_v<T, std::string>) return "string";
    else                                               return "double";
}, var);                                                  // "string"
```

### Concurrency facilities

Several facilities take a callable in order to run it somewhere or at some time other than the current point of execution.

```cpp
std::thread t([&counter]{ ++counter; });                 t.join();
auto fut = std::async(std::launch::async, [](int x){ return x * 2; }, 21);   // fut.get() is 42
std::once_flag flag;
std::call_once(flag, [&inits]{ ++inits; });              // runs exactly once across all callers
```

These differ from the algorithms in one important respect covered under the pitfalls: they **copy** their arguments before invoking, so sharing an object with the callable requires `std::ref`.

### Ranges algorithms

The C++20 ranges algorithms are the same higher-order functions restated with concepts and with an extra facility. Each accepts a whole range instead of an iterator pair, constrains its callable parameters with concepts such as `std::predicate` and `std::strict_weak_order`, and accepts an optional **projection**, which is a second callable applied to each element before the primary callable sees it.

```cpp
struct Person { std::string name; int age; };
std::vector<Person> people{{"Ann",30}, {"Bo",25}, {"Cy",41}};

std::ranges::sort(people, {}, &Person::age);      // Bo(25) Ann(30) Cy(41)
std::ranges::count_if(v, [](int x){ return x > 4; });   // 3
```

The second argument above is the comparator, left as `{}` to mean the default, and the third is the projection. Because a pointer to a data member is callable under INVOKE, `&Person::age` serves directly as "sort by this field" without a comparator lambda being written at all.

## Category two — facilities that produce a callable

A smaller group takes a callable and hands back a new one. These are higher-order in the returning sense, and most of them perform *partial application*, meaning that some arguments are fixed in advance and the rest are supplied later.

```
   PARTIAL APPLICATION

   sub(a, b)  ──── a two-argument function
      │
      │  std::bind_front(sub, 100)
      ▼
   g(b)       ──── a one-argument callable, with a fixed at 100
      │
      │  g(40)
      ▼
     60       ──── the same as sub(100, 40)
```

```cpp
int sub(int a, int b) { return a - b; }

auto from100 = std::bind_front(sub, 100);          // C++20
from100(40);                                        // 60

using namespace std::placeholders;
auto flipped = std::bind(sub, _2, _1);              // C++11, and able to reorder
flipped(3, 10);                                     // 7, because it computes sub(10, 3)

auto not_pos = std::not_fn([](int x){ return x > 0; });   // C++17
not_pos(-2);                                        // true

auto scale = std::mem_fn(&Widget::scale);           // wraps a member pointer as a callable
Widget w;
scale(w, 3);                                        // 21, with the object passed first
```

`std::bind_back`, the mirror image of `bind_front` that fixes the *trailing* arguments, was added in C++23 and is not present in g++ 13.3.

Among these, `std::bind_front` and `std::bind_back` are the ones to prefer in new code. `std::bind` is more powerful because its placeholders can reorder arguments, but it copies its bound arguments eagerly, composes awkwardly, and produces dense diagnostics, so a lambda is usually clearer for anything beyond fixing leading or trailing arguments.

### Range adaptor closures

The range adaptors are the most-used callable-returning facilities in the library, although the fact is easy to miss because the result is usually consumed immediately by `operator|`.

```cpp
auto closure = std::views::filter([](int x){ return x % 2 == 0; });   // a callable, not a view
for (int x : v | closure) { /* 2 8 4 */ }
```

`std::views::filter(pred)` does not adapt anything on its own, because no range has been supplied yet. It returns a *closure object* that is waiting for one, and `range | closure` is defined to mean `closure(range)`. That equivalence is the whole of the pipe machinery.

## Category three — facilities that both consume and produce

The range adaptors belong to both categories at once, which is what makes pipelines composable.

```
   views::transform(f)          takes a callable f
            │
            ▼
      closure object            returns a callable
            │
            │   rng | closure   ==   closure(rng)
            ▼
      transform_view            returns a lazy view, itself a range
            │
            │   which can be piped into the next closure
            ▼
        ... and so on
```

```cpp
auto pipeline = v | std::views::filter([](int x){ return x % 2 == 0; })
                  | std::views::transform([](int x){ return x * 10; });
```

Nothing in that statement computes anything. Each adaptor takes a callable, returns a closure, and the closure returns a view that stores the callable for later. Work happens only when the pipeline is iterated, and an element that a `filter` rejects never reaches the `transform` at all.

## The vocabulary that ties the categories together

Three pieces of the library exist to serve the higher-order facilities rather than to be used directly very often.

**`std::invoke`** is the uniform way to call any callable, and is what the library's own specifications are written in terms of. Generic code that must call whatever it was handed should use it, because it removes the special case for member pointers.

**The invocability traits and concepts** answer, at compile time, whether a call is well-formed. `std::is_invocable_v<F, Args...>` is the trait form, and `std::invocable<F, Args...>`, `std::predicate<F, Args...>`, and `std::strict_weak_order<R, T, T>` are the concept forms used to constrain a higher-order function's own parameters.

**The standard function objects** are ready-made arguments for the facilities above, saving a lambda for the common cases.

| Family | Members | Typical use |
|---|---|---|
| Arithmetic | `plus`, `minus`, `multiplies`, `divides`, `modulus`, `negate` | the operation given to a fold |
| Comparison | `less`, `greater`, `less_equal`, `greater_equal`, `equal_to`, `not_equal_to` | the comparator given to a sort or container |
| Logical | `logical_and`, `logical_or`, `logical_not` | combining boolean results |
| Bitwise | `bit_and`, `bit_or`, `bit_xor`, `bit_not` | bit manipulation in a fold |
| Identity | `std::identity` | the default projection in ranges |

Written with empty angle brackets, as `std::less<>`, these become *transparent*, meaning their call operator is a template that accepts mixed operand types. For an associative container keyed by `std::string`, a transparent comparator additionally enables heterogeneous lookup, so a search with a `const char*` or a `std::string_view` does not construct a temporary `std::string`.

## How the callable is passed, and what it costs

Two mechanisms dominate, and the difference between them is measurable.

When the callable arrives as a **template parameter**, its concrete type is known at compile time, so the compiler can inline the call. This is how every standard algorithm and every container comparator receives it. When the callable arrives as a **`std::function`**, the type has been erased, so the call is indirect and generally cannot be inlined.

```cpp
template <class Pred>
long long count_template(const std::vector<int>& v, Pred p);          // inlines

long long count_erased(const std::vector<int>& v,
                       const std::function<bool(int)>& p);            // indirect call
```

Counting the multiples of three in a twenty-million-element vector, compiled with `-O2`, the two forms measured 17 to 19 milliseconds and 29 to 30 milliseconds respectively across repeated runs, so the erased form cost roughly one and a half times as much. The ratio is not dramatic, and it is a poor reason to contort an interface, but it explains why the library takes callables as template parameters everywhere it can.

Storage size follows the same pattern.

```cpp
sizeof(a captureless lambda);        // 1, the minimum an object may occupy
sizeof(std::function<bool(int)>);    // 32
```

A stateless comparator additionally costs a container nothing, because the empty base optimization allows it to occupy no storage of its own, whereas a comparator carrying data enlarges every container object that uses it.

```cpp
sizeof(std::set<int, Empty>);        // 48, the same as std::set<int>
sizeof(std::set<int, SmallState>);   // 48, the state fitting in existing padding
sizeof(std::set<int, BigState>);     // 104, where BigState is 64 bytes
```

## Common pitfalls and their solutions

### An algorithm may copy the callable, so accumulated state is lost

Algorithms take their callables **by value**, and are permitted to copy them freely. A functor that counts its own invocations therefore reports nothing to the caller.

```cpp
struct Counting {
    int calls = 0;
    bool operator()(int x) { ++calls; return x % 2 == 0; }
};

Counting c;
std::count_if(v.begin(), v.end(), c);
// the result is correct, but c.calls is still 0, because count_if worked on a copy
```

**Solution.** Two remedies exist, and the choice depends on which algorithm is involved. `std::for_each` is specified to return the callable it was given, so state can be recovered from the returned object directly, and that is the reason the return value exists at all.

```cpp
Counting back = std::for_each(v.begin(), v.end(), Counting{});
back.calls;      // 6, recovered from the returned functor
```

For every other algorithm, wrapping the callable in `std::ref` passes a reference wrapper by value, and the wrapper forwards each call to the original object, so the state accumulates where the caller can see it.

```cpp
Counting shared;
std::count_if(v.begin(), v.end(), std::ref(shared));
shared.calls;    // 6, because the calls reached the original
```

The deeper lesson is that a callable given to an algorithm should normally be **pure**, meaning that its result depends only on its arguments. A pure callable is unaffected by copying, by the order of evaluation, and by an algorithm calling it more times than expected, none of which are guaranteed.

### `remove_if` does not remove anything

The algorithms see only iterators and never the container, so no algorithm can change a container's size. `std::remove_if` shuffles the survivors to the front and returns the new logical end, leaving the tail in a valid but unspecified state.

```cpp
std::remove_if(r.begin(), r.end(), is_even);
r.size();        // still 6, and the tail holds unspecified values
```

**Solution.** The container has to be told to shrink, which is what the erase-remove idiom does by feeding the returned iterator to `erase`.

```cpp
r2.erase(std::remove_if(r2.begin(), r2.end(), is_even), r2.end());
r2.size();       // 3
```

C++20 adds a free function that performs both halves and takes the container directly, which is clearer and harder to get wrong.

```cpp
std::erase_if(r3, is_even);
r3.size();       // 3
```

### A comparator must be a strict weak order

Every ordering facility requires the comparator to be a *strict weak ordering*, of which the most commonly violated requirement is irreflexivity: `cmp(x, x)` must be `false`. Writing `<=` instead of `<` violates it, and the result is undefined behaviour rather than a merely wrong order.

```cpp
std::sort(v.begin(), v.end(), [](int a, int b){ return a <= b; });   // undefined behaviour
```

**Solution.** The comparator should be written with a strict comparison, and where several keys are compared in sequence, only the last should fall through to equality.

```cpp
std::sort(v.begin(), v.end(), [](int a, int b){ return a < b; });

std::sort(people.begin(), people.end(), [](const Person& x, const Person& y){
    if (x.age != y.age) return x.age < y.age;      // strict on the first key
    return x.name < y.name;                         // and strict on the tie-breaker
});
```

The failure mode is worth knowing because it is so quiet. In a plain build the sort above completed without incident and produced a plausible-looking result, whereas the same program compiled with `-D_GLIBCXX_DEBUG` aborted immediately inside `std::sort` with a diagnostic naming the invalid comparator. Enabling that mode in test builds turns an intermittent crash into a reproducible message.

### The arity of the callable must match what the algorithm calls it with

An algorithm calls its callable with a fixed number of arguments, and supplying a callable of the wrong arity fails to compile, sometimes at length.

```cpp
std::count_if(v.begin(), v.end(), std::greater<int>{});
// error: no match for call to '(std::greater<int>) (int&)'
```

`std::greater` is a *comparator*, taking two arguments, whereas `count_if` calls its callable with one.

**Solution.** The rule of thumb is to check what the algorithm's name implies about the call. A predicate for `count_if`, `find_if`, `copy_if`, `all_of`, and `partition` takes one element. A comparator for `sort`, `max_element`, and the container template argument takes two. The operation for the two-range `transform` takes two, while the operation for the one-range form takes one, and `generate` takes none.

```cpp
std::count_if(v.begin(), v.end(), [](int x){ return x > 3; });        // a unary predicate
std::sort(v.begin(), v.end(), std::greater<int>{});                    // a binary comparator
```

Where a binary comparator genuinely is the right logic, it becomes a unary predicate by fixing one operand, which is exactly what the callable-returning facilities are for.

```cpp
auto above_three = std::bind_front(std::less<int>{}, 3);   // yields 3 < x
std::count_if(v.begin(), v.end(), above_three);
```

### An overload set cannot be passed by name

A name referring to several functions selects none of them, so it cannot be deduced as a template argument.

```cpp
int    square(int);
double square(double);
std::transform(v.begin(), v.end(), out.begin(), square);
// error: no matching function for call to 'transform(...)' — unresolved overloaded function type
```

**Solution.** Wrapping the name in a lambda defers the choice to the point of call, which keeps the entire overload set available so that each element selects the matching overload.

```cpp
std::transform(v.begin(), v.end(), out.begin(), [](auto x){ return square(x); });
```

An explicit cast to the wanted function pointer type also works and documents which overload was intended, at the cost of needing revision whenever the overload set changes. The same considerations apply to function templates, whose names are likewise unresolved until arguments are supplied.

### Concurrency facilities copy their arguments

`std::thread` and `std::async` decay-copy every argument before invoking the callable, so a function taking a reference parameter will not compile unless sharing is requested.

```cpp
void worker(int& counter) { ++counter; }
std::thread bad(worker, counter);
// error: static assertion failed: std::thread arguments must be invocable after conversion to rvalues
```

**Solution.** `std::ref` states that the original object is to be shared, and the wrapper is unwrapped when the call is finally made.

```cpp
std::thread good(worker, std::ref(counter));
good.join();
```

The copying is deliberate rather than an oversight, because the new thread may outlive the scope where its arguments were created, and a silent reference to a destroyed object would be far harder to diagnose than a compile error. The obligation that comes with `std::ref` is that the shared object must outlive the thread, which in practice means joining before it goes out of scope.

### A stored callable may outlive what it captured

A lambda captured by reference is safe only while the referent lives, and a higher-order facility that *stores* the callable rather than calling it immediately makes that easy to get wrong.

```cpp
std::function<int(int)> make_adder() {
    int base = 1;
    return [&base](int x){ return x + base; };   // base dies when the function returns
}
```

**Solution.** A callable that is stored, returned, or handed to another thread should capture by value, so that it carries what it needs.

```cpp
std::function<int(int)> make_adder() {
    int base = 1;
    return [base](int x){ return x + base; };    // the value is copied into the closure
}
```

Capture by reference remains correct and preferable for a callable that is consumed immediately, which describes every algorithm call, because the referent cannot go out of scope while the algorithm runs.

### Laziness changes how often a callable runs

In a ranges pipeline nothing is computed until the result is iterated, and the number of calls depends on the order of the adaptors rather than on the number of elements.

```cpp
auto pipeline = v | std::views::transform(expensive)
                  | std::views::filter([](int y){ return y % 2 == 0; });
```

A `filter` downstream of a `transform` must evaluate the transform once to test the predicate and again to yield the element, so the expensive function runs about three times more often than the reverse ordering requires.

**Solution.** Where the predicate can be expressed in terms of the untransformed input, filtering first does the same work on fewer elements and evaluates the expensive operation only for those that survive.

```cpp
auto pipeline = v | std::views::filter([](int y){ return y % 2 == 0; })
                  | std::views::transform(expensive);
```

The same reasoning explains why a lazy pipeline should be materialised into a container when it will be traversed more than once, since each traversal re-runs every stage.

### Four habits that prevent most trouble

1. Keep callables pure, because an algorithm may copy them, may call them more often than expected, and in the case of `std::reduce` may call them in an unspecified order.
2. Match the arity to the algorithm, remembering that a predicate takes one argument and a comparator takes two, and that `std::greater` is a comparator rather than a predicate.
3. Write comparators with strict comparisons, and to enable `-D_GLIBCXX_DEBUG` in test builds so that a violated ordering is reported rather than left to corrupt memory quietly.
4. Prefer passing callables as template parameters, which is what the standard facilities do, and to reach for `std::function` only where a fixed, named type is genuinely required.

Sources:
* https://www.youtube.com/watch?v=kcBlSmo3Xlk
* https://train.rse.ox.ac.uk/material/HPCu/software_architecture_and_design/functional/higher_order_functions_cpp
* https://kastner.ucsd.edu/wp-content/uploads/2013/08/admin/tcad18-hofs.pdf
