// ===========================================================================
// Partial application & reference wrappers — TODO exercise
//
// Five standard-library tools for adapting a callable, implemented from scratch:
//   std::bind        — general partial application with placeholders (_1, _2, …)
//   std::bind_front  — bind LEADING arguments                         (C++20)
//   std::bind_back   — bind TRAILING arguments                        (C++23)
//   std::ref         — make a copying interface hold a REFERENCE
//   std::cref        — the same, for a CONST reference
//
// Markers
//   TODO(n)   — implement from scratch.
//   QUESTION  — answer in a comment next to the marker.
//
// Flip a step's STEPn_READY to 1 when finished. Steps are independent; the file
// ships compiling and running with every step off.
//
//   Build : g++ -std=c++23 -Wall -Wextra -o bindtodo bind_adaptors_todo.cpp
//   Run   : ./bindtodo
//   (bind_front needs C++20; bind_back needs C++23. On a compiler that lacks
//    std::bind_back — e.g. g++ 13 — the GIVEN stand-in below is used instead.)
//
// Reference points used throughout
//   add(a, b) == a + b        sub(a, b) == a - b   (order is visible in results)
// ===========================================================================

#include <cassert>
#include <functional>
#include <iostream>
#include <tuple>
#include <utility>

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0

// ---------------------------------------------------------------------------
// GIVEN helpers and a stand-in for std::bind_back — do not modify.
// If the standard library provides std::bind_back (C++23), it is used; else a
// minimal equivalent is defined so this file builds on older compilers. Same
// contract: bind_back(f, back...)(front...) calls f(front..., back...).
// ---------------------------------------------------------------------------
inline int add(int a, int b) { return a + b; }
inline int sub(int a, int b) { return a - b; }
struct Counter { int n = 0; int operator()() { return ++n; } };
inline int read_config(const int& c) { return c; }

#if defined(__cpp_lib_bind_back)
using std::bind_back;
#else
template <class F, class... Back>
auto bind_back(F&& f, Back&&... back) {
    return [f = std::forward<F>(f), back = std::make_tuple(std::forward<Back>(back)...)]
           (auto&&... front) mutable -> decltype(auto) {
        return std::apply([&](auto&... b) -> decltype(auto) {
            return std::invoke(f, std::forward<decltype(front)>(front)..., b...);
        }, back);
    };
}
#endif

using std::placeholders::_1;
using std::placeholders::_2;

// ===========================================================================
// STEP 1 — std::bind: placeholders bind and reorder
// ===========================================================================
//
// std::bind(f, a1, a2, ...) builds a callable. A placeholder _N forwards the
// caller's N-th argument into that position; a plain value is BOUND there.
// Placeholders can also REORDER the call's arguments.
//
// TODO(1a)  bind_minus3(x) -> int
//   Use std::bind to fix sub's SECOND argument to 3, so bind_minus3(x) == x - 3.
//     auto f = std::bind(sub, _1, 3);   return f(x);
//
// TODO(1b)  bind_swap(a, b) -> int
//   Use std::bind with reordered placeholders so bind_swap(a, b) == b - a
//   (that is, it calls sub(b, a)).
//     auto f = std::bind(sub, _2, _1);  return f(a, b);
//
// QUESTION(1): std::bind stores its bound arguments by copy (after decay). Name
//   one consequence of that eager copy (revisited in step 4).
// ---------------------------------------------------------------------------
int bind_minus3(int /*x*/) {
    // TODO(1a)
    return 0;
}

int bind_swap(int /*a*/, int /*b*/) {
    // TODO(1b)
    return 0;
}

// ===========================================================================
// STEP 2 — std::bind_front: bind the LEADING argument(s)   (C++20)
// ===========================================================================
//
// std::bind_front(f, a)(rest...) calls f(a, rest...). No placeholders, no
// reordering — cleaner than std::bind for the common "fix the first argument".
//
// TODO(2a)  front_from10(x) -> int
//   std::bind_front(sub, 10)(x) computes sub(10, x) == 10 - x. Return it.
//
// TODO(2b)  front_add100(x) -> int
//   Use std::bind_front on `add` to fix the first argument to 100, so
//   front_add100(x) == 100 + x.
//
// QUESTION(2): std::bind_front(&Account::deposit, acc) is a common idiom for
//   binding a member function to its object. Which argument position does the
//   object occupy, and why might std::ref be needed around `acc`?
// ---------------------------------------------------------------------------
int front_from10(int /*x*/) {
    // TODO(2a)
    return 0;
}

int front_add100(int /*x*/) {
    // TODO(2b)
    return 0;
}

// ===========================================================================
// STEP 3 — std::bind_back: bind the TRAILING argument(s)   (C++23)
// ===========================================================================
//
// std::bind_back(f, a)(rest...) calls f(rest..., a) — the mirror of bind_front.
//
// TODO(3a)  back_minus10(x) -> int
//   bind_back(sub, 10)(x) computes sub(x, 10) == x - 10. Return it.
//
// TODO(3b)  back_half(x) -> int
//   Use bind_back on std::divides<int>{} to fix the divisor to 2, so
//   back_half(x) == x / 2  (that is, it calls divides(x, 2)).
//
// QUESTION(3): bind_front and bind_back together cover "fix the first args" and
//   "fix the last args" without placeholders. When is plain std::bind still
//   required?
// ---------------------------------------------------------------------------
int back_minus10(int /*x*/) {
    // TODO(3a)
    return 0;
}

int back_half(int /*x*/) {
    // TODO(3b)
    return 0;
}

// ===========================================================================
// STEP 4 — std::ref: share state through a copying interface
// ===========================================================================
//
// std::bind copies its arguments, so a bound stateful object is a COPY whose
// changes the caller never sees. std::ref(x) stores a reference_wrapper so the
// call reaches the ORIGINAL. A reference_wrapper is itself callable when it
// wraps a callable.
//
// TODO(4a)  share_counter(c) -> int
//   Bind c through std::ref so three calls advance the SAME counter, then
//   return c.n (which must be 3).
//     auto tick = std::bind(std::ref(c));  tick(); tick(); tick();  return c.n;
//
// TODO(4b)  ref_direct(c) -> int
//   A reference_wrapper is callable directly (no std::bind). Wrap c with
//   std::ref, call it twice, and return c.n (which must be 2).
//     auto rc = std::ref(c);  rc(); rc();  return c.n;
//
// QUESTION(4): contrast this with FIXME(B)-style bugs where a std::function
//   COPY reset the state. What does std::ref change about ownership?
// ---------------------------------------------------------------------------
int share_counter(Counter& /*c*/) {
    // TODO(4a)
    return 0;
}

int ref_direct(Counter& /*c*/) {
    // TODO(4b)
    return 0;
}

// ===========================================================================
// STEP 5 — std::cref: pass a CONST reference
// ===========================================================================
//
// std::cref(x) is std::ref for a const reference: a callable can read x without
// copying it. (Like std::ref, it does NOT extend x's lifetime — so it is safe
// here only because the bound callable is invoked immediately.)
//
// TODO(5a)  cref_read(config) -> int
//   Bind read_config (which takes a const int&) to config via std::cref, then
//   call the result and return it. cref_read(42) must return 42.
//     return std::bind(read_config, std::cref(config))();
//
// QUESTION(5): std::ref yields reference_wrapper<T>; std::cref yields
//   reference_wrapper<const T>. Which one permits the callable to MODIFY the
//   referent, and which forbids it?
// ---------------------------------------------------------------------------
int cref_read(int /*config*/) {
    // TODO(5a)
    return 0;
}

// ===========================================================================
// Tests — do not modify. Each assert names what the task must produce.
// ===========================================================================
int main() {
    std::cout << std::boolalpha;

    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        assert(bind_minus3(10) == 7);            // x - 3
        assert(bind_swap(3, 10) == 7);           // sub(10, 3) == 7
        std::cout << "step 1  std::bind placeholders ...... ok\n";
    }
#else
    std::cout << "step 1  std::bind placeholders ...... TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        assert(front_from10(3) == 7);            // sub(10, 3) == 7
        assert(front_add100(5) == 105);          // add(100, 5) == 105
        std::cout << "step 2  std::bind_front (leading) ... ok\n";
    }
#else
    std::cout << "step 2  std::bind_front (leading) ... TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        assert(back_minus10(3) == -7);           // sub(3, 10) == -7
        assert(back_half(10) == 5);              // divides(10, 2) == 5
        std::cout << "step 3  std::bind_back (trailing) ... ok\n";
    }
#else
    std::cout << "step 3  std::bind_back (trailing) ... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        Counter c1;
        assert(share_counter(c1) == 3);          // bound via std::ref -> shared
        assert(c1.n == 3);
        Counter c2;
        assert(ref_direct(c2) == 2);             // reference_wrapper is callable
        std::cout << "step 4  std::ref (shared state) ..... ok\n";
    }
#else
    std::cout << "step 4  std::ref (shared state) ..... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        assert(cref_read(42) == 42);             // read through a const reference
        std::cout << "step 5  std::cref (const ref) ....... ok\n";
    }
#else
    std::cout << "step 5  std::cref (const ref) ....... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
