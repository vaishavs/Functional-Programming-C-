// ===========================================================================
// Type deduction — TODO / DEBUG exercise
//
// SECTION A — TODO  : six functions and templates to implement from scratch.
// SECTION B — FIXME : five shipped items that are wrong on purpose.
//
// Markers
//   TODO(n)   — implement from scratch.
//   FIXME(n)  — shipped code is wrong on purpose; repair it.
//   HINT      — concrete syntax for the implementation or the repair.
//
// Flip a step's READY macro to 1 to activate its test. The file ships compiling
// and running with every step off. Most tests fail with an assertion until the
// step is done; the two marked STATIC fail at compile time instead, because a
// wrong answer there is a wrong TYPE rather than a wrong value.
//
//   Build : g++ -std=c++20 -Wall -Wextra -o deduction type_deduction_todo.cpp
//   Run   : ./deduction
// ===========================================================================

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define TODO1_READY 0
#define TODO2_READY 0
#define TODO3_READY 0
#define TODO4_READY 0
#define TODO5_READY 0
#define TODO6_READY 0
#define FIX1_READY  0
#define FIX2_READY  0
#define FIX3_READY  0
#define FIX4_READY  0
#define FIX5_READY  0

// ###########################################################################
// SECTION A — TODO
// ###########################################################################

// =============================================================================
// TODO 1 — an accessor that preserves the reference it was given
// =============================================================================
// TODO(1): element_at(c, i) must return whatever c[i] returns, keeping the
//   reference so that element_at(v, 1) = 77 modifies v.
//   HINT: declare the return type as decltype(auto) rather than auto, and
//   forward the container so the correct overload of operator[] is chosen:
//       template <class Container>
//       decltype(auto) element_at(Container&& c, std::size_t i)
//       { return std::forward<Container>(c)[i]; }
//   The stub below returns auto, which strips the reference and yields a copy.
// =============================================================================
template <class Container>
auto element_at(Container&& c, std::size_t i) {
    // TODO(1)
    return std::forward<Container>(c)[i];
}

// =============================================================================
// TODO 2 — report the value category that was deduced
// =============================================================================
// TODO(2): category(arg) must return the string "lvalue" when called with an
//   lvalue and "rvalue" when called with an rvalue.
//   HINT: the parameter is already a forwarding reference, so T is deduced as
//   an lvalue reference for an lvalue argument and as a plain type otherwise.
//   Test that with std::is_lvalue_reference_v<T> and return the matching
//   string literal. The stub always claims "rvalue".
// =============================================================================
template <class T>
const char* category(T&&) {
    // TODO(2)
    return "rvalue";
}

// =============================================================================
// TODO 3 — a deduction guide (STATIC: a wrong answer fails to compile)
// =============================================================================
// TODO(3): add a deduction guide so that Wrapper w{"hi"} deduces
//   Wrapper<std::string> rather than Wrapper<const char*>, while Wrapper{42}
//   continues to deduce Wrapper<int>.
//   HINT: two guides are needed, written just below the class:
//       template <class T> Wrapper(T) -> Wrapper<T>;
//       Wrapper(const char*)  -> Wrapper<std::string>;
//   Without the second line the string literal keeps its pointer type.
// =============================================================================
template <class T> struct Wrapper { T v; };
// TODO(3): write the two deduction guides here.

// =============================================================================
// TODO 4 — a stripping alias that leaves arrays alone (STATIC)
// =============================================================================
// TODO(4): plain_t<T> must remove references and cv-qualifiers while leaving
//   array types intact, so plain_t<const int&> is int and plain_t<int(&)[5]>
//   is int[5].
//   HINT: std::decay_t also converts arrays to pointers, which is not wanted
//   here; the trait that stops short of that is std::remove_cvref_t.
// =============================================================================
template <class T>
using plain_t = std::decay_t<T>;      // TODO(4): the wrong trait is used here

// =============================================================================
// TODO 5 — a function returning a reference to a global
// =============================================================================
// TODO(5): global_ref() must return int&, so that global_ref() = 8 assigns to
//   gi and the assertion on gi afterwards holds.
//   HINT: the return type is already decltype(auto); what decides the answer is
//   the return statement. Returning the bare name gi applies the entity rule
//   and yields int, whereas returning (gi) applies the value-category rule and
//   yields int&, because a parenthesised name is an lvalue expression.
// =============================================================================
inline int gi = 7;
decltype(auto) global_ref() {
    // TODO(5)
    return gi;
}

// =============================================================================
// TODO 6 — a compile-time question about a callable's result (STATIC)
// =============================================================================
// TODO(6): returns_bool<F, Args...>() must report whether F can be called with
//   Args... AND whether the result is usable as a bool.
//   HINT: std::is_invocable_v answers only the first half of that question, so
//   a lambda returning std::string would pass. The trait that answers both is
//   std::is_invocable_r_v<bool, F, Args...>.
// =============================================================================
template <class F, class... Args>
constexpr bool returns_bool() {
    // TODO(6)
    return std::is_invocable_v<F, Args...>;
}

// ###########################################################################
// SECTION B — FIXME
// ###########################################################################

// =============================================================================
// FIXME 1 — the caller's write is lost
// =============================================================================
// first_of(v) is meant to hand back a reference to the first element, so that
// first_of(f) = 99 modifies f. As shipped it hands back a copy and the write
// goes nowhere.
//   HINT: the return type is the whole problem. Compare it with TODO 1.
// =============================================================================
auto first_of(std::vector<int>& v) {
    // FIXME(1)
    return v[0];
}

// =============================================================================
// FIXME 2 — the snapshot changes after it was taken
// =============================================================================
// flag_snapshot(vb) is meant to capture the current value of vb[0] so that a
// later write to the vector does not affect what was captured. As shipped the
// captured object tracks the vector instead.
//   HINT: std::vector<bool> returns a proxy object rather than a bool, and auto
//   deduces that proxy faithfully. Force the intended type, either by declaring
//   the variable bool or by writing bool(vb[0]).
// =============================================================================
bool flag_snapshot(std::vector<bool>& vb) {
    auto captured = vb[0];      // FIXME(2)
    return captured;
}

// =============================================================================
// FIXME 3 — the loop modifies nothing
// =============================================================================
// bump_all(m) is meant to add one to every value in the map. As shipped the
// map is unchanged after the call.
//   HINT: a structured binding applies its auto to a hidden object, and plain
//   auto copies that object, so the names bind to pieces of a copy. Bind to the
//   element itself instead.
// =============================================================================
void bump_all(std::map<std::string, int>& m) {
    for (auto [key, value] : m) {   // FIXME(3)
        (void)key;
        value += 1;
    }
}

// =============================================================================
// FIXME 4 — the wrong constructor is selected
// =============================================================================
// three_halves() is meant to return a vector holding three copies of 0.5. As
// shipped it returns a vector of a different size entirely.
//   HINT: braces select the initializer-list constructor and parentheses select
//   the count-and-value constructor, so the braced form below builds a list of
//   the two numbers written rather than three copies of one of them.
// =============================================================================
std::vector<double> three_halves() {
    // FIXME(4)
    return std::vector<double>{3, 0.5};
}

// =============================================================================
// FIXME 5 — the deduced return type is a reference (STATIC)
// =============================================================================
// describe_size(v) is meant to return a plain std::size_t by value. As shipped
// the return type is a reference, which the static assertion in the test
// rejects, and which would dangle if the expression named a local.
//   HINT: the parentheses around the returned expression switch decltype(auto)
//   from the entity rule to the value-category rule. Remove them, or declare
//   the return type as auto when a value is what is wanted.
// =============================================================================
inline std::size_t cached_size = 0;
decltype(auto) describe_size(const std::vector<int>& v) {
    cached_size = v.size();
    // FIXME(5)
    return (cached_size);
}

// ===========================================================================
// Tests — do not modify.
// ===========================================================================
int main() {
    std::vector<int> v{10, 20, 30};
    std::map<std::string, int> m{{"a", 1}, {"b", 2}};

#if TODO1_READY
    {
        element_at(v, 1) = 77;
        assert(v[1] == 77);
        static_assert(std::is_same_v<decltype(element_at(v, 1)), int&>);
        std::cout << "todo 1  reference-preserving accessor ... ok\n";
    }
#else
    std::cout << "todo 1  reference-preserving accessor ... TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        int x = 1;
        assert(std::string(category(x)) == "lvalue");
        assert(std::string(category(5)) == "rvalue");
        std::cout << "todo 2  value category reported ......... ok\n";
    }
#else
    std::cout << "todo 2  value category reported ......... TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY
    {
        Wrapper wi{42};
        Wrapper ws{"hi"};
        static_assert(std::is_same_v<decltype(wi), Wrapper<int>>);
        static_assert(std::is_same_v<decltype(ws), Wrapper<std::string>>);
        std::cout << "todo 3  deduction guide ................. ok\n";
    }
#else
    std::cout << "todo 3  deduction guide ................. TODO (flip TODO3_READY)\n";
#endif

#if TODO4_READY
    {
        static_assert(std::is_same_v<plain_t<const int&>, int>);
        static_assert(std::is_same_v<plain_t<int(&)[5]>, int[5]>);
        std::cout << "todo 4  stripping alias ................. ok\n";
    }
#else
    std::cout << "todo 4  stripping alias ................. TODO (flip TODO4_READY)\n";
#endif

#if TODO5_READY
    {
        static_assert(std::is_same_v<decltype(global_ref()), int&>);
        global_ref() = 8;
        assert(gi == 8);
        std::cout << "todo 5  reference return type ........... ok\n";
    }
#else
    std::cout << "todo 5  reference return type ........... TODO (flip TODO5_READY)\n";
#endif

#if TODO6_READY
    {
        static_assert(returns_bool<decltype([](int x){ return x > 0; }), int>());
        static_assert(!returns_bool<decltype([](int){ return std::string("x"); }), int>());
        std::cout << "todo 6  result-type question ............ ok\n";
    }
#else
    std::cout << "todo 6  result-type question ............ TODO (flip TODO6_READY)\n";
#endif

#if FIX1_READY
    {
        std::vector<int> f{1, 2, 3};
        first_of(f) = 99;
        assert(f[0] == 99);
        static_assert(std::is_same_v<decltype(first_of(f)), int&>);
        std::cout << "fix  1  write reaches the container ..... ok\n";
    }
#else
    std::cout << "fix  1  write reaches the container ..... TODO (flip FIX1_READY)\n";
#endif

#if FIX2_READY
    {
        std::vector<bool> vb{true, false};
        bool snap = flag_snapshot(vb);
        vb[0] = false;
        assert(snap == true);
        std::cout << "fix  2  snapshot is independent ......... ok\n";
    }
#else
    std::cout << "fix  2  snapshot is independent ......... TODO (flip FIX2_READY)\n";
#endif

#if FIX3_READY
    {
        bump_all(m);
        assert(m["a"] == 2);
        assert(m["b"] == 3);
        std::cout << "fix  3  loop modifies the map ........... ok\n";
    }
#else
    std::cout << "fix  3  loop modifies the map ........... TODO (flip FIX3_READY)\n";
#endif

#if FIX4_READY
    {
        assert(three_halves().size() == 3);
        assert(three_halves()[0] == 0.5);
        std::cout << "fix  4  correct constructor selected .... ok\n";
    }
#else
    std::cout << "fix  4  correct constructor selected .... TODO (flip FIX4_READY)\n";
#endif

#if FIX5_READY
    {
        static_assert(std::is_same_v<decltype(describe_size(v)), std::size_t>);
        assert(describe_size(v) == 3);
        std::cout << "fix  5  return type is a value .......... ok\n";
    }
#else
    std::cout << "fix  5  return type is a value .......... TODO (flip FIX5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
