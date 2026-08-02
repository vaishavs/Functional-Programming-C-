// =============================================================================
// Writing a custom range operation — TODO exercise
// =============================================================================
// One operation, implemented twice, to make the two styles directly comparable:
//
//   SECTION A — an EAGER ALGORITHM: a plain function template that takes a
//               range, walks it now, and returns a finished result.
//   SECTION B — a LAZY VIEW ADAPTOR: a view class plus the closure object that
//               makes `rng | stride(n)` work. Computes nothing until iterated.
//
// The operation is STRIDE: keep every Nth element, starting with the first.
//   {0,1,2,3,4,5,6,7,8,9} with n = 3  ->  {0, 3, 6, 9}
// It is deliberately not in C++20 (C++23 adds std::views::stride), so there is
// something real to build, with a standard counterpart to compare against.
//
// Markers
//   TODO(n)   — implement from scratch.
//   HINT      — a concrete nudge toward the implementation.
//   GIVEN     — provided scaffolding; read it, don't rewrite it.
//
// Flip a step's TODOn_READY to 1 when finished. The file ships compiling and
// running with every step off. Steps are cumulative in SECTION B: the view's
// iterator (TODO 3) is used by begin() (TODO 4), which is used by the adaptor
// (TODO 5).
//
//   Build : g++ -std=c++20 -Wall -Wextra -o stride try_05.cpp
//   Run   : ./stride
// =============================================================================

#include <cassert>
#include <concepts>
#include <iostream>
#include <iterator>
#include <ranges>
#include <vector>

#define TODO1_READY 0
#define TODO2_READY 0
#define TODO3_READY 0
#define TODO4_READY 0
#define TODO5_READY 0

// #############################################################################
// SECTION A — the eager algorithm (a plain function template)
// #############################################################################

// =============================================================================
// TODO 1 — an algorithm that takes a RANGE
// =============================================================================
// Three helper aliases do the type work, so nothing has to be hard-coded to
// `int` or to `std::vector`:
//   std::ranges::range_value_t<R>       — the element type ("int")
//   std::ranges::range_difference_t<R>  — the signed type used for distances
//   std::ranges::iterator_t<R>          — the range's iterator type
//
// The loop must be written as `while (it != last)`, never `it < last` and never
// arithmetic on `last`.
//
// TODO(T1): stride_copy(r, n) -> std::vector<range_value_t<R>>
//   Return a new vector holding every nth element of `r`, starting with the
//   first: n == 3 over {0..9} gives {0, 3, 6, 9}. `r` is left untouched.
//   HINT: take iterators once, then loop —
//     auto it = std::ranges::begin(r);
//     auto last = std::ranges::end(r);
//     while (it != last) {
//         out.push_back(*it);
//         // advance up to n times, but STOP at last:
//         for (diff i = 0; i < n && it != last; ++i) ++it;
//     }
//   That inner `&& it != last` guard is the whole trick: stepping an iterator
//   past the end is undefined behaviour even if the value is never read, and
//   the last stride will usually run off the end without it.
// =============================================================================
template <std::ranges::input_range R>
std::vector<std::ranges::range_value_t<R>>
stride_copy(R&& /*r*/, std::ranges::range_difference_t<R> /*n*/) {
    // TODO(T1)
    return {};
}

// =============================================================================
// TODO 2 — the same algorithm in the standard library's TWO forms
// =============================================================================
// TODO(T2a): stride_sum(first, last, n) -> iter_value_t<I>
//   Sum every nth element, starting with the first. Over {0..9} with n == 2
//   that is 0 + 2 + 4 + 6 + 8 == 20.
//   HINT: the same loop as TODO 1, accumulating into a
//   `std::iter_value_t<I> total{};` instead of pushing back. Value-initializing
//   with `{}` gives the right zero for any arithmetic element type.
//
// TODO(T2b): stride_sum(r, n) -> range_value_t<R>
//   The range overload. One line, no loop of its own.
//   HINT: return stride_sum(std::ranges::begin(r), std::ranges::end(r), n);
//   (No ambiguity with T2a: an iterator is not a range, and a range is not an
//   iterator, so exactly one overload is ever viable.)
// =============================================================================
template <std::input_iterator I, std::sentinel_for<I> S>
std::iter_value_t<I> stride_sum(I /*first*/, S /*last*/, std::iter_difference_t<I> /*n*/) {
    // TODO(T2a)
    return {};
}

template <std::ranges::input_range R>
std::ranges::range_value_t<R> stride_sum(R&& /*r*/, std::ranges::range_difference_t<R> /*n*/) {
    // TODO(T2b)
    return {};
}

// #############################################################################
// SECTION B — the lazy view adaptor
// #############################################################################
// Three pieces are needed, and the exercise builds them in order:
//   1. an iterator that knows how to step by n              (TODO 3)
//   2. the view class exposing begin()/end()                (TODO 4)
//   3. a closure object so `rng | stride(n)` works          (TODO 5)

// -----------------------------------------------------------------------------
// GIVEN — the view's skeleton. Members, constructors, end(), and the deduction
// guide are provided; the bodies marked TODO are the exercise.
//
// The class inherits `std::ranges::view_interface`, a CRTP base that generates
// the boilerplate a view is expected to have — `empty()`, `front()`, `back()`,
// `operator bool`, and `size()`/`operator[]` when the iterators can support them
// — all in terms of the begin()/end() written below. 
// The base range is constrained to `forward_range` so it can be traversed more
// than once, and to `view` so this type stays cheap to copy.
// -----------------------------------------------------------------------------
template <std::ranges::forward_range V>
    requires std::ranges::view<V>
class stride_view : public std::ranges::view_interface<stride_view<V>> {
    V base_{};                                       // the range being adapted
    std::ranges::range_difference_t<V> n_ = 1;       // the stride

public:
    stride_view() requires std::default_initializable<V> = default;
    constexpr stride_view(V base, std::ranges::range_difference_t<V> n)
        : base_(std::move(base)), n_(n) {}

    // =========================================================================
    // TODO 3 — the iterator: where the work actually happens
    // =========================================================================
    // For `std::forward_iterator` it must provide: the member type aliases
    // below, default construction, `*`, prefix and postfix `++`, and `==`.
    // This iterator carries three things: the current position, the base range's
    // END (so stepping can stop safely — see the guard in TODO 1), and stride.
    //
    // TODO(T3a): operator*
    //   Return the element at the current position.
    //   HINT: one line — `return *cur_;`. The return type is `decltype(auto)`
    //   on purpose: it preserves whatever the base iterator yields, a real
    //   reference (so callers can write through it) or a prvalue, rather than
    //   silently copying.
    //
    // TODO(T3b): operator++ (prefix)
    //   Advance up to n positions, STOPPING at the end.
    //   HINT: the same bounded loop as TODO 1 —
    //     for (difference_type i = 0; i < n_ && cur_ != end_; ++i) ++cur_;
    //     return *this;
    //   Without `&& cur_ != end_`, the final stride steps past the end: UB, and
    //   the `== default_sentinel` test below would then never become true, so
    //   the loop would run off into memory instead of finishing.
    //
    // (Postfix `++` and both `==` overloads are GIVEN — postfix is always the
    // same "copy, pre-increment, return the copy" shape, and equality just
    // compares positions.)
    // =========================================================================
    class iterator {
        std::ranges::iterator_t<V> cur_{};           // where this iterator is
        std::ranges::sentinel_t<V> end_{};           // the base range's end
        std::ranges::range_difference_t<V> n_ = 1;   // the stride

    public:
        using value_type = std::ranges::range_value_t<V>;
        using difference_type = std::ranges::range_difference_t<V>;
        using iterator_concept = std::forward_iterator_tag;

        iterator() = default;
        constexpr iterator(std::ranges::iterator_t<V> cur, std::ranges::sentinel_t<V> e,
                           difference_type n)
            : cur_(std::move(cur)), end_(std::move(e)), n_(n) {}

        constexpr decltype(auto) operator*() const {
            // TODO(T3a)
            return value_type{};                     // stub: ignores the position
        }

        constexpr iterator& operator++() {
            // TODO(T3b)
            ++cur_;                                  // stub: steps 1, not n
            return *this;
        }

        // GIVEN
        constexpr iterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
        friend constexpr bool operator==(const iterator& a, const iterator& b) {
            return a.cur_ == b.cur_;
        }
        friend constexpr bool operator==(const iterator& a, std::default_sentinel_t) {
            return a.cur_ == a.end_;
        }
    };

    // =========================================================================
    // TODO 4 — begin(): hand out a configured iterator
    // =========================================================================
    // A view's begin() builds its iterator from the base range.
    //
    // TODO(T4): begin()
    //   Return an iterator positioned at the base range's first element,
    //   carrying the base's end and this view's stride.
    //   HINT: one line —
    //     return iterator{std::ranges::begin(base_), std::ranges::end(base_), n_};
    //
    // end() is GIVEN, and returns `std::default_sentinel`, where the actual
    // comparison is done by `operator==(iterator, default_sentinel_t)` above,
    // which asks the ITERATOR whether it has reached the base's end.
    // =========================================================================
    constexpr iterator begin() {
        // TODO(T4)
        return iterator{};                           // stub: an empty range
    }

    constexpr std::default_sentinel_t end() const { return {}; }   // GIVEN
};

// GIVEN — class template argument deduction. `std::views::all_t<R>` turns
// whatever was passed in into a view: an lvalue container becomes a `ref_view`
// (a non-owning reference), while an rvalue view is taken by value. This is why
// stride_view never copies a caller's vector.
template <class R>
stride_view(R&&, std::ranges::range_difference_t<R>) -> stride_view<std::views::all_t<R>>;

// =============================================================================
// TODO 5 — the adaptor closure: making `rng | stride(n)` work
// =============================================================================
// `stride(2)` does not adapt anything by itself — there is no range yet. It
// returns a CLOSURE: a small object holding the arguments (here just n) and
// waiting for a range. Two things then make it usable:
//
//   closure(rng)   — call it directly, like a function
//   rng | closure  — pipe syntax, which is defined to mean exactly closure(rng)
//
// TODO(T5a): operator()
//   Build the view from the incoming range.
//   HINT: wrap the range with std::views::all so it becomes a view, then
//   construct:
//     return stride_view(std::views::all(std::forward<R>(r)),
//                        static_cast<std::ranges::range_difference_t<R>>(n));
//
// TODO(T5b): operator|
//   Make the pipe mean the call.
//   HINT: one line — `return c(std::forward<R>(r));`
// =============================================================================
struct stride_closure {
    std::ptrdiff_t n;

    template <std::ranges::viewable_range R>
        requires std::ranges::forward_range<R>
    constexpr auto operator()(R&& r) const {
        // TODO(T5a)
        return stride_view(std::views::all(std::forward<R>(r)),
                           static_cast<std::ranges::range_difference_t<R>>(1)); // stub: always stride 1
    }

    template <std::ranges::viewable_range R>
    friend constexpr auto operator|(R&& r, const stride_closure& c) {
        // TODO(T5b)
        (void)c;
        return std::views::all(std::forward<R>(r));  // stub: ignores the stride entirely
    }
};

// GIVEN — the user-facing factory. Calling stride(n) produces the closure.
constexpr stride_closure stride(std::ptrdiff_t n) { return stride_closure{n}; }

// GIVEN — test helper: drain any range into a vector<int>.
template <class R>
std::vector<int> to_vec(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) out.push_back(x);
    return out;
}

// GIVEN — counts how many times the transform in TODO 5's laziness test runs.
int g_calls = 0;

// =============================================================================
// Tests — do not modify. Each assert names the task it checks.
// =============================================================================
int main() {
    const std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

#if TODO1_READY
    {
        assert((stride_copy(v, 2) == std::vector<int>{0, 2, 4, 6, 8}));
        assert((stride_copy(v, 3) == std::vector<int>{0, 3, 6, 9}));
        assert((stride_copy(v, 1) == v));                    // stride 1 keeps everything
        assert(stride_copy(std::vector<int>{}, 2).empty());  // empty in, empty out
        std::cout << "todo 1  eager algorithm (range form) .. ok\n";
    }
#else
    std::cout << "todo 1  eager algorithm (range form) .. TODO (flip TODO1_READY)\n";
#endif

#if TODO2_READY
    {
        assert(stride_sum(v.begin(), v.end(), 2) == 20);     // 0+2+4+6+8
        assert(stride_sum(v, 3) == 18);                      // 0+3+6+9
        assert(stride_sum(v.begin() + 1, v.end(), 4) == 15); // 1+5+9, a sub-range
        std::cout << "todo 2  iterator/sentinel + range form  ok\n";
    }
#else
    std::cout << "todo 2  iterator/sentinel + range form  TODO (flip TODO2_READY)\n";
#endif

#if TODO3_READY && TODO4_READY
    {
        // The view used directly, without the pipe machinery of TODO 5.
        assert((to_vec(stride_view(std::views::all(v), 3)) == std::vector<int>{0, 3, 6, 9}));
        assert((to_vec(stride_view(std::views::all(v), 4)) == std::vector<int>{0, 4, 8}));
        std::cout << "todo 3+4 view: iterator and begin() ... ok\n";
    }
#else
    std::cout << "todo 3+4 view: iterator and begin() ... TODO (flip TODO3_READY and TODO4_READY)\n";
#endif

#if TODO3_READY && TODO4_READY && TODO5_READY
    {
        // Direct call and pipe must agree — the pipe is only a forwarder.
        assert((to_vec(stride(2)(v)) == std::vector<int>{0, 2, 4, 6, 8}));
        assert((to_vec(v | stride(2)) == std::vector<int>{0, 2, 4, 6, 8}));

        // Composes with the standard adaptors, in either position.
        assert((to_vec(v | stride(3) | std::views::transform([](int x) { return x * 10; }))
                == std::vector<int>{0, 30, 60, 90}));
        assert((to_vec(v | std::views::filter([](int x) { return x % 2 == 0; }) | stride(2))
                == std::vector<int>{0, 4, 8}));

        // Laziness: nothing is computed until the range is walked, and the
        // skipped elements are never computed at all.
        g_calls = 0;
        auto lazy = v | std::views::transform([](int x) { ++g_calls; return x; }) | stride(3);
        assert(g_calls == 0);                                // building it did no work
        assert((to_vec(lazy) == std::vector<int>{0, 3, 6, 9}));
        assert(g_calls == 4);                                // 4 yielded, 6 never touched

        // view_interface supplies these from begin()/end() alone.
        auto sv = v | stride(5);
        assert(!sv.empty());
        assert(sv.front() == 0);

        std::cout << "todo 5  adaptor closure and pipe ..... ok\n";
    }
#else
    std::cout << "todo 5  adaptor closure and pipe ..... TODO (flip TODO5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
