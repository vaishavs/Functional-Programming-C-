# Bonus: Type Deduction in C++
Guessing at a deduced type is never necessary, because the compiler can be asked directly. The helper below prints any type as a readable string on GCC and Clang, and it is used throughout this tutorial to turn claims into observations.

```cpp
#include <string_view>

template <class T>
constexpr std::string_view type_name() {
    std::string_view p = __PRETTY_FUNCTION__;
    auto s = p.find("T = ") + 4;
    auto e = p.find(";", s);
    return p.substr(s, e - s);
}

#define SHOW(...) std::cout << #__VA_ARGS__ << "  ->  " << type_name<__VA_ARGS__>() << "\n"

int x = 5;
SHOW(decltype(x));          // decltype(x)  ->  int
```

A second technique requires no helper at all. Declaring an incomplete class template and then instantiating it with the type in question, as in `template <class> struct TD; TD<decltype(x)> probe;`, produces an error message that names the type. The printing version is easier to read and has the advantage of working at run time, so it is the one used below.

---

## The taxonomy, and the two rule sets underneath it

### The five groups

The diagram below sorts every deduction mechanism by the information the compiler works backwards from.

```
                     What does the compiler learn the type FROM?
                                        │
   ┌────────────────┬───────────────────┼──────────────────┬──────────────────┐
   │                │                   │                  │                  │
an INITIALIZER   a RETURN           call ARGUMENTS   constructor ARGS      nothing —
  expression      expression                          or a VALUE        the type is named
   │                │                   │                  │                  │
GROUP I          GROUP II            GROUP III          GROUP IV           GROUP V
─────────        ─────────           ─────────          ─────────          ─────────
auto x = e       auto f() {...}      template<class T>  CTAD               decltype(e)
auto& x = e      decltype(auto) f()  auto parameters    deduction guides   declval<T>()
auto&& x = e     trailing -> T       generic lambdas    template<auto N>   decay_t
decltype(auto) x                     forwarding refs                       remove_cvref_t
structured bindings                  abbreviated tmpl                      common_type_t
```

Each group draws on a different source of information, and that difference is what makes the rules diverge. The first four groups deduce a type from something the programmer wrote elsewhere in the program, whereas the fifth group performs no deduction at all and instead names or transforms a type that is already fully determined.

### The two rule sets

Cutting across those five groups there are only two sets of rules, and knowing which set a given mechanism uses answers most questions that arise about it.

| Rule set | Behaviour | Mechanisms that use it |
|---|---|---|
| Template-deduction rules | The type is stripped, meaning that references and top-level `const` and `volatile` qualifiers are discarded, arrays decay to pointers, and functions decay to function pointers. | `auto` variables, `auto` parameters, `auto` return types, template parameters, CTAD, and the hidden object behind a structured binding |
| `decltype` rules | The type is kept exactly as declared, so references, `const`, array extents, and function types all survive. | `decltype(e)`, and `decltype(auto)` in both variables and return types |

```
    auto      ──────►   strips   ──────►   a VALUE type by default
    decltype  ──────►   keeps    ──────►   the EXACT declared type, references and all
```

Almost every surprise in this area reduces to one of the two rule sets being applied where the other was expected. The remainder of the tutorial works through the groups in turn, and §7 collects the failures that follow from confusing them.

---

## Group I — Deduction from an initializer

In this group the compiler is given an initializer, written either as `= expr` or as `{expr}`, and works backwards from it to a type for the variable being declared.

### `auto` variables

A variable declared with `auto` is deduced using the template-deduction rules, which means the type is stripped. The four common spellings behave as follows.

```
auto x = expr;         References are dropped, top-level const and volatile are dropped,
                       arrays decay to pointers, and functions decay to function pointers.

auto& x = expr;        Qualifiers such as const are kept and nothing decays, but the
                       initializer must be an lvalue.

const auto& x = expr;  Anything binds, including a temporary, and the result is always const.

auto&& x = expr;       This is a forwarding reference: an lvalue initializer yields T&,
                       while an rvalue initializer yields T&&.
```

Each of those forms was compiled and inspected, and the deduced types are shown in the comments below.

```cpp
const int  ci   = 5;
const int& cref = ci;
int arr[3]{};
int func(int);

auto a1 = ci;      // int              — the const qualifier was dropped
auto a2 = cref;    // int              — both the reference and the const were dropped
auto a3 = arr;     // int*             — the array decayed to a pointer
auto a4 = func;    // int (*)(int)     — the function decayed to a function pointer
auto& a5 = ci;     // const int&       — the const qualifier was kept
auto&& a6 = gi;    // int&             — the initializer was an lvalue
auto&& a7 = 42;    // int&&            — the initializer was an rvalue
```

Choosing among these forms is usually straightforward. Plain `auto` is appropriate when a value is wanted and a copy is acceptable. The form `const auto&` is the right default for read-only access to an object that may be large or that may be a temporary, because it avoids a copy while still binding to anything. The form `auto&` is appropriate when the original object is to be modified through the new name. Finally, `auto&&` belongs in generic code and in range-based `for` loops, where the element type is not known in advance and either category must be accommodated.

### Braced initialisers are the one genuine exception

The only place where `auto` and template deduction truly disagree is the treatment of a braced initialiser.

```cpp
auto b1 = 1;       // int
auto b2{1};        // int                        (this is the rule from C++17 onwards)
auto b3 = {1};     // std::initializer_list<int>
auto b4 = {1, 2};  // std::initializer_list<int>
```

The form written with `= {...}` deduces a `std::initializer_list`, whereas a template parameter would never deduce that type from the same argument, as §4.3 demonstrates. Before C++17 the form `auto b2{1}` also produced an `initializer_list`, which is why older books and older advice disagree with the behaviour shown here.

### `decltype(auto)` variables

The `decltype(auto)` form, introduced in C++14, occupies the same syntactic position as `auto` but selects the other rule set. It deduces positionally in the same way that `auto` does, and then applies the rules of `decltype`.

```cpp
int  gi = 5;
auto           v1 = gi;    // int   — auto strips, as always
decltype(auto) v2 = gi;    // int   — the entity rule of §6.1 applies
decltype(auto) v3 = (gi);  // int&  — the parenthesis rule of §6.1 applies
```

### Structured bindings

A structured binding, added in C++17, introduces names for the individual pieces of a tuple, a pair, an array, or a public aggregate. The `auto` that appears in front of the bracketed name list applies to a hidden object rather than to the individual names, and that hidden object is deduced by the rules of §2.1.

```cpp
std::pair<int, std::string> ps{1, "x"};

auto  [n1, s1] = ps;   n1 = 99;    // the pair was copied, so ps.first is still 1
auto& [n2, s2] = ps;   n2 = 99;    // the pair was referenced, so ps.first is now 99
const auto& [n3, s3] = ps;         // read-only access with no copy
```

One special rule catches almost everyone the first time. Applying `decltype` to a binding name reports the referenced type rather than a reference type, and this holds even when the binding was introduced with `auto&`.

```cpp
auto& [n2, s2] = ps;
decltype(n2);        // int, and specifically not int&
```

The customary idiom in a loop is to write `const auto&` when the elements are only read and `auto&` when they are modified, because plain `auto` silently copies each element.

```cpp
std::map<std::string, int> m{{"a", 1}};
for (const auto& [key, value] : m) std::cout << key << "=" << value << "\n";
```
### Common pitfalls

#### A variable declared with `auto` copies silently.
Because `auto` strips references, a function that returns `const T&` yields a fresh `T` when its result is captured with plain `auto`, and in a loop over a container of strings that copy can dominate the run time.

```cpp
auto row = matrix.get_row(i);            // a copy is made, however large the row is
const auto& row2 = matrix.get_row(i);    // no copy is made
```

The solution is to choose the form deliberately: `const auto&` when the object is only observed, `auto&` when it is modified, and plain `auto` only when a copy is genuinely wanted.

#### A variable declared with `auto` can capture a proxy type.
Some expressions return a proxy object rather than the value they appear to return, and `auto` faithfully deduces the proxy.

```cpp
std::vector<bool> vb{true, false};
auto p1 = vb[0];          // std::_Bit_reference, which is not bool
auto p2 = bool(vb[0]);    // bool
```

The container `std::vector<bool>` stores bits and therefore returns a proxy, so the deduced variable holds a reference back into the vector; the variable can consequently change meaning later, or dangle if the vector is destroyed. Expression-template libraries such as Eigen and the range views of the standard library behave the same way. The solution is to name the intended type explicitly or to apply a cast, as the second line above does.

#### A non-const `auto&` cannot bind to a temporary.

```cpp
auto& r = x + 1;
// error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'
```

The solution is to write `const auto&`, which extends the lifetime of the temporary, or to write `auto&&`, which binds to either category.

#### A declaration of `auto` without an initializer is ill-formed.

```cpp
auto x;    // error: declaration of 'auto x' has no initializer
```

This follows directly from the definition of the group, since an initializer is the only source of information available. For the same reason, non-static data members cannot be declared with `auto` at all, and only `static constexpr` members may use it.

#### Brace initialisation does not behave uniformly.
The form `auto x = {1};` deduces `std::initializer_list<int>`, the form `auto x{1};` deduces `int` from C++17 onwards, and a braced list passed to a plain `T` parameter does not deduce at all, as §4.3 shows. The solution is to prefer `=` with an ordinary value for scalars and to be explicit whenever a list is genuinely intended.

## Plain `auto` in a range-based `for` loop over a map copies every element.

```cpp
for (auto [k, v] : m)         // every pair is copied
for (const auto& [k, v] : m)  // nothing is copied
```

The structured binding syntax is pleasant enough that the copy is easy to overlook, which is exactly what makes this a common source of accidental pessimisation.

---

## Group II — Deduction from a return statement

In this group the compiler is given one or more `return` statements and works backwards from them to the function's return type. There are four spellings, and they exhibit three distinct behaviours.

```cpp
int            f1();                     // explicit, so no deduction takes place
auto           f2() -> int;              // a trailing return type, which is also explicit
auto           f3() { return gi; }       // deduced using the template-deduction rules, giving int
decltype(auto) f4() { return (gi); }     // deduced using the decltype rules, giving int&
```

The behaviour of the two deduced forms was verified directly.

```cpp
int gi = 5;
auto           ret_plain()  { return gi;  }   // int
decltype(auto) ret_entity() { return gi;  }   // int
decltype(auto) ret_paren()  { return (gi); }  // int&, purely because of the parentheses
```

The `decltype(auto)` form exists chiefly for this position, where the goal is to forward a return type exactly as the underlying operation produced it.

```cpp
template <class Container, class Index>
decltype(auto) at(Container&& c, Index i) {
    return std::forward<Container>(c)[i];   // the reference survives if operator[] returned one
}
```

Had the return type been written as plain `auto`, the reference would have been stripped and every caller would have received a copy, which would silently break an expression such as `at(v, 0) = 42;`.

Several constraints apply to any deduced return type. The definition of the function must be visible at every call site, because the caller cannot otherwise know the type, which makes deduced return types well suited to headers and templates but unsuitable for separately compiled interfaces. When a function contains more than one `return` statement, every one of them must deduce the same type; a function containing both `return 1;` and `return 1.0;` is an error rather than a promotion to `double`. A recursive function must reach at least one `return` statement that does not depend on the recursion before the recursive call appears, so that the type is known by then. Trailing return types remain useful in cases where the return type depends on the parameter types, as in `auto add(T a, U b) -> decltype(a + b)`.

### Common pitfalls and solutions

#### A pair of parentheses around a returned name changes the deduced type.
This is the single most dangerous character sequence in the topic.

```cpp
decltype(auto) bad() { int local = 42; return (local); }   // the return type is int&
std::cout << bad();
// warning: reference to local variable 'local' returned [-Wreturn-local-addr]
// AddressSanitizer: SEGV
```

Writing `return local;` deduces `int`, whereas writing `return (local);` deduces `int&` and therefore returns a reference to an object that has already been destroyed. GCC issues a warning in this case only because the example is simple enough to analyse; routed through a wrapper function the same mistake goes undetected. The solution is never to parenthesise a returned value unless a reference is genuinely intended.

#### A deduced `auto` return type strips references.

```cpp
auto           get(std::vector<int>& v) { return v[0]; }   // returns int, which is a copy
decltype(auto) ref(std::vector<int>& v) { return v[0]; }   // returns int&, which is writable
```

An attempt to write `get(v) = 42;` fails to compile, which at least makes the problem visible; the subtler cost is a copy on a hot path that nothing draws attention to.

---

## Group III — Deduction from call arguments

In this group the compiler examines the arguments supplied at a call site and works backwards to the template parameters. This is the original deduction engine in the language, and it is the engine that Groups I and II borrow.

### Template parameter deduction

What the compiler deduces depends on how the parameter was declared, as summarised in the table below.

| Parameter form | What `T` becomes | Is `const` preserved? | Does decay occur? |
|---|---|---|---|
| `f(T)`, taking the argument by value | the stripped type | No, top-level `const` is dropped | Yes, arrays and functions decay |
| `f(T&)`, taking an lvalue reference | the type with its qualifiers intact | Yes | No |
| `f(const T&)` | the bare type, with `const` supplied by the parameter form itself | The form adds it | No |
| `f(T&&)`, a forwarding reference | `T&` for an lvalue argument and `T` for an rvalue argument | Yes | No |

Those rules were verified by passing a `const int` named `ci`, an `int&` named `r`, and an `int[3]` named `a2` to three differently declared templates.

```cpp
template <class T> void by_value(T);        // by_value(ci) deduces T = int
                                             // by_value(r)  deduces T = int
                                             // by_value(a2) deduces T = int*
template <class T> void by_fwd(T&&);        // by_fwd(gi)   deduces T = int&
                                             // by_fwd(5)    deduces T = int
                                             // by_fwd(ci)   deduces T = const int&
template <class T> void by_cref(const T&);  // by_cref(ci)  deduces T = int
                                             // by_cref(a2)  deduces T = int[3], with no decay
```

### Reference collapsing

Forwarding references work because a reference to a reference collapses into a single reference, and because only one of the four possible combinations yields an rvalue reference.

```
T&  &   collapses to  T&
T&  &&  collapses to  T&
T&& &   collapses to  T&
T&& &&  collapses to  T&&      (an rvalue reference survives only when BOTH are &&)
```

A convenient way to remember the table is that the lvalue reference always wins. This is the reason that `by_fwd(gi)` deduces `T = int&`, which makes the parameter type `int& &&` and therefore `int&`, and it is also the reason that `std::forward<T>` is able to restore the original value category.

### Non-deduced contexts

Certain positions cannot be deduced from at all, and the resulting diagnostics do not always make the reason obvious.

```cpp
template <class T> struct Id { using type = T; };
template <class T> void g(typename Id<T>::type);

g(1);   // error: no matching function for call to 'g(int)'
```

The parameter `T` appears to the left of a `::`, so the compiler would have to invert an arbitrary mapping in order to recover it, which is not something deduction attempts. The remedy is either to supply the argument explicitly by writing `g<int>(1)` or to restructure the signature so that `T` appears in a deducible position.

A braced list is likewise not deducible for a plain `T` parameter, which is the precise contrast with the behaviour of `auto` described in §2.2.

```cpp
template <class T> void f(T);
f({1, 2});   // error: no matching function for call to 'f(<brace-enclosed initializer list>)'
```

The remedy here is to declare the parameter as `std::initializer_list<int>` explicitly, or to construct a container at the call site with something like `f(std::vector{1,2})`.

### `auto` parameters

Writing `auto` in a parameter list turns the function into a template, and the parameter is deduced using the by-value rules described in §4.1. Four related spellings exist.

```cpp
auto gl = [](auto a, auto b) { return a + b; };        // a generic lambda, from C++14
auto twice(auto x) { return x + x; }                    // an abbreviated function template, C++20
auto sum(std::integral auto a, std::integral auto b) { return a + b; }   // constrained, C++20

auto named = []<class T>(const std::vector<T>& v) { return v.size(); };  // templated lambda, C++20
```

The constrained form is preferable whenever the set of acceptable types is known in advance, because a violation is then reported at the call site with the name of the unsatisfied concept rather than as a cascade of errors from deep inside the function body. The explicit `[]<class T>` form is the one to reach for when the deduced type needs a name, either so that two parameters can be required to have the same type or so that the type can be referred to inside the body.

### Common pitfalls and solutions

#### The token `auto&&` does not always mean rvalue reference.
In any context where deduction takes place, both `auto&&` and `T&&` are forwarding references, and an lvalue argument therefore produces an lvalue reference. The `&&` in a non-deduced declaration such as `void f(std::string&&)` genuinely is an rvalue reference. The distinguishing question is always whether deduction is occurring.

#### Deduction cannot be expected in a non-deduced context.
Neither `typename Id<T>::type` nor a braced initialiser list can be deduced from, and the resulting diagnostics tend to report only that no matching function was found.

#### An unconstrained `auto` parameter accepts everything and fails late.
Because no requirement is stated, an unsuitable argument is diagnosed deep inside the function body rather than at the call site. The solution is to constrain the parameter, using a concept such as `std::integral auto` or `std::ranges::input_range auto`, or a `requires` clause, so that the error names the actual problem where the call was written.

---

## Group IV — Deduction of template arguments for a type

The first three groups deduce the type of a variable, of a return value, or of a parameter. This group is different in that it deduces the template arguments of a class, or the type of a value used as a template argument.

### Class template argument deduction

Since C++17 a class template can deduce its own template arguments from the arguments passed to a constructor, which allows the angle brackets to be omitted entirely.

```cpp
std::vector v1{1, 2, 3};      // deduces std::vector<int>
std::pair   pr{1, 2.5};       // deduces std::pair<int, double>
std::vector v2(3, 0.5);       // deduces std::vector<double> via the (count, value) constructor
```

The compiler performs this by synthesising a set of implicit deduction guides from the declared constructors and then choosing among them by ordinary overload resolution.

### User-defined deduction guides

When the implicit guides are absent or produce an unwanted answer, explicit guides can be declared instead.

```cpp
template <class T> struct Box { T v; };
template <class T> Box(T) -> Box<T>;
Box(const char*)  -> Box<std::string>;      // without this guide the result would be Box<const char*>

Box b1{42};        // Box<int>
Box b2{"hello"};   // Box<std::string>, obtained through the guide above
```

Aggregates participate as well from C++20 onwards, so a declaration such as `struct Agg { int a; double b; };` supports `Agg ag{1, 2.5};` without any guide being written.

### `auto` non-type template parameters

Since C++17 a template can accept a value whose type is itself deduced, which is written `template <auto N>`.

```cpp
template <auto N> struct Fixed { static constexpr auto value = N; };

Fixed<42>::value;    // const int
Fixed<'c'>::value;   // const char
```

This replaces the older `template <class T, T N>` pattern, in which the caller was obliged to spell out both the type and the value. The facility is commonly used for compile-time tags, for sizes, and for enum-valued policy parameters.

### Common pitfalls and solutions

#### Class template argument deduction is all or nothing.
A declaration such as `std::pair<int> p{1, 2.5};` is an error, because partial argument lists are not deduced. Either every argument is written explicitly or none is.

#### Braces and parentheses select different constructors, and the difference is easy to miss.

```cpp
std::vector v2(3, 0.5);   // three elements, each equal to 0.5
std::vector v3{3, 0.5};   // two elements, and the deduced type is std::vector<double>
```

In the second line the literal `3` is silently converted to `3.0`. That conversion is permitted only because `3` is a constant expression whose value is exactly representable as a `double`; the same code written with a variable in place of the literal would be rejected as narrowing. When the distinction matters, the type should be written out explicitly.

#### Class template argument deduction does not apply everywhere.
It was not available for alias templates in C++17, although C++20 added that capability, and it never applies to a bare `auto` data member, because it is fundamentally a feature of the point of construction.

---

## Group V — Type computation without deduction

Nothing is inferred from an initializer, a return statement, or an argument in this group. These mechanisms instead name a type that is already determined, or compute a new type from one that is already known.

### `decltype`

The `decltype` operator reports a type exactly, and it operates in two distinct modes depending on the form of its operand.

```
decltype( name )              yields the DECLARED type of that entity, verbatim.
decltype( member-access )     yields the declared type of that member, verbatim.

decltype( any other expr )    yields a type determined by the expression's VALUE CATEGORY:
        a prvalue   yields  T
        an lvalue   yields  T&
        an xvalue   yields  T&&
```

The second mode applies to anything wrapped in an extra pair of parentheses, which is why the two expressions below differ even though they mention the same variable.

```cpp
int gi = 5;

decltype(gi)     // int, because this is the declared type of the entity
decltype((gi))   // int&, because (gi) is an expression and that expression is an lvalue
```

A wider selection of operands was compiled and inspected, and the results appear below.

```cpp
const int  ci   = 5;
int&       ref  = gi;
const int& cref = gi;
int arr[3]{};
int func(int);

decltype(ci)        // const int
decltype(ref)       // int&
decltype(cref)      // const int&
decltype(arr)       // int[3], since no decay takes place
decltype(func)      // int(int), which is a function type rather than a function pointer
decltype(func(1))   // int, and the call is never evaluated
decltype(gi + 1)    // int, because the expression is a prvalue
```

Two properties of `decltype` matter repeatedly in practice. The first is that the operand is never evaluated, so `decltype(func(1))` calls nothing at all and merely asks what the call would produce; this makes the operator safe to apply to expensive or side-effecting expressions, and it is also what allows `std::declval` to work. The second is that no decay occurs, so arrays remain arrays and functions remain functions, which is precisely the opposite of the behaviour of `auto`.

### `std::declval`

The `decltype` operator requires an expression, but constructing a suitable expression may be impossible when the type in question has no default constructor or is abstract. The function template `std::declval<T>()` supplies a fictional value of type `T` for use in unevaluated contexts.

```cpp
decltype(std::declval<std::vector<int>&>().front())   // int&
```

Because `std::declval` has no definition, it must appear only in unevaluated contexts; calling it in real code is a link error, and that error is deliberate rather than an oversight.

### Type-transformation traits

The traits below compute a new type from an existing one, and each corresponds to a rule that has already appeared in an earlier group.

| Utility | What it produces |
|---|---|
| `std::decay_t<T>` | The type that `auto x = ...` would deduce, meaning that references and cv-qualifiers are stripped and that arrays and functions decay. |
| `std::remove_cvref_t<T>`, added in C++20 | The type with references and cv-qualifiers stripped, but with arrays and functions left intact. |
| `std::common_type_t<A, B>` | The single type to which both `A` and `B` convert. |

```cpp
std::decay_t<int[5]>                       // int*
std::decay_t<int(int)>                     // int (*)(int)
std::remove_cvref_t<const volatile int&>   // int
std::common_type_t<int, double>            // double
```

The trait `std::decay_t` is simply the Group I rule set expressed as a type transformation, which makes it useful for reproducing the behaviour of `auto` explicitly inside a template.

### Common pitfalls and solutions

#### The parenthesis rule is easy to forget.
The expressions `decltype(x)` and `decltype((x))` differ, and because `decltype(auto)` adopts these rules the difference propagates into return types, as pitfall II.1 shows.

#### Calling `std::declval` in evaluated code is an error.
The function template has no definition, so any use outside an unevaluated context fails at link time, and that failure is intentional.

#### The trait `std::decay_t` is often reached for when `std::remove_cvref_t` was meant.
In addition to stripping references and cv-qualifiers, `std::decay_t` converts arrays to pointers and functions to function pointers, which is rarely the intent when the goal is merely to obtain a plain value type.

---

## Quick reference

### The five groups at a glance

| Group | What it deduces from | Mechanisms | Rule set in use |
|---|---|---|---|
| I | An initializer expression | `auto`, `auto&`, `auto&&`, `decltype(auto)` variables, and structured bindings | Template-deduction rules, except that `decltype(auto)` uses the `decltype` rules |
| II | A return statement | Deduced `auto` return types, `decltype(auto)` return types, and trailing `-> T` | Template-deduction rules, except that `decltype(auto)` uses the `decltype` rules |
| III | The arguments at a call site | Template parameters, forwarding references, `auto` parameters, generic and templated lambdas | Template-deduction rules |
| IV | Constructor arguments, or a value | Class template argument deduction, deduction guides, and `template <auto N>` | Template-deduction rules, applied through deduction guides |
| V | Nothing, because the type is named outright | `decltype`, `std::declval`, `std::decay_t`, `std::remove_cvref_t`, `std::common_type_t` | The `decltype` rules, or an explicit transformation |

### What each form keeps and what it discards

Each row below states how one spelling treats the four properties that deduction can either preserve or remove.

| Form | References | Top-level `const` | Arrays | Functions |
|---|---|---|---|---|
| `auto x = e;` | Dropped | Dropped | Decay to a pointer | Decay to a pointer |
| `auto& x = e;` | Binds to the object | Kept | Kept as `T(&)[N]` | Kept |
| `auto&& x = e;` | Forwarding reference | Kept | Kept | Kept |
| `decltype(e)` | Kept | Kept | Kept as `T[N]` | Kept as `T(Args)` |
| `decltype((e))` | An `&` is added for lvalues | Kept | Not applicable | Not applicable |
| `decltype(auto) x = e;` | As for `decltype` | As for `decltype` | Kept | Kept |
| `std::decay_t<T>` | Dropped | Dropped | Decay to a pointer | Decay to a pointer |
| `std::remove_cvref_t<T>` | Dropped | Dropped | Kept | Kept |

### Four rules that cover most everyday use

1. The keyword `auto` strips types while the operator `decltype` keeps them, so the choice between them follows from whether references and `const` are meant to survive.
2. Parentheses change the answer that `decltype` gives, because `decltype(x)` reports the declared type of an entity whereas `decltype((x))` reports `T&` for any lvalue.
3. The token `auto&&` denotes a forwarding reference wherever deduction is taking place, and it denotes an rvalue reference only where no deduction occurs.
4. Whenever the deduced type is in doubt, it should be printed rather than guessed, using the helper given in §0.


---

## Type Deduction in `std::ranges` and Views

The ranges library is the most deduction-dependent part of the standard library, and the dependence is not a matter of style. Several range types cannot be written out by hand at all, which turns `auto` from a convenience into a requirement, and the library's own vocabulary consists almost entirely of deduction machinery exposed under readable names. 

The type of even a modest pipeline is large, and printing it makes the point better than any description.

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6};
auto pipe = v | rv::filter([](int x){ return x % 2 == 0; })
              | rv::transform([](int x){ return x * 10; });

// The deduced type is:
//   std::ranges::transform_view<
//       std::ranges::filter_view<
//           std::ranges::ref_view<std::vector<int>>,
//           main()::<lambda(int)>>,
//       main()::<lambda(int)>>
```

Two features of that type make it unusable as a written declaration. The first is length, which grows with every stage added. The second is decisive: the type embeds the closure types of the two lambdas, and a closure type has no name that can be written in source code. Even a second lambda with an identical body would produce a different type, so an attempt to declare a variable of the pipeline's type and assign a freshly built pipeline to it fails.

```cpp
auto p = v | rv::filter([](int x){ return x % 2 == 0; });
decltype(p) q = v | rv::filter([](int x){ return x % 2 == 0; });
// error: conversion from 'filter_view<[...],main()::<lambda(int)>>'
//        to non-scalar type 'filter_view<[...],main()::<lambda(int)>>' requested
```

The two lambdas are written identically and are nonetheless distinct types. This is why Group I deduction, in the form of `auto` on the left of a pipeline, is the only practical way to hold the result, and why Group II deduction, in the form of a deduced return type, is the only practical way to return one.

### Group I in ranges: storing a pipeline

#### What `auto` actually deduces

Writing `auto pipe = ...` deduces a **view object**, which is a small value holding iterators or a pointer to the source together with any predicates supplied. It does not deduce a container, and it does not hold elements. The sizes below make the distinction concrete.

```cpp
std::vector<int> v(1000, 1);
auto pipe = v | rv::filter([](int x){ return x % 2 == 0; })
              | rv::transform([](int x){ return x * 2; });

sizeof(rv::all(v));   //  8 bytes, a ref_view holding a pointer to the vector
sizeof(pipe);         // 16 bytes, the whole two-stage pipeline
// the vector's own elements occupy 4000 bytes, none of which the pipeline owns
```

The size depends on what the adaptors were given. Captureless lambdas are empty types and add nothing, as above, whereas passing named functions stores a function pointer per stage and brings the same pipeline to 32 bytes. Either figure is negligible beside the elements, which is the point.

Because a view is this small, copying one is cheap, and that fact drives the interface conventions discussed under Group III below.

#### The reference type decides the loop variable

Range-based `for` performs Group I deduction on each element, and what it may deduce is fixed by the range's *reference type*, which the library exposes as `std::ranges::range_reference_t`.

```cpp
std::ranges::range_reference_t<std::vector<int>&>;                    // int&
std::ranges::range_reference_t<decltype(v | rv::transform(doubler))>; // int
```

A `std::vector` yields real references, so `auto&` binds. A `transform_view` yields computed prvalues, so `auto&` has nothing to bind to.

```cpp
for (auto& x : v | rv::transform([](int y){ return y * 2; })) { /* ... */ }
// error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'
```

The remedy is `auto&&`, which is a forwarding reference and therefore binds to a reference where one exists and to a prvalue where one does not. This is the reason `auto&&` is the recommended default in range-based `for` over any pipeline: it is the single spelling that is correct regardless of what the reference type turns out to be.

```cpp
for (auto&& x : v | rv::transform([](int y){ return y * 2; })) { /* ... */ }   // always valid
```

#### Structured bindings over `zip` and `enumerate`

The C++23 views `zip` and `enumerate` yield tuples, and the exact reference type determines how a structured binding behaves. The observed reference types are informative.

```cpp
std::ranges::range_reference_t<decltype(rv::enumerate(names))>;  // std::tuple<long, std::string&>
std::ranges::range_reference_t<decltype(rv::zip(a, b))>;         // std::pair<int&, int&>
```

Both are tuples of **references**. A plain `auto` binding therefore copies the tuple, but the copy holds the same references, so a write through a binding still reaches the underlying range.

```cpp
std::vector<std::string> names{"a", "b"};
for (auto  [i, s] : rv::enumerate(names)) s += "!";   // names becomes {"a!", "b!"}
for (auto&& [i, s] : rv::enumerate(names)) s += "?";  // names becomes {"a!?", "b!?"}
```

This differs from a structured binding over a `std::pair` that holds values rather than references, where a plain `auto` genuinely does copy and the write is lost.

```cpp
std::pair<int, std::string> p{1, "x"};
auto [a, b] = p;  b += "!";     // p.second is still "x", because the pair was copied
```

The distinction is worth stating plainly, because the two cases are written identically. What a structured binding copies is the range's reference type, and whether a write survives depends on whether that type holds values or references. For `zip` and `enumerate` it holds references and writes survive; for an aggregate of values it does not and they do not. `auto&&` remains the better default nonetheless, because it avoids copying the proxy and continues to work when the reference type is a prvalue.

#### The caching in `filter_view` blocks `const auto&`

A `filter_view` caches the position of its first element on the first call to `begin()`, which makes that member function non-`const` and the view as a whole not const-iterable. A helper taking `const auto&` therefore fails to compile.

```cpp
void print(const auto& r) { for (auto x : r) std::cout << x; }
print(v | rv::filter([](int x){ return x % 2; }));
// error: passing 'const std::ranges::filter_view<...>' as 'this' argument discards qualifiers
```

The remedy follows from the size measurements above: views are cheap, so taking them by value or by forwarding reference costs nothing.

```cpp
void print(std::ranges::input_range auto&& r) { for (auto&& x : r) std::cout << x; }
```

### Group II in ranges: returning a pipeline

Because the type cannot be written, a function that returns a pipeline must use a deduced return type.

```cpp
auto evens_of(std::vector<int>& v) {
    return v | rv::filter([](int x){ return x % 2 == 0; });
}

for (int x : evens_of(v)) { /* yields 2, 4, 6 */ }
```

This is convenient and carries a hazard that the deduction itself conceals. The deduced type contains a `ref_view` referring to whatever was piped in, and nothing in the signature shows it. When the source is a local object, the returned view outlives it.

```cpp
auto broken() {
    std::vector<int> local{1, 2, 3, 4};
    return local | rv::filter([](int x){ return x % 2 == 0; });   // a ref_view over `local`
}                                                                  // `local` is destroyed here
// reading the result reports stack-use-after-return under AddressSanitizer
```

The parameter version above is safe because the caller owns the vector; the local version is not. The distinction is exactly the one drawn under Group IV below, where `views::all_t` decides between referring and owning, and it is invisible at the point where `auto` appears.

`decltype(auto)` has its own role here, in element access rather than in returning pipelines. A helper that must forward an element with its own value category intact uses the decltype rules rather than the template-deduction rules.

```cpp
template <std::ranges::random_access_range R>
decltype(auto) nth(R&& r, std::ranges::range_difference_t<R> i) {
    return std::ranges::begin(std::forward<R>(r))[i];
}

nth(v, 0) = 99;                 // the return type is int&, so the write reaches the vector
decltype(nth(v, 0));            // int&
decltype(nth(transform_pipe, 0)); // int, because a transform_view yields a prvalue
```

Had the return type been written as plain `auto`, the reference would have been stripped and the assignment would not compile.

### Group III in ranges: deducing from arguments

#### Constrained `auto` parameters

The ranges library is designed around constrained parameters, which are abbreviated function templates whose `auto` is qualified by a concept. This is Group III deduction with a requirement attached.

```cpp
void take_any(std::ranges::input_range auto&& r) {
    std::cout << "value type: "
              << type_name<std::ranges::range_value_t<decltype(r)>>() << "\n";
}

take_any(v);                                                  // value type: int
take_any(v | rv::transform([](int x){ return std::to_string(x); }));  // value type: std::string
```

The concept is not decoration. An unconstrained `auto` parameter accepts any argument and fails deep inside the body, whereas `std::ranges::input_range auto&&` reports an unsuitable argument at the call site and names the unsatisfied requirement.

Adaptors constrain their range parameter with `std::ranges::viewable_range`, which asks whether the argument can safely be turned into a view. The concept exists precisely because deduction must decide between two very different outcomes, and that decision is the subject of the next section.

### Group IV in ranges: `views::all_t` and the ownership decision

Every adaptor's deduction guide routes its range argument through `std::views::all_t`, and that alias is where the library decides whether the resulting view will *refer to* the source or *own* it.

```cpp
std::views::all_t<std::vector<int>&>;   // std::ranges::ref_view<std::vector<int>>
std::views::all_t<std::vector<int>>;    // std::ranges::owning_view<std::vector<int>>
```

An lvalue container yields a `ref_view`, which stores a pointer and owns nothing. An rvalue container yields an `owning_view`, which moves the container in and keeps it alive. The consequence is that the widely repeated warning that a view over a temporary always dangles is not correct: piping an rvalue container is safe, because deduction selected ownership.

```cpp
auto owning = std::vector<int>{1, 2, 3, 4} | rv::filter([](int x){ return x % 2 == 0; });
for (int x : owning) { /* yields 2 and 4 — the vector was moved into the pipeline */ }
```

What dangles is a view over a **named local**, because a named lvalue selects `ref_view`, as the broken function under Group II showed. The deciding factor is the value category of the argument at the point where the pipeline was built, and `views::all_t` is the mechanism that reads it.

Class template argument deduction appears elsewhere in the library as well. The view types carry deduction guides, and `std::ranges::subrange` is the most commonly constructed of them.

```cpp
auto sr = std::ranges::subrange(v.begin(), v.end());
// deduces std::ranges::subrange<__normal_iterator<int*, vector<int>>,
//                               __normal_iterator<int*, vector<int>>,
//                               std::ranges::subrange_kind::sized>
```

The third template argument is deduced too: the guide determines that the iterator and sentinel support a difference operation and therefore selects the sized specialisation, which allows `size()` to be O(1).

### Group V in ranges: the alias templates are deduction machinery

The library's vocabulary types are Group V constructs, meaning they compute a type from types already known rather than deducing from an initializer or an argument. Each is defined through `decltype` applied to an expression involving `std::declval`, and the definition can be reproduced in a few lines.

```cpp
template <class R>
using my_range_value_t =
    std::remove_cvref_t<decltype(*std::ranges::begin(std::declval<R&>()))>;

std::is_same_v<my_range_value_t<std::vector<int>>,
               std::ranges::range_value_t<std::vector<int>>>;      // true

auto pipe = v | rv::transform([](int x){ return x * 1.5; });
std::is_same_v<my_range_value_t<decltype(pipe)>,
               std::ranges::range_value_t<decltype(pipe)>>;        // true, and the type is double
```

Nothing is evaluated in that definition: `std::declval<R&>()` manufactures a fictional range, `std::ranges::begin` is never actually called, and `decltype` merely classifies the result. The whole family works the same way.

| Alias | What it names, for a range `R` |
|---|---|
| `std::ranges::iterator_t<R>` | The type returned by `ranges::begin(r)` |
| `std::ranges::sentinel_t<R>` | The type returned by `ranges::end(r)`, which need not equal the iterator type |
| `std::ranges::range_reference_t<R>` | The type of `*it`, which decides what a loop variable may bind to |
| `std::ranges::range_value_t<R>` | That reference type with references and cv-qualifiers removed |
| `std::ranges::range_difference_t<R>` | The signed type used for distances |
| `std::ranges::borrowed_iterator_t<R>` | An iterator when the range is safe to return one from, and `std::ranges::dangling` otherwise |

The values observed for the two-stage pipeline from the opening section illustrate why these aliases are needed rather than optional. Its `range_value_t` is a plain `int`, but its `iterator_t` is a nested class of the outer view, and its `sentinel_t` happens to be that same type here while in general it need not be.

#### `dangling` is a deduced return type used as a safety device

The last alias in the table deserves separate attention, because it shows deduction being used to prevent a bug rather than merely to save typing. An algorithm that would return an iterator into an rvalue range instead returns `std::ranges::dangling`.

```cpp
std::ranges::borrowed_iterator_t<std::vector<int>&>;   // __normal_iterator<int*, vector<int>>
std::ranges::borrowed_iterator_t<std::vector<int>>;    // std::ranges::dangling

auto it = std::ranges::find(std::vector<int>{1, 2, 3}, 2);
std::cout << *it;
// error: no match for 'operator*' (operand type is 'std::ranges::dangling')
```

The deduced type carries the safety property, and the error arrives at the point of use rather than as undefined behaviour at run time. Naming the range restores the ordinary iterator type, because an lvalue range is a borrowed range for this purpose.

```cpp
auto data = make_vector();
auto it = std::ranges::find(data, 2);      // a real iterator into `data`
```

---

### The five groups, as they appear in ranges

| Group | Deduces from | Where it appears in ranges |
|---|---|---|
| I | An initializer | `auto pipe = rng \| adaptor(...)`, which is mandatory because the type is unnameable; `auto&&` as the loop variable in range-based `for`; structured bindings over `zip` and `enumerate` |
| II | A return statement | Functions returning pipelines, which must use a deduced return type; `decltype(auto)` for element access that preserves the reference |
| III | Call arguments | Constrained parameters such as `std::ranges::input_range auto&&`; forwarding a range with `std::forward` into `views::all` |
| IV | Constructor arguments or a value | Deduction guides on every view; `views::all_t` choosing `ref_view` or `owning_view`; `subrange` deducing its kind |
| V | Nothing, the type is named | `iterator_t`, `sentinel_t`, `range_value_t`, `range_reference_t`, `range_difference_t`, and `borrowed_iterator_t`, all defined through `decltype` and `declval` |

### Pitfalls that live at the intersection

#### A deduced pipeline is a view, not a snapshot.
Group I deduces a lightweight object that refers to the source, so a later change to the source changes what the pipeline yields. Materialising into a container is the remedy where a snapshot is intended, and `std::ranges::to` provides it from C++23 onwards, although libstdc++ ships that facility only from GCC 14, so an explicit `ranges::copy` into a `back_inserter` remains the portable form.

#### A deduced return type hides whether the result refers or owns.
The signature `auto f(...)` reveals nothing about the `ref_view` inside, so the lifetime obligation has to be reasoned out from what was piped in rather than read off the declaration.

#### `auto&` is wrong more often than it looks.
Any pipeline whose reference type is a prvalue rejects it, and that includes every `transform_view` whose function returns by value. `auto&&` is correct in both cases.

#### `const auto&` is wrong for several view types.
The caching that makes `filter_view` efficient also makes it non-const-iterable, and `drop_while_view` and `split_view` behave the same way. Taking views by value or by forwarding reference avoids the problem entirely.

#### Reading a deduced type is often faster than reasoning about it.
Given how large these types are, printing `std::ranges::range_reference_t<decltype(pipe)>` answers questions about binding far more quickly than working through the adaptors by hand.

---

### The run-time counterpart: type erasure

All of the above is compile-time deduction, and the concrete pipeline type is fixed before the program runs. That property is what allows the optimiser to inline an entire pipeline, and it is also what prevents two different pipelines from being stored in the same variable, returned from the same virtual function, or held in a container together.

The standard library offers no type-erased view. A facility of that kind exists in range-v3 under the name `any_view`, and a proposal exists to standardise something similar, but no such type is present in C++23 or in libstdc++ 13. Where run-time flexibility is genuinely required, the erasure has to be built in the ordinary way, most simply by erasing the *consumer* rather than the range.

```cpp
void consume(std::function<void(int)> sink, std::ranges::input_range auto&& r) {
    for (auto&& x : r) sink(x);
}

int total = 0;
consume([&](int x){ total += x; }, v | rv::filter([](int x){ return x % 2 == 0; }));
// total is 6
```

Here, the pipeline keeps its concrete deduced type, which the template parameter absorbs, and only the callback is erased. Erasing the range itself instead requires a hand-written wrapper holding a virtual iterator interface, and the cost is the one type erasure always carries: an indirect call for every increment and every dereference, which is precisely the cost that deduced pipeline types avoid.

Sources:

* https://medium.com/@nubb/c-9-type-deduction-in-c-208c804dd792
* Effective Modern C++ by Scott Meyers
* https://www.linkedin.com/pulse/deferred-type-deduction-c-implementing-type-erased-dynamic-gholami-emktf/
