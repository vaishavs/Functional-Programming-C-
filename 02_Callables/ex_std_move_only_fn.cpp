// ===========================================================================
// std::move_only_function<Signature>   — C++23, header <functional>
//
//   Build : g++ -std=c++23 -Wall -Wextra -o mof std_move_only_function.cpp
//
// A type-erased wrapper (like std::function) that holds ANY callable matching
// Signature behind one fixed type — but, unlike std::function, it is MOVE-ONLY.
// That single relaxation lets it store callables std::function cannot: lambdas
// that captured a std::unique_ptr (or any move-only resource) by move. Its
// Signature may also carry const / & / && / noexcept qualifiers, which the type
// enforces at the call site — a precision std::function lacks.
//
// Properties at a glance
//   * owns its target (may allocate; small targets use a small-buffer optimum)
//   * movable, NOT copyable
//   * Signature qualifiers (const, &, &&, noexcept) constrain how it is called
//   * empty when default-constructed or moved-from; calling an empty one is
//     UNDEFINED BEHAVIOUR (std::function throws, this does not) — guard with
//     the explicit bool conversion
//   * no target()/target_type() inspection (leaner than std::function)
// ===========================================================================
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <type_traits>

int times3(int x) { return x * 3; }

int main() {
    std::cout << std::boolalpha;

    // -- 1. The headline: hold a MOVE-ONLY callable -------------------------
    // A lambda that captured a unique_ptr by move is itself move-only, so
    // std::function REJECTS it. std::move_only_function accepts it.
    {
        auto resource = std::make_unique<int>(100);
        std::move_only_function<int(int)> f =
            [r = std::move(resource)](int x) { return *r + x; };
        std::cout << "1. move-only target        : " << f(5) << "\n"; // 105
        assert(f(5) == 105);
    }

    // -- 2. Constructs from any matching callable ---------------------------
    {
        std::move_only_function<int(int)> a = times3;                 // function ptr
        std::move_only_function<int(int)> b = [](int x){ return x+1; }; // lambda
        using namespace std::placeholders;
        std::move_only_function<int(int)> c = std::bind(times3, _1);  // bind result
        std::cout << "2. fn / lambda / bind      : "
                  << a(4) << " " << b(4) << " " << c(4) << "\n";      // 12 5 12
    }

    // -- 3. Move-only semantics ---------------------------------------------
    {
        std::move_only_function<int(int)> src = [](int x){ return x*x; };
        std::move_only_function<int(int)> dst = std::move(src);       // MOVE ok
        // std::move_only_function<int(int)> copy = dst;              // ERROR: no copy
        static_assert(!std::is_copy_constructible_v<std::move_only_function<int(int)>>);
        static_assert( std::is_move_constructible_v<std::move_only_function<int(int)>>);
        std::cout << "3. moved target            : " << dst(9) << "\n"; // 81
        std::cout << "   source now empty        : " << bool(src) << "\n"; // false
    }

    // -- 4. Qualifier-aware signatures --------------------------------------
    // 4a. const : callable through a const wrapper; target invoked as const.
    {
        std::move_only_function<int(int) const> f = [](int x){ return x*2; };
        const auto& cref = f;               // a const view of the wrapper
        std::cout << "4a. const signature        : " << cref(21) << "\n"; // 42
        // A non-const signature could NOT be called through `cref` — ill-formed.
    }
    // 4b. noexcept : the call itself is noexcept; the target must be too.
    {
        std::move_only_function<int(int) noexcept> f = [](int x) noexcept { return x+1; };
        static_assert(noexcept(f(0)));      // the call operator is noexcept
        std::cout << "4b. noexcept signature     : " << f(41) << "\n"; // 42
    }
    // 4c. &&  : the wrapper may only be called on an rvalue — a "call once and
    //           consume" contract, useful for one-shot work that moves state out.
    {
        std::move_only_function<std::string() &&> once =
            [s = std::string("consumed")]() mutable { return std::move(s); };
        std::string got = std::move(once)();   // must call on an rvalue
        std::cout << "4c. &&  (call-once)        : " << got << "\n"; // consumed
    }

    // -- 5. Empty state — calling empty is UNDEFINED BEHAVIOUR, so GUARD it --
    // A key contrast with std::function: std::function throws
    // std::bad_function_call when called empty, but calling an empty
    // move_only_function is undefined behaviour (the lean design omits the
    // check). Always test the bool conversion before invoking.
    {
        std::move_only_function<int()> f;                // default = empty
        assert(!f);
        int guarded = f ? f() : -1;                      // do NOT call when empty
        std::cout << "5. empty -> guarded call   : " << guarded << "\n"; // -1
        f = [] { return 7; };
        std::cout << "   non-empty -> call        : " << (f ? f() : -1) << "\n"; // 7
    }

    // -- 6. Reassignment ----------------------------------------------------
    {
        std::move_only_function<int(int)> f = [](int x){ return x+1; };
        f = [](int x){ return x*x; };                    // rebind to a new target
        std::cout << "6. after reassignment      : " << f(6) << "\n"; // 36
    }

    // -- 7. Realistic use: a queue of move-only work items ------------------
    // Each task owns a unique_ptr, so the queue could not be std::function.
    {
        std::queue<std::move_only_function<int()>> tasks;
        for (int i = 1; i <= 3; ++i) {
            auto payload = std::make_unique<int>(i * 10);
            tasks.push([p = std::move(payload)] { return *p; });
        }
        std::cout << "7. draining task queue     :";
        int sum = 0;
        while (!tasks.empty()) {
            std::move_only_function<int()> t = std::move(tasks.front());
            tasks.pop();
            int v = t();
            sum += v;
            std::cout << " " << v;
        }
        std::cout << "  (sum=" << sum << ")\n";           // 10 20 30 (sum=60)
        assert(sum == 60);
    }

    // -- Notes --------------------------------------------------------------
    // Prefer std::move_only_function over std::function whenever the target
    // need not be copied — task queues, one-shot deferred actions, anything
    // owning a move-only resource. It is leaner (no copy machinery, no target
    // inspection) and its qualified signatures document intent. When the
    // callable MUST be copied (stored and duplicated), a copyable wrapper is
    // required instead: std::function today, or std::copyable_function (C++26).
    std::cout << "\ndone\n";
}
