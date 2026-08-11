# The box model Functor
A *box* functor (**not** a [callable object](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#functors)) is a *type constructor*. 

A functor takes a type `T` to a new type `F<T>` — paired with one operation (`transform` in C++, `map`/`fmap` elsewhere). This operation takes a function `A → B` and an `F<A>` and produces `F<B>`, applying the function to whatever sits inside while leaving the box's *structure* untouched. In other words, given any function `f : A → B`, it produces a function `transform(f) : F<A> → F<B>`.

In simpler terms, consider a template class `F`. This template class is considered a functor if it defines a `map` or `transform` method. This method should take two arguments:

1. A value of type `F<T1>`.
2. A function t that transforms a value of type `T1` to a value of type `T2`.

The `map`/`transform` method then applies the function to the value inside the structure `F` and returns a new structure `F<T2>` with the transformed value.

The operation every functor must provide looks like this:

```
transform : (A → B)  applied to  F<A>   yields   F<B>
```
In C++, it looks something like this:
```cpp
template
    template<typename> class Context,
    typename Function,
    typename Input
>
auto transform(Function fn, Context<Input> value);
// Function fn is defined as:
// [](Input x) -> Output { ... }
```
Meaning:
```
transform:    (Input -> Output)            -> Context<Input> -> Context<Output>
                   no Context
```
The function is bare: `Input -> Output`. It is not wrapped.

Usage example:
```cpp
Context<int> value;

auto result = transform(
    [](int x) { return std::to_string(x); },
    value
);
```
A functor doesn't just lift *values* into boxes; it lifts **functions** into the "box world."

```
   plain world:           f : T ──────────────────────▶ U
                            │ transform               │ transform
                            ▼                         ▼
   box world:   transform(f) : F<T> ───────────────▶ F<U>
```

Suppose there is a plain value and a plain function, something like:
```cpp
int n = 7;
auto f = [](int x) { return x * 2; };
int m = f(n);   // 14
```

Easy. But real programs rarely hand out bare values. Values come *wrapped* in context:

- A value that might be **absent**.
- A value that lives at the end of a **collection** of others.
- A value that will only **arrive later**, from another thread.
- A value sitting behind a **pointer** that may or may not own anything.
- A value that is obtained only by **running a computation** given some input.

Each "context" is a kind of box. The annoying-but-tempting approach is to *open* every box, pull the value out, apply the plain function, and put the result back in a new box — by hand, every single time, for every box shape. That is repetitive and error-prone.

The functor pattern says: each box shape provides **one** operation — be it `transform` (or `map` or `fmap`) — that does the open/apply/re-box process *internally*, correctly, every time.

In C++, each box grows its *own* mapping operation — `views::transform` for ranges, a member `transform` on the sum-type family, and yet another shape on senders. It is not possible to write "for any functor `F`, do this" and have the whole library satisfy it. The box-functor pattern is **ad hoc and per-type** — a recurring *shape* to notice by convention, rather than one named interface every box implements.

```
         f : A ──► B
                                    "lift f to work on boxes"
   ┌─────────┐                            ┌─────────┐
   │ Box<A>  │ ───────  transform(f) ───► │ Box<B>  │
   │  ┌───┐  │                            │  ┌───┐  │
   │  │ a │  │                            │  │ b │  │     b = f(a)
   │  └───┘  │                            │  └───┘  │
   └─────────┘                            └─────────┘
        the box stays the same shape; only the contents change
```
A functor never changes *what kind* of box there is — only what is inside.

With `f = (x ⇒ x*x)`:
```
   ┌───────────────┐                      ┌───────────────┐
   │  ╔═════════╗  │  transform(·, x*x)   │  ╔═════════╗  │
   │  ║    3    ║  │  ─────────────────▶  │  ║    9    ║  │
   │  ╚═════════╝  │                      │  ╚═════════╝  │
   │   Box<int>    │                      │   Box<int>    │
   └───────────────┘                      └───────────────┘
```

For a range, the preserved structure is the *length and ordering* — transform rewrites every element but never changes how many there are or what order they sit in:
```cpp
std::vector<int> v{1, 2, 3};
auto doubled = v | std::views::transform([](int n){ return n * 2; });
// a lazy view yielding 2, 4, 6 — three elements in, three out
```
```
   ┌───┬───┬───┐    transform(×2)    ┌───┬───┬───┐
   │ 1 │ 2 │ 3 │  ────────────────►  │ 2 │ 4 │ 6 │
   └───┴───┴───┘                     └───┴───┴───┘
     vector<int>                       view<int>
   structure preserved = count + positions; only payloads differ

```
So, `Box` preserves nothing but "one value," a range preserves length, a presence-context would preserve emptiness, an async context would preserve "when". The functor operation writes the payload transformation once and applies correctly regardless of which structure is in play. The point is to reach inside and transform the contents without opening the container at the call site and without changing *what kind* of container it is.

It is also possible to nest boxes. If `F` and `G` are functors, then so is their **composition** `F<G<T>>`.
```
   ┌──────────────────────────────────┐
   │  vector<                         │   transform each element's inner box
   │    ┌──────────────┐              │   ───────────────────────────────▶
   │    │ shared_ptr<T>│  ...         │
   │    └──────────────┘              │
   │  >                               │
   └──────────────────────────────────┘
```
This "functors compose" property is exactly why it is possible to stack `vector` of `future` of `pair` and still meaningfully `map` through all of it.

## Laws of a functor
A genuine functor must obey two laws:

**Law 1 — Identity.** Mapping the identity function does nothing.

```
transform(identity)  ==  identity
```

```
  Identity law — mapping id changes nothing:

       F<A> ──── transform(id) ────► F<A>
          |                          |
          └────────── id ────────────┘        (both paths equal)

```
In words: if `f` leaves every value untouched, then `transform(f)` must leave the whole box untouched. A `transform` that secretly reversed a vector, even while applying the identity to each element, would violate this — and would be a trap.

**Law 2 — Composition.** Mapping `f` then mapping `g` is the same as mapping "`g`-after-`f`" once.

```
transform(g) ∘ transform(f)   ==   transform(g ∘ f)
```
```
  Composition law — two maps fuse into one:

           transform(f)        transform(g)
     F<A> ─────────────► F<B> ─────────────► F<C>
        │                                     ▲
        └────────── transform(g ∘ f) ─────────┘   (both paths equal)

```
In words: two passes can be fused into one and get the same answer. This is what lets compilers and range pipelines optimize chains of maps, and lets `box.transform(f).transform(g)` be treated as a single conceptual step.

In C++:
```cpp
#include <functional>   // std::identity (C++20)

// Identity:     o.transform(std::identity{})            ≡  o
// Composition:  o.transform(f).transform(g)             ≡  o.transform([&](auto x){ return g(f(x)); })
```
These laws can be visualized as a **commuting square**.

```
        A  ───────────  f  ───────────►  B
        │                                │
   put in box                       put in box
        │                                │
        ▼                                ▼
      F<A> ──────  transform(f)  ──────► F<B>
```
If that square commutes for every `f`, the box is a real functor.

Together, these guarantee precisely what the metaphor promises: that transform *only* touches contents and *never* the structure. A transform that dropped elements from a range under some condition would satisfy the type signature but violate the laws and would not be a functor in any meaningful sense.

## Pitfalls
* C++ cannot enforce these laws by default. Correctness here is the programmer's responsibility.
* C++ assumes that the mapping functions are pure functions (functions that only use but don't modify the arguments passed to them in order to calculate the result). The expectation is that if the mapping function is called multiple times with the same arguments, it must return the same result every time, leave no trace that it was ever invoked, and never alter the state of the program.
* Views in C++ are **lazy**, so the mapping function is re-invoked on each access rather than once, so an impure callable (one that mutates state, prints, or returns something time-dependent) can run an unpredictable number of times at surprising moments, quietly breaking the referential transparency the laws presuppose.
* Lifetime is another thing: a view that refers to a temporary range dangles. The guarantees are therefore only as clean as the purity and lifetimes of what is fed in, and the compiler verifies none of it — the laws are a contract, not a checked property.

## Contravariant functors and bifunctors
Two refinements complete the picture. The functors above are *covariant* — arrows keep their direction. A *contravariant* functor reverses them, which is the natural shape for boxes that **consume** rather than hold a value, such as predicates or comparators.
```
   Covariant (holds a value):
        A → B        lifts to        F<A> → F<B>       (same direction)

   Contravariant (consumes a value):
        B → A        lifts to        F<A> → F<B>       (reversed!)
```
For example:
```cpp
std::function<bool(int)> is_even = [](int n){ return n % 2 == 0; };

// adapt an int-predicate into a string-predicate via length: string → int
std::function<bool(std::string)> has_even_length =
    [=](const std::string& s){ return is_even(s.size()); };

```
The conversion supplied here goes `string → int` (take the length), yet it produces a predicate on *strings* out of a predicate on *ints* — the arrow flips. That reversal is the contravariant functor in action, and it is why "map" feels backwards for callbacks, comparators, and consumers: the data flows *into* them, so adapting them runs against the direction of the conversion.

A *bifunctor* is a functor in two type parameters at once, with two independent channels. A `std::pair` is the clean standard example — the **product** bifunctor, where both channels are always present and each can be mapped on its own:
```cpp
std::pair<int, std::string> p{21, "hi"};
auto left  = std::pair{ p.first * 2, p.second };       // map first  channel → (42, "hi")
auto right = std::pair{ p.first, p.second + "!" };     // map second channel → (21, "hi!")
```
```
                ┌──────────────────────────┐
   map first ─► │   first  channel  : A    │   both channels always present;
                ├──────────────────────────┤   each maps independently
   map second ► │   second channel  : B    │
                └──────────────────────────┘
                       pair<A, B>

```
## Examples and use cases
### 1. Identity functor
The simplest possible box is the **identity functor**: a context that adds no structure beyond "holds exactly one value."
```
            f : A → B
   ┌──────────┐   transform   ┌──────────┐
   │  a : A   │  ───────────► │ f(a) : B │     identity functor:
   └──────────┘               └──────────┘     the "box" adds no structure,
     Box<A>                     Box<B>          so only the content changes

```

```cpp
#include <iostream>

template <typename T>
struct Box {
    T value;

   // The functor operation. 
   // Naming it `transform` echoes std::transform and std::optional::transform.
   // transform: (T -> F)  on  Box<T>  yields  Box<F>
   template <typename F>
   auto transform(F&& f) -> Box<std::invoke_result_t<F, const T&>>
   {
      return { f(value) };   // new box, same shape, new contents
   }
};

int main() 
{
    Box<int> b{21};

    // Identity law
    Box<int>    doubled = b.transform([](int x){ return x * 2; });        // holds 42
    Box<std::string> txt = doubled.transform(                              // holds "value:42"
        [](int x){ return "value:" + std::to_string(x); });

    // Composition
    // Chaining is just two sealed steps
    // b.transform(g).transform(f) == b.transform([](int x){ return f(g(x)); })
    auto result = Box<int>{10}
                    .transform([](int x){ return x + 5; })   // Box<int> 15
                    .transform([](int x){ return x * x; });   // Box<int> 225
    std::cout << "result.value = " << result.value << std::endl;
    return 0;
}
```

### 2. The many-slots box - `std::vector`
```
   ┌──────────────────────────┐            ┌────────────────────────────┐
   │ ╔═══╦═══╦═══╦═══╗        │  x => x*x  │ ╔═══╦═══╦═══╦════╗         │
   │ ║ 1 ║ 2 ║ 3 ║ 4 ║        │  ────────▶ │ ║ 1 ║ 4 ║ 9 ║ 16 ║         │
   │ ╚═══╩═══╩═══╩═══╝        │            │ ╚═══╩═══╩═══╩════╝         │
   │   vector<int> (len 4)    │            │  vector<int> (still len 4) │
   └──────────────────────────┘            └────────────────────────────┘
```

```cpp
#include <vector>
#include <ranges>
#include <algorithm>
#include <iterator>
#include <iostream>

int main() 
{
    std::vector<int> v{1, 2, 3, 4};

    // (a) Eager — std::transform / std::ranges::transform IS the vector functor's map:
    std::vector<int> squares;
    std::ranges::transform(v, std::back_inserter(squares),
                        [](int x){ return x * x; });        // {1, 4, 9, 16}

    // (b) Lazy — std::views::transform (C++20). The box is now *lazy*:
    //     nothing is computed until iteration happens. A functor can defer work.
    auto lazy = v | std::views::transform([](int x){ return x * x; });
    for (int s : lazy) {
        std::cout << s << " ";
    }
    std::cout<<std::endl;

    return 0;
}
```

### 3. The "one value, or nothing" box - smart pointers
A `std::unique_ptr<T>` (or `std::shared_ptr<T>`) is a box that holds *either* one value *or* nothing (when it is null). Mapping over it applies `f` if there's a value, and leaves an empty box empty — *without ever calling `f`*.
```
 unique_ptr<int>         fmap(+1)        unique_ptr<int>
 ┌─────────┐                            ┌─────────┐
 │  ┌───┐  │  ─────────────────────►    │  ┌───┐  │
 │  │ 8 │  │                            │  │ 9 │  │       f runs once
 │  └───┘  │                            │  └───┘  │
 └─────────┘                            └─────────┘


 unique_ptr<int>         fmap(+1)        unique_ptr<int>
 ┌─────────┐                            ┌─────────┐
 │ (null)  │  ─────────────────────►    │ (null)  │       f never runs
 └─────────┘                            └─────────┘
```
Usage:
```cpp
#include <memory>

template <typename T, typename F>
auto transform(const std::shared_ptr<T>& p, F&& f) -> std::shared_ptr<std::invoke_result_t<F, const T&>>
{
    if (!p) return nullptr; // empty stays empty
    return std::make_shared<std::invoke_result_t<F, const T&>>(f(*p));
}
```
Usage:
```cpp
auto p     = std::make_shared<int>(10);
auto q     = transform(p, [](int x){ return x + 5; });       // shared_ptr<int> -> 15
std::shared_ptr<int> empty;
auto still = transform(empty, [](int x){ return x + 5; });   // still null
```
The "shape" preserved here is *whether the box is empty*. `f` runs zero or one time, never changing that fact.

### 4. The key-value box - `std::pair<A, T>`
```
   ┌───────────────────────┐                ┌────────────────────────┐
   │  first ║   "celsius"  │  t => t*9/5+32 │  first ║   "celsius"   │ (untouched ✓)
   │ ───────╫──────────────│   ───────────▶ │ ───────╫───────────────│
   │ second ║      100     │                │ second ║      212      │
   │     pair<string,int>  │                │     pair<string,int>   │
   └───────────────────────┘                └────────────────────────┘
```

```cpp
#include <utility>

template <typename A, typename B, typename F>
auto transform(const std::pair<A, B>& p, F&& f) -> std::pair<A, std::invoke_result_t<F, const B&>>
{
    return { p.first, f(p.second) };        // first preserved
}
```
Usage:
```cpp
std::pair<std::string, int> temp{"celsius", 100};
auto fahrenheit = transform(temp, [](int c){ return c * 9 / 5 + 32; });
// {"celsius", 212}
```

Preserved shape: the entire first element. This is the seed of the **Writer functor**, where the first slot accumulates context.

### 5. The value-arriving-later box - `std::future`
A future is a box whose contents *don't exist yet*. Mapping over it schedules `f` to run **once the value arrives**, yielding a future of the transformed result. The preserved shape is the "later-ness" — the result is still something asynchronous.

```
   ┌───────────────────┐                       ┌───────────────────┐
   │ ╔═══════════════╗ │   transform(·, *2)    │ ╔═══════════════╗ │
   │ ║   <pending>   ║ │  ──────────────────▶  │ ║   <pending>   ║ │
   │ ╚═══════════════╝ │  (runs when ready)    │ ╚═══════════════╝ │
   │   future<int>     │                       │   future<int>     │
   └───────────────────┘                       └───────────────────┘
```

```cpp
#include <future>
#include <iostream>

// Standard std::future has no built-in .then yet, but the functor shape is clear.
// (Real code would use executors or std::experimental::future::then.)
template <typename T, typename F>
auto transform(std::future<T> fut, F f) -> std::future<std::invoke_result_t<F, T>>
{
    return std::async(std::launch::deferred,
                    [fut = std::move(fut), f = std::move(f)]() mutable {
                        return f(fut.get()); // apply once it's ready
                    });
}

int main()
{
    std::future<int> later = std::async([]{ return 21; });
    std::future<int> mapped = transform(std::move(later), [](int x){ return x * 2; });
    int result = mapped.get();   // 42
    std::cout << "result = " << result << std::endl;
    return 0;
}
```
The `std::future` represents the "delay/effect" behaviour of the context.

### 6. The function as a box (the on-demand box)
Whenever there is a shape `F<A>` with a lawful way to turn `A`s into `B`s underneath the shape, that shape is a functor — even when the shape itself is "a computation waiting for an argument."
```
         R                       R                        R
         │                       │                        │
   ┌─────▼─────┐            ┌─────▼─────┐            ┌──────▼──────┐
   │  h: R→A   │  fmap(f)   │  h: R→A   │   then     │  result:    │
   │           │ ─────────► │  f: A→B   │ ─────────► │   R → B     │
   └───────────┘            └───────────┘            └─────────────┘
   (give R, get A)        (compose f after h)        (give R, get B)
```
```cpp
#include <utility>
#include <string>
#include <iostream>

// h : R -> A,  f : A -> B   ==>   result : R -> B
template <typename H, typename F>
auto transform(H h, F f) {
    return [h = std::move(h), f = std::move(f)](auto&& r) {
        return f( h(r) );   // f after h
    };
}

int main()
{
    auto length     = [](const std::string& s){ return (int)s.size(); }; // string -> int
    auto is_even    = [](int n){ return n % 2 == 0; };                   // int    -> bool

    auto length_is_even = transform(length, is_even);   // string -> bool

    bool a = length_is_even("hello");   // length 5 -> false
    bool b = length_is_even("code");    // length 4 -> true

    std::cout << "a = " << a << ", b = " << b << std::endl;
    return 0;
}
```

### 7. A generic functor with concepts (C++20)
C++20 concepts help describe the surface requirement — "X is a functor" — as a **concept** that checks *any* `transform(x, f)`.
```cpp
#include <iostream>
#include <concepts>
#include <utility>

template <typename Fa, typename Func>
concept Transformable = requires(Fa fa, Func f) {
    { fa.transform(f) };   // member transform that compiles
};

// A helper that works on anything satisfying the concept:
template <typename Func, typename Fa>
    requires Transformable<Fa, Func>
auto map_over(Fa&& fa, Func&& f) {
    return fa.transform(f);
}

template <typename T>
struct Box {
    T value;

   // The functor operation. 
   // Naming it `transform` echoes std::transform and std::optional::transform.
   // transform: (T -> F)  on  Box<T>  yields  Box<F>
   template <typename F>
   auto transform(F&& f) -> Box<std::invoke_result_t<F, const T&>> {
      return { f(value) };   // new box, same shape, new contents
   }
};

int main() {
    Box<int> b{5};
    auto b2 = map_over(b, [](int x){ return x + 1; });   // Box<int> holding 6
    std::cout << "b2.value = " << b2.value << std::endl;
    return 0;
}
```

## Summary

| Box (functor)              | "Box" intuition                          | How many values   | Map operation in this lesson                  | Flavor of context        |
|----------------------------|------------------------------------------|-------------------|-----------------------------------------------|--------------------------|
| `Box<T>` (Identity)        | a single sealed parcel                   | exactly 1         | member `transform`                            | none (pure value)        |
| `std::vector<T>`           | a row of slots                           | 0 … n             | `std::views::transform` / `std::transform`    | multiplicity             |
| `std::unique_ptr<T>`       | a parcel that may be empty               | 0 or 1            | free `fmap` (null stays null)                 | presence / absence       |
| `std::future<T>`           | a parcel that arrives later              | 1 (eventually)    | free `fmap` via `std::async`                  | delay / async effect     |
| `R → A` (a function)       | a value computed on demand from input    | 1 per input       | composition (`fmap_fn` = `f` after `h`)       | dependency on an input   |

Across every row, the same two truths hold: the **box kind is preserved**, and `f` only ever touches the *contents*, never the *shape*. That invariance is the entire point of the box model.

Sources:
* https://youtu.be/2FbeGrbXe2M?si=fDdzJEODvrJQRyP5
* https://youtu.be/DiisKQAkGM4?si=1KeFu5De7bmUMG2T
* https://medium.com/@lettier/your-easy-guide-to-monads-applicatives-functors-862048d61610
* https://softwaremill.com/functional-containers-summary-functor-vs-applicative-vs-monad/
* https://www.adit.io/posts/2013-04-17-functors,_applicatives,_and_monads_in_pictures.html
* https://bartoszmilewski.com/2021/02/16/functorio/
