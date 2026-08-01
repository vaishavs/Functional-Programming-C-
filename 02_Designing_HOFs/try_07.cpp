// ===========================================================================
// Type-erased callable wrappers — DEBUG + TODO exercise (one file)
//
// Four standard "hold any callable behind a fixed signature" types, contrasted:
//   std::function          — owns, COPYABLE, may be empty (empty call THROWS)   [C++11]
//   std::function_ref      — non-owning VIEW of a callable (cheap, no lifetime) [C++26]
//   std::move_only_function— owns, MOVE-ONLY (holds move-only targets)          [C++23]
//   std::copyable_function — owns, COPYABLE; the modern redesign of function    [C++26]
//
// The file has TWO sections, kept separate:
//   SECTION A — DEBUG : four functions shipped with a bug the tests convict.
//   SECTION B — TODO  : four functions to implement from scratch.
//
// Markers
//   FIXME(n)  — shipped code is wrong on purpose; repair it (SECTION A).
//   TODO(n)   — implement from scratch (SECTION B).
//
// Flip a step's <NAME>_READY to 1 to activate its test. The file ships
// compiling and running with every step off. A DEBUG test asserts the CORRECT
// value, so it FAILS (or aborts) until the FIXME is repaired.
//
//   Build : g++ -std=c++23 -Wall -Wextra -o erasure erasure_wrappers_exercise.cpp
//   Run   : ./erasure
//   (std::move_only_function needs C++23. std::function_ref and
//    std::copyable_function are C++26; on a compiler that lacks them — e.g.
//    g++ 13 — the GIVEN stand-ins below are used automatically. Real std:: types
//    are used on a C++26 library.)
// ===========================================================================

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>

#define DBG1_READY  0
#define DBG2_READY  0
#define DBG3_READY  0
#define DBG4_READY  0
#define TODO1_READY 0
#define TODO2_READY 0
#define TODO3_READY 0
#define TODO4_READY 0

// ---------------------------------------------------------------------------
// GIVEN — stand-ins for the two C++26 wrappers (do not modify).
// If the standard library provides the real type, it is used; otherwise a
// minimal equivalent with the same essential behaviour is defined so this file
// builds on a pre-C++26 compiler.
// ---------------------------------------------------------------------------

// std::function_ref — a NON-OWNING view: stores only a pointer to the callable
// plus a thunk. Valid only while the referent it points at is alive.
#if defined(__cpp_lib_function_ref)
using std::function_ref;
#else
template <class Sig> class function_ref;
template <class R, class... Args>
class function_ref<R(Args...)> {
    union Storage { void* obj; void (*fun)(); } stg_{};
    R (*call_)(Storage, Args...) = nullptr;
    template <class T> static T* obj_cast(Storage s) { return static_cast<T*>(s.obj); }
public:
    template <class F, class DF = std::remove_reference_t<F>,
              class = std::enable_if_t<!std::is_same_v<std::decay_t<F>, function_ref> &&
                                       !std::is_function_v<DF> &&
                                       std::is_invocable_r_v<R, F&, Args...>>>
    function_ref(F&& f) noexcept {
        stg_.obj = const_cast<void*>(static_cast<const void*>(std::addressof(f)));
        call_ = [](Storage s, Args... a) -> R {
            return std::invoke(*obj_cast<DF>(s), std::forward<Args>(a)...); };
    }
    template <class F, class = std::enable_if_t<std::is_function_v<F>>>
    function_ref(F* f) noexcept {
        stg_.fun = reinterpret_cast<void(*)()>(f);
        call_ = [](Storage s, Args... a) -> R {
            return reinterpret_cast<F*>(s.fun)(std::forward<Args>(a)...); };
    }
    R operator()(Args... a) const { return call_(stg_, std::forward<Args>(a)...); }
};
#endif

// std::copyable_function — an OWNING, COPYABLE wrapper. Copying it deep-copies
// the stored callable, so each copy owns an independent target.
#if defined(__cpp_lib_copyable_function)
using std::copyable_function;
#else
template <class Sig> class copyable_function;
template <class R, class... Args>
class copyable_function<R(Args...)> {
    struct Base { virtual ~Base() = default; virtual R call(Args...) = 0;
                  virtual std::unique_ptr<Base> clone() const = 0; };
    template <class F> struct Impl : Base {
        F f; explicit Impl(F fn) : f(std::move(fn)) {}
        R call(Args... a) override { return std::invoke(f, std::forward<Args>(a)...); }
        std::unique_ptr<Base> clone() const override { return std::make_unique<Impl>(f); }
    };
    std::unique_ptr<Base> p_;
public:
    copyable_function() = default;
    template <class F, class = std::enable_if_t<!std::is_same_v<std::decay_t<F>, copyable_function> &&
                                                std::is_invocable_r_v<R, std::decay_t<F>&, Args...>>>
    copyable_function(F&& f) : p_(std::make_unique<Impl<std::decay_t<F>>>(std::forward<F>(f))) {}
    copyable_function(const copyable_function& o) : p_(o.p_ ? o.p_->clone() : nullptr) {}
    copyable_function(copyable_function&&) noexcept = default;
    copyable_function& operator=(const copyable_function& o) { p_ = o.p_ ? o.p_->clone() : nullptr; return *this; }
    copyable_function& operator=(copyable_function&&) noexcept = default;
    explicit operator bool() const noexcept { return static_cast<bool>(p_); }
    R operator()(Args... a) const { return p_->call(std::forward<Args>(a)...); }
};
#endif

// ###########################################################################
// SECTION A — DEBUG (FIXME)
// ###########################################################################

// ===========================================================================
// DEBUG 1 — std::function: an empty wrapper throws when called
// ===========================================================================
//
// A default-constructed std::function is empty; calling an empty std::function
// throws std::bad_function_call.
//
// FIXME(D1): make_adder(n) should return a callable that adds n, but the
//   assignment is guarded by n > 0, so make_adder(0) returns an EMPTY
//   std::function and calling it throws std::bad_function_call.
// ---------------------------------------------------------------------------
std::function<int(int)> make_adder(int n) {
    std::function<int(int)> f;
    if (n > 0)                                   // FIXME(D1): n == 0 leaves f empty
        f = [n](int x) { return x + n; };
    return f;
}

// ===========================================================================
// DEBUG 2 — std::function_ref: a non-owning view must not outlive its referent
// ===========================================================================
//
// std::function_ref stores only a pointer to a callable; it owns nothing.
//
// FIXME(D2): make_handler returns an owning std::function, but builds it from a
//   function_ref that points at the LOCAL lambda `local`. std::function copies
//   the VIEW (dangling once make_handler returns), not the lambda, so the
//   returned callable reads freed stack.
// ---------------------------------------------------------------------------
std::function<int(int)> make_handler() {
    int base = 100;
    auto local = [base](int x) { return x + base; };
    function_ref<int(int)> ref = local;          // a VIEW of local
    return ref;                                  // FIXME(D2): stores the dangling VIEW
}

// ===========================================================================
// DEBUG 3 — std::move_only_function: calling an empty one is undefined behaviour
// ===========================================================================
//
// Unlike std::function, an empty std::move_only_function does NOT throw when
// called — it is undefined behaviour.
//
// FIXME(D3): make_scaler(factor) should return a callable that multiplies by
//   factor, but the assignment is guarded by factor > 1, so make_scaler(1)
//   returns an EMPTY move_only_function and calling it is undefined behaviour.
// ---------------------------------------------------------------------------
std::move_only_function<int(int)> make_scaler(int factor) {
    std::move_only_function<int(int)> f;
    if (factor > 1)                              // FIXME(D3): factor == 1 leaves f empty
        f = [factor](int x) { return x * factor; };
    return f;
}

// ===========================================================================
// DEBUG 4 — std::copyable_function: copying duplicates the target's state
// ===========================================================================
//
// std::copyable_function is copyable, and a copy owns an INDEPENDENT copy of the
// target, so a by-value capture is duplicated on copy (a by-reference capture is
// shared).
//
// FIXME(D4): tally() means for the wrapper and its copy to increment the SAME
//   counter (expecting 3), but the lambda captures the counter BY VALUE, so the
//   copy bumps its own private counter and the external `total` never moves.
// ---------------------------------------------------------------------------
int tally() {
    int total = 0;
    copyable_function<void()> f = [total]() mutable { ++total; };  // FIXME(D4): by-value copy
    copyable_function<void()> g = f;                              // copy -> independent counter
    f();
    g();
    g();
    return total;
}

// ###########################################################################
// SECTION B — TODO
// ###########################################################################

// ===========================================================================
// TODO 1 — std::function: owning, copyable, reassignable
// ===========================================================================
//
// TODO(T1): make_multiplier(k) -> std::function<int(int)>
//   Return a std::function that multiplies its argument by k. (The test also
//   reassigns the same std::function to a different multiplier.)
// ---------------------------------------------------------------------------
std::function<int(int)> make_multiplier(int /*k*/) {
    // TODO(T1)
    return [](int) { return 0; };
}

// ===========================================================================
// TODO 2 — std::function_ref: the correct, synchronous use
// ===========================================================================
//
// TODO(T2): apply_twice(f, x) -> int
//   f is a function_ref<int(int)> (a non-owning view used synchronously).
//   Return f(f(x)).
// ---------------------------------------------------------------------------
int apply_twice(function_ref<int(int)> /*f*/, int /*x*/) {
    // TODO(T2)
    return 0;
}

// ===========================================================================
// TODO 3 — std::move_only_function: hold a move-only target
// ===========================================================================
//
// TODO(T3): make_owner(ptr) -> std::move_only_function<int(int)>
//   Return a move_only_function that OWNS the given unique_ptr and, when called
//   with x, returns *ptr + x. (std::function could not hold this closure.)
// ---------------------------------------------------------------------------
std::move_only_function<int(int)> make_owner(std::unique_ptr<int> /*ptr*/) {
    // TODO(T3)
    return [](int) { return 0; };
}

// ===========================================================================
// TODO 4 — std::copyable_function: a target that must be copied
// ===========================================================================
//
// TODO(T4): duplicate(f, x) -> int
//   f is a copyable_function<int(int)>. Make a COPY of f, then return
//   f(x) + copy(x). (move_only_function could not be copied.)
// ---------------------------------------------------------------------------
int duplicate(copyable_function<int(int)> /*f*/, int /*x*/) {
    // TODO(T4)
    return 0;
}

// ===========================================================================
// Tests — do not modify. Each assert names the mistake or task it checks.
// ===========================================================================
int main() {
    std::cout << std::boolalpha;

    // -------- SECTION A: DEBUG --------
#if DBG1_READY
    {
        assert(make_adder(0)(5) == 5);           // FIXME(D1): empty std::function throws
        std::cout << "debug 1  std::function (empty=throw) .. ok\n";
    }
#else
    std::cout << "debug 1  std::function (empty=throw) .. TODO (flip DBG1_READY)\n";
#endif

#if DBG2_READY
    {
        // FIXME(D2): the returned callable must survive make_handler's return. A
        // stack clobber is interposed so a stored dangling VIEW reliably reads
        // garbage and fails; owning the lambda returns 105.
        auto h = make_handler();
        volatile int stack_noise[32];
        for (int i = 0; i < 32; ++i) stack_noise[i] = i;
        (void)stack_noise;
        assert(h(5) == 105);
        std::cout << "debug 2  std::function_ref (dangling) . ok\n";
    }
#else
    std::cout << "debug 2  std::function_ref (dangling) . TODO (flip DBG2_READY)\n";
#endif

#if DBG3_READY
    {
        assert(make_scaler(1)(5) == 5);          // FIXME(D3): empty move_only_function is UB
        std::cout << "debug 3  std::move_only_function (UB) . ok\n";
    }
#else
    std::cout << "debug 3  std::move_only_function (UB) . TODO (flip DBG3_READY)\n";
#endif

#if DBG4_READY
    {
        assert(tally() == 3);                    // FIXME(D4): by-value capture duplicated on copy
        std::cout << "debug 4  std::copyable_function (copy) . ok\n";
    }
#else
    std::cout << "debug 4  std::copyable_function (copy) . TODO (flip DBG4_READY)\n";
#endif

    // -------- SECTION B: TODO --------
#if TODO1_READY
    {
        auto f = make_multiplier(3);
        assert(f(4) == 12);
        f = make_multiplier(10);                 // reassignable
        assert(f(4) == 40);
        std::cout << "todo  1  std::function ............... ok\n";
    }
#else
    std::cout << "todo  1  std::function ............... TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        assert(apply_twice([](int v) { return v + 1; }, 40) == 42);
        std::cout << "todo  2  std::function_ref ........... ok\n";
    }
#else
    std::cout << "todo  2  std::function_ref ........... TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY
    {
        assert(make_owner(std::make_unique<int>(100))(5) == 105);
        std::cout << "todo  3  std::move_only_function ..... ok\n";
    }
#else
    std::cout << "todo  3  std::move_only_function ..... TODO (flip TODO3_READY)\n";
#endif

#if TODO4_READY
    {
        assert(duplicate([](int v) { return v * 2; }, 5) == 20);   // 10 + 10
        std::cout << "todo  4  std::copyable_function ...... ok\n";
    }
#else
    std::cout << "todo  4  std::copyable_function ...... TODO (flip TODO4_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
