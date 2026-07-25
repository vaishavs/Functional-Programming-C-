// ===========================================================================
// Currying & Partial Application in C++ — TODO exercise (detailed hints)
//
// Complete the five TODOs sequentially. Turn on STEPn_READY to 1 to enable tests.
// Compiles and runs out of the box (all steps report TODO).
//
//   Build : g++ -std=c++20 -Wall -Wextra -o curry currying_partial_todo.cpp
//   Run   : ./curry
//
// Ground rules:
//   * Standard library only (no std::bind, no std::bind_front).
//   * Value semantics: intermediate stages are independent, reusable values (steps 1–4).
//   * Must compile cleanly under -Wall -Wextra.
//
// Hint structure (read in order):
//   SPEC   — The contract (tested behaviors).
//   HINTS  — The underlying C++ mechanics.
//   SHAPE  — The code skeleton (load-bearing parts elided).
// ===========================================================================

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0  // bonus

// ---------------------------------------------------------------------------
// STEP 1 — curry2: two-argument warm-up
//
// SPEC
//   * curry2(f)(a)(b) == f(a, b)
//   * Nested generic lambdas capturing by value.
//   * Intermediate stages are independent, reusable values (no dangling refs).
//
// HINTS
//   * Return a unary generic lambda; its body returns another; innermost applies f.
//   * Move the parameter f into the outer closure: f = std::move(f).
//   * The inner closure must COPY both f and a to prevent dangling references.
//   * A non-mutable lambda's operator() is const, making by-value captures const.
//     Thus, the callable must be const-invocable.
//
// SHAPE
//     return [ <...> ](auto a) {
//         return [ <...> ](auto b) { return <...>; };
//     };
// ---------------------------------------------------------------------------
template <class F>
auto curry2(F f) {
    // TODO(1)
}

// ---------------------------------------------------------------------------
// STEP 2 — partial: fix leading arguments
//
// SPEC
//   * partial(f, bound...)(rest...) == std::invoke(f, bound..., rest...)
//   * bound... are copied/moved at bind time (mutating originals won't affect it).
//   * rest... are perfectly forwarded at call time.
//   * Preserves reference return types. Supports member function pointers.
//
// HINTS
//   * Bind time: Use C++20 pack expansion in init-capture: ...bound = std::forward<Bound>(bound)
//   * Capture f via forwarding: f = std::forward<F>(f)
//   * Call time: Take auto&&... rest and std::forward<decltype(rest)>(rest)...
//   * Return type: Use -> decltype(auto) to avoid decaying references.
//   * Use std::invoke to properly support member function pointers.
//   * Const mechanics apply: f and bound... are const inside the closure.
//
// SHAPE
//     return [ <capture f>, <pack init-capture bound...> ]
//            (auto&&... rest) -> <...> {
//         return std::invoke( <...> );
//     };
// ---------------------------------------------------------------------------
template <class F, class... Bound>
auto partial(F&& f, Bound&&... bound) {
    // TODO(2)
}

// ---------------------------------------------------------------------------
// STEP 3 — curry: arbitrary fixed arity
//
// SPEC
//   * curry(f)(a1)...(an) == f(a1, ..., an)
//   * EAGER: Invokes immediately once f is fully invocable.
//   * No tuples. Uses recursion on a wrapped callable. Reusable stages.
//
// HINTS
//   1. Returns a unary lambda taking auto a.
//   2. Use if constexpr (std::is_invocable_v<F const&, decltype(a)&>) to branch.
//   3. Base case: return f(a).
//   4. Recursive case: Wrap in a new lambda w(rest...) == f(a, rest...) and return curry(w).
//   5. CRITICAL SFINAE: The wrapper MUST have an explicit trailing return type:
//      -> decltype(f(a, std::forward<decltype(rest)>(rest)...))
//      This ensures is_invocable correctly reports false for incomplete arguments
//      instead of triggering a hard compile error.
//
// SHAPE
//     return [f = std::move(f)](auto a) {
//         if constexpr ( <invocable with just a?> ) {
//             return <...>;
//         } else {
//             return curry([f, a](auto&&... rest) -> <trailing type> {
//                 return <...>;
//             });
//         }
//     };
// ---------------------------------------------------------------------------
template <class F>
auto curry(F f) {
    // TODO(3)
}

// ---------------------------------------------------------------------------
// STEP 4 — Curried<F, Bound...>: apply argument GROUPS
//
// SPEC
//   * Apply multiple args at once: cv(2)(3, 4) == cv(2, 3, 4) == 24
//   * If f is invocable, invoke it. Else, append args to the bound tuple.
//   * const-callable and reusable.
//
// HINTS
//   1. Trait: std::is_invocable_v<F const&, Bound const&..., Args...>
//   2. Invoke branch: std::apply a lambda over bound_ receiving auto const&... b,
//      then std::invoke(f_, b..., std::forward<Args>(args)...). Avoids tuple copies.
//   3. Bind branch: Return a new Curried object merging tuples:
//      std::tuple_cat(bound_, std::make_tuple(std::forward<Args>(args)...))
//   4. Type naming: Rely on CTAD (e.g., Curried{f_, <tuple>}).
//   5. Qualify operator() with const& to preserve reusability.
//   6. static_assert(sizeof...(Args) > 0) prevents useless zero-argument calls.
//
// SHAPE (const& overload)
//     if constexpr ( <trait> ) {
//         return std::apply(
//             [ <...> ](auto const&... b) { return std::invoke( <...> ); },
//             bound_);
//     } else {
//         return Curried{ <...>, std::tuple_cat( <...>, std::make_tuple( <...> )) };
//     }
// ---------------------------------------------------------------------------
// STEP 5 (bonus) — move-aware Curried
//
// SPEC
//   * Implement an &&-qualified overload for one-shot, move-only pipelines.
//
// HINTS
//   1. Rvalues (temporaries or std::move'd lvalues) will prefer the && overload.
//   2. Inside &&, cannibalize *this using std::move(f_) and std::move(bound_).
//   3. Trait: std::is_invocable_v<F, Bound..., Args...>. Non-refs are treated as rvalues.
//   4. Invoke branch: std::apply over std::move(bound_). Accept elements as auto&&... b.
//   5. Bind branch: Move both tuples:
//      std::tuple_cat(std::move(bound_), std::make_tuple(std::forward<Args>(args)...))
//   6. Safety check: In the const& bind branch, add:
//      static_assert((std::is_copy_constructible_v<Bound> && ...),
//                    "bound state is move-only: invoke this Curried as an rvalue (std::move it)");
//
// SHAPE (&& overload)
//   Mirror the const& shape, with moves in place of const accesses.
// ---------------------------------------------------------------------------
template <class F, class... Bound>
class Curried {
    F f_;
    std::tuple<Bound...> bound_;

public:
    Curried(F f, std::tuple<Bound...> bound)
        : f_(std::move(f)), bound_(std::move(bound)) {}

    template <class... Args>
    auto operator()(Args&&... args) const& {
        static_assert(sizeof...(Args) > 0, "apply at least one argument");
        // TODO(4)
    }

#if STEP5_READY
    template <class... Args>
    auto operator()(Args&&... args) && {
        static_assert(sizeof...(Args) > 0, "apply at least one argument");
        // TODO(5): the move-aware twin of the overload above.
    }
#endif
};

template <class F>
auto curried(F f) {
    return Curried<F>{std::move(f), {}};
}

// ===========================================================================
// Tests — do not modify below this line. Each assert is annotated with the
// property it pins down (and the wrong implementation it catches).
// ===========================================================================
int main() {
    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        auto add = [](int a, int b) { return a + b; };
        assert(curry2(add)(2)(3) == 5);           // basic contract

        auto inc = curry2(add)(1);
        // inc outlives the temporary; catches dangling references or consumed state.
        assert(inc(41) == 42);
        assert(inc(1) == 2);

        auto repeat = [](std::string s, int n) {
            std::string out;
            while (n-- > 0) out += s;
            return out;
        };
        // heterogeneous arguments decay to string at final call
        assert(curry2(repeat)("ab")(3) == "ababab");
        std::cout << "step 1  curry2 ................. ok\n";
    }
#else
    std::cout << "step 1  curry2 ................. TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        auto concat = [](std::string a, std::string b) { return a + b; };
        std::string greeting = "Hello, ";
        auto greet = partial(concat, greeting);
        greeting = "Bye, ";  // bound by value; mutation shouldn't affect partial application
        assert(greet("World") == "Hello, World");

        struct Account {
            int balance;
            int deposit(int amount) const { return balance + amount; }
        };
        // catches plain calls instead of std::invoke
        auto deposit100 = partial(&Account::deposit, Account{100});
        assert(deposit100(50) == 150);

        auto pick = [](bool first, int& a, int& b) -> int& { return first ? a : b; };
        int x = 1, y = 2;
        // catches a decayed return type (plain auto)
        partial(pick, true)(x, y) = 99;
        assert(x == 99 && y == 2);
        std::cout << "step 2  partial ................ ok\n";
    }
#else
    std::cout << "step 2  partial ................ TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        auto add3 = [](int a, int b, int c) { return a + b + c; };
        assert(curry(add3)(1)(2)(3) == 6);        // basic contract

        auto f1 = curry(add3)(1);
        auto f12 = f1(2);
        // catches shared/consumed state or dangling captures
        assert(f12(3) == 6);
        assert(f12(39) == 42);
        assert(f1(10)(20) == 31);

        auto neg = [](int v) { return -v; };
        assert(curry(neg)(5) == -5);              // eager: unary applies immediately

        auto join4 = [](std::string a, std::string b, std::string c, std::string d) {
            return a + b + c + d;
        };
        // tests SFINAE-friendliness across intermediate arities
        assert(curry(join4)("a")("b")("c")("d") == "abcd");
        std::cout << "step 3  curry .................. ok\n";
    }
#else
    std::cout << "step 3  curry .................. TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        auto volume = [](int l, int w, int h) { return l * w * h; };
        auto cv = curried(volume);
        // catches early/late is_invocable evaluation
        assert(cv(2)(3)(4) == 24);
        assert(cv(2, 3)(4) == 24);
        assert(cv(2)(3, 4) == 24);
        assert(cv(2, 3, 4) == 24);

        auto base = cv(2, 3);
        assert(base(4) == 24);
        assert(base(10) == 60);  // bound state must survive previous call
        std::cout << "step 4  Curried groups ......... ok\n";
    }
#else
    std::cout << "step 4  Curried groups ......... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 (bonus) --------------------------------------------------
#if STEP5_READY
    {
        auto consume = [](std::unique_ptr<int> p, int x) { return *p + x; };
        // chain rides the && overload, moving pointers forward
        assert(curried(consume)(std::make_unique<int>(40))(2) == 42);

        auto stage = curried(consume)(std::make_unique<int>(40));
        // lvalue with move-only state needs std::move (trips static_assert otherwise)
        assert(std::move(stage)(2) == 42);
        std::cout << "step 5  move-aware Curried ..... ok\n";
    }
#else
    std::cout << "step 5  move-aware Curried ..... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
