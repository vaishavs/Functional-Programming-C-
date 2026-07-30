// ===========================================================================
// Partial application & reference wrappers — DEBUG (FIXME) exercise
//
// Five standard-library tools for adapting a callable, each shipped with a bug
// the tests convict:
//   std::bind        — general partial application with placeholders (_1, _2, …)
//   std::bind_front  — bind LEADING arguments                         (C++20)
//   std::bind_back   — bind TRAILING arguments                        (C++23)
//   std::ref         — make a copying interface hold a REFERENCE
//   std::cref        — the same, for a CONST reference
//
// Markers
//   FIXME(n)  — shipped code is wrong on purpose; repair it so the test passes.
//   PREDICT   — commit to the BUGGY result before compiling, then confirm.
//   QUESTION  — answer in a comment next to the marker.
//
// Flip a step's STEPn_READY to 1 to activate its test. With a step off the test
// is skipped; the file ships compiling and running with every step off. Each
// test asserts the CORRECT value, so it FAILS until the FIXME is repaired.
//
//   Build : g++ -std=c++23 -Wall -Wextra -o binddbg bind_adaptors_debug.cpp
//   Run   : ./binddbg
//   (bind_front needs C++20; bind_back needs C++23. On a compiler that lacks
//    std::bind_back — e.g. g++ 13 — the GIVEN stand-in below is used instead.)
//
// Reference point used throughout
//   sub(a, b) == a - b   — subtraction is not commutative, so argument ORDER
//   is visible in every result.
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
// minimal equivalent is defined so this file builds on older compilers. It has
// the same contract: bind_back(f, back...)(front...) calls f(front..., back...).
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
// STEP 1 — std::bind: placeholders name the CALL's arguments
// ===========================================================================
//
// std::bind(f, a1, a2, ...) builds a callable. A placeholder _N in a position
// means "forward the caller's N-th argument here"; a non-placeholder value is
// BOUND (stored) in that position. Placeholders can also REORDER arguments.
//
// FIXME(1): subtract3(x) is meant to compute x - 3, but the bound value and the
//   placeholder sit in the wrong positions, so it computes 3 - x instead.
//   PREDICT subtract3(10) as shipped, then repair so it returns 7.
//
// QUESTION(1): in std::bind(sub, _1, 3), which of sub's parameters does _1
//   stand for, and what does the literal 3 do?
// ---------------------------------------------------------------------------
int subtract3(int x) {
    // FIXME(1): arguments swapped -> computes sub(3, x) == 3 - x
    return std::bind(sub, 3, _1)(x);
}

// ===========================================================================
// STEP 2 — std::bind_front: bind the LEADING argument(s)   (C++20)
// ===========================================================================
//
// std::bind_front(f, a)(rest...) calls f(a, rest...). No placeholders, no
// reordering — a is nailed to the FRONT of the argument list.
//
// FIXME(2): take10(x) is meant to compute x - 10 (subtract 10 FROM x). But
//   bind_front nails 10 to the FRONT, so it computes sub(10, x) == 10 - x.
//   PREDICT take10(3) as shipped, then repair so it returns -7.
//   (Hint: another tool in this file binds the OTHER end.)
//
// QUESTION(2): which parameter of sub does bind_front(sub, 10) fix?
// ---------------------------------------------------------------------------
int take10(int x) {
    // FIXME(2): binds the wrong end -> computes sub(10, x) == 10 - x
    return std::bind_front(sub, 10)(x);
}

// ===========================================================================
// STEP 3 — std::bind_back: bind the TRAILING argument(s)   (C++23)
// ===========================================================================
//
// std::bind_back(f, a)(rest...) calls f(rest..., a). a is nailed to the BACK.
// It is the mirror image of bind_front.
//
// FIXME(3): from10(x) is meant to compute 10 - x (subtract x FROM 10). But
//   bind_back nails 10 to the BACK, so it computes sub(x, 10) == x - 10.
//   PREDICT from10(3) as shipped, then repair so it returns 7.
//
// QUESTION(3): complete the rule —
//   bind_front(f, a)(b) == f(__, __);   bind_back(f, a)(b) == f(__, __).
// ---------------------------------------------------------------------------
int from10(int x) {
    // FIXME(3): binds the wrong end -> computes sub(x, 10) == x - 10
    return bind_back(sub, 10)(x);
}

// ===========================================================================
// STEP 4 — std::ref: make a copying interface hold a REFERENCE
// ===========================================================================
//
// std::bind (like std::thread and std::function) COPIES its arguments by
// default. Wrapping an argument in std::ref(x) stores a reference_wrapper, so
// the call reaches the ORIGINAL x and shares its state.
//
// FIXME(4): advance_counter() calls a bound Counter three times, expecting the
//   local counter to finish at 3. But std::bind COPIED the counter, so the
//   original never advances.
//   PREDICT advance_counter() as shipped, then repair with std::ref so it
//   returns 3.
//
// QUESTION(4): why does std::bind copy by default, and what would storing a raw
//   reference (rather than a reference_wrapper) risk?
// ---------------------------------------------------------------------------
int advance_counter() {
    Counter c;
    // FIXME(4): binds a COPY of c; the original c never advances
    auto tick = std::bind(c);
    tick();
    tick();
    tick();
    return c.n;
}

// ===========================================================================
// STEP 5 — std::cref: a CONST reference wrapper (and a lifetime trap)
// ===========================================================================
//
// std::cref(x) is std::ref for a const reference — a callable can read x
// without copying it. Like std::ref, std::cref does NOT extend x's lifetime.
//
// FIXME(5): make_reader() captures a local `config` by std::cref and returns
//   the bound callable. `config` dies when make_reader returns, so the returned
//   reader reads freed stack on every call.
//   PREDICT reader() as shipped — it will NOT reliably be 42 (AddressSanitizer
//   reports stack-use-after-return). Repair by binding `config` BY VALUE.
//
// QUESTION(5): state the one-line rule for std::ref / std::cref and lifetimes,
//   and explain why binding by value fixes this case.
// ---------------------------------------------------------------------------
std::function<int()> make_reader() {
    int config = 42;
    // FIXME(5): cref stores a reference to a local that dies at return
    return std::bind(read_config, std::cref(config));
}

// ===========================================================================
// Tests — do not modify. Each assert names the mistake it catches.
// ===========================================================================
int main() {
    std::cout << std::boolalpha;

    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        assert(subtract3(10) == 7);              // FIXME(1): x - 3, not 3 - x
        std::cout << "step 1  std::bind placeholders ...... ok\n";
    }
#else
    std::cout << "step 1  std::bind placeholders ...... TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        assert(take10(3) == -7);                 // FIXME(2): x - 10, not 10 - x
        std::cout << "step 2  std::bind_front (leading) ... ok\n";
    }
#else
    std::cout << "step 2  std::bind_front (leading) ... TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        assert(from10(3) == 7);                  // FIXME(3): 10 - x, not x - 10
        std::cout << "step 3  std::bind_back (trailing) ... ok\n";
    }
#else
    std::cout << "step 3  std::bind_back (trailing) ... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        assert(advance_counter() == 3);          // FIXME(4): std::ref to share state
        std::cout << "step 4  std::ref (shared state) ..... ok\n";
    }
#else
    std::cout << "step 4  std::ref (shared state) ..... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        // FIXME(5): the reader must survive make_reader's return. A stack
        // clobber is interposed so a dangling cref reliably reads garbage and
        // fails; binding by value returns 42.
        auto reader = make_reader();
        volatile int stack_noise[32];
        for (int i = 0; i < 32; ++i) stack_noise[i] = i;
        (void)stack_noise;
        assert(reader() == 42);
        std::cout << "step 5  std::cref (lifetime) ........ ok\n";
    }
#else
    std::cout << "step 5  std::cref (lifetime) ........ TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
