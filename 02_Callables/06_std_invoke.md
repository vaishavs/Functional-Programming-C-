# `std::invoke` and its family

Calling a function in C++ looks simple, because the syntax is just `f(args...)`. The complication is that C++ has several different things that can be called, and they do not all share that syntax. A pointer to a member function needs an object and a special operator, while a pointer to a data member is not a function at all. Generic code cannot afford to carry a separate branch for each of these forms. The `std::invoke` provides a single interface capable of calling every callable form.

## Background — why a plain call is not enough

A call expression is any expression of the form `E(a, b, c)`, and the language rules decide what `E` is permitted to be. The following four calls are all well-formed, because each left-hand operand is something the language accepts in that position.

```cpp
int add(int a, int b) { return a + b; }
struct Times { int k; int operator()(int x) const { return k * x; } };

add(2, 3);                              // an ordinary function
int (*fp)(int, int) = add;  fp(2, 3);   // a function pointer
Times{3}(7);                            // an object that defines operator()
[](int x){ return x + 1; }(41);         // a lambda
```

Pointers to members, by contrast, cannot be called this way at all.

```cpp
struct Widget {
    int value = 10;
    int scale(int k) const { return value * k; }
};

Widget w;
auto pmf = &Widget::scale;      // the type is int (Widget::*)(int) const
auto pmd = &Widget::value;      // the type is int Widget::*

pmf(w, 4);      // ill-formed, because a pointer to member is not callable in this position
pmd(w);         // ill-formed, and a data member is not a function in the first place
```

Reaching a member through such a pointer requires dedicated operators, and the resulting syntax is awkward enough that it is easy to get wrong.

```cpp
(w.*pmf)(4);          // yields 40, and the parentheses around w.*pmf are mandatory
(&w->*pmf)(4);        // the same call written through a pointer to the object
w.*pmd;               // yields 10, reading the member
```

Consider what happens when a generic utility such as a thread launcher, a callback store, or an algorithm must accept any of these forms. Handling each form separately would require several overloads together with a combinatorial spread of `const` and reference variants. The standard avoids that by defining a single conceptual operation named INVOKE that covers every form, and by exposing that operation as `std::invoke`.

## The INVOKE operation and its seven cases

INVOKE is specified by cases on the first argument. The standard enumerates seven of them in `[func.require]/1`, and the numbering used below is the standard's own numbering, so each case can be looked up directly in the specification. The seven cases fall into three groups.

### Which group does `f` belong to?

```
                        std::invoke(f, a1, a2, ..., aN)
                                       │
                        ┌──────────────┴──────────────┐
                        │   what kind of entity is f?  │
                        └──────────────┬──────────────┘
          ┌────────────────────────────┼────────────────────────────┐
          │                            │                            │
╔═════════▼══════════════╗  ╔══════════▼═════════════╗  ╔═══════════▼════════════╗
║  GROUP A:              ║  ║  GROUP B:              ║  ║  GROUP C:              ║
║  pointer to            ║  ║  pointer to            ║  ║  everything else       ║
║  MEMBER FUNCTION       ║  ║  DATA MEMBER           ║  ║                        ║
║  R (C::*)(Args...)     ║  ║  T C::*                ║  ║  function              ║
║                        ║  ║                        ║  ║  function pointer      ║
║  a1      = the object  ║  ║  a1 = the object       ║  ║  function reference    ║
║  a2..aN  = the args    ║  ║  N must be 1           ║  ║  lambda / functor      ║
╚═════════╤══════════════╝  ╚══════════╤═════════════╝  ╚═══════════╤════════════╝
          │                            │                            │
    cases 1 · 2 · 3              cases 4 · 5 · 6                  case 7
```

### Within a member-pointer group, what is `a1`?

Groups A and B ask the same three questions about `a1`, in the same order, and they differ only in what each answer produces.

In Group A, the entity `f` is a pointer to a member function, and `a1` supplies the object on which that member function runs.

| # | Condition on `a1` | The expression evaluates to |
|:-:|---|---|
| 1 | `a1` is a `C`, or is derived from `C`, whether as an object or as a reference | `(a1.*f)(a2, ..., aN)` |
| 2 | `a1` is a `std::reference_wrapper` | `(a1.get().*f)(a2, ..., aN)` |
| 3 | `a1` is anything else that can be dereferenced, such as a `T*`, a `unique_ptr`, or a `shared_ptr` | `((*a1).*f)(a2, ..., aN)` |

In Group B, the entity `f` is a pointer to a data member, and `N` must be exactly 1, which means no further arguments may be supplied.

| # | Condition on `a1` | The expression evaluates to |
|:-:|---|---|
| 4 | `a1` is a `C`, or is derived from `C`, whether as an object or as a reference | `a1.*f` |
| 5 | `a1` is a `std::reference_wrapper` | `a1.get().*f` |
| 6 | `a1` is anything else that can be dereferenced, such as a `T*`, a `unique_ptr`, or a `shared_ptr` | `(*a1).*f` |

In Group C, the entity `f` is anything other than a pointer to a member, and no object is involved.

| # | Condition | The expression evaluates to |
|:-:|---|---|
| 7 | This case always applies once Groups A and B have been ruled out | `f(a1, ..., aN)` |

Cases 3 and 6 act as the catch-all within their respective groups, because anything that can be dereferenced qualifies for them, and that is precisely why smart pointers work without any call to `.get()`.

Two observations explain most of the design. The first is that the object always comes first: for member pointers, INVOKE receives the object as its first argument rather than through any special syntax, and this convention is the single most important thing to remember about the whole operation. The second is that a data-member case is a read rather than a call, since `std::invoke(&Widget::value, w)` simply evaluates to `w.value`. Folding that read into INVOKE is what allows one interface to accept an instruction to extract a field alongside genuine callables.

The formal requirement that a type must satisfy in order to be used this way is named Cpp17Callable. Anything satisfying it can be stored in a `std::function`, passed to a `std::thread`, or used with any of the facilities described later in this tutorial.

## `std::invoke` in practice

The function template `std::invoke` is declared in `<functional>` and has been available since C++17. The examples below use the following declarations.

```cpp
#include <functional>

struct Widget {
    int value = 10;
    int scale(int k) const   { return value * k; }
    int bump(int k)          { value += k; return value; }
    virtual int poly() const { return 1; }
    virtual ~Widget() = default;
};
struct Derived : Widget { int poly() const override { return 2; } };

int add(int a, int b) { return a + b; }
struct Times { int k; int operator()(int x) const { return k * x; } };

Widget w;
Widget* pw = &w;
```

All seven cases are exercised below, labelled with the case numbers introduced above and annotated with the results that were actually observed.

```cpp
auto up = std::make_unique<Widget>();

// GROUP A — f is a pointer to a member function
std::invoke(&Widget::scale, w, 4);              // case 1 yields 40, using an object
std::invoke(&Widget::scale, std::ref(w), 6);    // case 2 yields 60, using a reference_wrapper
std::invoke(&Widget::scale, pw, 5);             // case 3 yields 50, using a raw pointer
std::invoke(&Widget::scale, up, 3);             // case 3 yields 30, using a unique_ptr

// GROUP B — f is a pointer to a data member, so no arguments follow the object
std::invoke(&Widget::value, w);                 // case 4 yields 10, using an object
std::invoke(&Widget::value, std::ref(w));       // case 5 yields 10, using a reference_wrapper
std::invoke(&Widget::value, pw);                // case 6 yields 10, using a raw pointer
std::invoke(&Widget::value, up);                // case 6 yields 10, using a unique_ptr

// GROUP C — f is anything else
std::invoke(add, 2, 3);                         // case 7 yields 5, calling a function
std::invoke(Times{3}, 7);                       // case 7 yields 21, calling a functor
std::invoke([](int x){ return x + 1; }, 41);    // case 7 yields 42, calling a lambda
```

Three further properties are easy to overlook.

Virtual dispatch continues to work through a member pointer, because the pointer names the virtual member while the object determines which override runs.

```cpp
Derived d;
Widget& base = d;
std::invoke(&Widget::poly, base);        // yields 2, because Derived::poly runs
```

A data-member access yields an lvalue, which means the result can be written through rather than merely read.

```cpp
std::invoke(&Widget::value, w) = 99;     // w.value now holds 99
```

Smart pointers work as the object argument, as cases 3 and 6 above show, because the fallback expression is `(*a1).*f` and any dereferenceable type satisfies it, so no call to `.get()` is required.

In optimized builds `std::invoke` costs nothing at run time, because it exists to provide uniformity rather than indirection. For a direct call on a callable whose form is already known, such as a lambda or an ordinary function, plain `f(x)` remains the natural choice. The value of `std::invoke` appears when the callable might be a pointer to a member, or when the surrounding code is generic over whatever callable it is handed.

## `std::invoke_r`

The function template `std::invoke` returns whatever the underlying call returns. The C++23 addition, `std::invoke_r<R>`, performs the same INVOKE operation and then converts the result to the type `R`.

```cpp
double r = std::invoke_r<double>(add, 3, 2);     // yields 5, converted to double
std::invoke_r<void>(add, 1, 1);                  // the result is deliberately discarded
```

The `void` form is the more useful of the two, because it makes the intention to call something and ignore its answer explicit. That matters when the return value is marked `[[nodiscard]]`, and it matters when a generic wrapper must present a `void` signature over a callable that happens to return a value. Before C++23 the same effect required a cast or a lambda wrapper, whereas `invoke_r` states the intent directly, and it is what the type-erased wrappers use internally in order to honour a declared return type.

## How `constexpr` and `noexcept` propagate

Both `constexpr` and `noexcept` were introduced in C++11, and significantly expanded in later versions, to make C++ code faster, safer, and more expressive. They nevertheless solve entirely different problems: `constexpr` is about **compile-time computation**, while `noexcept` is about **exception safety and optimization**.

The `constexpr` specifier tells the compiler that the value of a variable or the return value of a function **can be evaluated at compile time**. When `constexpr` is applied to a function, it essentially means that if compile-time constants are passed into the function, the result should be computed at compile time, and that otherwise the function should run normally at run time. Work is therefore shifted from run time to compile time, which results in faster programs and smaller binaries.

```cpp
constexpr int square(int x) {
    return x * x;
}

int main() {
    constexpr int a = square(5); // Computed by the compiler (Compile-time)

    int b = 10;
    int c = square(b);           // Computed by the CPU (Runtime)
}
```

The `noexcept` specifier tells the compiler that a function **is guaranteed not to throw an exception**.

```cpp
void doSomething() noexcept {
    // this function is promised never to throw an exception
}
```

One of its use cases is that a function marked `noexcept` relieves the compiler of generating the extra stack-unwinding boilerplate that would otherwise be needed to destroy objects safely if an exception passed through, which results in smaller and faster binaries. That benefit applies only to functions that genuinely promise not to throw in any way. If such a function does throw, whether directly or by calling something that lets an exception escape, the C++ runtime calls `std::terminate()` immediately and the program ends. An exception that escapes a `noexcept` function cannot be caught.

In modern C++, `constexpr` and `noexcept` interact directly with `std::invoke` and its associated utilities, namely `std::invoke_r`, `std::apply`, and the `std::is_invocable` type traits. The `constexpr` specifier enables uniform function invocation at compile time, while `noexcept` enables conditional exception-safety propagation and type-level introspection.

The `std::invoke` template itself is declared with a conditional `noexcept` specifier, so it is non-throwing if and only if the underlying operation being invoked is non-throwing. The implementation behaviour conceptually follows the sketch below.

```cpp
template <typename Callable, typename... Args>
constexpr decltype(auto) invoke(Callable&& f, Args&&... args)
    noexcept(std::is_nothrow_invocable_v<Callable, Args...>) {
    // Invocation logic
}
```

The function template `std::invoke` is `constexpr`, and it is conditionally `noexcept`. Both of these properties are inherited from the callable that is passed in rather than conferred by `std::invoke` itself.

```cpp
constexpr int cadd(int a, int b) { return a + b; }
int           add (int a, int b) { return a + b; }
int           twice(int x) noexcept { return 2 * x; }

static_assert(std::invoke(cadd, 2, 3) == 5);     // accepted, because cadd is constexpr

noexcept(std::invoke(add, 1, 2));                 // false
noexcept(std::invoke(twice, 1));                  // true
```

A common misreading holds that because `std::invoke` is `constexpr`, any call routed through it becomes usable at compile time. That is not the case, and wrapping a function that is not `constexpr` produces a clear diagnostic.

```cpp
static_assert(std::invoke(add, 2, 3) == 5);
// error: call to non-'constexpr' function 'int add(int, int)'
```

For `std::invoke` or `std::apply` to evaluate at compile time, two conditions must be met.

1. The target callable, whether a free function, a lambda, a member function pointer, or a functor, must be executable in a `constexpr` context.
2. All arguments passed to the callable must be valid constant expressions.

## The invocability traits

The traits declared in `<type_traits>` answer at compile time whether a particular call would be well-formed and what that call would produce. They serve as the query counterpart to `std::invoke`, and they follow exactly the same INVOKE rules, including all of the member-pointer cases.

| Trait | The question it answers |
|---|---|
| `std::is_invocable_v<F, Args...>` | Would the expression `std::invoke(f, args...)` be well-formed? |
| `std::is_invocable_r_v<R, F, Args...>` | Would it be well-formed, and would the result be convertible to `R`? |
| `std::invoke_result_t<F, Args...>` | What type would the call produce? |
| `std::is_nothrow_invocable_v<F, Args...>` | Would it be well-formed, and would the call be `noexcept`? |

```cpp
std::is_invocable_v<decltype(add), int, int>;                 // true
std::is_invocable_v<decltype(add), int>;                      // false, because the arity is wrong
std::is_invocable_r_v<double, decltype(add), int, int>;       // true
std::is_invocable_r_v<std::string, decltype(add), int, int>;  // false, because the result type is wrong
std::is_same_v<std::invoke_result_t<decltype(add), int, int>, int>;   // true

std::is_invocable_v<decltype(&Widget::scale), Widget&, int>;  // true, for a member function
std::is_invocable_v<decltype(&Widget::value), Widget&>;       // true, for a data member

std::is_nothrow_invocable_v<decltype(twice), int>;            // true
std::is_nothrow_invocable_v<decltype(add), int, int>;         // false
```

The value category of `F` matters here, and that fact is a frequent source of confusion. Each trait asks about invoking an object of exactly the type written, so a callable whose call operator is reference-qualified or non-`const` gives different answers depending on how it is spelled.

```cpp
struct OnlyRvalue { int operator()(int x) && { return x; } };   // callable only on rvalues
std::is_invocable_v<OnlyRvalue&, int>;      // false
std::is_invocable_v<OnlyRvalue,  int>;      // true

struct Mutating { int n = 0; int operator()() { return ++n; } };  // the call operator is not const
std::is_invocable_v<const Mutating&>;       // false
std::is_invocable_v<Mutating&>;             // true
```

The same consideration applies to the object argument of a member pointer, since a non-`const` member function cannot be invoked on a `const` object.

```cpp
struct M { int n; int bump() { return ++n; } };            // the member function is not const
std::is_invocable_v<decltype(&M::bump), const M&>;         // false
```

## The invocability concepts

C++20 packages the same questions as concepts declared in `<concepts>`, which read more clearly in a constraint and which produce better diagnostics when they are not satisfied.

| Concept | What it requires |
|---|---|
| `std::invocable<F, Args...>` | That `F` can be invoked with `Args...` |
| `std::regular_invocable<F, Args...>` | The same, together with a semantic promise that the call is equality-preserving, which the compiler cannot check |
| `std::predicate<F, Args...>` | The same as `invocable`, together with the requirement that the result is usable as a boolean |
| `std::relation<R, T, U>` | That `R` is a binary predicate over `T` and `U` |
| `std::equivalence_relation<R, T, U>` | The same, together with the promise that the relation is reflexive, symmetric, and transitive |
| `std::strict_weak_order<R, T, U>` | The same, together with the promise that the relation is a strict weak ordering, which is what sorting requires |

```cpp
template <std::invocable<int> F>
int call1(F&& f) { return (int)std::invoke(std::forward<F>(f), 10); }

template <class P> requires std::predicate<P, int>
bool holds(P p, int x) { return std::invoke(p, x); }

template <class R> requires std::strict_weak_order<R, int, int>
bool ord(R r, int a, int b) { return std::invoke(r, a, b); }

call1([](int x){ return x * 2; });      // yields 20
holds([](int x){ return x > 0; }, 5);   // yields true
ord(std::less<int>{}, 2, 3);            // yields true
```

The concept `std::invocable` says nothing whatever about the return type, which surprises anyone who expects it to mean that a callable is usable and returns something sensible.

```cpp
std::invocable<decltype([](int){ return std::string("x"); }), int>;   // true
```

Constraining the result requires either `std::is_invocable_r_v` or, where a boolean is wanted, `std::predicate`. The last three concepts in the table differ from one another only in semantic requirements that no compiler can verify, so their role is to document intent, and `std::strict_weak_order` is the one that an ordering parameter should name.

## Common pitfalls and their solutions

### Forgetting that the object comes first

```cpp
std::invoke(&Widget::scale, 4);          // error, because no object was supplied
std::invoke(&Widget::scale, w, 4);       // correct
```

The compiler's own account of the failure is not phrased in terms of a missing object at all. GCC reports it as an overload-resolution failure against `std::invoke` itself:

```
error: no matching function for call to 'invoke(int (Widget::*)(int) const, int)'
error: no type named 'type' in 'struct std::invoke_result<int (Widget::*)(int) const, int>'
```

Nothing in that message says a member function's object was left out — it says that no overload of `invoke` accepts a member pointer followed directly by an `int`, which is technically true and unhelpful in equal measure. The reason the mistake is so easy to make in the first place is that ordinary member-call syntax hides the object inside the dot: `w.scale(4)` never asks where `w` goes, because the dot fixes its position, and there is nothing resembling an argument list to reorganise. INVOKE has no such syntax to lean on. A pointer to a member function is, structurally, just a value — the same kind of value as a function pointer — so the only way to tell INVOKE which object to run it against is to pass that object as an ordinary argument, and case 1 of the specification fixes that argument's position as the very first one, ahead of every argument the member function itself will receive.

**Solution.** The reliable mental translation is to read `std::invoke(pmf, obj, args...)` as though it had been written `obj.member(args...)`, which places the object exactly where the syntax puts it and leaves the remaining arguments in their original order. Once that translation is automatic, the four spellings of the object become interchangeable rather than four separate rules to memorise, because every one of them denotes the same object by a different route.

```cpp
std::invoke(&Widget::scale, w, 4);            // an object, matching case 1
std::invoke(&Widget::scale, std::ref(w), 4);  // a reference_wrapper, matching case 2
std::invoke(&Widget::scale, pw, 4);           // a raw pointer, matching case 3
std::invoke(&Widget::scale, up, 4);           // a unique_ptr, also matching case 3
```

The trait form mirrors the same shape, and writing it that way is the quickest check when a constraint refuses a call that appears reasonable, since the object has to occupy the first position there as well. Omitting it there fails in the same silent, unhelpful way, reporting a plain `false` rather than pointing at what is missing.

```cpp
std::is_invocable_v<decltype(&Widget::scale), Widget&, int>;   // true, with the object first
std::is_invocable_v<decltype(&Widget::scale), int>;            // false, with the object missing
```

Because the diagnostic produced when the object is omitted names only the failure to match `invoke` — or, in the trait form, simply reports `false` without narrating why — recognising the shape of the call from memory is more useful in practice than reading the error closely. A dependable habit is to count the arguments against the member's own declared parameter list first, and only then ask whether an object precedes them; a mismatch of exactly one argument, with the call otherwise looking correct, is the signature of this particular mistake.

### Falling into the precedence trap of `.*`

```cpp
int (W::*p)(int) const = &W::f;
w.*p(3);
// error: must use '.*' or '->*' to call pointer-to-member function in 'p (...)'
```

The call operator binds more tightly than `.*`, so the expression above parses as `w.*(p(3))`, which is meaningless. GCC's diagnostic is unusually direct about the fix, spelling out the required parenthesisation inside the message itself: `'(... ->* p) (...)'`. That directness is worth noting, because pointer-to-member syntax is one of the few corners of C++ where the compiler volunteers the exact rewrite rather than merely naming what went wrong. The underlying cause is an ordinary entry in the operator-precedence table: `()` — the function-call operator — ranks above `.*` and `->*`, exactly as it ranks above unary `*` in an expression like `*p()`, which parses as "call `p`, then dereference the result" for the identical reason. Nothing about pointers to members is special here; the surprise comes entirely from expecting `.*` to behave like the ordinary `.` used for member access, which never faces this problem because there is no competing operator for it to lose to.

**Solution.** Three approaches resolve this, and the last of them removes the question permanently. The first is to parenthesise the member access so that it completes before the call operator is applied, which is the direct fix and is worth knowing because it appears in a great deal of existing code.

```cpp
(w.*p)(3);        // the member access completes first, then the result is called
(pw->*p)(3);      // the same call written through a pointer to the object
```

The second is to remember that the same precedence problem applies to `->*` exactly as it does to `.*`, so a pointer to the object does not avoid the issue and requires its own pair of parentheses in the same place; switching from `w` to `pw` changes which operator is needed but not whether parentheses are needed. The third, and the one that generic code should prefer, is to route the call through `std::invoke`, which takes the object as an ordinary argument and therefore has no precedence interaction at all — there is no operator being applied to `p` for another operator to outrank.

```cpp
std::invoke(p, w, 3);     // the same call, expressed uniformly and without parentheses
```

The third approach has a further advantage that matters in templates: the same expression continues to work when `p` turns out to be an ordinary callable rather than a member pointer, whereas the parenthesised form has to be rewritten. A generic function written once against `std::invoke` never needs to know, and never needs to be edited to accommodate, which of the two `p` happens to be.

```cpp
template <class Callable>
int run_with(Callable&& c, Widget& w, int k) {
    return std::invoke(std::forward<Callable>(c), w, k);   // works whether c is &Widget::scale or a lambda
}

run_with(&Widget::scale, w, 4);                               // a member pointer
run_with([](Widget& x, int k){ return x.value + k; }, w, 4);  // an ordinary callable
```

A version of `run_with` written against `(w.*p)(3)`-style syntax would have to branch on which kind of callable it received, precisely the branching that INVOKE was introduced to remove.

### Passing an overload set

A name that refers to several functions does not by itself select one of them, so it cannot be deduced as a template argument.

```cpp
int    square(int);
double square(double);

std::invoke(square, 3);
// error: no matching function for call to 'invoke(<unresolved overloaded function type>, int)'
```

The phrase `<unresolved overloaded function type>` in that message is the compiler's own name for the problem: `square` is not a value with a single type until overload resolution has picked one of its candidates, and template argument deduction — which is how `std::invoke`'s template parameter for the callable gets its type — needs a concrete type to deduce *from*. The name and the call happen the wrong way around for this to work: overload resolution is normally driven by the argument types at the point of call, but `std::invoke` is deduced before any such resolution can happen on its behalf, so the compiler is asked to deduce a type from something that does not have one yet.

**Solution.** Two remedies exist, and they differ in how much of the overload set survives. The preferred remedy is to wrap the name in a generic lambda, which defers the selection to the point of call and therefore keeps every overload available; each call then picks the overload that matches the argument it was given, and overloads added later are picked up without any change to the wrapper.

```cpp
auto any_square = [](auto&& x) -> decltype(auto) { return square(std::forward<decltype(x)>(x)); };

std::invoke(any_square, 3);      // yields 9, having selected the int overload
std::invoke(any_square, 1.5);    // yields 2.25, having selected the double overload
```

This works because the lambda itself is not overloaded — it is a single generic callable with one template `operator()` — and the overload resolution on `square` is postponed until that `operator()` is instantiated with a concrete argument type, at which point exactly one `square` is visible to ordinary lookup rules, chosen the same way it would be chosen at any other call site.

The alternative is to cast the name to the exact function pointer type, which resolves the ambiguity immediately but fixes the choice permanently at that point.

```cpp
std::invoke(static_cast<int(*)(int)>(square), 3);   // yields 9, and only ever the int overload
```

The cast is brittle in a way the lambda is not, because it names a signature that must be updated whenever the chosen overload's parameters change, and because it silently continues to compile while selecting an overload that may no longer be the intended one — adding a third overload `square(long)` changes nothing about which `square` the cast selects, even if the `long` overload was the one actually wanted for a particular call site. The lambda has no such staleness: it re-resolves on every instantiation, so a newly added overload becomes reachable automatically as soon as some call supplies an argument that prefers it.

The same considerations apply to function templates passed by name, since a template name is likewise not a single type until it is instantiated. They apply with even more force to constructors, since `std::invoke` cannot invoke a constructor at all: a type name is not a callable, has no address to take, and cannot appear as the first argument to `invoke` under any of the seven cases. Wrapping the construction in a lambda such as `[](auto&&... a){ return T(std::forward<decltype(a)>(a)...); }` is the only route, and it works for exactly the reason the overload-set wrapper does — the lambda is a genuine single callable that performs the ambiguous operation internally, at a point where enough information is available to resolve it.

### Supplying extra arguments to a data-member pointer

```cpp
std::invoke(&W::v, W{}, 5);
// error: no matching function for call to 'invoke(int W::*, W, int)'
```

A data-member case takes exactly the object and nothing further, because reading the member is all that it does, and there is no argument list through which a value could be passed. This follows directly from the type of the pointer itself: `&Widget::value` has type `int Widget::*`, which names a *field*, not a callable with parameters, so there is no place in that type for a second argument to attach to in the first place — the mistake is not so much a wrong argument count as an argument supplied to something that was never going to accept one, regardless of count. Cases 4 through 6 in the specification enforce exactly this by requiring `N` to equal 1, and the error message reports the same shape as every other failed match against `invoke`: an unresolved overload, naming the argument types that were tried and none of the available cases that would have accepted them.

**Solution.** The first step is to decide which of two different intentions the extra argument represented. If the intention was simply to read the member, then removing the argument is the whole fix, and the resulting expression is an lvalue, so it may equally be assigned through — the read and the write use the identical expression, differing only in whether it appears on the left or the right of `=`.

```cpp
std::invoke(&Widget::value, w);        // reads the member and yields 10
std::invoke(&Widget::value, w) = 42;   // writes through the same expression
```

If the intention was to compute something from the member and the extra argument — for instance, scaling the field by a factor supplied at the call site — then a data-member pointer was the wrong tool from the start, because it performs no computation whatsoever; it is INVOKE's designated way of expressing "read this field," and nothing about it can be coaxed into also multiplying, comparing, or otherwise combining that field with something else. A member function expresses that intention directly, and a lambda expresses it without requiring a change to the class, which matters when the field belongs to a type that cannot be modified.

```cpp
std::invoke(&Widget::scale, w, 2);                                   // a member function computes
auto scaled_field = [](const Widget& obj, int k){ return obj.value * k; };
std::invoke(scaled_field, w, 2);                                     // a lambda computes as well
```

Recognising which of the two situations applies is mostly a matter of asking whether the extra value was meant to travel *into* a computation or merely to be attached, out of habit, to a call that turned out not to need one; the second situation is the more common source of this particular mistake, arising when a data member and a similarly named member function exist side by side and the wrong one is reached for.

### Assuming that `is_invocable` implies that a call is a good idea

The trait reports only whether the call would compile, and it says nothing about whether the call is correct or safe. A callable that has already outlived something it depends on remains perfectly well-formed to invoke — the type system has no way to know that the underlying data is gone, because "gone" is a run-time fact and every one of these traits answers a question fixed entirely at compile time.

```cpp
auto make_dangling_predicate() {
    std::string local = "temp";
    return [&local](int x) { return x > 0 && !local.empty(); };   // captures local BY REFERENCE
}   // local's storage ends here

auto pred = make_dangling_predicate();
std::is_invocable_v<decltype(pred), int>;            // true
std::is_invocable_r_v<bool, decltype(pred), int>;    // also true
// pred(5);                                           // undefined behavior, and no trait warned about it
```

Both traits report exactly what a perfectly healthy predicate would report, because from the type system's point of view nothing distinguishes `pred` from a predicate that captured a `std::string` still very much alive; the closure's *shape* — what it captures and by what means — is identical in both cases, and shape is all a type-level trait can see.

**Solution.** The three traits answer three different questions, and a check that matters should name the one that corresponds to the property being relied upon rather than the most familiar one. `std::is_invocable_v` establishes only that the expression is well-formed, `std::is_invocable_r_v` additionally establishes that the result converts to the type the surrounding code expects, and `std::is_nothrow_invocable_v` additionally establishes that the call cannot throw, which is what an operation offering the strong exception guarantee needs to know.

```cpp
std::is_invocable_v<decltype(add), int, int>;               // true: the call compiles
std::is_invocable_r_v<double, decltype(add), int, int>;     // true: and the result fits a double
std::is_nothrow_invocable_v<decltype(add), int, int>;       // false: but the call may throw
```

No trait in the family reports anything about lifetimes, and that gap is worth stating explicitly because it is the one most often assumed to be covered — reaching for `is_invocable` after a refactor is a natural instinct, and passing that check feels like a clean bill of health, when in fact it only ever certified the shape of the call, never the validity of what the call reaches into. A callable that captures a reference to a local object satisfies every trait listed here and still produces undefined behaviour when it is invoked after that object has been destroyed, so the lifetime of everything a callable refers to remains the responsibility of the code that stores or forwards it, tracked by the same discipline that would apply to a raw reference or pointer used the same way — the trait family simply has nothing to add to that discipline, in either direction.

### Writing the wrong value category in a trait

```cpp
std::is_invocable_v<Mutating&>;         // true, which is the question usually intended
std::is_invocable_v<const Mutating&>;   // false, because the call operator is not const
```

**Solution.** The type written between the angle brackets should be the exact type the code will actually hold at the point where the call happens, including its reference and `const` qualification, because the trait answers a question about that type and about no other; `Mutating` and `const Mutating&` are different types with different call operators visible to overload resolution, and the trait treats them exactly that differently. Inside a template the correct spelling follows mechanically from how the parameter was declared: a parameter taken by value and then called is held as an lvalue, so `F&` is the type to ask about, whereas a forwarding parameter that is passed on with `std::forward` is invoked as `F&&`, since that is the value category `std::forward` will produce for it.

```cpp
template <class F> requires std::is_invocable_v<F&, int>
int call_named(F f) { return static_cast<int>(std::invoke(f, 1)); }

template <class F> requires std::is_invocable_v<F&&, int>
int call_forwarded(F&& f) { return static_cast<int>(std::invoke(std::forward<F>(f), 1)); }
```

Mismatching the two produces the least helpful kind of failure, which is a constraint that appears to pass when tested on its own and then fails inside the function where the call actually occurs. Consider a version of `call_named` constrained against `F` instead of `F&`:

```cpp
template <class F> requires std::is_invocable_v<F, int>   // asks about F, not F&
int call_named_wrong(F f) { return static_cast<int>(std::invoke(f, 1)); }
```

For an ordinary lambda this passes every test thrown at it, because a lambda's call operator is `const` by default and a `const` call operator is reachable from `F`, `F&`, and `const F&` alike, so the distinction never surfaces. It surfaces only once a *stateful, non-`const`* callable is substituted, at which point `is_invocable_v<F, int>` still reports `true` — a prvalue can still be called once, since a temporary is a valid target for a non-`const` member function — while the actual call inside the function body operates on the *parameter* `f`, which is a named lvalue of type `F`, i.e. effectively `F&` at the point of use. The constraint was never asking the question the function body depends on, and a stateful callable makes the effect easy to observe directly, since asking about `const Accum&` reports `false` for a call operator that is not `const`, even though asking about `Accum&` reports `true` for exactly the same type:

```cpp
struct Accum { int n = 0; int operator()(int x) { return n += x; } };   // not const

std::is_invocable_v<Accum&, int>;         // true
std::is_invocable_v<const Accum&, int>;   // false
```

The discrepancy between what a constraint checks and what the function body actually does is the entire danger here, and it is a danger specifically because both spellings compile without complaint in the common case; only a call chain that happens to route a `const` reference to the callable, or that happens to hold it in a way the constraint did not anticipate, exposes the mismatch, typically far from the line where the constraint was written.

### Expecting `std::invoke` to make a call `constexpr`

As described earlier, `std::invoke` propagates the property and does not confer it, so the callable itself must be `constexpr` before any call through `std::invoke` can appear in a constant expression. `std::invoke` being declared `constexpr` only means that *its own definition* contains nothing that would disqualify it from compile-time evaluation — a single, uniform dispatch to whichever of the seven INVOKE cases applies — not that everything reachable through it inherits that property by association.

```cpp
static_assert(std::invoke(add, 2, 3) == 5);
// error: call to non-'constexpr' function 'int add(int, int)'
```

The diagnostic here is unusually precise, because it names the actual offending function — `add` — rather than `std::invoke` itself, which confirms that the failure is being attributed correctly to the callable rather than to the dispatch mechanism wrapping it; the wrapper is transparent to the compiler in exactly the sense that matters for this diagnosis.

**Solution.** Two conditions have to hold together, and a failure of either produces the same diagnostic, so both are worth checking when one appears. The first condition is that the callable is usable in a constant expression, which for a free function means marking it `constexpr` and for a lambda means that the lambda body qualifies, since a lambda is implicitly `constexpr` whenever its body permits — no `constexpr` keyword needs to be written on a lambda for this to happen, and most lambdas satisfy it without anyone having thought about it.

```cpp
constexpr int cadd(int a, int b) { return a + b; }
static_assert(std::invoke(cadd, 2, 3) == 5);          // the free function is constexpr

constexpr auto clam = [](int a, int b){ return a * b; };
static_assert(std::invoke(clam, 6, 7) == 42);         // the lambda qualifies as well
```

The second condition is that every argument is itself a constant expression, which is easy to overlook when a call that worked in a `static_assert` is later moved into a context where one argument comes from a run-time variable:

```cpp
int b = 10;                          // an ordinary run-time variable
int c = std::invoke(cadd, b, 3);     // perfectly valid: cadd still runs, just at run time now
```

In that situation the call remains perfectly valid and simply runs at run time, which is the intended behaviour of `constexpr` rather than a failure — the same function serves both a compile-time caller and a run-time caller without being written twice, and `std::invoke(cadd, b, 3)` above compiles and executes exactly as `cadd(b, 3)` would have. The only error arises when a constant expression was demanded by the surrounding context, such as a `static_assert`, an array bound, or a `constexpr` variable initializer, and the value supplied to fill it turns out not to be one — the failure belongs to the context requiring compile-time evaluation, not to `std::invoke`, which would have been equally content to run the same call later.

### Applying a null member pointer

A pointer to a member may hold a null value, and applying a null one is undefined behaviour in exactly the way that dereferencing a null object pointer is — a default-constructed `int (Widget::*)(int) const` is null in the same sense that a default-constructed `int*` is null, and INVOKE performs no check for either before proceeding. Every one of the seven cases assumes, without verifying, that the entity being invoked actually refers to something; a null member pointer satisfies the *type* requirements of case 1 perfectly well, so nothing about overload resolution or the trait family flags it, and the failure — where it manifests at all in an observable way — happens only once the call is actually made.

```cpp
int (Widget::*maybe)(int) const = nullptr;
std::invoke(maybe, w, 2);   // undefined behaviour; nothing before this line signals danger
```

This is worth separating clearly from the earlier pitfall about a *dangling* callable: a dangling closure is well-formed and non-null but refers to storage that no longer exists, while a null member pointer refers to nothing from the moment it was set, and neither condition is visible to `is_invocable`, `is_invocable_r`, or `is_nothrow_invocable`, since all three ask only about types.

**Solution.** A member pointer should be treated with the same discipline as any other pointer, which means checking it against `nullptr` before it is applied and never assuming that a default-constructed one is usable, since a default-constructed member pointer is null exactly as a default-constructed object pointer is.

```cpp
std::optional<int> result = maybe ? std::optional<int>{std::invoke(maybe, w, 2)}
                                  : std::nullopt;     // the call happens only when non-null
```

Where the possibility of absence is part of the design rather than an accident, expressing it in the type is preferable to relying on a null value that nothing forces the reader to check. A small wrapper illustrates the idea directly: rather than storing the member pointer as-is and trusting every call site to remember the check, the wrapper performs the check once, at the single point where the pointer is actually applied, and every caller inherits that guarantee automatically.

```cpp
class SafeMemberCall {
public:
    explicit SafeMemberCall(int (Widget::*pmf)(int) const) : pmf_(pmf) {}
    std::optional<int> operator()(const Widget& w, int k) const {
        return pmf_ ? std::optional<int>{std::invoke(pmf_, w, k)} : std::nullopt;
    }
private:
    int (Widget::*pmf_)(int) const;
};
```

A `std::optional` holding the member pointer, or a wrapper of this shape that refuses to expose the raw pointer at all, both make the check impossible to forget, whereas a bare member pointer places that obligation on every call site independently — and a member pointer, unlike a `std::unique_ptr` or a reference, gives no compiler warning of any kind when it is used without having been checked, since assigning `nullptr` to it is completely ordinary, valid, silent initialization.

### Forgetting the decay-copy performed by `std::thread` and `std::async`

Both `std::thread` and `std::async` store their callable and its arguments internally so that the new thread of execution can hold onto them independently of whatever the calling thread does next, and "store independently" is implemented by applying `std::decay` to each one and copying the decayed result — a plain value, with references, top-level `const`, and array/function types converted to pointers exactly as they would be by passing through an ordinary by-value parameter. That decision is what makes concurrent code safe by default: nothing a `std::thread` retains can dangle merely because the launching function returned, because nothing it retains is a reference to that function's storage in the first place. Asking to bypass this default — because the intention really is for the new thread to reach back into the caller's data — is exactly the situation `std::ref` exists for, and forgetting to ask produces a failure that is caught at compile time rather than manifesting as a race or a stale read at run time.

```cpp
void worker(int& counter) { ++counter; }

int counter = 0;
std::thread t(worker, counter);   // no std::ref -- does this even compile?
```

It does not, and the diagnostic names the mechanism directly:

```
error: static assertion failed: std::thread arguments must be invocable after conversion to rvalues
```

The message is precise once the decay-copy is understood: `counter`, after conversion to the rvalue that decay-copying produces, is a plain `int`, and a plain `int` cannot bind to the `int&` that `worker` declares, so the internally-generated call is simply ill-formed — the same ill-formedness `std::is_invocable_v` would report if asked directly about a plain `int` against a parameter of type `int&`. This is, in a specific sense, a fortunate failure mode: because reference binding is checked by the type system, a mismatched `std::thread` call is rejected before the program ever runs, rather than compiling into a thread that silently increments a copy nobody looks at.

**Solution.** Wrapping the argument in `std::ref` states that sharing is intended, and the wrapper is unwrapped when the call is finally made — `std::thread`'s invocation mechanism recognises a `std::reference_wrapper` specifically and passes the referenced object through, so a function taking a reference parameter receives the caller's object rather than a copy of it.

```cpp
void worker(int& counter) { ++counter; }

int counter = 0;
std::thread t(worker, std::ref(counter));   // sharing is requested explicitly
t.join();                                    // counter now holds 1
```

Two further points are worth knowing before that remedy is applied. The first is that `std::ref` reintroduces exactly the lifetime obligation the copy was avoiding, so the referenced object must outlive the thread, which in practice means joining the thread before the object goes out of scope rather than detaching it — a detached thread holding a `std::reference_wrapper` to a local variable of the function that detached it is the `std::thread` analogue of the dangling-callable pitfall described earlier, arrived at from a different direction. The second is that the decay-copy applies to the callable itself and not only to the arguments, so a stateful function object passed to a thread is copied too, and observing its accumulated state afterwards requires the same wrapper.

```cpp
Mutating shared;
std::thread t2(std::ref(shared));   // the callable is shared rather than copied
t2.join();                           // shared.n now reflects the call
```

Without that second `std::ref`, `t2` would run its own private copy of `shared` to completion, leaving the caller's `shared.n` at its original value — a mistake that, unlike the reference-parameter case above, produces no compiler diagnostic at all, since `std::thread` never requires a callable to be anything other than callable, and a copied functor is exactly as invocable as the original.

### Reaching for `std::invoke` where a plain call would be clearer

Writing `std::invoke(f, x)` for a known lambda or ordinary function adds noise without any corresponding benefit — the two spellings compile to the same thing once optimisation is applied, so the choice is purely one of what the reader has to parse, and `std::invoke(f, x)` asks the reader to first confirm that `f` is not a member pointer before concluding that the extra ceremony was unnecessary.

**Solution.** The facility earns its place in three situations, and outside them the plain call expression communicates more with fewer tokens.

The first situation is a call through a pointer to a member, where `std::invoke` replaces the precedence-sensitive `.*` and `->*` syntax with an ordinary function call, as covered above.

```cpp
std::invoke(&Widget::scale, w, 4);           // a member pointer, where invoke is the better form
```

The second is generic code that must call whatever callable it was handed, where the concrete form is unknown at the point the code is written and only INVOKE covers every possibility without a branch for each one.

```cpp
template <class F, class... Args>
decltype(auto) apply_to(F&& f, Args&&... a) {
    return std::invoke(std::forward<F>(f), std::forward<Args>(a)...);
}
```

The third is a context where uniformity is itself the goal, such as a component that must treat a member pointer and an ordinary callable identically so that both can be stored side by side and dispatched through the same code path. A small callback registry illustrates this directly: each entry is an arbitrary `Cpp17Callable`, and the registry's dispatch loop is written once, against `std::invoke`, rather than once per kind of callable it might be asked to hold.

```cpp
struct EventLog {
    std::vector<std::function<void(const Widget&)>> handlers;
    void fire(const Widget& w) {
        for (auto& h : handlers) std::invoke(h, w);   // h might wrap a lambda or a bound member pointer
    }
};

EventLog log;
log.handlers.push_back([](const Widget& w) { std::cout << w.value << '\n'; });
log.handlers.push_back(std::bind_front(&Widget::poly, std::placeholders::_1));
```

Outside these three situations — a known lambda called once, an ordinary function called by name, a functor whose type is already concrete at the call site — `f(x)` says everything `std::invoke(f, x)` says, in fewer characters and without inviting the reader to wonder whether `f` might secretly be a member pointer. The judgement call is not about performance, since both forms are equivalent once compiled, but about whether the extra explicitness pays for itself at that particular call site; a codebase that routes every call through `std::invoke` out of uniform habit has traded a small, real readability cost for a generality it is not using.

```cpp
auto lambda = [](int x){ return x + 1; };
lambda(41);                                  // a plain call, which is clearer here
```

## Quick reference

| Task | How it is written |
|---|---|
| Call anything at all, uniformly | `std::invoke(f, args...)` |
| Call a member function | `std::invoke(&C::m, obj, args...)`, with the object first |
| Read a data member | `std::invoke(&C::field, obj)`, with no further arguments |
| Call and convert the result | `std::invoke_r<R>(f, args...)`, from C++23 |
| Call and discard the result | `std::invoke_r<void>(f, args...)`, from C++23 |
| Call with a tuple of arguments | `std::apply(f, tup)`, from C++17 |
| Ask whether a call would compile | `std::is_invocable_v<F, Args...>` |
| Ask the same, with a required result type | `std::is_invocable_r_v<R, F, Args...>` |
| Ask the same, and whether it can throw | `std::is_nothrow_invocable_v<F, Args...>` |
| Name the type a call would produce | `std::invoke_result_t<F, Args...>` |
| Constrain a template on callability | `std::invocable<F, Args...>`, from C++20 |
| Constrain a template on a boolean result | `std::predicate<F, Args...>`, from C++20 |
| Constrain a template on an ordering | `std::strict_weak_order<R, T, T>`, from C++20 |
| Share an argument rather than copy it | `std::ref(x)` or `std::cref(x)` |
| Fix the leading arguments of a callable | `std::bind_front(f, args...)`, from C++20 |

### Three facts worth memorising

1. The object is always the first argument in both member-pointer groups, and that convention is the entire reason the INVOKE operation exists in the form it does.
2. A pointer to a data member is callable through INVOKE, and that the resulting case is a read which takes no arguments and yields an lvalue.
3. The `std::invoke` propagates properties rather than conferring them, so the `constexpr` status, the `noexcept` status, and the return type all come from the callable that was passed in.

Sources:

* https://devblogs.microsoft.com/oldnewthing/20220401-00/?p=106426
* https://www.rangakrish.com/index.php/2018/10/14/c17-stdapply-and-stdinvoke/
* https://medium.com/@sireanu.roland/why-should-you-use-noexcept-f095aed9e6c4
* https://towardsdev.com/constexpr-part-i-the-engine-of-modern-c-meta-programming-d9896938f8ba
