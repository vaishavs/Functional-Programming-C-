// ===========================================================================
// STL algorithms + lambdas — TODO exercise
//
// Built around one small pipeline over a vector:
//        keep the odd numbers  ->  double them  ->  total them up
// Each step below implements one piece, and the last step composes them.
//
// Markers
//   TODO(n)   — implement from scratch.
//   HINT      — a concrete nudge toward the implementation.
//
//   (std::erase_if needs C++20; everything else is C++17.)
//
// Two of these steps have undefined behaviour as their classic failure mode, so
// a wrong answer can appear to "work" in a plain build. Rebuild with sanitizers
// to have such mistakes named exactly:
// ===========================================================================

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

#define TODO1_READY 0
#define TODO2_READY 0
#define TODO3_READY 0
#define TODO4_READY 0
#define TODO5_READY 0
#define TODO6_READY 0
#define TODO7_READY 0
#define TODO8_READY 0

// ===========================================================================
// TODO 1 — the erase-remove idiom
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
// Deleting them is the CONTAINER's job — hence "erase-remove".
//
// TODO(T1): keep_odds(v) -> std::vector<int>
//   Erase the even numbers from v so only the odds remain, then return v.
//   The test checks the contents AND that size() really shrank to 3.
//   HINT: feed what remove_if returns straight into erase, together with the
//   real end:
//     v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
//   Mind the predicate polarity: remove_if discards every element for which the
//   predicate is TRUE, so `pred` must be true for the values to THROW AWAY.
// ---------------------------------------------------------------------------
std::vector<int> keep_odds(std::vector<int> /*v*/) {
    // TODO(T1)
    return {};
}

// ===========================================================================
// TODO 2 — std::erase_if: the modern one-liner                      (C++20)
// ===========================================================================
//
// The idiom in TODO 1 is a two-part dance because an algorithm cannot resize a
// container. C++20 adds a free function that does both halves at once and takes
// the CONTAINER, not an iterator pair.
//
// TODO(T2): keep_odds_modern(v) -> std::vector<int>
//   Same result as TODO 1, in one line.
//   HINT: std::erase_if(v, pred); then return v. No iterators, and no return
//   value to capture. The predicate polarity matches remove_if: true means
//   "erase this one".
// ---------------------------------------------------------------------------
std::vector<int> keep_odds_modern(std::vector<int> /*v*/) {
    // TODO(T2)
    return {};
}

// ===========================================================================
// TODO 3 — std::copy_if: filter WITHOUT modifying the source
// ===========================================================================
//
// remove_if and erase_if both mutate their container. When the original must be
// left intact, copy the survivors into a fresh container instead. Note that the
// predicate polarity FLIPS: copy_if keeps what is true, while remove_if and
// erase_if discard what is true.
//
// TODO(T3): odds_copy(in) -> std::vector<int>
//   Return a new vector holding only the odd numbers of `in`, leaving `in`
//   untouched (the test checks that too).
//   HINT: declare an empty `out`, then let it grow as results arrive:
//     std::copy_if(in.begin(), in.end(), std::back_inserter(out), pred);
//   Here `pred` must be true for the values to KEEP.
// ---------------------------------------------------------------------------
std::vector<int> odds_copy(const std::vector<int>& /*in*/) {
    // TODO(T3)
    return {};
}

// ===========================================================================
// TODO 4 — std::transform needs a destination that has room
// ===========================================================================
//
// std::transform(first, last, d_first, op) writes its results by assigning
// THROUGH d_first, exactly like a copy loop. It never allocates and never grows
// the destination. Handing it the begin() of an empty vector means writing into
// storage that does not exist — undefined behaviour, and in practice a segfault,
// since an empty vector's begin() is a null pointer.
//
// Note also that a 3-argument call, std::transform(first, last, op), does not
// compile at all: no such overload exists. The destination is not optional.
//
// TODO(T4): doubled_copy(in) -> std::vector<int>
//   Return a new vector with every element of `in` doubled.
//   HINT: two approaches work. Either size the destination up front
//   (out.resize(in.size()) before transforming into out.begin()), or use an
//   iterator that GROWS the container on each write:
//     std::transform(in.begin(), in.end(), std::back_inserter(out), op);
//   back_inserter (from <iterator>) turns each write into a push_back, so `out`
//   may start empty.
// ---------------------------------------------------------------------------
std::vector<int> doubled_copy(const std::vector<int>& /*in*/) {
    // TODO(T4)
    return {};
}

// ===========================================================================
// TODO 5 — std::accumulate's init argument fixes the ACCUMULATOR TYPE
// ===========================================================================
//
// std::accumulate deduces its internal accumulator from the THIRD argument, not
// from the element type. An int init over double elements squeezes every
// partial sum back into an int, truncating at each step:
//   0  + 1.5  = 1.5  -> 1
//   1  + 2.25 = 3.25 -> 3
//   3  + 3.75 = 6.75 -> 6      final answer 6, not 7.5
// The return type follows the same deduction, so assigning the result to a
// double variable cannot rescue an already-truncated value.
//
// TODO(T5): sum_prices(prices) -> double
//   Return the exact total of `prices`. For {1.5, 2.25, 3.75} that is 7.5.
//   HINT: the whole task is choosing the init so the accumulator is a double —
//   write the literal as 0.0, not 0.
// ---------------------------------------------------------------------------
double sum_prices(const std::vector<double>& /*prices*/) {
    // TODO(T5)
    return 0.0;
}

// ===========================================================================
// TODO 6 — std::accumulate with a custom binary operation
// ===========================================================================
//
// The four-argument form replaces `+` with any binary operation. Its first
// parameter is the running accumulator (whose type comes from the init, per
// TODO 5) and its second is the current element — the two are NOT
// interchangeable and often differ in type, as they do here.
//
// TODO(T6): sum_halves(v) -> double
//   Return the sum of half of each element: {2, 6, 10} -> 1 + 3 + 5 == 9.0.
//   HINT: init with 0.0 so the accumulator is a double, then supply a lambda
//   taking (double acc, int n) and returning acc + n * 0.5. Halving with n / 2
//   would divide two ints and truncate, so prefer n * 0.5.
// ---------------------------------------------------------------------------
double sum_halves(const std::vector<int>& /*v*/) {
    // TODO(T6)
    return 0.0;
}

// ===========================================================================
// TODO 7 — std::transform_reduce: map and fold in a single pass     (C++17)
// ===========================================================================
//
// TODO 4 followed by a sum walks the data twice and allocates an intermediate
// vector. transform_reduce fuses the two: it applies a unary op to each element
// and folds the results with a binary op, allocating nothing.
//
// TODO(T7): total_doubled(v) -> int
//   Return the sum of every element doubled: {1, 3, 5} -> 2 + 6 + 10 == 18.
//   HINT: the argument order is (first, last, init, binary_op, unary_op):
//     std::transform_reduce(v.begin(), v.end(), 0, std::plus<>{},
//                           /* unary op doubling one element */);
//   std::plus<> comes from <functional>; the init 0 fixes the accumulator to int.
// ---------------------------------------------------------------------------
int total_doubled(const std::vector<int>& /*v*/) {
    // TODO(T7)
    return 0;
}

// ===========================================================================
// TODO 8 — compose the pipeline (and keep the temporaries alive)
// ===========================================================================
//
// A function returning by value produces a TEMPORARY that lives only until the
// end of the full expression. Calling such a function TWICE produces two
// unrelated temporaries, so begin() from one and end() from the other do not
// delimit a range at all: the pair points into two different objects, both
// already dead by the time the algorithm runs. Written as a pipeline it reads
// perfectly plausible, which is what makes it easy to do by accident:
//     std::accumulate(doubled_copy(keep_odds(nums)).begin(),      // temporary A
//                     doubled_copy(keep_odds(nums)).end(), 0.0);  // temporary B
//
// TODO(T8): pipeline_total(nums) -> double
//   Keep the odds, double them, and return the total as a double.
//   For {1, 2, 3, 4, 5} that is (1+3+5) doubled == 2 + 6 + 10 == 18.0.
//   HINT: evaluate the pipeline ONCE into a named local, then take both
//   iterators from that single object:
//     const std::vector<int> stage = doubled_copy(keep_odds(nums));
//     return std::accumulate(stage.begin(), stage.end(), 0.0);
//   (Reuses TODO 1 and TODO 4, so finish those first.)
// ---------------------------------------------------------------------------
double pipeline_total(const std::vector<int>& /*nums*/) {
    // TODO(T8)
    return 0.0;
}

// ===========================================================================
// Tests — do not modify. Each assert names the task it checks.
// ===========================================================================
int main() {
    const std::vector<int> nums{1, 2, 3, 4, 5};

#if TODO1_READY
    {
        assert((keep_odds(nums) == std::vector<int>{1, 3, 5}));
        assert(keep_odds(nums).size() == 3);        // the size really shrank
        std::cout << "todo 1  erase-remove idiom ............ ok\n";
    }
#else
    std::cout << "todo 1  erase-remove idiom ............ TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        assert((keep_odds_modern(nums) == std::vector<int>{1, 3, 5}));
        std::cout << "todo 2  std::erase_if ................. ok\n";
    }
#else
    std::cout << "todo 2  std::erase_if ................. TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY
    {
        std::vector<int> src = nums;
        assert((odds_copy(src) == std::vector<int>{1, 3, 5}));
        assert(src == nums);                        // source left untouched
        std::cout << "todo 3  std::copy_if .................. ok\n";
    }
#else
    std::cout << "todo 3  std::copy_if .................. TODO (flip TODO3_READY)\n";
#endif

#if TODO4_READY
    {
        assert((doubled_copy({1, 3, 5}) == std::vector<int>{2, 6, 10}));
        assert(doubled_copy({}).empty());           // empty in, empty out
        std::cout << "todo 4  std::transform (destination) .. ok\n";
    }
#else
    std::cout << "todo 4  std::transform (destination) .. TODO (flip TODO4_READY)\n";
#endif

#if TODO5_READY
    {
        assert(sum_prices({1.5, 2.25, 3.75}) == 7.5);
        std::cout << "todo 5  std::accumulate (init type) ... ok\n";
    }
#else
    std::cout << "todo 5  std::accumulate (init type) ... TODO (flip TODO5_READY)\n";
#endif

#if TODO6_READY
    {
        assert(sum_halves({2, 6, 10}) == 9.0);      // 1 + 3 + 5
        std::cout << "todo 6  std::accumulate (custom op) ... ok\n";
    }
#else
    std::cout << "todo 6  std::accumulate (custom op) ... TODO (flip TODO6_READY)\n";
#endif

#if TODO7_READY
    {
        assert(total_doubled({1, 3, 5}) == 18);
        std::cout << "todo 7  std::transform_reduce ......... ok\n";
    }
#else
    std::cout << "todo 7  std::transform_reduce ......... TODO (flip TODO7_READY)\n";
#endif

#if TODO8_READY
    {
        assert(pipeline_total(nums) == 18.0);       // (1+3+5) doubled = 2+6+10
        std::cout << "todo 8  compose the pipeline .......... ok\n";
    }
#else
    std::cout << "todo 8  compose the pipeline .......... TODO (flip TODO8_READY)\n";
#endif

    // The original program's payoff line, printed once the pipeline is built.
#if TODO8_READY
    std::cout << "\nFinal Total: " << pipeline_total(nums) << "\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
