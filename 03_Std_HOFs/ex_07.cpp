// ===========================================================================
// Type deduction in C++ — a runnable demonstration
// ===========================================================================
// This program demonstrates every deduction mechanism, grouped by the question
// that explains why their rules differ: what does the compiler learn the type
// from? Each section prints what was actually deduced, so no claim here has to
// be taken on trust.
//
//   Build : g++ -std=c++20 -Wall -Wextra -o type_deduction_demo ex_07.cpp
//   Run   : ./type_deduction_demo
//
// Constructs that cannot compile are shown commented out, with the diagnostic
// that g++ 13.3 actually produces quoted beneath them.
// ===========================================================================

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// A helper that prints any type as a readable string, so that a deduced type
// can be observed rather than guessed. It works on GCC and Clang because both
// place the substituted template argument inside __PRETTY_FUNCTION__.
// ---------------------------------------------------------------------------
template <class T>
constexpr std::string_view type_name() {
    std::string_view p = __PRETTY_FUNCTION__;
    auto s = p.find("T = ") + 4;
    auto e = p.find(";", s);
    return p.substr(s, e - s);
}
#define SHOW(...) std::cout << "    " << #__VA_ARGS__ << "  ->  " \
                            << type_name<__VA_ARGS__>() << "\n"

static void heading(const char* text) { std::cout << "\n== " << text << " ==\n"; }

// A type that counts how many times it has been copied, which makes an
// accidental copy visible at run time rather than merely theoretical.
struct Loud {
    int id = 0;
    static inline int copies = 0;
    Loud() = default;
    Loud(int i) : id(i) {}
    Loud(const Loud& other) : id(other.id) { ++copies; }
    Loud& operator=(const Loud&) { ++copies; return *this; }
};

int   gi = 5;
int   func(int) { return 0; }
static const std::vector<Loud>& borrow_row();

// ===========================================================================
// GROUP I — deduction from an initializer
// ===========================================================================
static void group_one() {
    heading("GROUP I: deduction from an initializer");

    const int  ci   = 5;
    const int& cref = ci;
    int arr[3]{};

    // The plain `auto` form uses the template-deduction rules, which means that
    // references and top-level const qualifiers are discarded, and that arrays
    // and functions decay to pointers.
    auto a1 = ci;      SHOW(decltype(a1));   // the const qualifier was discarded
    auto a2 = cref;    SHOW(decltype(a2));   // both the reference and the const went
    auto a3 = arr;     SHOW(decltype(a3));   // the array decayed to a pointer
    auto a4 = func;    SHOW(decltype(a4));   // the function decayed to a pointer

    // Adding an ampersand changes the rule set that applies, so qualifiers are
    // preserved and no decay takes place.
    auto& a5 = ci;     SHOW(decltype(a5));
    auto& a6 = arr;    SHOW(decltype(a6));

    // The `auto&&` form is a forwarding reference, so the value category of the
    // initializer decides whether an lvalue or an rvalue reference results.
    auto&& a7 = gi;    SHOW(decltype(a7));   // the initializer was an lvalue
    auto&& a8 = 42;    SHOW(decltype(a8));   // the initializer was an rvalue

    // A braced initializer is the single place where `auto` and template
    // deduction genuinely disagree with one another.
    auto b1 = 1;       SHOW(decltype(b1));
    auto b2{1};        SHOW(decltype(b2));   // this yields int from C++17 onwards
    auto b3 = {1};     SHOW(decltype(b3));   // the = {...} form yields an initializer_list
    auto b4 = {1, 2};  SHOW(decltype(b4));

    // The `decltype(auto)` form occupies the same position as `auto` but
    // selects the decltype rules instead, which keep everything.
    decltype(auto) c1 = gi;    SHOW(decltype(c1));
    decltype(auto) c2 = (gi);  SHOW(decltype(c2));  // the parentheses change the answer
}

// ===========================================================================
// GROUP I continued — structured bindings
// ===========================================================================
static void structured_bindings() {
    heading("GROUP I: structured bindings");

    std::pair<int, std::string> ps{1, "x"};

    // The `auto` in front applies to a hidden object rather than to the
    // individual names, so a plain `auto` copies the whole pair.
    auto [n1, s1] = ps;
    n1 = 99;
    std::cout << "    after `auto [n1, s1]` and n1 = 99, ps.first is " << ps.first
              << ", because the pair was copied\n";

    // Writing `auto&` binds the hidden object to the original, so assignments
    // through the names reach the original pair.
    auto& [n2, s2] = ps;
    n2 = 99;
    std::cout << "    after `auto& [n2, s2]` and n2 = 99, ps.first is " << ps.first
              << ", because the pair was referenced\n";

    // Applying decltype to a binding name reports the referenced type and never
    // a reference type, which holds even when the binding used `auto&`.
    SHOW(decltype(n2));

    // In a loop over a map the read-only form avoids copying each pair, which
    // matters because the key is a std::string here.
    std::map<std::string, int> m{{"a", 1}, {"b", 2}};
    std::cout << "    reading a map with `const auto& [k, v]`:";
    for (const auto& [k, v] : m) std::cout << " " << k << "=" << v;
    std::cout << "\n";
}

// ===========================================================================
// GROUP II — deduction from a return statement
// ===========================================================================
// A deduced `auto` return type applies the template-deduction rules, so the
// reference produced by operator[] is stripped and the caller receives a copy.
static auto ret_by_value(std::vector<int>& v) { return v[0]; }

// A `decltype(auto)` return type applies the decltype rules, so the reference
// survives and the caller can assign through the result.
static decltype(auto) ret_by_reference(std::vector<int>& v) { return v[0]; }

// A trailing return type is not deduction at all; it merely moves the written
// type to a position where it can mention the parameters.
static auto add_trailing(int a, double b) -> decltype(a + b) { return a + b; }

static void group_two() {
    heading("GROUP II: deduction from a return statement");

    std::vector<int> v{10, 20, 30};
    SHOW(decltype(ret_by_value(v)));
    SHOW(decltype(ret_by_reference(v)));
    SHOW(decltype(add_trailing(1, 2.5)));

    // Because the second function returns a reference, an assignment through
    // the call reaches the vector itself.
    ret_by_reference(v) = 99;
    std::cout << "    after `ret_by_reference(v) = 99`, v[0] is " << v[0] << "\n";

    // The first function returns a copy, so the equivalent assignment is
    // rejected outright rather than silently doing nothing.
    //   ret_by_value(v) = 99;
    // error: lvalue required as left operand of assignment
}

// ===========================================================================
// GROUP III — deduction from call arguments
// ===========================================================================
template <class T> void by_value(T)        { std::cout << "    by_value  deduced T = " << type_name<T>() << "\n"; }
template <class T> void by_ref(T&)         { std::cout << "    by_ref    deduced T = " << type_name<T>() << "\n"; }
template <class T> void by_cref(const T&)  { std::cout << "    by_cref   deduced T = " << type_name<T>() << "\n"; }
template <class T> void by_fwd(T&&)        { std::cout << "    by_fwd    deduced T = " << type_name<T>() << "\n"; }

// A pair of overloads that report the value category they were called with,
// which makes perfect forwarding observable rather than theoretical.
static std::string probe(const std::string&) { return "lvalue"; }
static std::string probe(std::string&&)      { return "rvalue"; }

// This wrapper preserves the value category of its argument, because the
// parameter is a forwarding reference and std::forward restores the category
// that reference collapsing recorded in T.
template <class T>
static std::string forwarding_wrapper(T&& arg) { return probe(std::forward<T>(arg)); }

// This wrapper loses the category, because a named parameter is always an
// lvalue once it has a name, whatever category the caller supplied.
template <class T>
static std::string naive_wrapper(T&& arg) { return probe(arg); }

// An `auto` parameter turns the function into a template, and constraining it
// means that an unsuitable argument is reported at the call site.
static auto twice(auto x) { return x + x; }
static auto sum_integral(std::integral auto a, std::integral auto b) { return a + b; }

static void group_three() {
    heading("GROUP III: deduction from call arguments");

    const int ci = 1;
    int&      r  = gi;
    int       a2[3]{};

    by_value(ci);   // the const is dropped because the argument is copied
    by_value(r);    // the reference is dropped for the same reason
    by_value(a2);   // the array decays to a pointer
    by_ref(ci);     // the const is part of the deduced type here
    by_cref(a2);    // no decay occurs, so the array extent survives
    by_fwd(gi);     // an lvalue argument makes T an lvalue reference
    by_fwd(5);      // an rvalue argument leaves T as a plain value type
    by_fwd(ci);     // the const is preserved along with the reference

    std::string s = "text";
    std::cout << "    forwarding_wrapper(s)            selects the " << forwarding_wrapper(s) << " overload\n";
    std::cout << "    forwarding_wrapper(std::move(s)) selects the " << forwarding_wrapper(std::move(s)) << " overload\n";
    std::cout << "    naive_wrapper(std::move(s))      selects the " << naive_wrapper(std::move(s))
              << " overload, because a named parameter is itself an lvalue\n";

    std::cout << "    twice(21) is " << twice(21)
              << " and twice(std::string(\"ab\")) is " << twice(std::string("ab")) << "\n";
    std::cout << "    sum_integral(2, 3) is " << sum_integral(2, 3) << "\n";

    // Some positions cannot be deduced from at all, because doing so would
    // require inverting an arbitrary mapping.
    //   template <class T> struct Id { using type = T; };
    //   template <class T> void g(typename Id<T>::type);
    //   g(1);
    // error: no matching function for call to 'g(int)'
    //
    // A braced list is likewise not deducible for a plain T parameter, which is
    // the precise contrast with the `auto` behaviour shown in group one.
    //   by_value({1, 2});
    // error: no matching function for call to 'by_value(<brace-enclosed initializer list>)'
}

// ===========================================================================
// GROUP IV — deduction of template arguments for a type
// ===========================================================================
template <class T> struct Box { T v; };
template <class T> Box(T) -> Box<T>;            // an explicit guide for the general case
Box(const char*) -> Box<std::string>;           // a guide that overrides the obvious answer

struct Agg { int a; double b; };                 // aggregates deduce from C++20 onwards

template <auto N> struct Fixed { static constexpr auto value = N; };

static void group_four() {
    heading("GROUP IV: deduction of template arguments for a type");

    std::vector v1{1, 2, 3};   SHOW(decltype(v1));
    std::pair   pr{1, 2.5};    SHOW(decltype(pr));

    // Braces and parentheses select different constructors, and the difference
    // changes both the element count and the deduced element type.
    std::vector v2(3, 0.5);    SHOW(decltype(v2));
    std::vector v3{3, 0.5};    SHOW(decltype(v3));
    std::cout << "    v2 holds " << v2.size() << " elements while v3 holds " << v3.size()
              << ", and the literal 3 in v3 became 3.0\n";

    Box b1{42};                SHOW(decltype(b1));
    Box b2{"hello"};           SHOW(decltype(b2));   // the guide turned char* into string
    Agg ag{1, 2.5};            SHOW(decltype(ag));
    (void)ag;

    SHOW(decltype(Fixed<42>::value));
    SHOW(decltype(Fixed<'c'>::value));

    // Class template argument deduction is all or nothing, so a partially
    // written argument list is rejected.
    //   std::pair<int> p{1, 2.5};
    // error: wrong number of template arguments (1, should be 2)
}

// ===========================================================================
// GROUP V — type computation without deduction
// ===========================================================================
static void group_five() {
    heading("GROUP V: type computation without deduction");

    const int  ci   = 5;
    int&       ref  = gi;
    const int& cref = gi;
    int arr[3]{};
    (void)arr;

    // The decltype operator reports the declared type of an entity verbatim.
    SHOW(decltype(gi));
    SHOW(decltype(ci));
    SHOW(decltype(ref));
    SHOW(decltype(cref));
    SHOW(decltype(arr));       // no decay takes place here
    SHOW(decltype(func));      // this is a function type rather than a pointer

    // Any other expression is classified by its value category instead, and a
    // pair of parentheses is enough to switch to that second mode.
    SHOW(decltype((gi)));      // an lvalue expression yields a reference type
    SHOW(decltype(gi + 1));    // a prvalue expression yields a plain value type
    SHOW(decltype(func(1)));   // the call is never evaluated, only classified

    // std::declval supplies a fictional value for an unevaluated context, which
    // allows a type to be queried without constructing an object at all.
    SHOW(decltype(std::declval<std::vector<int>&>().front()));

    // The transformation traits compute one type from another, and decay_t is
    // exactly the group one rule set expressed as a trait.
    SHOW(std::decay_t<int[5]>);
    SHOW(std::decay_t<int(int)>);
    SHOW(std::remove_cvref_t<const volatile int&>);   // arrays would survive this one
    SHOW(std::common_type_t<int, double>);
}

// ===========================================================================
// PITFALLS, each shown together with the solution that avoids it
// ===========================================================================
static const std::vector<Loud>& borrow_row() {
    static std::vector<Loud> row{Loud{1}, Loud{2}, Loud{3}};
    return row;
}

static void pitfalls() {
    heading("PITFALLS and their solutions");

    // Pitfall one: a variable declared with plain `auto` copies, because the
    // reference returned by the function is stripped away. The first call is
    // made before counting begins, so that the one-off cost of initialising the
    // function-local static is not attributed to the deduction being measured.
    borrow_row();
    Loud::copies = 0;
    auto copied = borrow_row();
    std::cout << "    `auto copied = borrow_row();` performed " << Loud::copies
              << " element copies\n";

    // The solution is to write const auto&, which observes without copying.
    Loud::copies = 0;
    const auto& observed = borrow_row();
    std::cout << "    `const auto& observed = borrow_row();` performed " << Loud::copies
              << " element copies\n";
    (void)copied; (void)observed;

    // Pitfall two: `auto` faithfully deduces a proxy type, so the variable keeps
    // referring back into the container instead of holding a value.
    std::vector<bool> vb{true, false};
    auto proxy = vb[0];
    SHOW(decltype(proxy));
    vb[0] = false;
    std::cout << "    after changing the vector, the proxy variable now reads "
              << std::boolalpha << bool(proxy) << "\n";

    // The solution is to name the intended type or to cast to it explicitly.
    vb[0] = true;
    auto value = bool(vb[0]);
    SHOW(decltype(value));
    vb[0] = false;
    std::cout << "    after the same change, the bool variable still reads " << value << "\n";

    // Pitfall three: a non-const `auto&` cannot bind to a temporary, whereas
    // both const auto& and auto&& can.
    //   auto& bad = gi + 1;
    // error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'
    const auto& ok1 = gi + 1;
    auto&&      ok2 = gi + 1;
    std::cout << "    `const auto&` and `auto&&` both bind to the temporary, giving "
              << ok1 << " and " << ok2 << "\n";

    // Pitfall four: a declaration with no initializer has nothing to deduce
    // from, and non-static data members may not use auto at all.
    //   auto nothing;
    // error: declaration of 'auto nothing' has no initializer

    // Pitfall five: returning a parenthesised local through decltype(auto)
    // deduces a reference to an object that is about to be destroyed. The
    // function below is left commented out because calling it is undefined
    // behaviour, and AddressSanitizer reports a segmentation fault when it runs.
    //   decltype(auto) bad() { int local = 42; return (local); }
    // warning: reference to local variable 'local' returned [-Wreturn-local-addr]

    // Pitfall six: `auto&&` denotes a forwarding reference wherever deduction
    // occurs, so it is not a promise that the initializer was an rvalue. The
    // group one output above shows the same spelling producing int& and int&&.

    // Pitfall seven: decay_t also converts arrays and functions to pointers,
    // which is rarely wanted when the goal is only to remove references and
    // const, so remove_cvref_t is usually the correct choice.
    SHOW(std::decay_t<int[5]>);
    SHOW(std::remove_cvref_t<int(&)[5]>);
}

int main() {
    std::cout << "Type deduction, demonstrated with observed results\n";
    group_one();
    structured_bindings();
    group_two();
    group_three();
    group_four();
    group_five();
    pitfalls();
    std::cout << "\ndone\n";
    return 0;
}
