// ===========================================================================
// std::invoke and its family — TODO / DEBUG exercise
// ===========================================================================
// SECTION A — DEBUG : five functions ship with a bug that the tests convict.
// SECTION B — TODO  : five functions to implement from scratch.
//
// Markers
//   FIXME(n)  — shipped code is wrong on purpose; repair it.
//   TODO(n)   — implement from scratch.
//   HINT      — a concrete nudge toward the fix or the implementation.
//
// Flip a step's READY macro to 1 to activate its test. The file ships compiling
// and running with every step off, and every test fails until the work is done.
//
//   Build : g++ -std=c++23 -Wall -Wextra -pthread -o siex try_09.cpp
//   Run   : ./siex
// ===========================================================================

#include <cassert>
#include <concepts>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>

#define DBG1_READY  0
#define DBG2_READY  0
#define DBG3_READY  0
#define DBG4_READY  0
#define DBG5_READY  0
#define TODO1_READY 0
#define TODO2_READY 0
#define TODO3_READY 0
#define TODO4_READY 0
#define TODO5_READY 0

// GIVEN — the subjects used by both sections.
struct Account {
    int balance = 100;
    int deposit(int amount) { balance += amount; return balance; }
    int scaled(int k) const { return balance * k; }
};
struct Counter {
    int n = 0;
    int operator()() { return ++n; }        // the call operator is deliberately not const
};
inline int add(int a, int b) { return a + b; }
inline double halve(int x) { return x / 2.0; }

// ###########################################################################
// SECTION A — DEBUG
// ###########################################################################

// ---------------------------------------------------------------------------
// FIXME(1): apply_deposit should deposit into the caller's account, but the
//   caller's balance never changes. The test asserts that it does.
//   HINT: look at how the object reaches std::invoke, not at std::invoke itself.
// ---------------------------------------------------------------------------
inline int apply_deposit(Account acc, int amount) {          // FIXME(1)
    return std::invoke(&Account::deposit, acc, amount);
}

// ---------------------------------------------------------------------------
// FIXME(2): set_balance should write the new balance into the caller's account
//   through a pointer to the data member, but the caller sees no change.
//   The test asserts that the write lands in the caller's object.
//   HINT: the data-member case yields an lvalue, so the assignment is fine; the
//   problem is which object that lvalue belongs to.
// ---------------------------------------------------------------------------
inline void set_balance(Account acc, int value) {            // FIXME(2)
    std::invoke(&Account::balance, acc) = value;
}

// ---------------------------------------------------------------------------
// FIXME(3): advance_thrice should call the counter three times and report the
//   caller's final count, but the caller's counter stays at zero.
//   The test asserts that the caller's counter reached three.
//   HINT: std::bind copies what it is given unless told otherwise.
// ---------------------------------------------------------------------------
inline int advance_thrice(Counter& c) {
    auto tick = std::bind(c);                                // FIXME(3)
    tick(); tick(); tick();
    return c.n;
}

// ---------------------------------------------------------------------------
// FIXME(4): half_of should hand back the exact half of its argument, but the
//   fractional part disappears. The test asserts that half of 5 is 2.5.
//   HINT: the result is converted to the type named in the call.
// ---------------------------------------------------------------------------
inline double half_of(int x) {
    return std::invoke_r<int>(halve, x);                     // FIXME(4)
}

// ---------------------------------------------------------------------------
// FIXME(5): callable_as_const should report whether a Counter can be invoked
//   through a const reference, which it cannot. The test asserts false.
//   HINT: the trait asks about exactly the type written between the angle
//   brackets, so check which type that is.
// ---------------------------------------------------------------------------
inline constexpr bool callable_as_const() {
    return std::is_invocable_v<Counter&>;                    // FIXME(5)
}

// ###########################################################################
// SECTION B — TODO
// ###########################################################################

// ---------------------------------------------------------------------------
// TODO(1): scale_balance(acc, k) must return acc.scaled(k), reached through a
//   pointer to the member function rather than by calling it directly.
//   HINT: form the pointer with &Account::scaled, then pass the object as the
//   first argument to std::invoke, followed by k.
// ---------------------------------------------------------------------------
inline int scale_balance(const Account& /*acc*/, int /*k*/) {
    // TODO(1)
    return 0;
}

// ---------------------------------------------------------------------------
// TODO(2): read_balance(acc) must return acc.balance, reached through a pointer
//   to the data member rather than by naming the field directly.
//   HINT: this case takes the object and nothing else after it.
// ---------------------------------------------------------------------------
inline int read_balance(const Account& /*acc*/) {
    // TODO(2)
    return 0;
}

// ---------------------------------------------------------------------------
// TODO(3): call_twice(f, x) must return f(f(x)), must accept any callable that
//   takes one int (a lambda, a plain function, a functor), and must be
//   constrained so that an unsuitable callable is rejected at the call site.
//   HINT: constrain F with std::invocable<int>, and perform the calls with
//   std::invoke rather than with plain call syntax.
// ---------------------------------------------------------------------------
template <class F>
inline int call_twice(F&& /*f*/, int /*x*/) {
    // TODO(3)
    return 0;
}

// ---------------------------------------------------------------------------
// TODO(4): sum_tuple(t) must call add with the two elements of the tuple.
//   HINT: one standard facility unpacks a tuple straight into a call.
// ---------------------------------------------------------------------------
inline int sum_tuple(const std::tuple<int, int>& /*t*/) {
    // TODO(4)
    return 0;
}

// ---------------------------------------------------------------------------
// TODO(5): run_and_discard(f, x) must call f with x, discard whatever it
//   returns, and itself return void, without triggering a nodiscard warning or
//   naming the result type anywhere.
//   HINT: there is a form of invoke that takes the desired result type, and
//   void is a permitted choice for it.
// ---------------------------------------------------------------------------
template <class F>
inline void run_and_discard(F&& /*f*/, int /*x*/) {
    // TODO(5)
}

// GIVEN — records that run_and_discard actually performed the call.
inline int g_side_effect = 0;
inline int record(int x) { g_side_effect += x; return x; }

// ===========================================================================
// Tests — do not modify. Each assertion names the task it checks.
// ===========================================================================
int main() {
    std::cout << std::boolalpha;

#if DBG1_READY
    {
        Account a;
        int reported = apply_deposit(a, 50);
        assert(reported == 150);
        assert(a.balance == 150);      // FIXME(1): the caller must see the deposit
        std::cout << "debug 1  depositing into the caller ... ok\n";
    }
#else
    std::cout << "debug 1  depositing into the caller ... TODO (flip DBG1_READY)\n";
#endif

#if DBG2_READY
    {
        Account a;
        set_balance(a, 77);
        assert(a.balance == 77);       // FIXME(2): the write must land in the caller
        std::cout << "debug 2  writing through a data member  ok\n";
    }
#else
    std::cout << "debug 2  writing through a data member  TODO (flip DBG2_READY)\n";
#endif

#if DBG3_READY
    {
        Counter c;
        int reported = advance_thrice(c);
        assert(reported == 3);
        assert(c.n == 3);              // FIXME(3): the counter must be shared
        std::cout << "debug 3  sharing a stateful callable .. ok\n";
    }
#else
    std::cout << "debug 3  sharing a stateful callable .. TODO (flip DBG3_READY)\n";
#endif

#if DBG4_READY
    {
        assert(half_of(5) == 2.5);     // FIXME(4): the fraction must survive
        std::cout << "debug 4  choosing the result type ..... ok\n";
    }
#else
    std::cout << "debug 4  choosing the result type ..... TODO (flip DBG4_READY)\n";
#endif

#if DBG5_READY
    {
        static_assert(callable_as_const() == false);   // FIXME(5): ask about the const type
        std::cout << "debug 5  asking about the right type .. ok\n";
    }
#else
    std::cout << "debug 5  asking about the right type .. TODO (flip DBG5_READY)\n";
#endif

#if TODO1_READY
    {
        Account a;
        assert(scale_balance(a, 3) == 300);
        std::cout << "todo  1  calling a member function .... ok\n";
    }
#else
    std::cout << "todo  1  calling a member function .... TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        Account a;
        assert(read_balance(a) == 100);
        std::cout << "todo  2  reading a data member ........ ok\n";
    }
#else
    std::cout << "todo  2  reading a data member ........ TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY
    {
        struct Doubler { int operator()(int x) const { return x * 2; } };
        assert(call_twice([](int x){ return x + 1; }, 40) == 42);
        assert(call_twice(Doubler{}, 3) == 12);
        std::cout << "todo  3  a constrained generic caller . ok\n";
    }
#else
    std::cout << "todo  3  a constrained generic caller . TODO (flip TODO3_READY)\n";
#endif

#if TODO4_READY
    {
        assert(sum_tuple(std::make_tuple(6, 7)) == 13);
        std::cout << "todo  4  calling with a tuple ......... ok\n";
    }
#else
    std::cout << "todo  4  calling with a tuple ......... TODO (flip TODO4_READY)\n";
#endif

#if TODO5_READY
    {
        g_side_effect = 0;
        run_and_discard(record, 5);
        assert(g_side_effect == 5);    // the call must really have happened
        static_assert(std::is_void_v<decltype(run_and_discard(record, 5))>);
        std::cout << "todo  5  calling and discarding ....... ok\n";
    }
#else
    std::cout << "todo  5  calling and discarding ....... TODO (flip TODO5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
