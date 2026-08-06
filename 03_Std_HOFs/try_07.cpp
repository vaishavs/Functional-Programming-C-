// ===========================================================================
// Type deduction in std::ranges and views — TODO / DEBUG exercise
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
//   Build : g++ -std=c++23 -Wall -Wextra -o drex try_07.cpp
//   Run   : ./drex
// ===========================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace rv = std::views;

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

// GIVEN — counts how many times the predicate has been evaluated.
inline int g_pred_calls = 0;
inline bool is_even(int x) { g_pred_calls++; return x % 2 == 0; }

// ###########################################################################
// SECTION A — DEBUG
// ###########################################################################

// ---------------------------------------------------------------------------
// FIXME(1): double_evens must double every even element of the caller's vector
//   in place, but the caller's vector comes back unchanged. The test asserts
//   that the doubling is visible in the caller.
//   HINT: the loop variable is deduced from the range's reference type, and the
//   form used here drops that reference.
// ---------------------------------------------------------------------------
inline void double_evens(std::vector<int>& v) {
    for (auto x : v | rv::filter([](int y){ return y % 2 == 0; })) {   // FIXME(1)
        x *= 2;
    }
}

// ---------------------------------------------------------------------------
// FIXME(2): snapshot_evens must capture the even elements as they are at the
//   moment of the call, so that later edits to the source do not change the
//   result. The test edits the source afterwards and asserts the result held
//   still. Note that the return type must stay `auto`.
//   HINT: what is returned currently refers to the source rather than holding
//   anything of its own.
// ---------------------------------------------------------------------------
inline auto snapshot_evens(std::vector<int>& v) {
    return v | rv::filter([](int y){ return y % 2 == 0; });            // FIXME(2)
}

// ---------------------------------------------------------------------------
// FIXME(3): append_bang must append an exclamation mark to the second member of
//   every pair in the caller's vector, but the caller's data is untouched. The
//   test asserts that the change is visible.
//   HINT: the elements here are pairs that hold values, not tuples that hold
//   references, so the binding form decides whether the write survives.
// ---------------------------------------------------------------------------
inline void append_bang(std::vector<std::pair<int, std::string>>& v) {
    for (auto [n, s] : v) {                                            // FIXME(3)
        (void)n;
        s += "!";
    }
}

// ---------------------------------------------------------------------------
// FIXME(4): count_evens_thrice must report the same count on each of three
//   passes while evaluating the predicate no more than once per element in
//   total. As shipped it evaluates the predicate 18 times over six elements,
//   and the test asserts a total of at most 6.
//   HINT: what `auto` stores here is a lazy view rather than a result, so every
//   traversal recomputes. Note that merely moving the pipeline out of the loop
//   is not enough, because each distance call still walks the whole range; the
//   count itself has to be obtained once and then reused.
// ---------------------------------------------------------------------------
inline int count_evens_thrice(const std::vector<int>& v) {
    int last = 0;
    for (int pass = 0; pass < 3; ++pass) {
        auto evens = v | rv::filter(is_even);                          // FIXME(4)
        last = static_cast<int>(std::ranges::distance(evens));
    }
    return last;
}

// ---------------------------------------------------------------------------
// FIXME(5): pair_up must combine two vectors elementwise and must refuse to
//   proceed when their lengths differ, because silently dropping elements is
//   not acceptable here. It currently returns a short result instead. The test
//   passes mismatched vectors and asserts that the mismatch is reported by
//   returning an empty vector.
//   HINT: the view used here stops at the shorter input without saying so, so
//   the lengths have to be compared before the view is built.
// ---------------------------------------------------------------------------
inline std::vector<int> pair_up(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> out;                                              // FIXME(5)
    for (auto [x, y] : rv::zip(a, b)) out.push_back(x + y);
    return out;
}

// ###########################################################################
// SECTION B — TODO
// ###########################################################################

// ---------------------------------------------------------------------------
// TODO(1): evens_doubled(v) must return a lazy pipeline over the caller's
//   vector that keeps the even elements and doubles them. The result is
//   iterated by the caller, and the caller's vector outlives it.
//   HINT: the type cannot be written out, so the return type must be deduced;
//   compose rv::filter with rv::transform and return the pipeline itself.
// ---------------------------------------------------------------------------
inline std::vector<int> evens_doubled(std::vector<int>& /*v*/) {
    // TODO(1)
    return {};
}

// ---------------------------------------------------------------------------
// TODO(2): element_of(r, i) must hand back the i-th element with its own value
//   category intact, so that a caller can assign through the call when the
//   underlying range is writable.
//   HINT: a plain `auto` return type would strip the reference; the parameter
//   should be a forwarding reference, and the index type is available as
//   std::ranges::range_difference_t<R>.
// ---------------------------------------------------------------------------
template <std::ranges::random_access_range R>
inline auto element_of(R&& r, std::ranges::range_difference_t<R> i) {
    // TODO(2)
    return *(std::ranges::begin(r) + i);
}

// ---------------------------------------------------------------------------
// TODO(3): describe(r) must accept any input range and return the name of its
//   value type as a string, using the library's alias rather than assuming int.
//   HINT: constrain the parameter with std::ranges::input_range auto&&, and ask
//   for std::ranges::range_value_t of decltype(r).
// ---------------------------------------------------------------------------
inline std::string describe(auto&& /*r*/) {
    // TODO(3)
    return "";
}

// ---------------------------------------------------------------------------
// TODO(4): value_type_of<R> must name the element type of R without using
//   std::ranges::range_value_t, so that value_type_of<std::vector<int>> is int
//   and value_type_of over a transform view producing doubles is double.
//   HINT: apply decltype to a dereferenced begin iterator obtained from
//   std::declval<R&>(), then strip the reference and cv-qualifiers.
// ---------------------------------------------------------------------------
template <class R>
using value_type_of = R;                                               // TODO(4)

// ---------------------------------------------------------------------------
// TODO(5): materialise(r) must copy any input range into a vector whose element
//   type is the range's own value type, so that materialising a range of
//   strings yields a vector of strings.
//   HINT: name the result type with std::ranges::range_value_t, then fill it
//   with std::ranges::copy and std::back_inserter. Note that std::ranges::to is
//   not available in this library.
// ---------------------------------------------------------------------------
inline std::vector<int> materialise(auto&& /*r*/) {
    // TODO(5)
    return {};
}

// ===========================================================================
// Tests — do not modify. Each assertion names the task it checks.
// ===========================================================================
int main() {
    std::cout << std::boolalpha;

#if DBG1_READY
    {
        std::vector<int> v{1, 2, 3, 4};
        double_evens(v);
        assert((v == std::vector<int>{1, 4, 3, 8}));   // FIXME(1): the write must reach the caller
        std::cout << "debug 1  writing through a filter view  ok\n";
    }
#else
    std::cout << "debug 1  writing through a filter view  TODO (flip DBG1_READY)\n";
#endif

#if DBG2_READY
    {
        std::vector<int> v{1, 2, 3, 4};
        auto snap = snapshot_evens(v);
        v[1] = 7;                                       // the source changes after the call
        std::vector<int> seen;
        for (auto&& x : snap) seen.push_back(x);
        assert((seen == std::vector<int>{2, 4}));       // FIXME(2): the result must not follow v
        std::cout << "debug 2  taking a real snapshot ....... ok\n";
    }
#else
    std::cout << "debug 2  taking a real snapshot ....... TODO (flip DBG2_READY)\n";
#endif

#if DBG3_READY
    {
        std::vector<std::pair<int, std::string>> v{{1, "a"}, {2, "b"}};
        append_bang(v);
        assert(v[0].second == "a!");                    // FIXME(3): the write must reach the caller
        assert(v[1].second == "b!");
        std::cout << "debug 3  binding to pair elements ..... ok\n";
    }
#else
    std::cout << "debug 3  binding to pair elements ..... TODO (flip DBG3_READY)\n";
#endif

#if DBG4_READY
    {
        std::vector<int> v{1, 2, 3, 4, 5, 6};
        g_pred_calls = 0;
        assert(count_evens_thrice(v) == 3);
        assert(g_pred_calls <= 6);                      // FIXME(4): one pass over six elements
        std::cout << "debug 4  reusing one pipeline ......... ok\n";
    }
#else
    std::cout << "debug 4  reusing one pipeline ......... TODO (flip DBG4_READY)\n";
#endif

#if DBG5_READY
    {
        assert((pair_up({1, 2, 3}, {10, 20, 30}) == std::vector<int>{11, 22, 33}));
        assert(pair_up({1, 2, 3}, {10, 20}).empty());   // FIXME(5): a mismatch must be reported
        std::cout << "debug 5  refusing a length mismatch ... ok\n";
    }
#else
    std::cout << "debug 5  refusing a length mismatch ... TODO (flip DBG5_READY)\n";
#endif

#if TODO1_READY
    {
        std::vector<int> v{1, 2, 3, 4, 5, 6};
        std::vector<int> seen;
        for (auto&& x : evens_doubled(v)) seen.push_back(x);
        assert((seen == std::vector<int>{4, 8, 12}));
        static_assert(!std::is_same_v<decltype(evens_doubled(v)), std::vector<int>>,
                      "the result must be a lazy pipeline, not a materialised vector");
        std::cout << "todo  1  returning a pipeline ......... ok\n";
    }
#else
    std::cout << "todo  1  returning a pipeline ......... TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        std::vector<int> v{10, 20, 30};
        element_of(v, 0) = 99;
        assert(v[0] == 99);
        static_assert(std::is_reference_v<decltype(element_of(v, 0))>);
        auto pipe = v | rv::transform([](int x){ return x * 2; });
        static_assert(!std::is_reference_v<decltype(element_of(pipe, 0))>);
        std::cout << "todo  2  forwarding an element ........ ok\n";
    }
#else
    std::cout << "todo  2  forwarding an element ........ TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY
    {
        std::vector<int> v{1, 2, 3};
        assert(describe(v) == "int");
        assert(describe(v | rv::transform([](int x){ return x * 1.5; })) == "double");
        std::cout << "todo  3  naming the value type ........ ok\n";
    }
#else
    std::cout << "todo  3  naming the value type ........ TODO (flip TODO3_READY)\n";
#endif

#if TODO4_READY
    {
        static_assert(std::is_same_v<value_type_of<std::vector<int>>, int>);
        static_assert(std::is_same_v<value_type_of<std::vector<std::string>>, std::string>);
        std::vector<int> v{1};
        using Pipe = decltype(v | rv::transform([](int x){ return x * 1.5; }));
        static_assert(std::is_same_v<value_type_of<Pipe>, double>);
        std::cout << "todo  4  rebuilding range_value_t ..... ok\n";
    }
#else
    std::cout << "todo  4  rebuilding range_value_t ..... TODO (flip TODO4_READY)\n";
#endif

#if TODO5_READY
    {
        std::vector<int> v{1, 2, 3, 4};
        auto out = materialise(v | rv::filter([](int x){ return x % 2 == 0; }));
        assert((out == std::vector<int>{2, 4}));
        std::vector<std::string> words{"a", "b"};
        auto out2 = materialise(words | rv::transform([](const std::string& s){ return s + "!"; }));
        static_assert(std::is_same_v<decltype(out2), std::vector<std::string>>);
        assert((out2 == std::vector<std::string>{"a!", "b!"}));
        std::cout << "todo  5  materialising a pipeline ..... ok\n";
    }
#else
    std::cout << "todo  5  materialising a pipeline ..... TODO (flip TODO5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
