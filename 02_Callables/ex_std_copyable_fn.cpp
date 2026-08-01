// ===========================================================================
// std::copyable_function<Signature>   — C++26, header <functional>
//
//   Build : g++ -std=c++2c -Wall -Wextra -o cf std_copyable_function.cpp
//           (requires a C++26 library: libstdc++ from GCC 15+, or recent
//            libc++. NOT available in older toolchains such as g++ 13.)
//
//   *** ILLUSTRATIVE FILE ***
//   This program uses a C++26 facility absent from the g++ 13.3 toolchain used
//   to verify the sibling examples, so it was NOT compiled here. The copyable-
//   storage behaviour in sections 1–6 was verified by compiling an equivalent
//   std::function program (std::function is the closest existing copyable,
//   type-erased wrapper); the values shown in the comments are that program's
//   observed output. Section 7 exercises the qualifier-aware signatures, which
//   are specific to std::copyable_function and have no std::function analogue.
//
// What it is
//   The modern, COPYABLE type-erased callable wrapper — effectively the C++26
//   redesign of std::function. It owns any callable matching Signature, is both
//   copyable and movable, and (unlike std::function) accepts const/&/&&/noexcept
//   qualifiers in Signature that constrain how it may be called.
//
// How it relates to its two siblings
//   * vs std::function (C++11): same job (owning, copyable erasure) but with the
//     cleaned-up design — qualifier-aware signatures, a call operator whose
//     const-ness follows the signature instead of being unconditionally const,
//     no RTTI-based target()/target_type() inspection, and fewer surprising
//     conversions. New code preferring a copyable wrapper should choose this.
//   * vs std::move_only_function (C++23): identical modern design, differing only
//     on copyability. copyable_function requires a COPY-CONSTRUCTIBLE target and
//     is itself copyable; move_only_function accepts move-only targets and is
//     move-only. Choose copyable_function when the callable must be duplicated
//     (stored and copied); choose move_only_function when it need not be copied
//     or owns a move-only resource (e.g. a captured std::unique_ptr).
//
// Properties at a glance
//   * owns its target (may allocate; small targets use a small-buffer optimum)
//   * copyable AND movable; the target must be copy-constructible
//   * Signature qualifiers (const, &, &&, noexcept) constrain how it is called
//   * empty when default-constructed or moved-from; calling an empty one is
//     UNDEFINED BEHAVIOUR (std::function throws, this does not — the lean design
//     it shares with move_only_function) — guard with the explicit bool conversion
//   * no target()/target_type() inspection
// ===========================================================================
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

int times3(int x) { return x * 3; }

int main() {
    std::cout << std::boolalpha;

    // The defining trait: copyable, and itself copy-constructible.
    static_assert(std::is_copy_constructible_v<std::copyable_function<int(int)>>);
    static_assert(std::is_move_constructible_v<std::copyable_function<int(int)>>);

    // -- 1. Basic construction and call -------------------------------------
    {
        std::copyable_function<int(int)> f = [](int x){ return x * 2; };
        std::cout << "1. basic result            : " << f(21) << "\n"; // 42
    }

    // -- 2. Copyable: a copy is an INDEPENDENT wrapper ----------------------
    // Rebinding the original does not disturb the copy — the capability that
    // distinguishes this from std::move_only_function.
    {
        std::copyable_function<int(int)> f = [](int x){ return x + 1; };
        std::copyable_function<int(int)> g = f;          // COPY
        f = [](int x){ return x * 100; };                // rebind original only
        std::cout << "2. copy is independent     : g=" << g(5) << " f=" << f(5) << "\n"; // g=6 f=500
    }

    // -- 3. Reassignment ----------------------------------------------------
    {
        std::copyable_function<int(int)> f = times3;
        f = [](int x){ return x - 1; };                  // rebind to a new target
        std::cout << "3. after reassignment      : " << f(10) << "\n"; // 9
    }

    // -- 4. Stored in a container that is itself copied ---------------------
    // Because the element type is copyable, the whole container copies.
    {
        std::vector<std::copyable_function<int(int)>> ops{
            [](int x){ return x + 1; }, times3, [](int x){ return x * x; } };
        auto copy = ops;                                 // deep-copies every callable
        std::cout << "4. container (copied)      :";
        int acc = 0;
        for (auto& op : copy) { int v = op(4); acc += v; std::cout << " " << v; }
        std::cout << "  (sum=" << acc << ")\n";          // 5 12 16 (sum=33)
    }

    // -- 5. Empty state — calling empty is UB, so GUARD it ------------------
    // Same contract as move_only_function: no bad_function_call, just UB on an
    // empty call. Test the bool conversion first.
    {
        std::copyable_function<int()> f;                 // default = empty
        assert(!f);
        std::cout << "5. empty -> guarded call   : " << (f ? f() : -1) << "\n"; // -1
    }

    // -- 6. The target must be COPY-CONSTRUCTIBLE ---------------------------
    // A capture of a copyable resource (shared_ptr) is fine and copies with the
    // wrapper. A move-only capture (unique_ptr) would NOT compile here — that is
    // precisely the case std::move_only_function exists for.
    {
        auto shared = std::make_shared<int>(100);
        std::copyable_function<int(int)> f = [s = shared](int x){ return *s + x; };
        auto g = f;                                      // ok: captured shared_ptr copies
        std::cout << "6. copyable target         : f=" << f(5) << " g=" << g(5) << "\n"; // 105 105
        // std::copyable_function<int(int)> bad =
        //     [p = std::make_unique<int>(1)](int x){ return *p + x; }; // ERROR: move-only
    }

    // -- 7. Qualifier-aware signatures (C++26-specific; illustrative) --------
    // These have no std::function equivalent and were not exercised by the
    // verification twin.
    {
        // 7a. const: callable through a const wrapper; target invoked as const.
        std::copyable_function<int(int) const> cf = [](int x){ return x * 2; };
        const auto& cref = cf;
        std::cout << "7a. const signature        : " << cref(21) << "\n"; // 42
        // A non-const signature could NOT be called through `cref` — ill-formed.

        // 7b. noexcept: the call operator is noexcept; the target must be too.
        std::copyable_function<int(int) noexcept> nf = [](int x) noexcept { return x + 1; };
        static_assert(noexcept(nf(0)));
        std::cout << "7b. noexcept signature     : " << nf(41) << "\n"; // 42
    }

    // -- Notes --------------------------------------------------------------
    // Reach for std::copyable_function when a callable must be stored AND copied
    // behind a fixed, named type — a member duplicated with its owner, a value
    // handed to several consumers, a container of callables that is itself
    // copied. When copying is not required, std::move_only_function is leaner
    // and also accepts move-only targets. Within generic code that need not
    // erase the type at all, a template parameter constrained by std::invocable
    // keeps the concrete type and its inlining, avoiding the erasure cost of any
    // of these wrappers.
    std::cout << "\ndone\n";
}
