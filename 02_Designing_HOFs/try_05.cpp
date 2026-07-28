// ===========================================================================
// Callable entities in C++ — TODO/FIXME exercise
//
// Every way C++ lets something be "called", in one file:
//   1. free / namespace / member / static-member functions
//   2. function pointers, and pointers to member FUNCTIONS
//   3. references to functions
//   4. functors (class types with operator())
//   5. lambdas (captureless, capturing, generic)
//   6. std::function (owning) and function_ref (non-owning)
//   7. function-like macros — and why they are NOT in the same family
//   8. objects with a conversion-to-function-pointer (surrogate call functions)
//   9. standard adaptors: bind, reference_wrapper
//  10. standard function objects, and querying callability (is_invocable, ...)
//
// Markers
//   TODO(n)     — implement from scratch.
//   FIXME(n)    — shipped code is wrong on purpose; the tests convict it.
//   PREDICT     — commit to an answer before compiling.
//   QUESTION    — answer in a comment next to the marker.
//
// Flip a step's STEPn_READY to 1 when finished. Steps are cumulative — enable
// them in order. Ships compiling and running with every step off.
//
//   Build : g++ -std=c++17 -Wall -Wextra -o callable callable_entities_todo.cpp
//   Run   : ./callable
//
// The unifying idea
//   std::invoke(f, args...) calls ALL of entities 1–6 with one syntax,
//   including the awkward member-pointer case (object goes first). Entity 8
//   explains how an object with no operator() can still be called: a
//   conversion to a function pointer. Entity 7, the macro, is the outlier: it
//   is preprocessor text substitution, has no type, no address, and cannot be
//   passed to std::invoke — which is exactly the point step 7 makes.
//
// Convention used by every test
//   * Nothing prints. Results are returned/compared with ==.
//   * function_ref below is a minimal stand-in for C++26 std::function_ref
//     (absent from most current compilers). It is GIVEN; do not modify it.
// ===========================================================================

#include <array>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0
#define STEP6_READY 0
#define STEP8_READY 0
#define STEP9_READY 0
#define STEP10_READY 0

// ---------------------------------------------------------------------------
// GIVEN: a minimal non-owning function_ref (stands in for std::function_ref).
// Stores a pointer to the callable plus a thunk. A union lets it hold either
// an object address (functors, lambdas) or a function pointer (functions,
// function pointers) — the two cannot share one pointer type portably.
// ---------------------------------------------------------------------------
template <class Sig> class function_ref;
template <class R, class... Args>
class function_ref<R(Args...)> {
    union Storage { void* obj; void (*fun)(); } stg_{};
    R (*call_)(Storage, Args...) = nullptr;
    template <class T> static T* obj_cast(Storage s) { return static_cast<T*>(s.obj); }
public:
    template <class F, class DF = std::remove_reference_t<F>,
              class = std::enable_if_t<
                  !std::is_same_v<std::decay_t<F>, function_ref> &&
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

// ===========================================================================
// STEP 1 — functions: free, namespace, member, static member
// ===========================================================================
//
// These are GIVEN. The tasks are about CALLING them, including the member
// cases whose call syntax is unusual.
//
// TODO(1a)  member_deposit(acc, amount) -> int
//   Call the member function Account::deposit on `acc`. The plain call is
//   acc.deposit(amount); write exactly that.
//
// TODO(1b)  via_pmf(acc, amount) -> int
//   Same effect, but reach deposit through a POINTER TO MEMBER FUNCTION.
//   * Form the pointer: int (Account::*p)(int) = &Account::deposit;
//     (the &Class:: prefix is mandatory for member pointers — unlike free
//     functions, a member function name does not decay to a pointer).
//   * Call through it with the .* operator: (acc.*p)(amount).
//
// TODO(1c)  via_invoke_pmf(acc, amount) -> int
//   Same again, through std::invoke. The member-pointer rule: the OBJECT is
//   the first argument. std::invoke(&Account::deposit, acc, amount).
//   This is why std::invoke exists — call syntax cannot express this.
//
// QUESTION(1): Account::fee is a STATIC member function. Does calling it need
//   an object? Write the call in a comment, and note how its type differs
//   from a non-static member function's type.
// ---------------------------------------------------------------------------
struct Account {
    int balance;
    int deposit(int amount) { balance += amount; return balance; }
    static int fee() { return 5; }
};
namespace geo {
inline double area(double w, double h) { return w * h; }
}

int member_deposit(Account& /*acc*/, int /*amount*/) {
    // TODO(1a)
    return 0;
}

int via_pmf(Account& /*acc*/, int /*amount*/) {
    // TODO(1b)
    return 0;
}

int via_invoke_pmf(Account& /*acc*/, int /*amount*/) {
    // TODO(1c)
    return 0;
}

// ===========================================================================
// STEP 2 — function pointers and dispatch tables
// ===========================================================================
//
// A free function name DECAYS to a pointer, so &f and f are interchangeable
// when initialising a function pointer.
//
// TODO(2a)  apply_binop(f, a, b) -> int
//   f is a pointer to a binary int operation. Call it: f(a, b).
//   The parameter type is spelled with the BinOp alias below.
//
// TODO(2b)  make_dispatch() -> std::map<std::string, BinOp>
//   Return a table mapping "add"->add, "sub"->sub, "mul"->mul (all given).
//   Storing functions in a container is the point: a function pointer is a
//   value. Brace-init the map with the three pairs.
//
// TODO(2c)  run(table, name, a, b) -> int
//   Look up `name` in `table` and call the found operation on (a, b).
//   (Assume the key exists.)
// ---------------------------------------------------------------------------
inline int add(int a, int b) { return a + b; }
inline int sub(int a, int b) { return a - b; }
inline int mul(int a, int b) { return a * b; }
using BinOp = int (*)(int, int);

int apply_binop(BinOp /*f*/, int /*a*/, int /*b*/) {
    // TODO(2a)
    return 0;
}

std::map<std::string, BinOp> make_dispatch() {
    // TODO(2b)
    return {};
}

int run(const std::map<std::string, BinOp>& /*table*/, const std::string& /*name*/,
        int /*a*/, int /*b*/) {
    // TODO(2c)
    return 0;
}

// ===========================================================================
// STEP 3 — references to functions
// ===========================================================================
//
// A function reference int(&)(int,int) binds to a function directly. Calling
// it looks identical to calling the function; the reference-ness is invisible
// at the call site. It cannot be rebound and cannot be null — the two ways it
// differs from a function pointer.
//
// TODO(3a)  apply_fref(f, a, b) -> int
//   f is a REFERENCE to a binary int function (parameter type: int(&)(int,int)).
//   Call it: f(a, b). No & needed to call, just as with a pointer.
//
// TODO(3b)  bind_and_call(a, b) -> int
//   Inside the body, declare a function reference bound to `sub` (from step 2)
//   and call it on (a, b).
//   * Syntax: int (&r)(int, int) = sub;  then  return r(a, b);
//
// QUESTION(3): a function reference and a function pointer both let `sub` be
//   called indirectly. Name one thing a pointer can do that a reference
//   cannot, and one guarantee a reference gives that a pointer does not.
// ---------------------------------------------------------------------------
int apply_fref(int (&/*f*/)(int, int), int /*a*/, int /*b*/) {
    // TODO(3a)
    return 0;
}

int bind_and_call(int /*a*/, int /*b*/) {
    // TODO(3b)
    return 0;
}

// ===========================================================================
// STEP 4 — functors (class types with operator())
// ===========================================================================
//
// A functor carries STATE its operator() can use — the capability a plain
// function pointer lacks. Standard examples: std::plus, std::negate, std::less.
//
// TODO(4a)  Scale — a functor multiplying its argument by a stored factor.
//   Give it an int member `k` and a const operator()(int x) returning k * x.
//   The struct is declared below with the body missing.
//
// TODO(4b)  apply_scale(k, x) -> int
//   Construct a Scale{k} and call it on x.
//
// TODO(4c)  Counter — a STATEFUL functor returning 1, 2, 3, ... on successive
//   calls. Give it an int member `n` initialised to 0 and a NON-const
//   operator()() that pre-increments and returns n. (Non-const because it
//   mutates — the mirror of a mutable lambda.)
//
// QUESTION(4): std::invoke(Scale{3}, 10) works. Why can a functor go through
//   std::invoke with no special member-pointer handling, while Account::deposit
//   from step 1 needs the object passed separately?
// ---------------------------------------------------------------------------
struct Scale {
    int k;
    // TODO(4a): add the operator()
};

int apply_scale(int /*k*/, int /*x*/) {
    // TODO(4b)
    return 0;
}

struct Counter {
    int n = 0;
    // TODO(4c): add the operator()
};

// ===========================================================================
// STEP 5 — lambdas: captureless, capturing, generic
// ===========================================================================
//
// A lambda is an unnamed functor the compiler writes. A CAPTURELESS lambda
// additionally converts to a plain function pointer; a capturing one does not
// (it has state, like any functor).
//
// TODO(5a)  compose_twice(f, x)  [template on F]  -> whatever f returns
//   Return f(f(x)). This accepts ANY callable — function, functor, lambda —
//   because F is a template parameter. Templates are how "generic over all
//   callables" is spelled without type erasure.
//
// TODO(5b)  as_function_pointer() -> int(*)(int)
//   Return a CAPTURELESS lambda that adds 1, converted to a function pointer.
//   The conversion is implicit: `int (*p)(int) = [](int x){ return x + 1; };`
//
// TODO(5c)  double_it(x)  [template]  -> whatever the GENERIC lambda returns
//   Inside, define a GENERIC lambda `[](auto v) { return v + v; }` and apply
//   it to x. Because it is generic, double_it works for int AND std::string.
//
// QUESTION(5): why does a capturing lambda NOT convert to a function pointer,
//   while a captureless one does? (Relate it to what a function pointer can
//   and cannot store.)
// ---------------------------------------------------------------------------
template <class F>
auto compose_twice(F /*f*/, int /*x*/) {
    // TODO(5a)
    return 0;
}

int (*as_function_pointer())(int) {
    // TODO(5b)
    return nullptr;
}

template <class T>
auto double_it(T /*x*/) {
    // TODO(5c)
    return T{};
}

// ===========================================================================
// STEP 6 — type erasure (std::function) and non-owning refs (function_ref)
// ===========================================================================
//
// std::function<Sig> OWNS a copy of any callable matching Sig — heap-allocating
// if needed. function_ref<Sig> (given at the top) merely POINTS at one, storing
// nothing, valid only while the referent lives.
//
// TODO(6a)  reassignable(a, b) -> int
//   Declare std::function<int(int,int)> holding `add`, then REASSIGN it to
//   `mul`, then call it on (a, b). The point: one variable, different callables
//   over its lifetime — impossible with a function pointer across incompatible
//   types, trivial here because std::function erases the concrete type.
//
// TODO(6b)  call_through_ref(f, x) -> int
//   f is a function_ref<int(int)>. Call it: f(x). The parameter is non-owning,
//   so the caller keeps ownership of whatever it passed.
//
// FIXME(A) — the dangling capture.
//   make_incrementer returns a std::function built from a lambda that captures
//   a local BY REFERENCE ([&base]). std::function owns the lambda, but the
//   lambda holds a reference to `base`, which dies when make_incrementer
//   returns — so every later call reads freed stack.
//   PREDICT the value inc(41) returns before running: it will NOT reliably be
//   42 (often a large garbage number; AddressSanitizer reports
//   stack-use-after-return).
//   Repair: capture BY VALUE ([base]) so the lambda carries its own copy.
//   QUESTION(A): function_ref (given at the top) is ALSO non-owning — it points
//   at a callable it does not own. State the one-line rule for when returning a
//   function_ref is safe, and why capture-by-value does not rescue it the way
//   it rescues this std::function.
//
// FIXME(B) — std::function copies; the copy has its own state.
//   count_calls stores a STATEFUL Counter-like lambda in a std::function,
//   intending each call to advance the count. It hands back the wrong number
//   because a fresh std::function is built INSIDE the loop each iteration,
//   resetting the captured state every time.
//   PREDICT the returned vector before running.
//   Repair: build the std::function ONCE, outside the loop, so its single
//   captured state persists across calls.
// ---------------------------------------------------------------------------
int reassignable(int /*a*/, int /*b*/) {
    // TODO(6a)
    return 0;
}

int call_through_ref(function_ref<int(int)> /*f*/, int /*x*/) {
    // TODO(6b)
    return 0;
}

inline std::function<int(int)> make_incrementer() {
    int base = 1;
    // FIXME(A): [&base] captures a reference to a local that dies at return.
    return [&base](int x) { return x + base; };
}

inline std::vector<int> count_calls(int times) {
    std::vector<int> out;
    for (int i = 0; i < times; ++i) {
        // FIXME(B): a fresh std::function (fresh state) is built every iteration.
        std::function<int()> tick = [n = 0]() mutable { return ++n; };
        out.push_back(tick());
    }
    return out;
}

// ===========================================================================
// STEP 7 — function-like macros, and why they are a different species
// ===========================================================================
//
// A macro is preprocessor TEXT SUBSTITUTION. It has no type, no address, no
// scope, no overloading — it cannot be passed to std::invoke, stored in a
// std::function, or referenced. The tasks here are about seeing its failure
// modes and replacing it with a real callable.
//
// The three classic macro hazards (all GIVEN, all buggy on purpose):
//   SQ_BAD   — no parentheses: precedence corrupts the expansion.
//   SQ_PAREN — parenthesised: fixes precedence, but STILL double-evaluates.
//   MAX_MAC  — evaluates whichever argument "wins" a SECOND time.
//
// TODO(7a)  sq_fn — a constexpr function replacing SQ_BAD/SQ_PAREN.
//   constexpr int sq_fn(int x) { return x * x; }  — evaluates x exactly once,
//   respects precedence, has a type and an address.
//
// FIXME(C) — the precedence bug.
//   bad_square uses SQ_BAD. PREDICT bad_square(1 + 2) before running: the
//   expansion is 1 + 2 * 1 + 2, which is NOT 9. Repair by calling sq_fn.
//
// FIXME(D) — the double-evaluation bug.
//   squared_ticks calls SQ_PAREN on an argument WITH A SIDE EFFECT (a counter
//   bump). Parentheses do not help here: the argument text appears twice, so
//   the side effect happens twice. PREDICT the counter value before running.
//   Repair by calling sq_fn, which takes the argument by value once.
//   QUESTION(D): name one thing a function-like macro can still do in C++ that
//   no function, lambda, or template can. (Hint: it operates on TEXT, before
//   types exist — think stringizing or token pasting.)
// ---------------------------------------------------------------------------
#define SQ_BAD(x)      x * x
#define SQ_PAREN(x)    ((x) * (x))
#define MAX_MAC(a, b)  ((a) > (b) ? (a) : (b))

// TODO(7a): define sq_fn here
// constexpr int sq_fn(int x) { ... }

inline int g_ticks = 0;
inline int tick() { ++g_ticks; return 5; }

inline int bad_square(int a, int b) {
    // FIXME(C): SQ_BAD expands without parentheses.
    return SQ_BAD(a + b);
}

inline int squared_ticks() {
    g_ticks = 0;
    // FIXME(D): the argument's side effect fires twice.
    int r = SQ_PAREN(tick());
    return r;   // the test checks BOTH r and g_ticks
}

// ===========================================================================
// STEP 8 — surrogate call functions (objects callable via a conversion)
// ===========================================================================
//
// An object can be callable with obj(args) even with NO operator() — if its
// class provides a conversion to a POINTER TO FUNCTION. The compiler
// synthesises a hidden "surrogate call function" that converts the object and
// calls through the pointer. This is not exotic trivia: it is EXACTLY how a
// captureless lambda (step 5b) becomes a function pointer.
//
// TODO(8a)  Handler — complete the conversion operator so a Handler is callable.
//   The struct below declares `using Fn = int(*)(int);` and nothing else.
//   Add:  operator Fn() const { return [](int x){ return x + 1; }; }
//   Note there is deliberately NO operator() — the conversion is what makes
//   Handler{}(41) legal.
//
// TODO(8b)  call_object(h, x) -> int
//   Call the Handler: h(x). An object with no operator(), called like a function.
//
// GIVEN exhibit: Dual has TWO conversions. The compiler makes one surrogate per
// conversion, and ordinary overload resolution picks by the call's arguments —
// here by ARITY. Dual{}(9) selects the (int) surrogate; Dual{}(9,9) the
// (int,int) one. Nothing to implement; the tests read it.
//
// QUESTION(8): if a class had BOTH an operator() and a conversion-to-function-
//   pointer that matched the same call, what would obj(args) do? (Consider that
//   overload resolution sees them together.)
// ---------------------------------------------------------------------------
struct Handler {
    using Fn = int (*)(int);
    // TODO(8a): add the conversion operator (operator Fn)
};

int call_object(Handler /*h*/, int /*x*/) {
    // TODO(8b)
    return 0;
}

struct Dual {                                   // GIVEN exhibit — do not modify
    using FnI = long (*)(int);
    using FnP = long (*)(int, int);
    operator FnI() const { return [](int) { return 1L; }; }
    operator FnP() const { return [](int, int) { return 2L; }; }
};

// ===========================================================================
// STEP 9 — the standard adaptors: bind, reference_wrapper
// ===========================================================================
//
// Each of these is itself a function object the library hands out — none is a
// new KIND of callable, they WRAP or TRANSFORM the kinds above.
//
// TODO(9a)  bind_minus10(x) -> int
//   Partial application with std::bind: fix sub's SECOND argument to 10.
//     using namespace std::placeholders;
//     auto f = std::bind(sub, _1, 10);   return f(x);
//
// TODO(9b)  bind_flip(a, b) -> int
//   std::bind can also REORDER: bind sub as (_2, _1) so f(a,b) computes b - a.
//
// TODO(9c)  count_via_ref(c) -> int
//   std::reference_wrapper (from std::ref) forwards a call to the SAME object,
//   so state is shared (contrast FIXME(B), where a std::function COPY reset it).
//     auto rc = std::ref(c);  rc(); rc();  return c.n;   // c advanced to 2
//
// QUESTION(9): std::bind existed since C++11 but is now usually avoided for
//   partial application. Name the C++20 facility that fixes leading arguments
//   without placeholders, and one reason a lambda is often preferred over bind.
// ---------------------------------------------------------------------------
int bind_minus10(int /*x*/) {
    // TODO(9a)
    return 0;
}

int bind_flip(int /*a*/, int /*b*/) {
    // TODO(9b)
    return 0;
}

int count_via_ref(Counter& /*c*/) {
    // TODO(9c)
    return 0;
}

// ===========================================================================
// STEP 10 — standard function objects, and querying callability
// ===========================================================================
//
// The library ships ready-made function objects for the operators (std::plus,
// std::multiplies, std::less, ...). Written with an EMPTY type argument —
// std::less<> — they become TRANSPARENT: one templated operator() that accepts
// mixed operand types, letting a sorted container be searched without building
// a temporary key. Separately, std::is_invocable / std::invoke_result_t let
// code ASK, at compile time, whether a call is well-formed and what it yields.
//
// TODO(10a)  fold_mul(v) -> int
//   Reduce with a standard function object as the operation:
//     return std::accumulate(v.begin(), v.end(), 1, std::multiplies<>{});
//
// TODO(10b)  set_contains(s, key) -> bool
//   s is std::set<std::string, std::less<>> — a TRANSPARENT comparator. Look up
//   a const char* key WITHOUT constructing a std::string:
//     return s.count(key) != 0;
//   (Without the transparent std::less<>, this would build a temporary string.)
//
// TODO(10c)  accepts<F, Args...>()  [constexpr]  -> bool
//   Report whether F is callable with Args...:
//     return std::is_invocable_v<F, Args...>;
//   This is the trait form of "is it a callable" — the query behind std::invoke.
//
// QUESTION(10): std::invoke_result_t<BinOp, int, int> names a type. Which type,
//   and what is the C++20 CONCEPT that expresses the same "is invocable" query
//   as a template constraint (the concept form of std::is_invocable)?
// ---------------------------------------------------------------------------
int fold_mul(const std::vector<int>& /*v*/) {
    // TODO(10a)
    return 0;
}

bool set_contains(const std::set<std::string, std::less<>>& /*s*/, const char* /*key*/) {
    // TODO(10b)
    return false;
}

template <class F, class... Args>
constexpr bool accepts() {
    // TODO(10c)
    return false;
}

// ---------------------------------------------------------------------------
// Beyond C++17 (nothing to do — orientation only).
//   The picture above is complete for C++17. Newer standards add more callable
//   plumbing, left out here so the file builds on a C++17 compiler:
//     * std::bind_front (C++20) / std::bind_back (C++23) — placeholder-free
//       partial application (the QUESTION(9) answer for bind_front).
//     * the std::invocable / std::predicate / std::relation CONCEPTS (C++20) —
//       constraint forms of the is_invocable traits used in step 10.
//     * range adaptor closures (C++20): std::views::filter(pred) is a function
//       object; `range | closure` is `closure(range)`.
//     * std::move_only_function (C++23) — like std::function but holds a
//       move-only callable; std::function_ref (C++26) — the real version of the
//       non-owning view stubbed at the top of this file.
//     * std::packaged_task (C++11, needs <future>) — a callable wired to a
//       std::future so its result can be collected elsewhere.
// ---------------------------------------------------------------------------

// ===========================================================================
// Tests — do not modify. Each assert names the mistake it catches.
// ===========================================================================
int main() {
    std::cout << std::boolalpha;

    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        Account a{100};
        assert(member_deposit(a, 50) == 150);
        Account b{0};
        assert(via_pmf(b, 10) == 10);            // pointer-to-member call
        Account c{0};
        assert(via_invoke_pmf(c, 7) == 7);       // object-first through invoke
        assert(Account::fee() == 5);             // static member: no object
        assert(geo::area(3, 4) == 12);
        std::cout << "step 1  functions & member calls ..... ok\n";
    }
#else
    std::cout << "step 1  functions & member calls ..... TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        assert(apply_binop(add, 2, 3) == 5);
        assert(apply_binop(&mul, 4, 5) == 20);   // & is optional: decay
        auto t = make_dispatch();
        assert(t.size() == 3);
        assert(run(t, "sub", 10, 3) == 7);
        assert(run(t, "mul", 6, 7) == 42);
        std::cout << "step 2  function pointers & dispatch . ok\n";
    }
#else
    std::cout << "step 2  function pointers & dispatch . TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        assert(apply_fref(add, 8, 9) == 17);
        assert(apply_fref(mul, 6, 7) == 42);
        assert(bind_and_call(20, 5) == 15);      // reference bound to sub
        std::cout << "step 3  references to functions ...... ok\n";
    }
#else
    std::cout << "step 3  references to functions ...... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        Scale s{3};
        assert(s(14) == 42);                     // stored factor used
        assert(apply_scale(5, 8) == 40);
        assert(std::invoke(Scale{6}, 7) == 42);  // functor through invoke
        Counter c;
        assert(c() == 1);
        assert(c() == 2);
        assert(c() == 3);                        // state advances
        std::cout << "step 4  functors ..................... ok\n";
    }
#else
    std::cout << "step 4  functors ..................... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        assert(compose_twice([](int x) { return x * x; }, 3) == 81);   // (3^2)^2
        // compose_twice accepts ANY callable — here a plain function
        auto inc = [](int x) { return x + 1; };
        assert(compose_twice(inc, 40) == 42);
        auto fp = as_function_pointer();
        assert(fp(41) == 42);                    // captureless -> fptr
        assert(double_it(21) == 42);             // generic lambda on int
        assert(double_it(std::string("ab")) == "abab");   // ...and on string
        std::cout << "step 5  lambdas ...................... ok\n";
    }
#else
    std::cout << "step 5  lambdas ...................... TODO (flip STEP5_READY)\n";
#endif

    // ---- step 6 ----------------------------------------------------------
#if STEP6_READY
    {
        assert(reassignable(2, 2) == 4);         // add then reassigned to mul
        assert(call_through_ref([](int x) { return x * 10; }, 5) == 50);
        int (*fp)(int) = [](int x) { return x + 100; };
        assert(call_through_ref(fp, 5) == 105);  // fref over a function pointer

        // FIXME(A): the captured base must survive the return. Another call is
        // interposed first, so a by-reference capture reliably reads clobbered
        // stack and fails; a by-value capture returns 42.
        auto incr = make_incrementer();
        volatile int stack_noise[32];
        for (int i = 0; i < 32; ++i) stack_noise[i] = i;
        (void)stack_noise;
        assert(incr(41) == 42);

        // FIXME(B): state must persist across calls -> 1,2,3,4
        assert((count_calls(4) == std::vector<int>{1, 2, 3, 4}));
        std::cout << "step 6  std::function & function_ref . ok\n";
    }
#else
    std::cout << "step 6  std::function & function_ref . TODO (flip STEP6_READY)\n";
#endif

    // ---- step 7 (enabled with step 6) ------------------------------------
#if STEP6_READY
    {
        // FIXME(C): precedence bug -> must equal 9, not 5
        assert(bad_square(1, 2) == 9);
        // FIXME(D): side effect once, result correct
        assert(squared_ticks() == 25);
        assert(g_ticks == 1);                    // tick() called exactly once
        // MAX_MAC is left intact as a cautionary exhibit; sq_fn is the fix
        std::cout << "step 7  macros vs real callables ..... ok\n";
    }
#endif

    // ---- step 8 ----------------------------------------------------------
#if STEP8_READY
    {
        assert(Handler{}(41) == 42);             // object called with NO operator()
        Handler h;
        assert(call_object(h, 5) == 6);
        assert(Dual{}(9) == 1);                  // 1 arg  -> selects the (int) surrogate
        assert(Dual{}(9, 9) == 2);               // 2 args -> selects the (int,int) surrogate
        std::cout << "step 8  surrogate call functions ..... ok\n";
    }
#else
    std::cout << "step 8  surrogate call functions ..... TODO (flip STEP8_READY)\n";
#endif

    // ---- step 9 ----------------------------------------------------------
#if STEP9_READY
    {
        assert(bind_minus10(25) == 15);          // bind: fix the 2nd argument
        assert(bind_flip(3, 10) == 7);           // bind: reorder arguments (10 - 3)
        Counter c;
        assert(count_via_ref(c) == 2);           // reference_wrapper shares state
        assert(c.n == 2);
        std::cout << "step 9  adaptors: bind/ref ........... ok\n";
    }
#else
    std::cout << "step 9  adaptors: bind/ref ........... TODO (flip STEP9_READY)\n";
#endif

    // ---- step 10 ---------------------------------------------------------
#if STEP10_READY
    {
        assert(fold_mul(std::vector<int>{1, 2, 3, 4}) == 24);   // std::multiplies as the op
        std::set<std::string, std::less<>> s{"apple", "pear"};
        assert(set_contains(s, "apple") == true);   // transparent: lookup by const char*
        assert(set_contains(s, "kiwi") == false);
        static_assert(std::is_same_v<std::invoke_result_t<BinOp, int, int>, int>);
        assert((accepts<BinOp, int, int>()) == true);           // is_invocable: yes
        assert((accepts<BinOp, int>()) == false);               // wrong arity: no
        assert((accepts<decltype(&Account::deposit), Account&, int>()) == true);  // PMF is invocable
        std::cout << "step 10 std function objects & traits  ok\n";
    }
#else
    std::cout << "step 10 std function objects & traits  TODO (flip STEP10_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
