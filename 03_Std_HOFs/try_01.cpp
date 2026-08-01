// ===========================================================================
// STL algorithms + lambdas — DEBUG + TODO exercise (one file)
//
// Built around one small pipeline over a vector:
//        keep the odd numbers  ->  double them  ->  total them up
// Every classic way that pipeline goes wrong appears below as a FIXME, and
// SECTION B rebuilds the same pipeline with the modern tools as TODOs.
//
// Markers
//   FIXME(n)  — shipped code is wrong on purpose; repair it (SECTION A).
//   TODO(n)   — implement from scratch (SECTION B).
//   HINT      — a concrete nudge toward the fix / the implementation.
//
// Flip a step's <NAME>_READY to 1 to activate its test. The file ships
// compiling and running with every step off. A DEBUG test asserts the CORRECT
// result, so it FAILS (or crashes) until the FIXME is repaired.
//
// SECTION A is CUMULATIVE: DBG4 reuses the functions repaired in DBG1 and DBG2,
// so fix them in order. SECTION B steps are independent.
//
//   Build : g++ -std=c++20 -Wall -Wextra -o algo algorithm_pipeline_exercise.cpp
//   Run   : ./algo
//   (std::erase_if needs C++20; everything else is C++17.)
//
// Debugging aid: two of these bugs are undefined behaviour, which can appear to
// "work" in a plain build. Rebuild with sanitizers to see them named exactly:
//   g++ -std=c++20 -g -fsanitize=address,undefined algorithm_pipeline_exercise.cpp
// ===========================================================================

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

#define DBG1_READY  0
#define DBG2_READY  0
#define DBG3_READY  0
#define DBG4_READY  0
#define TODO1_READY 0
#define TODO2_READY 0
#define TODO3_READY 0
#define TODO4_READY 0
#define TODO5_READY 0

// ###########################################################################
// SECTION A — DEBUG (FIXME)
// ###########################################################################

// ===========================================================================
// DEBUG 1 — std::remove_if does NOT remove anything (two bugs)
// ===========================================================================
//
// Background: the standard algorithms only ever see ITERATORS, never the
// container, so no algorithm can change a container's SIZE. std::remove_if
// therefore only SHUFFLES the survivors to the front and returns an iterator to
// the new logical end. Everything from that iterator to end() is left in an
// unspecified state — the elements are still there, and size() is unchanged.
//
// Concretely, on {1, 2, 3, 4, 5} removing the evens leaves the container as
//   1 3 5 4 5          with the returned iterator pointing at index 3
//        ^ logical end   ^^^ stale leftovers, still counted by size()
// Deleting them is the CONTAINER's job, which is why the return value must be
// fed to erase — the "erase-remove idiom".
//
// FIXME(1): the return value of std::remove_if is discarded, so nothing is
//   erased and the stale tail survives.
//   HINT: capture what remove_if returns and hand it to erase, together with
//   the real end:  v.erase(std::remove_if(...), v.end());
//
// FIXME(2): the predicate is inverted. remove_if removes every element for
//   which the predicate is TRUE, so a predicate that is true for ODD numbers
//   throws away exactly the values this function is supposed to keep.
//   HINT: the goal is to KEEP the odds, so the predicate must be true for the
//   values to DISCARD — the even ones.
// ---------------------------------------------------------------------------
std::vector<int> keep_odds(std::vector<int> v) {
    // FIXME(2): this predicate is true for ODD numbers -> the odds get removed
    std::remove_if(v.begin(), v.end(), [](int n) {
        return n % 2 != 0;
    });
    // FIXME(1): the returned "new end" iterator is thrown away -> no erase
    return v;
}

// ===========================================================================
// DEBUG 2 — std::transform needs a destination that already has room
// ===========================================================================
//
// std::transform(first, last, d_first, op) writes its results by assigning
// THROUGH d_first, exactly like a copy loop. It never allocates and never grows
// the destination. Handing it the begin() of an empty vector means writing into
// storage that does not exist — undefined behaviour (here: a segfault, since an
// empty vector's begin() is a null pointer).
//
// Note also that a 3-argument call, std::transform(first, last, op), does not
// compile at all: no such overload exists. The destination is not optional.
//
// FIXME(3): `out` is empty, so out.begin() addresses nothing.
//   HINT: two repairs work. Either give the destination room up front
//   (out.resize(in.size()) before transforming), or use an iterator that GROWS
//   the container on each write: std::back_inserter(out), from <iterator>.
// ---------------------------------------------------------------------------
std::vector<int> double_all(const std::vector<int>& in) {
    std::vector<int> out;                       // empty: capacity 0, no elements
    // FIXME(3): writes through out.begin() into an empty vector
    std::transform(in.begin(), in.end(), out.begin(), [](int n) {
        return n * 2;
    });
    return out;
}

// ===========================================================================
// DEBUG 3 — std::accumulate's init argument fixes the ACCUMULATOR TYPE
// ===========================================================================
//
// std::accumulate deduces its internal accumulator from the THIRD argument, not
// from the element type. With an int init over double elements, every partial
// sum is squeezed back into an int, truncating at each step:
//   0  + 1.5  = 1.5  -> 1
//   1  + 2.25 = 3.25 -> 3
//   3  + 3.75 = 6.75 -> 6      final answer 6, not 7.5
// The return type follows the same deduction, so the caller's `double` variable
// receives an already-truncated value — the assignment cannot rescue it.
//
// FIXME(4): the init is the integer literal 0, so the sum truncates.
//   HINT: make the init a double literal so the accumulator is a double.
//   (The same rule applies in reverse: 0.0 over an int vector accumulates in
//   double, which is harmless but not always what is wanted.)
// ---------------------------------------------------------------------------
double sum_prices(const std::vector<double>& prices) {
    // FIXME(4): int init -> int accumulator -> truncated partial sums
    return std::accumulate(prices.begin(), prices.end(), 0);
}

// ===========================================================================
// DEBUG 4 — iterators from two DIFFERENT temporaries
// ===========================================================================
//
// A function returning by value produces a TEMPORARY that lives only until the
// end of the full expression. Calling the function twice produces two unrelated
// temporaries, so begin() from one and end() from the other do not delimit a
// range at all: the pair points into two different objects, both already dead
// by the time accumulate runs.
//
// This one is easy to write by accident precisely because it reads like a
// pipeline, and it can appear to work before it corrupts something.
//
// FIXME(5): begin() and end() come from two separate calls -> two temporaries.
//   HINT: evaluate the pipeline ONCE into a named local, then take both
//   iterators from that one object:
//     const std::vector<int> stage = double_all(keep_odds(nums));
//   (Requires DBG1 and DBG2 to be repaired first — this step reuses them.)
// ---------------------------------------------------------------------------
double pipeline_total(const std::vector<int>& nums) {
    // FIXME(5): two calls -> two temporaries -> begin()/end() do not match
    return std::accumulate(double_all(keep_odds(nums)).begin(),
                           double_all(keep_odds(nums)).end(), 0.0);
}

// ###########################################################################
// SECTION B — TODO
// ###########################################################################

// ===========================================================================
// TODO 1 — std::erase_if: the modern replacement for erase-remove   (C++20)
// ===========================================================================
//
// The erase-remove idiom repaired in DBG1 is a two-part dance because an
// algorithm cannot resize a container. C++20 adds a free function that does
// both halves at once and takes the CONTAINER, not an iterator pair.
//
// TODO(T1): keep_odds_modern(v) -> std::vector<int>
//   Erase the even numbers from v with std::erase_if, then return v.
//   HINT: one line, no iterators, no return value to capture:
//     std::erase_if(v, /* predicate true for the values to DISCARD */);
//   The predicate polarity is the same as remove_if: true means "erase this".
// ---------------------------------------------------------------------------
std::vector<int> keep_odds_modern(std::vector<int> /*v*/) {
    // TODO(T1)
    return {};
}

// ===========================================================================
// TODO 2 — std::copy_if: filter WITHOUT modifying the source
// ===========================================================================
//
// remove_if and erase_if both mutate their container. When the original must be
// left intact, copy the survivors into a fresh container instead. Note the
// predicate polarity flips: copy_if keeps what is TRUE, while remove_if/erase_if
// discard what is true.
//
// TODO(T2): odds_copy(in) -> std::vector<int>
//   Return a new vector holding only the odd numbers of `in`, leaving `in`
//   untouched (the test checks that too).
//   HINT: declare an empty `out`, then grow it as results arrive:
//     std::copy_if(in.begin(), in.end(), std::back_inserter(out), pred);
//   Here `pred` must be true for the values to KEEP.
// ---------------------------------------------------------------------------
std::vector<int> odds_copy(const std::vector<int>& /*in*/) {
    // TODO(T2)
    return {};
}

// ===========================================================================
// TODO 3 — std::transform into a fresh container
// ===========================================================================
//
// The same shape as TODO 2, but mapping values rather than filtering them —
// and the repair from DBG2 applied deliberately from the start.
//
// TODO(T3): doubled_copy(in) -> std::vector<int>
//   Return a new vector with every element of `in` doubled.
//   HINT: std::transform(in.begin(), in.end(), std::back_inserter(out), op);
//   back_inserter turns each write into a push_back, so `out` may start empty.
// ---------------------------------------------------------------------------
std::vector<int> doubled_copy(const std::vector<int>& /*in*/) {
    // TODO(T3)
    return {};
}

// ===========================================================================
// TODO 4 — std::accumulate with an explicit init type AND a custom operation
// ===========================================================================
//
// The four-argument form replaces `+` with any binary operation. Its first
// parameter is the running accumulator (whose type comes from the init, per
// DBG3) and its second is the current element — the two are NOT interchangeable
// and often differ in type, as they do here.
//
// TODO(T4): sum_halves(v) -> double
//   Return the sum of half of each element: {2, 6, 10} -> 1 + 3 + 5 == 9.0.
//   HINT: init with 0.0 so the accumulator is a double, then supply a lambda
//   taking (double acc, int n) and returning acc + n * 0.5. Halving with
//   n / 2 would divide two ints and truncate, so prefer n * 0.5.
// ---------------------------------------------------------------------------
double sum_halves(const std::vector<int>& /*v*/) {
    // TODO(T4)
    return 0.0;
}

// ===========================================================================
// TODO 5 — std::transform_reduce: map and fold in a single pass   (C++17)
// ===========================================================================
//
// TODO 3 followed by a sum walks the data twice and allocates an intermediate
// vector. transform_reduce fuses the two: it applies a unary op to each element
// and folds the results with a binary op, allocating nothing.
//
// TODO(T5): total_doubled(v) -> int
//   Return the sum of every element doubled: {1, 3, 5} -> 2 + 6 + 10 == 18.
//   HINT: the argument order is (first, last, init, binary_op, unary_op):
//     std::transform_reduce(v.begin(), v.end(), 0, std::plus<>{},
//                           /* unary op doubling one element */);
//   std::plus<> comes from <functional>; the init 0 fixes the accumulator to int.
// ---------------------------------------------------------------------------
int total_doubled(const std::vector<int>& /*v*/) {
    // TODO(T5)
    return 0;
}

// ===========================================================================
// Tests — do not modify. Each assert names the mistake or task it checks.
// ===========================================================================
int main() {
    const std::vector<int> nums{1, 2, 3, 4, 5};

    // -------- SECTION A: DEBUG --------
#if DBG1_READY
    {
        // FIXME(1) erase-remove, FIXME(2) predicate polarity
        assert((keep_odds(nums) == std::vector<int>{1, 3, 5}));
        assert(keep_odds(nums).size() == 3);        // size really shrank
        std::cout << "debug 1  remove_if: erase + predicate . ok\n";
    }
#else
    std::cout << "debug 1  remove_if: erase + predicate . TODO (flip DBG1_READY)\n";
#endif

#if DBG2_READY
    {
        // FIXME(3): the destination must have room (or grow)
        assert((double_all({1, 3, 5}) == std::vector<int>{2, 6, 10}));
        std::cout << "debug 2  transform: destination ....... ok\n";
    }
#else
    std::cout << "debug 2  transform: destination ....... TODO (flip DBG2_READY)\n";
#endif

#if DBG3_READY
    {
        // FIXME(4): int init truncates every partial sum
        assert(sum_prices({1.5, 2.25, 3.75}) == 7.5);
        std::cout << "debug 3  accumulate: init type ........ ok\n";
    }
#else
    std::cout << "debug 3  accumulate: init type ........ TODO (flip DBG3_READY)\n";
#endif

#if DBG4_READY
    {
        // FIXME(5): one named result, not two temporaries
        // (needs DBG1 and DBG2 already repaired)
        assert(pipeline_total(nums) == 18.0);       // (1+3+5) doubled = 2+6+10
        std::cout << "debug 4  iterators: one temporary ..... ok\n";
    }
#else
    std::cout << "debug 4  iterators: one temporary ..... TODO (flip DBG4_READY)\n";
#endif

    // -------- SECTION B: TODO --------
#if TODO1_READY
    {
        assert((keep_odds_modern(nums) == std::vector<int>{1, 3, 5}));
        std::cout << "todo  1  std::erase_if ................ ok\n";
    }
#else
    std::cout << "todo  1  std::erase_if ................ TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        std::vector<int> src = nums;
        assert((odds_copy(src) == std::vector<int>{1, 3, 5}));
        assert(src == nums);                        // source left untouched
        std::cout << "todo  2  std::copy_if ................. ok\n";
    }
#else
    std::cout << "todo  2  std::copy_if ................. TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY
    {
        assert((doubled_copy({1, 3, 5}) == std::vector<int>{2, 6, 10}));
        assert(doubled_copy({}).empty());           // empty in, empty out
        std::cout << "todo  3  std::transform (new vector) .. ok\n";
    }
#else
    std::cout << "todo  3  std::transform (new vector) .. TODO (flip TODO3_READY)\n";
#endif

#if TODO4_READY
    {
        assert(sum_halves({2, 6, 10}) == 9.0);      // 1 + 3 + 5
        std::cout << "todo  4  std::accumulate (custom op) .. ok\n";
    }
#else
    std::cout << "todo  4  std::accumulate (custom op) .. TODO (flip TODO4_READY)\n";
#endif

#if TODO5_READY
    {
        assert(total_doubled({1, 3, 5}) == 18);
        std::cout << "todo  5  std::transform_reduce ........ ok\n";
    }
#else
    std::cout << "todo  5  std::transform_reduce ........ TODO (flip TODO5_READY)\n";
#endif

    // The original program's payoff line, printed once SECTION A is repaired.
#if DBG1_READY && DBG2_READY && DBG4_READY
    std::cout << "\nFinal Total: " << pipeline_total(nums) << "\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
