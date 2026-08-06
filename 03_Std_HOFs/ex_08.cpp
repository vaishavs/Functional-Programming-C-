// ===========================================================================
// Type deduction in std::ranges and views — a runnable demonstration
// ===========================================================================
// The five deduction groups are visited in order, and the final section shows
// the run-time counterpart that deduction cannot cover.
//
//   Build : g++ -std=c++23 -Wall -Wextra -o deduction_in_ranges_demo ex_08.cpp
//   Run   : ./deduction_in_ranges_demo
// ===========================================================================

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rv = std::views;

// A helper that prints any type as a readable string, so that a deduced type
// can be observed rather than guessed.
template <class T>
constexpr std::string_view type_name() {
    std::string_view p = __PRETTY_FUNCTION__;
    auto s = p.find("T = ") + 4;
    auto e = p.find(";", s);
    return p.substr(s, e - s);
}
#define SHOW(...) std::cout << "    " << #__VA_ARGS__ << "\n       -> " \
                            << type_name<__VA_ARGS__>() << "\n"

static void heading(const char* text) { std::cout << "\n== " << text << " ==\n"; }

static bool is_even(int x) { return x % 2 == 0; }
static int  doubler(int x) { return x * 2; }

// ===========================================================================
// Why deduction is mandatory rather than optional
// ===========================================================================
static void why_mandatory() {
    heading("Why deduction is mandatory rather than optional");

    std::vector<int> v{1, 2, 3, 4, 5, 6};

    // The type of even a two-stage pipeline is long, and it embeds the closure
    // types of the lambdas, which have no names that can be written in source.
    auto pipe = v | rv::filter([](int x){ return x % 2 == 0; })
                  | rv::transform([](int x){ return x * 10; });
    SHOW(decltype(pipe));

    // Two lambdas written identically still have different types, so a variable
    // declared with the type of one pipeline cannot be assigned another.
    //   decltype(pipe) other = v | rv::filter([](int x){ return x % 2 == 0; })
    //                            | rv::transform([](int x){ return x * 10; });
    // error: conversion from 'transform_view<...>' to non-scalar type 'transform_view<...>' requested

    // A view is small because it owns none of the elements it presents, which is
    // why passing one by value costs nothing.
    std::vector<int> big(1000, 1);
    auto big_pipe = big | rv::filter(is_even) | rv::transform(doubler);
    std::cout << "    the vector's elements occupy " << big.size() * sizeof(int) << " bytes\n";
    std::cout << "    sizeof(ref_view) is " << sizeof(rv::all(big))
              << " and sizeof(the whole pipeline) is " << sizeof(big_pipe) << "\n";
}

// ===========================================================================
// GROUP I — deduction from an initializer
// ===========================================================================
static void group_one() {
    heading("GROUP I: storing a pipeline, and binding to its elements");

    std::vector<int> v{1, 2, 3, 4, 5, 6};

    // The reference type of a range decides what a loop variable may bind to,
    // and the library exposes it under a readable name.
    SHOW(std::ranges::range_reference_t<std::vector<int>&>);
    SHOW(std::ranges::range_reference_t<decltype(v | rv::transform(doubler))>);

    // Because a vector yields real references, auto& binds and a write reaches
    // the vector itself.
    for (auto& x : v | rv::filter(is_even)) x += 100;
    std::cout << "    after writing through a filter view, v holds";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";

    // Because a transform view yields computed prvalues, auto& has nothing to
    // bind to, and the diagnostic names the value category rather than ranges.
    //   for (auto& x : v | rv::transform(doubler)) { (void)x; }
    // error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'

    // The forwarding form binds in both situations, which is why it is the
    // right default over any pipeline whose reference type is not known.
    int total = 0;
    for (auto&& x : v | rv::transform(doubler)) total += x;
    std::cout << "    iterating the transform view with auto&& summed to " << total << "\n";

    // A filter view caches the position of its first element, so its begin is
    // not const and the whole view cannot be iterated through a const reference.
    //   auto print = [](const auto& r){ for (auto x : r) std::cout << x; };
    //   print(v | rv::filter(is_even));
    // error: passing 'const std::ranges::filter_view<...>' as 'this' argument discards qualifiers
    auto print_by_value = [](std::ranges::input_range auto&& r) {
        for (auto&& x : r) std::cout << " " << x;
    };
    std::cout << "    the same view printed through a forwarding parameter:";
    print_by_value(v | rv::filter(is_even));
    std::cout << "\n";
}

// ===========================================================================
// GROUP I continued — structured bindings over ranges
// ===========================================================================
static void structured_bindings() {
    heading("GROUP I: structured bindings depend on the reference type");

    // The C++23 tuple-producing views yield tuples whose members are references.
    std::vector<std::string> names{"a", "b"};
    SHOW(std::ranges::range_reference_t<decltype(rv::enumerate(names))>);

    std::vector<int> a{1, 2}, b{10, 20};
    SHOW(std::ranges::range_reference_t<decltype(rv::zip(a, b))>);

    // A plain auto binding copies the tuple, but the copy holds the same
    // references, so a write through a binding still reaches the original range.
    for (auto [i, s] : rv::enumerate(names)) { (void)i; s += "!"; }
    std::cout << "    after an `auto` binding over enumerate, names holds "
              << names[0] << " and " << names[1] << "\n";

    for (auto&& [i, s] : rv::enumerate(names)) { (void)i; s += "?"; }
    std::cout << "    after an `auto&&` binding over the same view, names holds "
              << names[0] << " and " << names[1] << "\n";

    // The identical syntax behaves differently over an aggregate that holds
    // values rather than references, where the copy does discard the write.
    std::pair<int, std::string> p{1, "x"};
    auto [n, s] = p;
    (void)n;
    s += "!";
    std::cout << "    after an `auto` binding over a pair of values, p.second is still \""
              << p.second << "\"\n";
}

// ===========================================================================
// GROUP II — deduction from a return statement
// ===========================================================================
// A function returning a pipeline must use a deduced return type, because the
// type cannot be written out.
static auto evens_of(std::vector<int>& v) {
    return v | rv::filter(is_even);
}

// A helper that must hand back an element with its own value category intact
// uses the decltype rules rather than the template-deduction rules.
template <std::ranges::random_access_range R>
static decltype(auto) nth(R&& r, std::ranges::range_difference_t<R> i) {
    return std::ranges::begin(std::forward<R>(r))[i];
}

static void group_two() {
    heading("GROUP II: returning pipelines and forwarding elements");

    std::vector<int> v{1, 2, 3, 4, 5, 6};

    std::cout << "    the returned pipeline yields";
    for (int x : evens_of(v)) std::cout << " " << x;
    std::cout << "\n";

    // The deduced return type of that function contains a ref_view referring to
    // the caller's vector, which is safe here because the caller owns it. The
    // same function written over a local would return a view onto a destroyed
    // object, and nothing in the signature would show it.
    //   auto broken() { std::vector<int> local{1,2,3}; return local | rv::filter(is_even); }
    // reading the result reports stack-use-after-return under AddressSanitizer

    // Element access through decltype(auto) preserves whatever the underlying
    // range produced, so a writable range stays writable.
    nth(v, 0) = 99;
    std::cout << "    after writing through nth, v[0] is " << v[0] << "\n";
    SHOW(decltype(nth(v, 0)));
    auto tpipe = v | rv::transform(doubler);
    SHOW(decltype(nth(tpipe, 0)));
}

// ===========================================================================
// GROUP III — deduction from call arguments
// ===========================================================================
// A constrained parameter is an abbreviated function template whose auto is
// qualified by a concept, so an unsuitable argument is reported at the call.
static void report_value_type(std::ranges::input_range auto&& r) {
    std::cout << "    the value type of the argument is "
              << type_name<std::ranges::range_value_t<decltype(r)>>() << "\n";
}

static void group_three() {
    heading("GROUP III: constrained parameters deduce and check at once");

    std::vector<int> v{1, 2, 3};
    report_value_type(v);
    report_value_type(v | rv::transform([](int x){ return std::to_string(x); }));
    report_value_type(v | rv::transform([](int x){ return x * 1.5; }));

    // An unconstrained parameter would accept anything and fail deep inside the
    // body instead, which is the reason the concepts appear in the signature.
    //   report_value_type(42);
    // error: no matching function for call to 'report_value_type(int)'
    //        note: constraints not satisfied
}

// ===========================================================================
// GROUP IV — deduction of template arguments for a type
// ===========================================================================
static void group_four() {
    heading("GROUP IV: views::all_t decides between referring and owning");

    // Every adaptor routes its range argument through this alias, and the alias
    // is where the choice between referring and owning is made.
    SHOW(std::views::all_t<std::vector<int>&>);
    SHOW(std::views::all_t<std::vector<int>>);

    // An rvalue container is therefore moved into the pipeline and kept alive,
    // which makes the common warning about views over temporaries too broad.
    auto owning = std::vector<int>{1, 2, 3, 4} | rv::filter(is_even);
    std::cout << "    a pipeline built from an rvalue container yields";
    for (int x : owning) std::cout << " " << x;
    std::cout << "\n";

    // Class template argument deduction also selects the kind of a subrange,
    // and the sized kind is chosen because the iterators support subtraction.
    std::vector<int> v{1, 2, 3, 4};
    auto sr = std::ranges::subrange(v.begin(), v.end());
    SHOW(decltype(sr));
    std::cout << "    the deduced subrange reports size " << sr.size() << "\n";
}

// ===========================================================================
// GROUP V — type computation without deduction
// ===========================================================================
// The library's alias templates are built from decltype applied to expressions
// involving declval, and the definition can be reproduced in a single line.
template <class R>
using my_range_value_t =
    std::remove_cvref_t<decltype(*std::ranges::begin(std::declval<R&>()))>;

static void group_five() {
    heading("GROUP V: the alias templates are decltype in disguise");

    std::vector<int> v{1, 2, 3};
    auto pipe = v | rv::transform([](int x){ return x * 1.5; });

    std::cout << std::boolalpha;
    std::cout << "    the reimplementation matches for a vector : "
              << std::is_same_v<my_range_value_t<std::vector<int>>,
                                std::ranges::range_value_t<std::vector<int>>> << "\n";
    std::cout << "    the reimplementation matches for a view   : "
              << std::is_same_v<my_range_value_t<decltype(pipe)>,
                                std::ranges::range_value_t<decltype(pipe)>> << "\n";

    SHOW(std::ranges::range_value_t<decltype(pipe)>);
    SHOW(std::ranges::range_difference_t<decltype(pipe)>);
    SHOW(std::ranges::iterator_t<std::vector<int>>);

    // The last alias in the family reports an iterator only when the range is
    // safe to return one from, and reports a marker type otherwise.
    SHOW(std::ranges::borrowed_iterator_t<std::vector<int>&>);
    SHOW(std::ranges::borrowed_iterator_t<std::vector<int>>);

    // An algorithm over an rvalue range therefore deduces the marker type, and
    // the mistake is caught where the result is used rather than at run time.
    //   auto it = std::ranges::find(std::vector<int>{1,2,3}, 2);
    //   std::cout << *it;
    // error: no match for 'operator*' (operand type is 'std::ranges::dangling')
    auto data = std::vector<int>{1, 2, 3};
    auto it = std::ranges::find(data, 2);
    std::cout << "    naming the range restores an ordinary iterator, pointing at " << *it << "\n";
}

// ===========================================================================
// The run-time counterpart that deduction cannot provide
// ===========================================================================
// No type-erased view exists in the standard library, so where a pipeline must
// cross an interface that cannot be a template, the erasure is applied to the
// consumer instead and the pipeline keeps its concrete deduced type.
static void consume(const std::function<void(int)>& sink,
                    std::ranges::input_range auto&& r) {
    for (auto&& x : r) sink(x);
}

static void type_erasure() {
    heading("The run-time counterpart: erasing the consumer");

    std::vector<int> v{1, 2, 3, 4};
    int total = 0;
    consume([&total](int x){ total += x; }, v | rv::filter(is_even));
    std::cout << "    erasing the callback rather than the range produced " << total << "\n";

    // Materialising is the other way to cross such a boundary, and it is what
    // std::ranges::to performs from C++23 onwards. That facility is absent from
    // libstdc++ 13, so the portable spelling remains an explicit copy.
    std::vector<int> snapshot;
    std::ranges::copy(v | rv::filter(is_even), std::back_inserter(snapshot));
    std::cout << "    materialising the same pipeline produced a vector of size "
              << snapshot.size() << "\n";
}

int main() {
    std::cout << "Type deduction in std::ranges, demonstrated with observed results\n";
    why_mandatory();
    group_one();
    structured_bindings();
    group_two();
    group_three();
    group_four();
    group_five();
    type_erasure();
    std::cout << "\ndone\n";
    return 0;
}
