// =============================================================================
// Boost.Range bug-hunting exercise — ANNOTATED EDITION
// =============================================================================
// 26 bugs (Bug 0-25) across Boost.Range adaptors, Boost.Iterator internals, and
// a few classic <algorithm> pitfalls. The CODE is exactly as originally written
// — every bug still there, unfixed, on purpose. Each note gives the rule broken,
// the consequence (compile error / UB / silent wrong answer), and the fix.
//
// Recurring threads:
//   - `const auto&` / `auto&&` on a temporary extends THAT temporary's life, but
//     not one used to BUILD it. Bugs 0, 5, 21, 24 are safe-today-but-fragile;
//     Bugs 11 and 13 genuinely dangle.
//   - `filtered` is never random access, and `dropped` isn't a Boost.Range name
//     (Bugs 7, 22, 24). `transformed` can be random access for READING, but is
//     writable only if the function returns a reference (Bugs 1, 14, 20).
//   - Views aren't Containers — no `.erase()` / `.insert()` (Bugs 10, 18).
//   - `remove_if` rearranges without shrinking (Bug 8).
// =============================================================================

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

using namespace std;
using namespace boost::adaptors;

int main() {
    vector<int> nums = {1, 2, 3, 4, 5};

    // --- BUG 0 — "Dangling" range reference (subtler than it looks) ----------
    // Not dangling: `nums | transformed(...)` is a prvalue, and binding
    // `const T&` directly to a prvalue extends its life, so `squares` lives to
    // the end of `main`. What IS fragile:
    //   1. it holds iterators into `nums` — dead if `nums` reallocates, is
    //      destroyed, or is moved from (Bug 25);
    //   2. extension reaches only the outermost temporary bound by the
    //      reference, never one fed INTO an adaptor (Bugs 11, 13);
    //   3. it is const and lazy — stores nothing, recomputes `x * x` per
    //      dereference. That, not a dead pointer, is what breaks Bug 1.
    // Works by accident of this expression's shape; materialize instead:
    //   boost::range::push_back(squares_v, nums | transformed(f));
    const auto& squares = nums | transformed([](int x) { return x * x; }); // fragile, not literally dangling — see note above

    // --- BUG 1 — Sorting a const, lazily-computed range in place -------------
    // `sort` needs a MUTABLE RandomAccessRange — it compares and swaps in place.
    // `squares` is const (Bug 0), and `transformed` returns each value BY VALUE,
    // so there is no lvalue to write back into; returning a value also caps the
    // iterator category at bidirectional, so sort's checks reject it first.
    // Expect a wall of template errors. Fix: push_back into a vector, sort that.
    boost::range::sort(squares); // won't compile: const, and the transform result isn't writable random-access

    // --- BUG 2 — Adaptor order changes MEANING (silent logic bug) ------------
    // Composition isn't commutative — each stage sees the previous stage's
    // OUTPUT. As written, `filtered` tests the SQUARED values (1,4,9,16,25) and
    // yields {16, 25}. Reversed, the predicate would see 1..5 and match nothing.
    // Both orders compile, both look reasonable: no crash, just wrong elements.
    // State explicitly which values the predicate is meant to see.
    auto filtered_squares = nums
        | transformed([](int x) { return x * x; })
        | filtered([](int x) { return x >= 10; }); // filters the SQUARED values — confirm that's what you meant

    // --- BUG 3 — Passing a container where copy() wants an output iterator ---
    // `copy`'s second argument must be an OutputIterator (`*out++ = value`);
    // `nums` is a container. `copy_vec` sits right above, unused — a strong
    // signal it was the intended destination. Worse, `filtered_squares` is a
    // lazy view reading from `nums`, so even if coerced to compile this would
    // overwrite its own source mid-walk.
    // Fix: boost::range::copy(filtered_squares, std::back_inserter(copy_vec));
    vector<int> copy_vec;
    boost::range::copy(filtered_squares, nums); // `nums` is a container, not an OutputIterator — and it aliases the source

    // --- BUG 4 — Stateful capture inside a lazy transform --------------------
    // Views are LAZY: the function reruns on every dereference, not once per
    // element (iterating twice, `boost::size`, a `find` before a `copy`). This
    // lambda mutates `offset` by reference, so re-reading the same element gives
    // a different answer — breaking every algorithm that assumes dereference is
    // idempotent. Functions fed to lazy adaptors must be pure.
    int offset = 0;
    auto offsetted = nums | transformed([&offset](int x) { return x + offset++; }); // impure: result depends on call count, not just x

    // --- BUG 5 — Splitting a range into loose begin/end iterators ------------
    // Safe here (`auto&&` extends the prvalue, `nums` is durable), but the
    // PATTERN is the bug: `it1`/`it2` are decoupled from whatever keeps the view
    // — and the predicate copy it owns — alive. One more layer of indirection
    // (returned from a helper, stored as a member) and they dangle with nothing
    // at the call site looking wrong. Iterate the range directly.
    auto&& temp_range = nums | filtered([](int x) { return x % 2 == 0; });
    auto it1 = boost::begin(temp_range);
    auto it2 = boost::end(temp_range);
    for (auto it = it1; it != it2; ++it) { // fragile pattern, even though this particular instance is memory-safe
        cout << *it << " ";
    }
    cout << endl;

    // --- BUG 6 — A function that doesn't exist in Boost.Range ----------------
    // `apply_filtered` is fabricated — no header, no declaration. Commented out
    // because uncommenting gives "undeclared identifier". The exercise is
    // spotting names that merely SOUND plausible beside real ones. For a lazy
    // view use `filtered`; to actually remove elements use `remove_erase_if`
    // (with the predicate inverted — it removes what matches).
    // boost::range::apply_filtered(nums, [](int x) { return x > 3; }); // fictional API — would fail to compile

    // --- BUG 7 — `dropped` isn't a real adaptor ------------------------------
    // Primary failure: no `dropped` in Boost.Range — <boost/range/adaptors.hpp>
    // has sliced, strided, filtered, transformed, reversed, uniqued and friends,
    // but not this. (It exists in the unrelated Oven library, and as `drop` in
    // range-v3 / C++20 <ranges> — easy to misattribute.) Unresolved identifier,
    // so nothing runs. Real tool: `sliced(N, boost::size(rng))`.
    // Secondary: the filtered range is EMPTY, so even a working "drop 1" would
    // advance past `end()` (UB) and then dereference it (UB again).
    auto dropped = nums | filtered([](int x) { return x > 100; }) | dropped(1); // `dropped` isn't real Boost.Range, and the filtered range is empty anyway
    cout << "First after drop: " << *boost::begin(dropped) << endl; // dereferences a likely-end() iterator: UB, not just "maybe wrong"

    // --- BUG 8 — remove_if rearranges; it does not shrink --------------------
    // The classic erase-remove trap: `remove_if` moves keepers to the front and
    // returns the new logical end; the tail is valid-but-unspecified and
    // `size()` is unchanged. The return value is discarded here and `.erase()`
    // never called, so `nums` still has 5 elements — no crash, no warning, just
    // wrong data downstream. Fix: boost::range::remove_erase_if(nums, pred);
    boost::range::remove_if(nums, [](int x) { return x % 2 == 1; }); // return value discarded — nums.size() is still 5

    // --- BUG 9 — unique_copy only collapses ADJACENT duplicates --------------
    // The `const` is a red herring — a const source is exactly what a copy
    // algorithm wants. The real trap is semantic: duplicates are dropped only
    // when immediately next to an equal value, and nothing is sorted first, so
    // {1, 2, 1, 3, 1} keeps all three 1's. This call can't even show the
    // failure — `nums` is already sorted and distinct, so it "looks correct" by
    // accident of the input. Sort first for real de-duplication.
    const auto& const_nums = nums;
    vector<int> unique_vec;
    boost::range::unique_copy(const_nums, back_inserter(unique_vec)); // only strips ADJACENT dupes; nums has none, so this proves nothing

    // --- BUG 10 — remove_erase_if needs a Container, not a view --------------
    // It runs the whole idiom internally: `remove_if`, then `.erase()` ON THE
    // ARGUMENT PASSED — so that argument must own its storage.
    // `make_iterator_range(...)` builds a non-owning `iterator_range`: it models
    // Range (has begin/end) but not Container, so expect "no member named
    // 'erase'". The Range-vs-Container split recurs in C++20 <ranges> too: a
    // view can re-window data, only a container can resize it. Use `nums`.
    boost::range::remove_erase_if(boost::make_iterator_range(nums.begin(), nums.end()),
        [](int x) { return x < 0; }); // iterator_range has no .erase() — it's a view, not a container

    // --- BUG 11 — sliced() over a temporary: genuine dangling ----------------
    // What Bug 0 warned about. The vector is an unnamed temporary passed as an
    // ARGUMENT to `operator|`, so it dies at the end of the full expression; it
    // is not the initializer bound by `sliced_ref`, so extension never reaches
    // it. Only the outer `sliced_range` survives — pointing into a freed buffer.
    // Any later read is UB. Fix: name the container so it outlives the view.
    const auto& sliced_ref = vector<int>{1, 2, 3, 4, 5} | sliced(1, 4); // the vector is a temporary; this genuinely dangles

    // --- BUG 12 — Lambda captures a block-scoped local by reference ----------
    // No UB here only because `with_local` never outlives the block, so nothing
    // reads through the dead reference. It's a template for an easy mistake: the
    // moment the range or its lambda escapes — returned, stored, passed on —
    // every dereference reads a destroyed `int`. Capture by value if it may be
    // used later.
    {
        int local = 42;
        auto with_local = nums | transformed([&local](int x) { return x + local; }); // fine only because it's never used past this line
    }

    // --- BUG 13 — A range returned over a function-local temporary -----------
    // Unambiguous, unlike Bugs 0/5/21: the temporary vector dies when
    // `make_range()` exits, before `r` ever binds. `auto&&`, `auto`, and
    // `const auto&` are all equally useless — reference binding extends the
    // small returned wrapper, it cannot resurrect a stack buffer from a function
    // that already returned. Return an owning container, or take one by
    // reference so its lifetime belongs to the caller.
    auto make_range = []() {
        return vector<int>{1, 2, 3} | filtered([](int x) { return x > 1; });
    };
    auto&& r = make_range(); // dangling before this line even finishes running — no binding style fixes it

    // --- BUG 14 — operator[] on an adaptor: real here, never assumable -------
    // `transform_iterator` preserves the base range's traversal category, so
    // over a `vector` this is random access and `indexed[2]` most likely
    // compiles, giving 9. What it isn't is durable: switch to a `std::list`, or
    // insert a `filtered` stage, and `operator[]` stops compiling or stops being
    // O(1) (Bug 24). The result is also a fresh prvalue, not a reference into
    // `nums` — readable, not writable (Bug 1). Safer: `*std::next(begin, i)`.
    auto indexed = nums | transformed([](int x) { return x * x; });
    cout << "Indexed access: " << indexed[2] << endl; // likely works here, but only because transformed+vector happens to stay random-access

    // --- BUG 15 — Writing into a transformed VIEW of an empty vector ---------
    // `copy` writes into EXISTING slots and never grows anything. `tmp_vec` is
    // empty, so the view spans zero elements with no room for `nums`'s five —
    // either a no-op or an out-of-bounds write (UB), since copy is driven by the
    // SOURCE's length and never learns the destination's size. And even
    // non-empty, a transformed view is no place to write: there is no reverse
    // mapping from the assigned value back to the original, unless the function
    // returns a real writable reference, which `x * x` does not.
    vector<int> tmp_vec;
    boost::range::copy(nums,
        tmp_vec | transformed([](int x) { return x * x; })
    ); // destination is both empty and not a meaningful place to "write" a squared value into

    // --- BUG 16 — for_each wants (first, last, unary_fn) ---------------------
    // Two mismatches: `std::for_each` takes a PAIR of iterators, not a range
    // object, so nothing matches; and `std::greater<int>` is BINARY, while
    // `for_each` calls `f(*it)` with one argument — no single-arg `operator()`.
    // Fix: a range-based for loop, or pass begin/end plus a unary lambda.
    std::for_each(squares, std::greater<int>{}); // wrong argument shape AND wrong arity of functor

    // --- BUG 17 — Dereferencing a find() result unchecked --------------------
    // `found_range` filters to elements > 10 — EMPTY. `find` signals failure by
    // returning `end()`, which here equals `begin()`. `*pos` dereferences a
    // sentinel: UB, full stop, not "maybe wrong". Every find-family result needs
    // its `if (pos != boost::end(...))` guard first.
    auto found_range = nums | filtered([](int x) { return x > 10; });
    auto pos = boost::range::find(found_range, 11);
    cout << "Found: " << *pos << endl; // pos == end() here — dereferencing it is UB, not just "maybe wrong"

    // --- BUG 18 — Calling a Container member (.erase()) on a view ------------
    // `filtered_range` is a non-owning window onto `nums`, so it has no
    // `.erase()`, `.insert()`, or `.push_back()` — the elements aren't its to
    // remove. A hard compile error, which makes this one of the SAFER mistakes
    // here. Included because "view onto data" vs "container that owns data" is a
    // common category error: same begin/end, very different powers. To erase,
    // call `remove_erase_if(nums, pred)` on the container itself.
    (nums | filtered([](int x) { return x > 3; })).erase( // filtered_range has no .erase() — it's a view, not a container
        nums.begin(), nums.end()
    );

    // --- BUG 19 — std::vector<T&> is ill-formed ------------------------------
    // `squares` was declared `const auto&` (Bug 0), and `decltype` on a name
    // whose declared type is a reference reproduces the reference — so this asks
    // for `vector<T&>`, which containers forbid (they need an Erasable object
    // type). Expect a deep error bottoming out in `std::allocator<T&>`. Even
    // with a proper value type, N stored views multiply every lifetime hazard
    // above by N.
    vector<decltype(squares)> vec_of_ranges; // decltype(squares) is a REFERENCE type — vector<T&> doesn't compile

    // --- BUG 20 — std::random_shuffle was removed in C++17 -------------------
    // Primary failure: deprecated in C++14, REMOVED in C++17 for its reliance on
    // `std::rand()` (poor quality, not thread-safe, globally seeded), so under a
    // modern standard <algorithm> doesn't declare the name at all. Replacement:
    // `std::shuffle(first, last, urbg)` with an explicit generator.
    // Also, on an older standard, it takes an iterator PAIR, not a range (as in
    // Bug 16) — and `squares` is const and unswappable anyway (Bugs 0, 1).
    std::random_shuffle(squares); // removed in C++17+, wrong argument shape, and squares isn't mutable/swappable anyway

    // --- BUG 21 — reversed over a named lvalue: the Bug 0 story again --------
    // `reversed` is a plain adaptor tag, not a call. The resulting prvalue is
    // lifetime-extended by `const auto&`, and `nums` is durable, so this is
    // memory-safe in practice. Flagged because it's structurally identical to
    // the genuinely-dangling Bugs 11 and 13 — nothing in the line tells you
    // which case you're in without tracing where `nums` came from.
    const auto& reversed_ref = nums | reversed; // safe today because `nums` is a durable lvalue — don't rely on that surviving a refactor
    for (int x : reversed_ref) {
        cout << x << " ";
    }
    cout << endl;

    // --- BUG 22 — Bug 7 again, plus: filtered ranges aren't Sized ------------
    // `dropped` still doesn't exist (Bug 7), so this fails to compile first.
    // Granting a working "skip N": filtering leaves {2, 4}, and dropping 100
    // means walking 100 steps through two elements. A filtered range can't
    // report its length without testing every element — it is not a Sized Range,
    // so there's no O(1) `.size()` to clamp against, and a naive implementation
    // stepping begin forward N times (all a bidirectional `filter_iterator`
    // allows, Bug 24) runs straight past `end()`: UB, then UB again on the
    // dereference below.
    auto dropped2 = nums | filtered([](int x) { return x % 2 == 0; }) | dropped(100); // `dropped` isn't real, and 100 far exceeds the 2 elements that would survive filtering
    cout << "dropped2 front: " << *boost::begin(dropped2) << endl; // near-certain UB: advances (and then dereferences) past end()

    // --- BUG 23 — count_if wants a unary predicate ---------------------------
    // `count_if` calls `pred(element)` with ONE argument; `std::greater<int>` is
    // a two-argument comparator with no unary overload, so nothing binds and it
    // fails to compile — Bug 16's mistake against a different algorithm. It also
    // carries no "greater than WHAT?".
    // Fix: boost::range::count_if(nums, [](int x){ return x > 3; });
    auto count = boost::range::count_if(nums, std::greater<int>{}); // std::greater<int> takes two arguments; count_if's predicate takes one

    // --- BUG 24 — Sorting a filtered range fails on iterator category --------
    // The lifetime story is a red herring: `auto&&` extends the prvalue and
    // `nums` is durable, so nothing dangles. The guaranteed failure is that
    // `boost::filter_iterator` is forced down to `bidirectional_traversal_tag`
    // in filter_iterator.hpp regardless of the base iterator — inherent to
    // filtering, since reaching the Nth survivor means testing everything in
    // between, so O(1) jumps are impossible. `sort` requires random access, so
    // no filtered range over any container will compile here.
    {
        auto&& local_range = nums | filtered([](int x) { return x > 2; });
        boost::range::sort(local_range); // fails to compile: filter_iterator is capped at bidirectional, sort needs random access
    } // (the lifetime story here is a red herring — this line was never going to compile)

    // --- BUG 25 — Reading a moved-from container, and every view over it -----
    // `std::move` moves nothing — it's a cast that lets the move constructor
    // steal the buffer, leaving `nums` valid but UNSPECIFIED (empty in practice
    // on every mainstream library, which is an implementation choice, not a
    // guarantee). Using it afterward for anything but destruction or
    // reassignment is unspecified behavior. The sharper point: `squares`,
    // `filtered_squares`, `offsetted`, `temp_range`, `found_range`, `indexed`,
    // `reversed_ref` and the rest are still in scope, each holding iterators
    // taken from `nums` before its buffer changed owners. They may still "work"
    // depending on move internals — which is the same mistake as everything else
    // here. Treat anything built over a container as invalid once it is moved from.
    vector<int> moved_nums = move(nums);
    auto moved_squares = moved_nums | transformed([](int x) { return x * x; });
    // If nums is moved from and later used, behavior is undefined — and by this point in
    // main(), nums has already been captured by several earlier lazy views built above.

    return 0;
}
