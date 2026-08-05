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

---

## The INVOKE operation and its seven cases

INVOKE is specified by cases on the first argument. The standard enumerates seven of them in `[func.require]/1`, and the numbering used below is the standard's own numbering, so each case can be looked up directly in the specification. The seven cases fall into three groups.

### Step 1 — which group does `f` belong to?

```
                        std::invoke(f, a1, a2, ..., aN)
                                       │
                        ┌──────────────┴──────────────┐
                        │   what kind of entity is f?  │
                        └──────────────┬──────────────┘
          ┌────────────────────────────┼────────────────────────────┐
          │                            │                            │
╔═════════▼══════════════╗  ╔══════════▼═════════════╗  ╔═══════════▼════════════╗
║  GROUP A               ║  ║  GROUP B               ║  ║  GROUP C               ║
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

### Step 2 — within a member-pointer group, what is `a1`?

Groups A and B ask the same three questions about `a1`, in the same order, and they differ only in what each answer produces.

In Group A the entity `f` is a pointer to a member function, and `a1` supplies the object on which that member function runs.

| # | Condition on `a1` | The expression evaluates to |
|:-:|---|---|
| 1 | `a1` is a `C`, or is derived from `C`, whether as an object or as a reference | `(a1.*f)(a2, ..., aN)` |
| 2 | `a1` is a `std::reference_wrapper` | `(a1.get().*f)(a2, ..., aN)` |
| 3 | `a1` is anything else that can be dereferenced, such as a `T*`, a `unique_ptr`, or a `shared_ptr` | `((*a1).*f)(a2, ..., aN)` |

In Group B the entity `f` is a pointer to a data member, and `N` must be exactly 1, which means no further arguments may be supplied.

| # | Condition on `a1` | The expression evaluates to |
|:-:|---|---|
| 4 | `a1` is a `C`, or is derived from `C`, whether as an object or as a reference | `a1.*f` |
| 5 | `a1` is a `std::reference_wrapper` | `a1.get().*f` |
| 6 | `a1` is anything else that can be dereferenced, such as a `T*`, a `unique_ptr`, or a `shared_ptr` | `(*a1).*f` |

In Group C the entity `f` is anything other than a pointer to a member, and no object is involved.

| # | Condition | The expression evaluates to |
|:-:|---|---|
| 7 | This case always applies once Groups A and B have been ruled out | `f(a1, ..., aN)` |

Cases 3 and 6 act as the catch-all within their respective groups, because anything that can be dereferenced qualifies for them, and that is precisely why smart pointers work without any call to `.get()`.

Two observations explain most of the design. The first is that the object always comes first: for member pointers, INVOKE receives the object as its first argument rather than through any special syntax, and this convention is the single most important thing to remember about the whole operation. The second is that a data-member case is a read rather than a call, since `std::invoke(&Widget::value, w)` simply evaluates to `w.value`. Folding that read into INVOKE is what allows one interface to accept an instruction to extract a field alongside genuine callables.

The formal requirement that a type must satisfy in order to be used this way is named Cpp17Callable. Anything satisfying it can be stored in a `std::function`, passed to a `std::thread`, or used with any of the facilities described later in this tutorial.

---

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

---

## `std::invoke_r`

The function template `std::invoke` returns whatever the underlying call returns. The C++23 addition `std::invoke_r<R>` performs the same INVOKE operation and then converts the result to the type `R`.

```cpp
double r = std::invoke_r<double>(add, 3, 2);     // yields 5, converted to double
std::invoke_r<void>(add, 1, 1);                  // the result is deliberately discarded
```

The `void` form is the more useful of the two, because it makes the intention to call something and ignore its answer explicit. That matters when the return value is marked `[[nodiscard]]`, and it matters when a generic wrapper must present a `void` signature over a callable that happens to return a value. Before C++23 the same effect required a cast or a lambda wrapper, whereas `invoke_r` states the intent directly, and it is what the type-erased wrappers use internally in order to honour a declared return type.

---

## How `constexpr` and `noexcept` propagate

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

---

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

---

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

---

## Related facilities built on INVOKE

Every facility described below is specified in terms of INVOKE, which is the reason each of them accepts member pointers as readily as it accepts ordinary callables.

### `std::apply`

The function template `std::apply`, added in C++17, calls a callable with the elements of a tuple as its arguments.

```cpp
#include <tuple>
auto args = std::make_tuple(6, 7);
std::apply(add, args);                                          // yields 13

std::apply(&Widget::scale, std::make_tuple(std::ref(w), 2));    // yields 200
```

The member-pointer form works because `std::apply` unpacks the tuple and hands the resulting elements to INVOKE, where the first of them becomes the object.

### `std::reference_wrapper`

This class template, usually created through `std::ref` or `std::cref`, serves two distinct roles that are often conflated. In the first role it acts as the object argument of a member pointer, where INVOKE unwraps it, as cases 2 and 5 describe. In the second role it acts as a callable in its own right, because a reference wrapper around a callable forwards its own `operator()` to the referent, which allows a large or non-copyable function object to be passed by reference into an interface that would otherwise copy it.

### `std::thread` and `std::async`

Both of these facilities copy their arguments before invoking the callable. This is a deliberate safety measure, and it means that a function taking a reference parameter will not compile unless the sharing is requested explicitly.

```cpp
void worker(int& counter) { ++counter; }

int counter = 0;
std::thread bad(worker, counter);
// error: static assertion failed: std::thread arguments must be invocable
//        after conversion to rvalues

std::thread good(worker, std::ref(counter));   // the sharing is now explicit
good.join();                                    // counter holds 1
```

The diagnostic is unusually clear about its cause. The underlying rule is that arguments are decay-copied and that sharing must be requested, which has the further benefit of making the sharing visible at the call site.

### `std::bind_front`

This C++20 facility fixes the leading arguments of a callable and forwards the rest, producing a new callable. One frequent use is to bind a member function to its object, which works precisely because of the convention that the object comes first.

```cpp
auto scale_w = std::bind_front(&Widget::scale, std::ref(w));
scale_w(4);      // yields 40
```

The mirror-image facility `std::bind_back` was added in C++23 and is not present in g++ 13.3.

### The type-erased wrappers

The class templates `std::function`, `std::move_only_function` from C++23, and `std::packaged_task` all store any Cpp17Callable behind a fixed signature, and all of them perform the call through INVOKE. That is why a pointer to a member function can be stored in a `std::function` whose first parameter is the object.

```cpp
std::function<int(const Widget&, int)> f = &Widget::scale;
f(w, 4);         // yields 40
```

---

## Common pitfalls and their solutions

### Forgetting that the object comes first

```cpp
std::invoke(&Widget::scale, 4);          // error, because no object was supplied
std::invoke(&Widget::scale, w, 4);       // correct
```

The remedy is to read `std::invoke(pmf, obj, args...)` as though it were written `obj.member(args...)`. The trait form mirrors the same shape, as in `std::is_invocable_v<decltype(&Widget::scale), Widget&, int>`.

### Falling into the precedence trap of `.*`

```cpp
int (W::*p)(int) const = &W::f;
w.*p(3);
// error: must use '.*' or '->*' to call pointer-to-member function in 'p (...)'
```

The call operator binds more tightly than `.*`, so the expression above parses as `w.*(p(3))`, which is meaningless. The parentheses are therefore mandatory, and the awkwardness of this syntax is one of the reasons that generic code prefers `std::invoke`.

```cpp
(w.*p)(3);                  // correct
std::invoke(p, w, 3);       // the same call, expressed uniformly
```

### Passing an overload set

A name that refers to several functions does not by itself select one of them, so it cannot be deduced as a template argument.

```cpp
int    square(int);
double square(double);

std::invoke(square, 3);
// error: no matching function for call to 'invoke(<unresolved overloaded function type>, int)'
```

The usual remedy is to wrap the name in a lambda, which defers the selection to the point of call, although an explicit cast to the desired function pointer type also works.

```cpp
std::invoke([](auto x){ return square(x); }, 3);        // the preferred form
std::invoke(static_cast<int(*)(int)>(square), 3);       // explicit, but brittle
```

The same considerations apply to function templates, and they apply with even more force to constructors, because `std::invoke` cannot invoke a constructor at all: a type name is not a callable. Wrapping the construction in a lambda such as `[](auto&&... a){ return T(a...); }` solves that case.

### Supplying extra arguments to a data-member pointer

```cpp
std::invoke(&W::v, W{}, 5);
// error: no matching function for call to 'invoke(int W::*, W, int)'
```

A data-member case takes exactly the object and nothing further, because reading the member is all that it does, and there is no argument list through which a value could be passed.

### Assuming that `is_invocable` implies that a call is a good idea

The trait reports only whether the call would compile, and it says nothing about whether the call is correct or safe. In particular it says nothing about the return type, which requires `std::is_invocable_r_v`, nothing about exception behaviour, which requires `std::is_nothrow_invocable_v`, and nothing at all about the lifetimes involved.

### Writing the wrong value category in a trait

```cpp
std::is_invocable_v<Mutating&>;         // true, which is the question usually intended
std::is_invocable_v<const Mutating&>;   // false, because the call operator is not const
```

The remedy is to write the trait using the exact type that the code will actually hold, including any reference and `const` qualification. Inside a template that type is usually `F&` for a named parameter or `F&&` for a forwarded one, and mismatching them produces constraints that appear to pass in isolation while failing at the call site.

### Expecting `std::invoke` to make a call `constexpr`

As described earlier, `std::invoke` propagates the property and does not confer it, so the callable itself must be `constexpr` before any call through `std::invoke` can appear in a constant expression.

### Applying a null member pointer

A pointer to a member may hold a null value, and applying a null one is undefined behaviour in exactly the way that dereferencing a null object pointer is. A null member pointer is therefore useful only as a sentinel that is checked before use.

### Forgetting the decay-copy performed by `std::thread` and `std::async`

The symptom is a failed static assertion mentioning invocability after conversion to rvalues, the remedy is `std::ref`, and the underlying diagnosis is that arguments are copied by design rather than shared.

### Reaching for `std::invoke` where a plain call would be clearer

Writing `std::invoke(f, x)` for a known lambda or ordinary function adds noise without any corresponding benefit. The facility is best reserved for member pointers, for generic code, and for places where uniformity across callable forms is the actual goal.

---

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

The first is that the object is always the first argument in both member-pointer groups, and that convention is the entire reason the INVOKE operation exists in the form it does.

The second is that a pointer to a data member is callable through INVOKE, and that the resulting case is a read which takes no arguments and yields an lvalue.

The third is that `std::invoke` propagates properties rather than conferring them, so the `constexpr` status, the `noexcept` status, and the return type all come from the callable that was passed in.
