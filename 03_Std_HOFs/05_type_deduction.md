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

Almost every surprise in this area reduces to one of the two rule sets being applied where the other was expected. The remainder of the tutorial works through the groups in turn, and each group closes with the failures that follow from confusing them.

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

The form written with `= {...}` deduces a `std::initializer_list`, whereas a template parameter would never deduce that type from the same argument, as the section on non-deduced contexts in Group III demonstrates. Before C++17 the form `auto b2{1}` also produced an `initializer_list`, which is why older books and older advice disagree with the behaviour shown here.

### `decltype(auto)` variables

The `decltype(auto)` form, introduced in C++14, occupies the same syntactic position as `auto` but selects the other rule set. It deduces positionally in the same way that `auto` does, and then applies the rules of `decltype`.

```cpp
int  gi = 5;
auto           v1 = gi;    // int   — auto strips, as always
decltype(auto) v2 = gi;    // int   — the entity rule described under decltype applies
decltype(auto) v3 = (gi);  // int&  — the parenthesis rule described under decltype applies
```

### Structured bindings

A structured binding, added in C++17, introduces names for the individual pieces of a tuple, a pair, an array, or a public aggregate. The `auto` that appears in front of the bracketed name list applies to a hidden object rather than to the individual names, and that hidden object is deduced by the rules given for `auto` variables above.

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

#### A variable declared with `auto` copies silently

Because `auto` strips references, a function that returns `const T&` yields a fresh `T` when its result is captured with plain `auto`, and in a loop over a container of strings that copy can dominate the run time.

```cpp
auto row = matrix.get_row(i);            // a copy is made, however large the row is
const auto& row2 = matrix.get_row(i);    // no copy is made
```

**Solution.** The remedy is not to avoid `auto` but to state the intent through the accompanying qualifiers, because each of the three forms answers a different question and none of them is a default that fits every situation. The form `const auto&` is appropriate whenever the object is merely inspected, and it costs nothing even when the initializer happens to be a temporary, because binding a `const` reference to a temporary extends that temporary's lifetime to match the reference. The form `auto&` is appropriate whenever the original object is to be modified through the new name, and it fails to compile when the initializer is not a modifiable lvalue, which is a useful check rather than an obstacle. Plain `auto` is appropriate only when an independent copy is genuinely wanted, for instance because the copy is about to be modified while the original is left untouched, or because the original is about to be destroyed.

```cpp
const auto& observed  = matrix.get_row(0);   // inspection only, and no copy is made
auto&       to_modify = matrix.row_ref(0);   // the original is to be changed through this name
auto        owned     = matrix.get_row(0);   // a copy is wanted deliberately
```

The same three-way choice applies to the loop variable of a range-based `for`, where the consequences are multiplied by the number of elements. That is the reason an unqualified `auto` in a loop over a container of strings or of vectors deserves a second look, even though the same spelling over a container of `int` costs nothing at all.

#### A variable declared with `auto` can capture a proxy type

Some expressions return a proxy object rather than the value they appear to return, and `auto` faithfully deduces the proxy.

```cpp
std::vector<bool> vb{true, false};
auto p1 = vb[0];          // std::_Bit_reference, which is not bool
auto p2 = bool(vb[0]);    // bool
```

The container `std::vector<bool>` stores bits and therefore returns a proxy, so the deduced variable holds a reference back into the vector; the variable can consequently change meaning later, or dangle if the vector is destroyed.

**Solution.** The remedy has two halves, and only the first of them is a matter of syntax. The first half is to force the intended type at the point of capture, for which three spellings serve equally well: naming the type outright, applying a functional-style conversion, or applying a `static_cast`. All three produce a genuine `bool` that keeps its value when the vector is modified afterwards, whereas a variable holding the proxy would silently change with it.

```cpp
bool b1 = vb[0];                      // the type is named outright
auto b2 = bool(vb[0]);                // a functional-style conversion
auto b3 = static_cast<bool>(vb[0]);   // an explicit cast, which is the most searchable of the three

vb[0] = false;                         // afterwards b1, b2 and b3 all still read true,
                                       // whereas a variable holding the proxy would read false
```

The second half is recognising that the question arises at all, since nothing in the source announces a proxy. The libraries that return proxies do so deliberately and tend to say so in their documentation: `std::vector<bool>` packs its elements into bits, expression-template libraries such as Eigen defer evaluation so that a whole expression can be optimised as a unit, and the range views of the standard library are lazy by design. When an element or a subexpression is captured from a library of that kind, printing the deduced type with the helper given at the start settles the matter in one line, and naming the type explicitly is the safer default whenever the answer is not immediately obvious.

#### A non-const `auto&` cannot bind to a temporary

```cpp
auto& r = x + 1;
// error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'
```

**Solution.** Two spellings accept a temporary, and the choice between them is a matter of intent rather than of taste. The form `const auto&` binds and extends the temporary's lifetime to that of the reference, and it announces that the result will only be read. The form `auto&&` binds and extends in the same way, and being a forwarding reference it accepts an lvalue just as readily while preserving whether the initializer was modifiable.

```cpp
const auto& t1 = gi + 1;   // deduced as const int&, and the temporary lives as long as t1
auto&&      t2 = gi + 1;   // deduced as int&&, and the temporary likewise lives as long as t2
```

In ordinary code where a value is simply being examined, `const auto&` is the clearer of the two because it states the restriction. In generic code, where the initializer might be an lvalue on one instantiation and a temporary on another, `auto&&` is the correct default precisely because it covers both without a second spelling. What should be avoided is reaching for plain `auto` merely to make the diagnostic disappear, since that introduces a copy the original code did not ask for and hides the question rather than answering it.

#### A declaration of `auto` without an initializer is ill-formed

```cpp
auto x;    // error: declaration of 'auto x' has no initializer
```

**Solution.** The declaration must either be given something to deduce from or must state its type outright. Supplying an initializer is usually the better of the two, because a variable that is declared in one place and assigned in another can be read before it is set, and initialising at the point of declaration removes that possibility entirely.

```cpp
auto x = 0;      // an initializer is supplied, so deduction has something to work from
int  y;          // the type is stated, and the variable is deliberately left uninitialised
int  z = 0;      // the type is stated and the value is initialised, which is safer still
```

The same reasoning explains a restriction that is otherwise puzzling. A non-static data member cannot be declared with `auto`, because the initializer of a member is not necessarily present in the class body and different constructors may supply different ones, so there would be no single expression to deduce from. Only a `static constexpr` member, whose initializer is fixed and appears in the class body, may use `auto`.

```cpp
struct S {
    // auto a = 0;                     // ill-formed: a non-static member cannot use auto
    int b = 0;                          // the type is written out, as it must be
    static constexpr auto c = 42;       // permitted, because the initializer is right here
};
```

#### Brace initialisation does not behave uniformly

The form `auto x = {1};` deduces `std::initializer_list<int>`, the form `auto x{1};` deduces `int` from C++17 onwards, and a braced list passed to a plain `T` parameter does not deduce at all, as the section on non-deduced contexts in Group III shows.

**Solution.** The safest approach is to make the intent explicit in the spelling rather than to rely on which of several similar-looking forms was written. For a single scalar value the plain `=` form is unambiguous under every standard and does not change meaning between C++11 and C++17. For a genuine list, naming the container removes any question of what was deduced. For a list that really is meant to be an `initializer_list`, saying so in the declaration documents an intention that would otherwise have to be inferred by a reader.

```cpp
auto scalar = 1;                            // int, under every standard
std::vector<int> vec{1, 2};                 // a container, named outright
std::initializer_list<int> list = {1, 2};   // an initializer_list, and evidently so
```

The reason this matters beyond style is a lifetime question. A deduced `initializer_list` refers to a backing array whose lifetime is tied to the initializer expression, so an `initializer_list` variable that outlives the statement which created it refers to storage that no longer exists. Naming the container copies the elements into storage the container owns, which removes that hazard at the same time as removing the ambiguity.

#### Plain `auto` in a range-based `for` loop over a map copies every element

```cpp
for (auto [k, v] : m)         // every pair is copied
for (const auto& [k, v] : m)  // nothing is copied
```

**Solution.** The binding form should be chosen from what the loop body does, exactly as for any other variable, and the structured-binding syntax does not change that reasoning even though it obscures it. A loop that only reads should use `const auto&`, which copies nothing and additionally prevents an accidental write. A loop that modifies the mapped values should use `auto&`, which reaches the container's own elements. A loop that genuinely needs its own copy, perhaps because it modifies that copy while leaving the container intact, should use plain `auto` and should say so in a comment, because a reader will otherwise assume the copy was an oversight.

```cpp
for (const auto& [key, value] : m) total += value;     // read-only, and nothing is copied
for (auto& [key, value] : m) value += 10;              // the map's own values are modified
for (auto [key, value] : m) { value += 10; }           // a copy is modified; the map is unchanged
```

The cost is easy to underestimate because nothing at the loop announces it. Each iteration of the first form copies a `std::string` key together with the mapped value, and the key copy involves an allocation whenever the string exceeds the small-string buffer, so a loop over a large map can spend more time allocating and releasing keys than doing its own work. One detail is worth knowing in advance: the key of a `std::map` is `const`, so `auto&` binds the key as `const std::string&` and only the mapped value is modifiable, which is the intended design rather than a limitation to work around.

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

#### A pair of parentheses around a returned name changes the deduced type

This is the single most dangerous character sequence in the topic.

```cpp
decltype(auto) bad() { int local = 42; return (local); }   // the return type is int&
std::cout << bad();
// warning: reference to local variable 'local' returned [-Wreturn-local-addr]
// AddressSanitizer: SEGV
```

Writing `return local;` deduces `int`, whereas writing `return (local);` deduces `int&` and therefore returns a reference to an object that has already been destroyed. GCC issues a warning in this case only because the example is simple enough to analyse; routed through a wrapper function the same mistake goes undetected.

**Solution.** Three separate habits together remove this class of error, and each of them addresses a different part of it. The first is never to write parentheses around a returned expression unless a reference is genuinely the intention, since the parentheses carry no other meaning in that position and their only effect is to switch `decltype` from the entity rule to the value-category rule.

```cpp
decltype(auto) plain() { int local = 42; return local; }    // int, and the value is copied out
```

The second habit applies whenever `decltype(auto)` really is meant to return a reference, and it is to confirm that the referent outlives the call. A reference to a function-local object never does, whereas a reference to an object with static storage duration, to a member of the object the function was called on, or to an element of a container the caller owns does.

```cpp
int& counter() { static int c = 41; return c; }
decltype(auto) safe_ref() { return (counter()); }   // int&, and the referent is a static object
```

The third habit is to prefer plain `auto` as the return type unless reference preservation is a stated requirement of the function, because `auto` strips references and therefore cannot produce this failure at all. `decltype(auto)` is the right choice for a forwarding wrapper such as the `at` function shown above, and the wrong choice for an ordinary function that merely happens to have been written with it.

#### A deduced `auto` return type strips references

```cpp
auto           get(std::vector<int>& v) { return v[0]; }   // returns int, which is a copy
decltype(auto) ref(std::vector<int>& v) { return v[0]; }   // returns int&, which is writable
```

An attempt to write `get(v) = 42;` fails to compile, which at least makes the problem visible; the subtler cost is a copy on a hot path that nothing draws attention to.

**Solution.** Where a reference is wanted, two spellings supply it, and the choice between them depends on whether the referent's type is known in advance. In a non-template function whose element type is fixed, writing the return type out is the clearest option available, because it states the contract in the signature, which is where a reader will look for it.

```cpp
int& get_explicit(std::vector<int>& v) { return v[0]; }    // the contract is visible in the signature
```

In a template, where the element type is not known and may itself turn out to be a reference or a proxy, `decltype(auto)` is the appropriate spelling, since it reproduces whatever the underlying expression produced without the author having to enumerate the possibilities in advance.

```cpp
template <class C>
decltype(auto) first(C&& c) { return *std::begin(std::forward<C>(c)); }
```

Where a copy is genuinely wanted, plain `auto` remains correct and should be left as it is. The point of this pitfall is not that deduced `auto` returns are a mistake, but that the decision between returning a copy and returning a reference is being made by the return-type spelling alone, so that spelling deserves as much attention as the body of the function receives.

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

The parameter `T` appears to the left of a `::`, so the compiler would have to invert an arbitrary mapping in order to recover it, which is not something deduction attempts.

A braced list is likewise not deducible for a plain `T` parameter, which is the precise contrast with the behaviour of `auto` described under braced initialisers in Group I.

```cpp
template <class T> void f(T);
f({1, 2});   // error: no matching function for call to 'f(<brace-enclosed initializer list>)'
```

### `auto` parameters

Writing `auto` in a parameter list turns the function into a template, and the parameter is deduced using the by-value rules described under template parameter deduction above. Four related spellings exist.

```cpp
auto gl = [](auto a, auto b) { return a + b; };        // a generic lambda, from C++14
auto twice(auto x) { return x + x; }                    // an abbreviated function template, C++20
auto sum(std::integral auto a, std::integral auto b) { return a + b; }   // constrained, C++20

auto named = []<class T>(const std::vector<T>& v) { return v.size(); };  // templated lambda, C++20
```

The constrained form is preferable whenever the set of acceptable types is known in advance, because a violation is then reported at the call site with the name of the unsatisfied concept rather than as a cascade of errors from deep inside the function body. The explicit `[]<class T>` form is the one to reach for when the deduced type needs a name, either so that two parameters can be required to have the same type or so that the type can be referred to inside the body.

### Common pitfalls and solutions

#### The token `auto&&` does not always mean rvalue reference

In any context where deduction takes place, both `auto&&` and `T&&` are forwarding references, and an lvalue argument therefore produces an lvalue reference. The `&&` in a non-deduced declaration such as `void f(std::string&&)` genuinely is an rvalue reference.

**Solution.** The distinguishing question is whether the `&&` is attached to something that is being deduced, and that question can be answered by inspection in every case. When the `&&` follows `auto`, or follows a template parameter belonging to the function itself, deduction is taking place and the parameter is a forwarding reference. When the `&&` follows a concrete type, or follows a template parameter belonging to the enclosing *class* template rather than to the function, no deduction happens at that position and the parameter genuinely is an rvalue reference.

```cpp
template <class T> void forwarding(T&& x);        // a forwarding reference: T is deduced here
void rvalue_only(std::string&& s);                 // an rvalue reference: the type is concrete

template <class T>
struct Holder {
    void take(T&& x);                              // an rvalue reference: T belongs to the class,
};                                                 // so nothing is deduced at this position
```

Where an rvalue reference is genuinely wanted from a function template, the way to obtain one is to remove the deduction rather than to hope that the spelling will supply it, most simply by naming the type or by constraining the parameter so that only an rvalue is accepted. Where a forwarding reference is wanted, an obligation comes with it: the parameter must be passed on with `std::forward<T>` at the point of use, because a parameter that has a name is itself an lvalue however it was declared, and forwarding is what restores the category the caller supplied.

#### Deduction cannot be expected in a non-deduced context

Neither `typename Id<T>::type` nor a braced initialiser list can be deduced from, and the resulting diagnostics tend to report only that no matching function was found.

**Solution.** Two remedies exist for the dependent-name case, and they differ in whether the signature is left alone. The first is to supply the template argument explicitly at the call site, which requires no change to the function at all and is appropriate when the non-deduced position was deliberate, for instance when it exists precisely to prevent deduction from an argument that should not drive it.

```cpp
template <class T> void g(typename Id<T>::type);
g<int>(1);      // the argument is supplied by hand, so nothing needs to be deduced
```

The second is to restructure the signature so that `T` appears somewhere deducible, which is appropriate when the indirection served no purpose to begin with and was simply an accident of how the code grew.

```cpp
template <class T> void g(T);
g(1);           // T is now in a deducible position, and the call needs no annotation
```

For the braced-list case the remedy is either to give the parameter a type that a braced list can initialise, or to construct the object at the call site so that an ordinary argument is passed and deduction proceeds normally.

```cpp
template <class T> void f(std::initializer_list<T>);
f({1, 2});                      // the parameter type now matches what a braced list produces

template <class T> void h(T);
h(std::vector{1, 2});           // an object is constructed first, and deduction proceeds normally
```

#### An unconstrained `auto` parameter accepts everything and fails late

Because no requirement is stated, an unsuitable argument is diagnosed deep inside the function body rather than at the call site.

**Solution.** Attaching a concept to the parameter moves the diagnosis to the call and names the requirement that was not satisfied, which is the difference between an error a reader can act upon and a page of instantiation backtrace. The standard library supplies concepts for the common cases, and a `requires` clause covers anything the library does not already name.

```cpp
auto sum_unconstrained(auto a, auto b) { return a + b; }                     // fails inside the body
auto sum(std::integral auto a, std::integral auto b) { return a + b; }       // fails at the call

template <class T> requires requires (T t) { t.begin(); t.end(); }
auto count_of(const T& c) { return std::distance(c.begin(), c.end()); }      // an ad-hoc requirement
```

A constraint brings two further benefits beyond the improved diagnostic. It documents the function's contract in the signature, where it can be read without opening the body, and it participates in overload resolution, so a constrained overload can coexist with a more general fallback for types that do not satisfy it. The accompanying cost is that the concept has to be chosen with some care, since a constraint that is too narrow rejects arguments that would have worked perfectly well, and one that is too broad restores the original problem in a new disguise.

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

#### Class template argument deduction is all or nothing

A declaration such as `std::pair<int> p{1, 2.5};` is an error, because partial argument lists are not deduced.

**Solution.** Either every argument is written explicitly or none is, and both spellings are perfectly ordinary. Writing them all out is appropriate when a specific instantiation is required, for instance because the deduced answer would not be the wanted one, and omitting them all is appropriate when the constructor arguments already determine the answer unambiguously.

```cpp
std::pair<int, double> p1{1, 2.5};   // every argument is written out
std::pair              p2{1, 2.5};   // no argument is written, and both are deduced
// std::pair<int>      p3{1, 2.5};   // ill-formed: a partial list is not completed by deduction
```

Where one member should have a specific type and the other should follow from its argument, the way to express that is to convert the argument rather than to write a partial list, because the conversion makes the intention visible at the point where it takes effect and leaves deduction to do the rest.

```cpp
std::pair p4{1, static_cast<float>(2.5)};   // deduces std::pair<int, float>
```

#### Braces and parentheses select different constructors, and the difference is easy to miss

```cpp
std::vector v2(3, 0.5);   // three elements, each equal to 0.5
std::vector v3{3, 0.5};   // two elements, and the deduced type is std::vector<double>
```

In the second line the literal `3` is silently converted to `3.0`. That conversion is permitted only because `3` is a constant expression whose value is exactly representable as a `double`; the same code written with a variable in place of the literal would be rejected as narrowing.

**Solution.** The reliable habit is to write the element type explicitly whenever a container is being sized and filled rather than listed, because the two constructors then remain distinguishable by their punctuation alone and no deduction contributes to the outcome. Braces can then be reserved for the case where the intention really is to list the elements one by one.

```cpp
std::vector<double> filled(3, 0.5);   // three elements, and the type is beyond doubt
std::vector<double> listed{3, 0.5};   // two elements, and the type is beyond doubt
std::vector         deduced{1, 2, 3}; // a plain list of homogeneous values, where deduction is safe
```

A second habit reduces the remaining risk further, which is to check the resulting size rather than to assume it, since the two forms differ in the number of elements as well as in the element type. It is also worth remembering that when a container is constructed from a count and a fill value, the parenthesis form is the only one that expresses that intention, and no braced spelling is equivalent to it.

#### Class template argument deduction does not apply everywhere

It was not available for alias templates in C++17, although C++20 added that capability, and it never applies to a bare `auto` data member, because it is fundamentally a feature of the point of construction.

**Solution.** Where deduction is unavailable the type has to be written, and the two situations call for different spellings. For a data member the type is written out in the class body, which is required in any case for a non-static member, and a `using` alias keeps the declaration readable when the type would otherwise be long enough to obscure the member's purpose.

```cpp
struct Holder {
    using Map = std::map<std::string, std::vector<int>>;
    Map data;                                   // the type is named, since auto is not available
};
```

For code that must also compile under C++17, an alias template that would have relied on deduction can be replaced by a small factory function, because function templates have deduced their arguments since long before class templates could, and therefore need no new language feature at all.

```cpp
template <class T> using Ptr = std::shared_ptr<T>;
// Ptr p = std::make_shared<int>(1);            // not deducible under C++17

template <class T> Ptr<T> make_ptr(T value) { return std::make_shared<T>(std::move(value)); }
auto p = make_ptr(1);                            // the function template deduces T as int
```

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

#### The parenthesis rule is easy to forget

The expressions `decltype(x)` and `decltype((x))` differ, and because `decltype(auto)` adopts these rules the difference propagates into return types, as the parenthesis pitfall in Group II shows.

**Solution.** Two practices keep this distinction from causing trouble. The first is to treat any parentheses appearing inside a `decltype` as significant rather than decorative, and to remove them unless a reference is intended. No other operator in the language changes its meaning when its operand is parenthesised, so the widespread habit of adding parentheses for visual clarity is actively harmful in this one position.

```cpp
decltype(gi)     // int, which is the declared type of the entity
decltype((gi))   // int&, which is what the extra parentheses requested
```

The second practice is to reach for the printing helper whenever the operand of a `decltype` is more complicated than a bare name, because the value-category rule depends on the whole expression rather than on its outermost operator alone. A subexpression that looks like a plain member access may turn out to be an lvalue, an xvalue, or a prvalue depending on how the object it names was obtained, and printing the answer takes considerably less time than deriving it from the rules.

#### Calling `std::declval` in evaluated code is an error

The function template has no definition, so any use outside an unevaluated context fails at link time.

**Solution.** The remedy is to confine `std::declval` to the contexts that never evaluate their operand, which are `decltype`, `sizeof`, `noexcept`, and the requirement bodies of a `requires` expression. Within those contexts it is exactly the right tool and is frequently the only one available, because it manufactures a value of a type that may have no usable constructor whatsoever.

```cpp
using Result = decltype(std::declval<Widget&>().compute(std::declval<const Input&>()));
static_assert(sizeof(std::declval<Widget>()) > 0);
constexpr bool nothrow = noexcept(std::declval<Widget&>().compute(std::declval<const Input&>()));
```

Where a real object is needed rather than a fictional one, the remedy is to construct an object, and where construction is genuinely impossible the surrounding code should be restated as a type query rather than as a computation. A link failure whose message names `declval` is almost always the sign that an expression intended purely as a query has escaped into evaluated code, most often because a surrounding `decltype` was dropped during an edit.

#### The trait `std::decay_t` is often reached for when `std::remove_cvref_t` was meant

In addition to stripping references and cv-qualifiers, `std::decay_t` converts arrays to pointers and functions to function pointers, which is rarely the intent when the goal is merely to obtain a plain value type.

**Solution.** The choice follows from a single question, which is whether the array and function conversions are wanted alongside the stripping. When the purpose is to reproduce what a by-value parameter or an `auto` variable would deduce, `std::decay_t` is correct precisely because it performs those conversions as well. When the purpose is only to remove references and cv-qualifiers, so that two types can be compared or a value can be stored, `std::remove_cvref_t` is correct, and it has the additional merit of saying exactly that in its name.

```cpp
std::decay_t<const int(&)[3]>         // const int*, since the array decayed to a pointer
std::remove_cvref_t<const int(&)[3]>  // int[3], since the extent survived and only const went
```

The difference is easy to overlook because the two traits agree on the scalar types that appear in most examples, and they diverge only for arrays and for functions. A trait chosen out of habit rather than by intent therefore compiles and behaves correctly right up until the day an array is passed to it, which is why the question above is worth asking at the moment the trait is written rather than later.

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
4. Whenever the deduced type is in doubt, it should be printed rather than guessed, using the helper given at the start of this tutorial.

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
