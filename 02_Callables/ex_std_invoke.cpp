// ===========================================================================
// std::invoke and the INVOKE family — a runnable demonstration
// ===========================================================================
// This program exercises all seven INVOKE cases, the traits and concepts that
// query callability, and the library facilities specified in terms of INVOKE.
// Each section prints what actually happened, so no claim has to be taken on
// trust. The case numbers used throughout are the standard's own numbering.
//
//   Build : g++ -std=c++23 -Wall -Wextra -pthread -o std_invoke_demo ex_std_invoke.cpp
//   Run   : ./std_invoke_demo
//
// Constructs that cannot compile are shown commented out, with the diagnostic
// that g++ 13.3 actually produces quoted beneath them.
// ===========================================================================

#include <concepts>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>

static void heading(const char* text) { std::cout << "\n== " << text << " ==\n"; }

// ---------------------------------------------------------------------------
// The subjects used throughout the demonstration.
// ---------------------------------------------------------------------------
struct Widget {
    int value = 10;
    int scale(int k) const   { return value * k; }   // a const member function
    int bump(int k)          { value += k; return value; }   // a mutating member function
    virtual int poly() const { return 1; }
    virtual ~Widget() = default;
};
struct Derived : Widget { int poly() const override { return 2; } };

static int add(int a, int b)   { return a + b; }
static constexpr int cadd(int a, int b) { return a + b; }   // usable in a constant expression
static int twice(int x) noexcept { return 2 * x; }           // promises not to throw
struct Times { int k; int operator()(int x) const { return k * x; } };

// A callable whose call operator is qualified so that it accepts only rvalues,
// which makes the value category visible to the traits.
struct OnlyRvalue { int operator()(int x) && { return x; } };
// A callable whose call operator is not const, so a const object cannot use it.
struct Mutating { int n = 0; int operator()() { return ++n; } };

static void worker(int& counter) { ++counter; }

// ===========================================================================
// The seven INVOKE cases
// ===========================================================================
static void seven_cases() {
    heading("The seven INVOKE cases");

    Widget w;
    Widget* pw = &w;
    auto up = std::make_unique<Widget>();

    // Group A covers a pointer to a member function, where the first argument
    // after the callable always identifies the object to call it on.
    std::cout << "    case 1, object            : " << std::invoke(&Widget::scale, w, 4) << "\n";
    std::cout << "    case 2, reference_wrapper : " << std::invoke(&Widget::scale, std::ref(w), 6) << "\n";
    std::cout << "    case 3, raw pointer       : " << std::invoke(&Widget::scale, pw, 5) << "\n";
    // Case three is a catch-all for anything that can be dereferenced, which is
    // why a smart pointer works without any call to .get() being needed.
    std::cout << "    case 3, unique_ptr        : " << std::invoke(&Widget::scale, up, 3) << "\n";

    // Group B covers a pointer to a data member, where exactly one argument is
    // permitted because the operation is a read rather than a call.
    std::cout << "    case 4, object            : " << std::invoke(&Widget::value, w) << "\n";
    std::cout << "    case 5, reference_wrapper : " << std::invoke(&Widget::value, std::ref(w)) << "\n";
    std::cout << "    case 6, raw pointer       : " << std::invoke(&Widget::value, pw) << "\n";
    std::cout << "    case 6, unique_ptr        : " << std::invoke(&Widget::value, up) << "\n";

    // Group C covers everything else, and no object is involved in it at all.
    std::cout << "    case 7, function          : " << std::invoke(add, 2, 3) << "\n";
    std::cout << "    case 7, functor           : " << std::invoke(Times{3}, 7) << "\n";
    std::cout << "    case 7, lambda            : " << std::invoke([](int x){ return x + 1; }, 41) << "\n";
}

// ===========================================================================
// Three properties of the member-pointer cases that are easy to overlook
// ===========================================================================
static void member_pointer_details() {
    heading("Further properties of the member-pointer cases");

    // A member pointer names the virtual member while the object decides which
    // override runs, so virtual dispatch continues to work through INVOKE.
    Derived d;
    Widget& base = d;
    std::cout << "    virtual dispatch through a member pointer yields "
              << std::invoke(&Widget::poly, base) << ", so the override ran\n";

    // A data-member case yields an lvalue, which means the result can be
    // assigned through rather than merely read.
    Widget w;
    std::invoke(&Widget::value, w) = 99;
    std::cout << "    after assigning through the data-member case, w.value is " << w.value << "\n";

    // A mutating member function requires a non-const object, and the change is
    // visible in the original because the object was passed by reference.
    std::cout << "    invoking a mutating member function yields "
              << std::invoke(&Widget::bump, w, 1) << "\n";

    // Passing the object by value would call the member on a temporary copy, so
    // the original would be left untouched. That is a mistake rather than a
    // feature, and it is silent, because the call itself succeeds.
    Widget original;
    std::invoke(&Widget::bump, Widget{original}, 5);
    std::cout << "    after invoking bump on a copy, the original still holds "
              << original.value << "\n";
}

// ===========================================================================
// std::invoke_r, and the propagation of constexpr and noexcept
// ===========================================================================
static void invoke_r_and_propagation() {
    heading("std::invoke_r, and the propagation of constexpr and noexcept");

    // The C++23 facility std::invoke_r performs the same operation and then
    // converts the result to the type named explicitly.
    double r = std::invoke_r<double>(add, 3, 2);
    std::cout << "    std::invoke_r<double> produced " << r << "\n";

    // Naming void discards the result deliberately, which is the clearest way
    // to call something for its effect while ignoring what it returns.
    std::invoke_r<void>(add, 1, 1);
    std::cout << "    std::invoke_r<void> compiled and discarded the result\n";

    // Both constexpr and noexcept are inherited from the callable rather than
    // conferred by std::invoke, so a constexpr callable is required here.
    static_assert(std::invoke(cadd, 2, 3) == 5);
    std::cout << "    a constexpr callable satisfied a static_assert through std::invoke\n";

    // Wrapping a function that is not constexpr fails, which shows that the
    // property is propagated rather than granted.
    //   static_assert(std::invoke(add, 2, 3) == 5);
    // error: call to non-'constexpr' function 'int add(int, int)'

    std::cout << std::boolalpha;
    std::cout << "    the call through a throwing function is noexcept: "
              << noexcept(std::invoke(add, 1, 2)) << "\n";
    std::cout << "    the call through a noexcept function is noexcept: "
              << noexcept(std::invoke(twice, 1)) << "\n";
}

// ===========================================================================
// The invocability traits
// ===========================================================================
static void traits() {
    heading("The invocability traits");

    std::cout << std::boolalpha;
    std::cout << "    is_invocable with the right arity        : "
              << std::is_invocable_v<decltype(add), int, int> << "\n";
    std::cout << "    is_invocable with the wrong arity        : "
              << std::is_invocable_v<decltype(add), int> << "\n";
    std::cout << "    is_invocable_r with a convertible result : "
              << std::is_invocable_r_v<double, decltype(add), int, int> << "\n";
    std::cout << "    is_invocable_r with an unrelated result  : "
              << std::is_invocable_r_v<std::string, decltype(add), int, int> << "\n";
    std::cout << "    invoke_result_t names int                : "
              << std::is_same_v<std::invoke_result_t<decltype(add), int, int>, int> << "\n";
    std::cout << "    is_nothrow_invocable for a noexcept call : "
              << std::is_nothrow_invocable_v<decltype(twice), int> << "\n";
    std::cout << "    is_nothrow_invocable for a throwing call : "
              << std::is_nothrow_invocable_v<decltype(add), int, int> << "\n";

    // The traits follow the same INVOKE rules, so both member-pointer forms are
    // reported as invocable provided the object is supplied first.
    std::cout << "    a member function with an object         : "
              << std::is_invocable_v<decltype(&Widget::scale), Widget&, int> << "\n";
    std::cout << "    a data member with an object             : "
              << std::is_invocable_v<decltype(&Widget::value), Widget&> << "\n";

    // The value category written into the trait matters, because each trait
    // asks about invoking an object of exactly the type named.
    std::cout << "    an rvalue-only callable named as lvalue  : "
              << std::is_invocable_v<OnlyRvalue&, int> << "\n";
    std::cout << "    the same callable named as an rvalue     : "
              << std::is_invocable_v<OnlyRvalue, int> << "\n";
    std::cout << "    a non-const call operator through const  : "
              << std::is_invocable_v<const Mutating&> << "\n";
    std::cout << "    the same callable named as non-const     : "
              << std::is_invocable_v<Mutating&> << "\n";

    // The same consideration applies to the object argument of a member
    // pointer, since a non-const member cannot run on a const object.
    struct M { int n; int bump() { return ++n; } };
    std::cout << "    a non-const member on a const object     : "
              << std::is_invocable_v<decltype(&M::bump), const M&> << "\n";
}

// ===========================================================================
// The invocability concepts
// ===========================================================================
template <std::invocable<int> F>
static int call_with_ten(F&& f) { return static_cast<int>(std::invoke(std::forward<F>(f), 10)); }

template <class P> requires std::predicate<P, int>
static bool holds(P p, int x) { return std::invoke(p, x); }

template <class R> requires std::strict_weak_order<R, int, int>
static bool ordered(R r, int a, int b) { return std::invoke(r, a, b); }

static void concepts_section() {
    heading("The invocability concepts");

    std::cout << std::boolalpha;
    std::cout << "    a template constrained by invocable      : " << call_with_ten([](int x){ return x * 2; }) << "\n";
    std::cout << "    a template constrained by predicate      : " << holds([](int x){ return x > 0; }, 5) << "\n";
    std::cout << "    a template constrained by strict order   : " << ordered(std::less<int>{}, 2, 3) << "\n";

    // The concept invocable places no requirement at all on the return type,
    // which is why a callable returning a string still satisfies it.
    std::cout << "    invocable accepts a string-returning call: "
              << std::invocable<decltype([](int){ return std::string("x"); }), int> << "\n";
    std::cout << "    constraining the result needs is_invocable_r or predicate\n";
}

// ===========================================================================
// Facilities specified in terms of INVOKE
// ===========================================================================
static void related_facilities() {
    heading("Facilities specified in terms of INVOKE");

    Widget w;

    // std::apply unpacks a tuple and hands the elements to INVOKE, so the
    // member-pointer convention that the object comes first applies here too.
    std::cout << "    std::apply on a function       : " << std::apply(add, std::make_tuple(6, 7)) << "\n";
    std::cout << "    std::apply on a member pointer : "
              << std::apply(&Widget::scale, std::make_tuple(std::ref(w), 2)) << "\n";

    // A reference wrapper around a callable is itself callable, forwarding to
    // the referent, which lets a stateful functor be shared rather than copied.
    Mutating counter;
    auto shared = std::ref(counter);
    shared(); shared(); shared();
    std::cout << "    a shared callable advanced to  : " << counter.n << "\n";

    // std::thread copies its arguments, so sharing must be requested with
    // std::ref, and the compiler rejects the call otherwise.
    int total = 0;
    std::thread t(worker, std::ref(total));
    t.join();
    std::cout << "    a thread sharing via std::ref  : " << total << "\n";
    //   std::thread bad(worker, total);
    // error: static assertion failed: std::thread arguments must be invocable
    //        after conversion to rvalues

    // std::bind_front fixes leading arguments, and binding a member function to
    // its object works precisely because the object is the first argument.
    auto scale_w = std::bind_front(&Widget::scale, std::ref(w));
    std::cout << "    std::bind_front on a member    : " << scale_w(4) << "\n";

    // A type-erased wrapper calls through INVOKE, so a member pointer fits a
    // signature whose first parameter is the object.
    std::function<int(const Widget&, int)> f = &Widget::scale;
    std::cout << "    a member pointer in std::function: " << f(w, 4) << "\n";
}

// ===========================================================================
// PITFALLS, each shown together with the solution that avoids it
// ===========================================================================
static int square(int x)      { return x * x; }
static double square(double x) { return x * x; }   // an overload set, deliberately

static void pitfalls() {
    heading("PITFALLS and their solutions");

    Widget w;

    // Pitfall one: the object must be supplied as the first argument, because
    // a member pointer carries no object of its own.
    //   std::invoke(&Widget::scale, 4);
    // error: no matching function for call to 'invoke(int (Widget::*)(int) const, int)'
    std::cout << "    supplying the object first gives " << std::invoke(&Widget::scale, w, 4) << "\n";

    // Pitfall two: the call operator binds more tightly than .*, so an
    // expression such as w.*p(3) parses as w.*(p(3)) and is rejected.
    //   int (Widget::*p)(int) const = &Widget::scale;
    //   w.*p(3);
    // error: must use '.*' or '->*' to call pointer-to-member function in 'p (...)'
    int (Widget::*p)(int) const = &Widget::scale;
    std::cout << "    the parenthesised member call gives " << (w.*p)(3)
              << ", and std::invoke gives " << std::invoke(p, w, 3) << "\n";

    // Pitfall three: a name that refers to several functions selects none of
    // them, so it cannot be deduced as a template argument.
    //   std::invoke(square, 3);
    // error: no matching function for call to 'invoke(<unresolved overloaded function type>, int)'
    //
    // Wrapping the name in a lambda defers the selection to the point of call,
    // which is the preferred solution because it survives later overloads.
    // The lambda also keeps the whole overload set available, so each call
    // selects the overload that matches the argument type it was given.
    auto any_square = [](auto x){ return square(x); };
    std::cout << "    an overload set wrapped in a lambda gives "
              << std::invoke(any_square, 3) << " for an int and "
              << std::invoke(any_square, 1.5) << " for a double\n";
    // An explicit cast also works, though it must be revisited whenever the
    // overload set changes.
    std::cout << "    the same call through an explicit cast gives "
              << std::invoke(static_cast<int(*)(int)>(square), 3) << "\n";

    // Pitfall four: a data-member case takes the object and nothing else,
    // because reading a member cannot consume an argument list.
    //   std::invoke(&Widget::value, w, 5);
    // error: no matching function for call to 'invoke(int Widget::*, Widget&, int)'
    std::cout << "    the data-member case with only an object gives "
              << std::invoke(&Widget::value, w) << "\n";

    // Pitfall five: a trait reports only whether a call would compile, and says
    // nothing about the result type, the exception behaviour, or any lifetime.
    std::cout << std::boolalpha;
    std::cout << "    is_invocable says the call compiles      : "
              << std::is_invocable_v<decltype(add), int, int> << "\n";
    std::cout << "    is_invocable_r additionally checks result: "
              << std::is_invocable_r_v<std::string, decltype(add), int, int> << "\n";

    // Pitfall six: writing the wrong value category into a trait produces a
    // constraint that passes in isolation and then fails at the call site, so
    // the type written should match the type the code will really hold.
    std::cout << "    a mutable functor named as const         : "
              << std::is_invocable_v<const Mutating&> << "\n";
    std::cout << "    the same functor named as non-const      : "
              << std::is_invocable_v<Mutating&> << "\n";

    // Pitfall seven: a pointer to a member may be null, and applying a null one
    // is undefined behaviour in the way that dereferencing a null pointer is.
    // The check below is the reason such a pointer is only useful as a sentinel.
    int (Widget::*maybe)(int) const = nullptr;
    std::cout << "    a null member pointer must be checked before use: "
              << (maybe == nullptr ? "it is null, so no call is made" : "safe to call") << "\n";

    // Pitfall eight: reaching for std::invoke where the callable form is known
    // adds noise without benefit, so a plain call remains the better choice.
    auto lambda = [](int x){ return x + 1; };
    std::cout << "    a plain call is clearer here and gives " << lambda(41) << "\n";
}

int main() {
    std::cout << "std::invoke and its family, demonstrated with observed results\n";
    seven_cases();
    member_pointer_details();
    invoke_r_and_propagation();
    traits();
    concepts_section();
    related_facilities();
    pitfalls();
    std::cout << "\ndone\n";
    return 0;
}
