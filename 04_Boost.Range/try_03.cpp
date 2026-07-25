// ===========================================================================
// Boost.Range — algorithms and adaptors: TODO/FIXME exercise
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
//   Build : g++ -std=c++17 -Wall -Wextra -o range boost_range_todo.cpp
//   Run   : ./range
//   (Requires Boost headers only — no linked Boost libraries.)
//
// Orientation (Boost.Range predates std::ranges; the vocabulary differs)
//   * Algorithms live in namespace boost:  boost::sort(v), boost::accumulate(v, 0).
//     Each takes a whole range instead of an iterator pair.
//   * Adaptors live in namespace boost::adaptors and compose with operator| :
//       v | filtered(pred) | transformed(fn)
//     They are lazy views, evaluated on traversal — NOT containers.
//   * There is no owning_view. An adaptor over a temporary DANGLES. Bind the
//     container to a name first. (FIXME(B) is exactly this.)
//   * There are no projections. Where std::ranges takes a projection, Boost
//     takes only a comparator/predicate; reach into members via a lambda.
//   * Materialise with boost::push_back(container, range) — Boost.Range has
//     no ranges::to.
//
// Convention used by every test
//   * Nothing prints. Results are returned/compared with ==.
//   * The helper to_vec (given below) materialises any single-pass range.
// ===========================================================================

#include <boost/range/adaptor/adjacent_filtered.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/replaced.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptor/strided.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/uniqued.hpp>
#include <boost/range/algorithm/binary_search.hpp>
#include <boost/range/algorithm/count_if.hpp>
#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm_ext/is_sorted.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/value_type.hpp>

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace boost::adaptors;

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0

// Given: materialise any range into a vector of its element type.
template <class R>
std::vector<typename boost::range_value<R>::type> to_vec(const R& r) {
    std::vector<typename boost::range_value<R>::type> out;
    boost::push_back(out, r);
    return out;
}

struct Emp {
    std::string name;
    std::string dept;
    int salary;
};

// ===========================================================================
// STEP 1 — first pipelines: filtered, transformed, and accumulate
// ===========================================================================
//
// TODO(1a)  evens(v) -> std::vector<int>
//   The even elements, in order.
//   * v | filtered(pred), then materialise with to_vec.
//   * The predicate is an ordinary callable returning bool.
//
// TODO(1b)  squares(v) -> std::vector<int>
//   Each element squared. v | transformed(fn), materialised.
//
// TODO(1c)  evens_times_ten(v) -> std::vector<int>
//   Even elements, each multiplied by 10. Chain filtered then transformed.
//   Order matters for cost (FIXME(C) revisits this) — filter first here.
//
// TODO(1d)  total(v) -> int
//   Sum of all elements. boost::accumulate(range, init). The init's TYPE
//   fixes the accumulation type; pass 0 for int.
//
// TODO(1e)  sum_of_squares(v) -> int
//   Accumulate directly over an adaptor — no intermediate container:
//     boost::accumulate(v | transformed(...), 0)
// ---------------------------------------------------------------------------
std::vector<int> evens(const std::vector<int>& /*v*/) {
    // TODO(1a)
    return {};
}

std::vector<int> squares(const std::vector<int>& /*v*/) {
    // TODO(1b)
    return {};
}

std::vector<int> evens_times_ten(const std::vector<int>& /*v*/) {
    // TODO(1c)
    return {};
}

int total(const std::vector<int>& /*v*/) {
    // TODO(1d)
    return 0;
}

int sum_of_squares(const std::vector<int>& /*v*/) {
    // TODO(1e)
    return 0;
}

// ===========================================================================
// STEP 2 — algorithms: sort, count_if, max_element, binary_search
// ===========================================================================
//
// Boost algorithms mutate or query a whole range. No projection parameter
// exists — supply a comparator lambda where std::ranges would take &Member.
//
// TODO(2a)  sorted_copy(v) -> std::vector<int>
//   A SORTED copy, leaving the argument untouched.
//   * boost::sort mutates in place and returns void, so copy first, then
//     sort the copy, then return it.
//
// TODO(2b)  count_above(v, threshold) -> std::ptrdiff_t
//   How many elements are strictly greater than threshold. boost::count_if.
//
// TODO(2c)  largest(v) -> int
//   boost::max_element returns an ITERATOR into v. Dereference it.
//   (Assume v is non-empty.)
//
// TODO(2d)  present_in_sorted(sorted, value) -> bool
//   boost::binary_search on an already-sorted range.
//
// TODO(2e)  names_by_salary_desc(staff) -> std::vector<std::string>
//   Names, highest salary first.
//   * boost::sort(copy, comparator) where the comparator compares
//     a.salary > b.salary. Take staff BY VALUE so the caller's data is safe.
//   * Then project to names: staff | transformed(get name) + to_vec.
// ---------------------------------------------------------------------------
std::vector<int> sorted_copy(const std::vector<int>& /*v*/) {
    // TODO(2a)
    return {};
}

std::ptrdiff_t count_above(const std::vector<int>& /*v*/, int /*threshold*/) {
    // TODO(2b)
    return 0;
}

int largest(const std::vector<int>& /*v*/) {
    // TODO(2c)
    return 0;
}

bool present_in_sorted(const std::vector<int>& /*sorted*/, int /*value*/) {
    // TODO(2d)
    return false;
}

std::vector<std::string> names_by_salary_desc(std::vector<Emp> /*staff*/) {
    // TODO(2e)
    return {};
}

// ===========================================================================
// STEP 3 — the adaptor catalog: reversed, sliced, strided, uniqued, maps
// ===========================================================================
//
// TODO(3a)  backwards(v) -> std::vector<int>          v | reversed
//
// TODO(3b)  middle(v, from, to) -> std::vector<int>
//   The half-open slice [from, to). v | sliced(from, to).
//   NOTE: sliced requires 0 <= from <= to <= size(); it does NOT clamp.
//
// TODO(3c)  every_third(v) -> std::vector<int>        v | strided(3)
//   strided(n) always includes the first element, then every nth after.
//
// TODO(3d)  dedupe_consecutive(v) -> std::vector<int>
//   Collapse consecutive equal runs to one element each: {1,1,2,3,3}->{1,2,3}.
//   v | uniqued. (Non-adjacent duplicates are NOT removed — {1,2,1}->{1,2,1}.)
//
// TODO(3e)  dept_names(m) -> std::vector<std::string>   where m is a map
//   The keys of a std::map<std::string,int>. m | map_keys.
//   (m | map_values would give the mapped ints.)
// ---------------------------------------------------------------------------
std::vector<int> backwards(const std::vector<int>& /*v*/) {
    // TODO(3a)
    return {};
}

std::vector<int> middle(const std::vector<int>& /*v*/, std::size_t /*from*/,
                        std::size_t /*to*/) {
    // TODO(3b)
    return {};
}

std::vector<int> every_third(const std::vector<int>& /*v*/) {
    // TODO(3c)
    return {};
}

std::vector<int> dedupe_consecutive(const std::vector<int>& /*v*/) {
    // TODO(3d)
    return {};
}

std::vector<std::string> dept_names(const std::map<std::string, int>& /*m*/) {
    // TODO(3e)
    return {};
}

// ===========================================================================
// STEP 4 — four defects. Predict each verdict, then repair.
// ===========================================================================
//
// FIXME(A) — remove without erase.
//   drop_evens should return only the odd values. boost::remove_if, like the
//   std counterpart, does NOT resize: it shuffles survivors to the front and
//   returns an iterator to the new logical end. The tail is stale.
//   PREDICT the returned vector's size for {1,2,3,4,5,6} before running.
//   Repair: either erase the tail (boost::remove_if(v,...) then v.erase(it,
//   v.end())) or use the one-shot boost::remove_erase_if(v, pred), which does
//   both. The test wants {1,3,5}.
//
// FIXME(B) — an adaptor over a temporary DANGLES.
//   first_over_four builds a filtered adaptor over the temporary returned by
//   source(), stores it, and only later materialises it. Boost.Range has no
//   owning_view: the adaptor holds a REFERENCE to the temporary, which is
//   destroyed at the end of the full expression that created it. What remains
//   is a dangling reference — undefined behaviour when traversed.
//   PREDICT what could happen before running (it may crash, may print
//   garbage, or may appear to work — UB is not guaranteed to fail).
//   Repair: bind the container to a named local FIRST, then adapt it, so the
//   container outlives the adaptor. QUESTION(B): why does the identical
//   pattern SUCCEED in std::ranges? (Name the C++20 view that has no
//   Boost.Range equivalent.)
//
// FIXME(C) — the re-evaluation trap. Adaptors do not memoise.
//   count_expensive_evens squares each value and keeps the even squares. The
//   answer is right; the cost is not. For the 8 inputs in the test, square()
//   runs TWELVE times, because a filtered adaptor downstream of transformed
//   evaluates the transform once to test the predicate and again to yield.
//   PREDICT the call count before running.
//   Repair: a pure reordering. x*x is even exactly when x is even, so the
//   predicate can test the cheap INPUT — filter first, transform the four
//   survivors. Same answer, 4 calls. The test asserts square_calls == 4.
//
// FIXME(D) — sliced past the end.
//   take_window returns v | sliced(from, to). sliced has a hard precondition
//   0 <= from <= to <= size(); it does not clamp, and violating it is
//   undefined behaviour (unlike std::views::take, which clamps). take_window
//   is called with a `to` that may exceed the size.
//   PREDICT what sliced(0, 100) does on a 6-element vector.
//   Repair: clamp `to` (and `from`) to the range size before slicing. The
//   test passes an over-long window and expects the whole vector back.
// ---------------------------------------------------------------------------
std::vector<int> drop_evens(std::vector<int> v) {
    // FIXME(A): the return value of remove_if is the whole point.
    boost::remove_if(v, [](int x) { return x % 2 == 0; });
    return v;
}

inline std::vector<int> source() { return {4, 7, 2, 9, 6, 1, 8}; }

inline std::vector<int> first_over_four() {
    // FIXME(B): source() is a temporary; the adaptor outlives it.
    auto view = source() | filtered([](int x) { return x > 4; });
    std::vector<int> out;
    boost::push_back(out, view);
    return out;
}

inline int square_calls = 0;
inline int square(int x) {
    ++square_calls;
    return x * x;
}

inline int count_expensive_evens(const std::vector<int>& data) {
    // FIXME(C): correct answer, 3x the calls.
    int n = 0;
    for (int s : data | transformed(square)
                      | filtered([](int x) { return x % 2 == 0; }))
        (void)s, ++n;
    return n;
}

inline std::vector<int> take_window(const std::vector<int>& v, std::size_t from,
                                    std::size_t to) {
    // FIXME(D): sliced does not clamp; to may exceed v.size().
    return to_vec(v | sliced(from, to));
}

// ===========================================================================
// STEP 5 — a small pipeline: irange, adjacent_filtered, and a mini report
// ===========================================================================
//
// TODO(5a)  first_n_squares(n) -> std::vector<int>
//   The squares 1, 4, 9, ..., n*n. Build the source with boost::irange(1,
//   n+1) — a lazy integer range — then transform. irange(a, b) is half-open;
//   irange(a, b, step) strides.
//
// TODO(5b)  rising_edges(v) -> std::vector<int>
//   Keep the first element, then each element strictly greater than its
//   predecessor: {1,3,2,4,4,5} -> {1,3,4,5}.
//   * v | adjacent_filtered(pred) keeps element[0] always, then keeps
//     element[i] iff pred(element[i-1], element[i]) is true.
//   * QUESTION(5b): what predicate turns adjacent_filtered into a
//     consecutive-duplicate remover (the uniqued of step 3d)?
//
// TODO(5c)  passing_average(grades, cutoff) -> double
//   The average of only the grades at or above cutoff. Combine:
//     * boost::count_if for the denominator,
//     * boost::accumulate over grades | filtered(...) for the numerator.
//   Assume at least one grade passes. Integer sum divided as double.
// ---------------------------------------------------------------------------
std::vector<int> first_n_squares(int /*n*/) {
    // TODO(5a)
    return {};
}

std::vector<int> rising_edges(const std::vector<int>& /*v*/) {
    // TODO(5b)
    return {};
}

double passing_average(const std::vector<int>& /*grades*/, int /*cutoff*/) {
    // TODO(5c)
    return 0.0;
}

// ===========================================================================
// Tests — do not modify. Each assert names the mistake it catches.
// ===========================================================================
int main() {
    std::cout << std::boolalpha;

    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        std::vector<int> v{4, 7, 2, 9, 6, 1, 8};

        assert((evens(v) == std::vector<int>{4, 2, 6, 8}));
        assert((squares(v) == std::vector<int>{16, 49, 4, 81, 36, 1, 64}));
        // filter-then-transform preserves order and applies the scale
        assert((evens_times_ten(v) == std::vector<int>{40, 20, 60, 80}));
        assert(total(v) == 37);
        // accumulate directly over an adaptor: no temporary container
        assert(sum_of_squares(v) == 251);
        std::cout << "step 1  filtered/transformed/accumulate  ok\n";
    }
#else
    std::cout << "step 1  filtered/transformed/accumulate  TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        std::vector<int> v{4, 7, 2, 9, 6, 1, 8};

        assert((sorted_copy(v) == std::vector<int>{1, 2, 4, 6, 7, 8, 9}));
        assert((v == std::vector<int>{4, 7, 2, 9, 6, 1, 8}));  // argument untouched

        assert(count_above(v, 5) == 4);   // 7 9 6 8
        assert(largest(v) == 9);

        std::vector<int> s{1, 2, 4, 6, 7, 8, 9};
        assert(present_in_sorted(s, 8));
        assert(!present_in_sorted(s, 5));

        std::vector<Emp> staff{{"ada", "x", 90}, {"bob", "y", 72},
                               {"cy", "x", 95}, {"dot", "y", 88}};
        assert((names_by_salary_desc(staff)
                == std::vector<std::string>{"cy", "ada", "dot", "bob"}));
        // taken by value — the caller's vector must be unchanged
        assert(staff.front().name == "ada");
        std::cout << "step 2  sort/count/max/search ......... ok\n";
    }
#else
    std::cout << "step 2  sort/count/max/search ......... TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        std::vector<int> v{4, 7, 2, 9, 6, 1, 8};

        assert((backwards(v) == std::vector<int>{8, 1, 6, 9, 2, 7, 4}));
        assert((middle(v, 2, 5) == std::vector<int>{2, 9, 6}));
        assert((every_third(v) == std::vector<int>{4, 9, 8}));

        assert((dedupe_consecutive({1, 1, 2, 3, 3, 3, 4})
                == std::vector<int>{1, 2, 3, 4}));
        // non-adjacent duplicates survive — catches a full-set dedupe
        assert((dedupe_consecutive({1, 2, 1}) == std::vector<int>{1, 2, 1}));

        std::map<std::string, int> m{{"eng", 3}, {"ops", 5}, {"qa", 2}};
        assert((dept_names(m) == std::vector<std::string>{"eng", "ops", "qa"}));
        std::cout << "step 3  adaptor catalog .............. ok\n";
    }
#else
    std::cout << "step 3  adaptor catalog .............. TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        // FIXME(A): remove_if without erase resizes nothing
        assert((drop_evens({1, 2, 3, 4, 5, 6}) == std::vector<int>{1, 3, 5}));
        assert((drop_evens({2, 4}) == std::vector<int>{}));

        // FIXME(B): the adaptor must not outlive its container
        assert((first_over_four() == std::vector<int>{7, 9, 6, 8}));

        // FIXME(C): same answer, 4 calls not 12
        square_calls = 0;
        assert(count_expensive_evens({1, 2, 3, 4, 5, 6, 7, 8}) == 4);
        assert(square_calls == 4);

        // FIXME(D): an over-long window must clamp, not invoke UB
        std::vector<int> v{4, 7, 2, 9, 6, 1};
        assert((take_window(v, 0, 100) == v));
        assert((take_window(v, 2, 4) == std::vector<int>{2, 9}));
        std::cout << "step 4  four traps repaired .......... ok\n";
    }
#else
    std::cout << "step 4  four traps repaired .......... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        assert((first_n_squares(5) == std::vector<int>{1, 4, 9, 16, 25}));
        assert((first_n_squares(1) == std::vector<int>{1}));

        assert((rising_edges({1, 3, 2, 4, 4, 5}) == std::vector<int>{1, 3, 4, 5}));
        assert((rising_edges({5, 4, 3}) == std::vector<int>{5}));  // only the first

        // eng-style: pass cutoff 60 over {55,90,70,40,85} -> avg of {90,70,85}
        assert(passing_average({55, 90, 70, 40, 85}, 60) == (90 + 70 + 85) / 3.0);
        assert(passing_average({100}, 50) == 100.0);
        std::cout << "step 5  irange/adjacent/report ....... ok\n";
    }
#else
    std::cout << "step 5  irange/adjacent/report ....... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
