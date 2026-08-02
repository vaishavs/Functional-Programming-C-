// =============================================================================
// Extending Boost.Range — TODO exercise
// =============================================================================
// Three things to build, in order:
//   PART 1 — plug a custom data structure into Boost.Range (TODO 1)
//   PART 2 — write two custom eager algorithms over any range (TODO 2, TODO 3)
//   PART 3 — write a custom lazy adaptor usable with `|`   (TODO 4, TODO 5)
//
// Markers
//   TODO(n)   — implement from scratch.
//   HINT      — concrete syntax for the implementation.
//   GIVEN     — provided scaffolding; read it, don't rewrite it.
//
// Flip a step's TODOn_READY to 1 to activate its test. The file ships compiling
// and running with every step off. Steps 2-5 fail with assertions until
// implemented; TODO 1a is a type declaration, so activating TODO 1 before
// writing it gives a COMPILE error naming range_mutable_iterator rather than a
// failed assertion. PART 3's tests also need PART 1 done.
//
//   Build : g++ -std=c++17 -Wall -Wextra -o brange boost_range_extension_exercise.cpp
//   Run   : ./brange
//   Needs Boost headers only (tested with Boost 1.83): apt install libboost-dev
// =============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/metafunctions.hpp>

#define TODO1_READY 0
#define TODO2_READY 0
#define TODO3_READY 0
#define TODO4_READY 0
#define TODO5_READY 0

// #############################################################################
// PART 1 — make a custom data structure usable as a Boost.Range
// #############################################################################

namespace demo {

// GIVEN — a fixed-capacity buffer with NO begin()/end() members, on purpose.
// Its elements live in `data[0] .. data[n-1]`, so its iterator type is `int*`
// (and `const int*` for a const object).
struct SmallVec {
    int data[8];
    std::size_t n;
};

// =============================================================================
// TODO 1 — hook SmallVec into Boost.Range (two halves; both are needed)
// =============================================================================
// TODO(1b): the four free functions below.
//   They must live in namespace demo so Boost finds them by ADL. Return the
//   first element / one-past-the-last element.
//   HINT: begin is `return v.data;`  end is `return v.data + v.n;`
//   The two stubs below both return v.data, which reports an empty range.
//
// TODO(1a): the two metafunction specializations, in namespace boost, just
//   below this namespace — see the commented skeleton there.
// =============================================================================
inline int* range_begin(SmallVec& v) {
    // TODO(1b)
    return v.data;
}
inline int* range_end(SmallVec& v) {
    // TODO(1b)
    return v.data;                    // stub: reports an empty range
}
inline const int* range_begin(const SmallVec& v) {
    // TODO(1b)
    return v.data;
}
inline const int* range_end(const SmallVec& v) {
    // TODO(1b)
    return v.data;                    // stub: reports an empty range
}

} // namespace demo

// TODO(1a): tell Boost.Range what SmallVec's iterator types are. Uncomment and
//   fill in the two typedefs.
//   HINT: mutable -> int*, const -> const int*
//
// namespace boost {
// template<> struct range_mutable_iterator<demo::SmallVec> { typedef /* ? */ type; };
// template<> struct range_const_iterator<demo::SmallVec>   { typedef /* ? */ type; };
// }

// #############################################################################
// PART 2 — custom eager algorithms
// #############################################################################

// =============================================================================
// TODO 2 — an algorithm returning a VALUE
// =============================================================================
// TODO(2): sum_of(rng) -> the sum of every element.
//   Must work for any single-pass range, including SmallVec once PART 1 is done.
//   HINT: the element type is `typename boost::range_value<SinglePassRange>::type`
//   (already spelled in the return type). Walk with:
//     typedef typename boost::range_iterator<const SinglePassRange>::type Iter;
//     for (Iter it = boost::begin(rng), last = boost::end(rng); it != last; ++it)
//         total += *it;
//   Use boost::begin / boost::end, not rng.begin() — SmallVec has no members.
// =============================================================================
template <class SinglePassRange>
typename boost::range_value<SinglePassRange>::type
sum_of(const SinglePassRange& /*rng*/) {
    // TODO(2)
    return 0;
}

// =============================================================================
// TODO 3 — an algorithm returning an ITERATOR
// =============================================================================
// TODO(3): find_max_element(rng) -> an iterator to the largest element, or
//   boost::end(rng) when the range is empty.
//   HINT: the return type is already spelled for you; use the same alias inside:
//     typedef typename boost::range_iterator<ForwardRange>::type Iter;
//     Iter it = boost::begin(rng), last = boost::end(rng);
//     if (it == last) return last;
//     Iter best = it;
//     for (++it; it != last; ++it) if (*best < *it) best = it;
//     return best;
//   Take the range by non-const reference (as declared) so the returned iterator
//   is mutable; compare with `<` only.
// =============================================================================
template <class ForwardRange>
typename boost::range_iterator<ForwardRange>::type
find_max_element(ForwardRange& rng) {
    // TODO(3)
    return boost::begin(rng);         // stub: always the first element
}

// #############################################################################
// PART 3 — a custom lazy adaptor:  rng | multiples_of(k)
// #############################################################################

namespace demo {
namespace adaptors {

// GIVEN — the holder carries the adaptor's argument until a range arrives.
struct multiples_holder {
    int k;
};

// =============================================================================
// TODO 4 — the predicate and the factory
// =============================================================================
// TODO(4a): is_multiple::operator() -> true when x is divisible by k.
//   HINT: `return k != 0 && x % k == 0;`  The k != 0 guard matters: `x % 0` is
//   undefined behaviour, and this predicate is copied into the adaptor where a
//   zero k would otherwise reach the modulo.
//
// TODO(4b): multiples_of(k) -> a multiples_holder carrying k.
//   HINT: one line — `return multiples_holder{k};`  The stub drops k.
//
// (A struct is used rather than a lambda because the adaptor stores and copies
// this predicate; the type must stay copy-assignable.)
// =============================================================================
struct is_multiple {
    int k;
    bool operator()(int /*x*/) const {
        // TODO(4a)
        return true;                  // stub: keeps everything
    }
};

inline multiples_holder multiples_of(int /*k*/) {
    // TODO(4b)
    return multiples_holder{1};       // stub: always k == 1, which keeps everything
}

// =============================================================================
// TODO 5 — the pipe operators
// =============================================================================
// TODO(5): both operator| overloads -> a filtered view of `r` keeping only the
//   multiples of the holder's k. Nothing is evaluated here; the work happens
//   when the result is iterated.
//   HINT: one line each —
//     return boost::adaptors::filter(r, is_multiple{h.k});
//   Two overloads are needed so the adaptor works on const and non-const ranges;
//   they differ only in the constness of Range in the signature. The stubs below
//   filter with k == 1, which keeps every element.
// =============================================================================
template <class Range>
inline boost::filtered_range<is_multiple, Range>
operator|(Range& r, multiples_holder /*h*/) {
    // TODO(5)
    return boost::adaptors::filter(r, is_multiple{1});
}

template <class Range>
inline boost::filtered_range<is_multiple, const Range>
operator|(const Range& r, multiples_holder /*h*/) {
    // TODO(5)
    return boost::adaptors::filter(r, is_multiple{1});
}

} // namespace adaptors
} // namespace demo

// #############################################################################
// GIVEN — test helpers
// #############################################################################

// Drains any Boost.Range into a vector<int>.
template <class R>
std::vector<int> to_vec(const R& r) {
    std::vector<int> out;
    for (typename boost::range_iterator<const R>::type it = boost::begin(r), e = boost::end(r);
         it != e; ++it)
        out.push_back(*it);
    return out;
}

// Counts how many times the adaptor chain actually computes an element.
int g_calls = 0;
struct counting_double {
    int operator()(int x) const { ++g_calls; return x * 2; }
};

// =============================================================================
// Tests — do not modify. Each assert names the task it checks.
// =============================================================================
int main() {
    demo::SmallVec sv{{3, 6, 7, 12, 5, 18}, 6};
    std::vector<int> v{4, 9, 2};
    (void)sv; (void)v;                          // GIVEN: unused while steps are gated off

#if TODO1_READY
    {
        assert(boost::begin(sv) == sv.data);
        assert(boost::end(sv) == sv.data + 6);
        assert(boost::size(sv) == 6u);
        assert(boost::distance(sv) == 6);
        assert((to_vec(sv) == std::vector<int>{3, 6, 7, 12, 5, 18}));
        // Boost.Range's own algorithms now work on SmallVec unchanged
        assert(boost::count(sv, 7) == 1);
        assert(*boost::max_element(sv) == 18);
        std::cout << "todo 1  SmallVec as a Boost.Range ..... ok\n";
    }
#else
    std::cout << "todo 1  SmallVec as a Boost.Range ..... TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        assert(sum_of(v) == 15);
        assert(sum_of(std::vector<int>{}) == 0);
    #if TODO1_READY
        assert(sum_of(sv) == 51);                  // the custom structure, same algorithm
    #endif
        std::cout << "todo 2  eager algorithm (value) ....... ok\n";
    }
#else
    std::cout << "todo 2  eager algorithm (value) ....... TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY
    {
        assert(*find_max_element(v) == 9);
        assert(find_max_element(v) == boost::begin(v) + 1);
        std::vector<int> empty_v;
        assert(find_max_element(empty_v) == boost::end(empty_v));
    #if TODO1_READY
        assert(*find_max_element(sv) == 18);
        demo::SmallVec empty_sv{{}, 0};
        assert(find_max_element(empty_sv) == boost::end(empty_sv));
    #endif
        std::cout << "todo 3  eager algorithm (iterator) .... ok\n";
    }
#else
    std::cout << "todo 3  eager algorithm (iterator) .... TODO (flip TODO3_READY)\n";
#endif

#if TODO4_READY
    {
        assert(demo::adaptors::is_multiple{3}(9) == true);
        assert(demo::adaptors::is_multiple{3}(7) == false);
        assert(demo::adaptors::is_multiple{0}(5) == false);   // no modulo by zero
        assert(demo::adaptors::multiples_of(4).k == 4);
        std::cout << "todo 4  adaptor predicate + factory ... ok\n";
    }
#else
    std::cout << "todo 4  adaptor predicate + factory ... TODO (flip TODO4_READY)\n";
#endif

#if TODO4_READY && TODO5_READY && TODO1_READY
    {
        using namespace demo::adaptors;
        assert((to_vec(sv | multiples_of(3)) == std::vector<int>{3, 6, 12, 18}));
        assert((to_vec(v | multiples_of(2)) == std::vector<int>{4, 2}));

        // composes with the built-in Boost.Range adaptors
        assert((to_vec(sv | multiples_of(3) | boost::adaptors::reversed)
                == std::vector<int>{18, 12, 6, 3}));

        // lazy: nothing runs until the result is walked, and only survivors run
        g_calls = 0;
        auto lazy = sv | multiples_of(3) | boost::adaptors::transformed(counting_double{});
        assert(g_calls == 0);
        assert((to_vec(lazy) == std::vector<int>{6, 12, 24, 36}));
        assert(g_calls == 4);                       // 4 of 6 elements survived the filter
        std::cout << "todo 5  lazy adaptor via operator| .... ok\n";
    }
#else
    std::cout << "todo 5  lazy adaptor via operator| .... TODO (flip TODO5_READY; needs TODO1 and TODO4)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
