# Standard Library Function Objects (Functors)

The C++ standard library ships a small collection of ready-made *function objects* — objects that can be called like functions — for the built-in operations: arithmetic, comparison, logical and bitwise operators, plus a few special-purpose ones (`std::identity`, `std::hash`). They let an operator be handed to an algorithm or a container as a value, without writing a lambda each time, and several of them (the *transparent* comparators and the container default comparators) are the idiomatic, efficient choice for everyday tasks. This file explains what they are, the background a first-time reader needs, each category with worked examples, and where they fit against lambdas and the other facilities in `<functional>`.



## What counts as a "standard function object", and where it lives

All of the function objects in this file are declared in the header `<functional>`. They fall into two groups that are easy to conflate but serve different purposes:

- **Operator function objects** — the subject of this file. Each wraps one built-in operation as a callable: `std::plus` performs `+`, `std::less` performs `<`,     `std::bit_and` performs `&`, and so on. These are tiny, usually stateless classes whose `operator()` simply applies the operator to its arguments.
- **Wrappers and adaptors** — also in `<functional>`, but distinct in role: `std::function`, `std::move_only_function`, `std::reference_wrapper`, `std::bind`, `std::bind_front`, `std::mem_fn`, and `std::not_fn` *store*, *transform*, or *erase* other callables rather than supplying an operation. They are covered elsewhere in this series; §11 marks the boundary.

The operator function objects are used in three recurring ways, illustrated throughout:

- as the *operation* argument to an algorithm (`std::accumulate`, `std::transform`);
- as the *comparator* argument to a sorting algorithm or a sorted/hashed container;
- as a *default template argument* that a container falls back on.



## Arithmetic function objects

Six objects wrap the arithmetic operators. Each is called with the operands and returns the result of the corresponding operator.

| Function object | Operation |
|---|---|
| `std::plus` | `a + b` |
| `std::minus` | `a - b` |
| `std::multiplies` | `a * b` |
| `std::divides` | `a / b` |
| `std::modulus` | `a % b` |
| `std::negate` | `-a` (unary) |

```cpp
#include <functional>

std::plus<int>{}(3, 4);      // 7
std::minus<>{}(10, 3);       // 7
std::multiplies<>{}(6, 7);   // 42
std::divides<>{}(20, 4);     // 5
std::modulus<>{}(17, 5);     // 2
std::negate<>{}(5);          // -5
```

Their primary use is as the *operation* passed to a numeric algorithm, which avoids writing a lambda that merely applies an operator:

```cpp
#include <numeric>
#include <algorithm>
#include <vector>

std::vector<int> v{1, 2, 3, 4};
std::accumulate(v.begin(), v.end(), 1, std::multiplies<>{});   // 24  (1*1*2*3*4)

std::vector<int> a{1, 2, 3}, b{10, 20, 30}, out(3);
std::transform(a.begin(), a.end(), b.begin(), out.begin(), std::plus<>{});
// out == {11, 22, 33}
```

Two cautions carry over directly from the operators these objects wrap: `std::divides` and `std::modulus` have undefined behaviour on division by zero, exactly as `/` and `%` do, and the result types follow the usual arithmetic conversions.



## Comparison function objects

Six objects wrap the relational and equality operators, returning `bool`:

| Function object | Operation |
|---|---|
| `std::equal_to` | `a == b` |
| `std::not_equal_to` | `a != b` |
| `std::less` | `a < b` |
| `std::greater` | `a > b` |
| `std::less_equal` | `a <= b` |
| `std::greater_equal` | `a >= b` |

```cpp
std::less<>{}(2, 5);       // true
std::greater<>{}(2, 5);    // false
std::equal_to<>{}(4, 4);   // true
```

Comparison objects are the standard vehicle for customizing *ordering*. As a comparator to a sort, `std::greater<>` produces descending order; as the ordering policy of a sorted container, it reverses the traversal order:

```cpp
std::vector<int> s{3, 1, 4, 1, 5};
std::sort(s.begin(), s.end(), std::greater<>{});   // s == {5, 4, 3, 1, 1}

std::set<int, std::greater<int>> desc{1, 2, 3};    // iterates 3, 2, 1
```

`std::less` is the single most important of all the standard function objects, because it is the **default** comparator for the ordered associative containers and the priority queue (§10). It is also the archetype of a *transparent* comparator (§7–§8).



## Logical function objects

Three objects wrap the logical operators, returning `bool`:

| Function object | Operation |
|---|---|
| `std::logical_and` | `a && b` |
| `std::logical_or` | `a \|\| b` |
| `std::logical_not` | `!a` (unary) |

```cpp
std::logical_and<>{}(true, false);   // false
std::logical_or<>{}(true, false);    // true
std::logical_not<>{}(false);         // true
```

One semantic difference from the raw operators is worth noting for correctness-sensitive code: the built-in `&&` and `||` *short-circuit* (the right operand is not evaluated when the left already decides the result), but `std::logical_and` and `std::logical_or` are ordinary function calls, so **both arguments are always evaluated** before the call. This matters only when an argument expression has side effects; for plain boolean values the behaviour is identical.



## Bitwise function objects (C++14)

Four objects wrap the bitwise operators. These were added in C++14.

| Function object | Operation |
|---|---|
| `std::bit_and` | `a & b` |
| `std::bit_or` | `a \| b` |
| `std::bit_xor` | `a ^ b` |
| `std::bit_not` | `~a` (unary) |

```cpp
std::bit_and<>{}(12, 10);        // 8
std::bit_or<>{}(12, 10);         // 14
std::bit_xor<>{}(12, 10);        // 6
std::bit_not<unsigned>{}(0u);    // 4294967295   (all bits set in a 32-bit unsigned)
```

As with the arithmetic objects, the result follows the semantics of the underlying operator, so bitwise operations on signed types inherit the operators' representation-dependent corners; unsigned operands (as above) give the most portable results.



## The common shape and its consequences

Every operator function object above shares the same design, and two of its properties explain much of their usefulness.

**They are (typically) empty.** An object like `std::plus<>` holds no data members. An empty class can be stored at *zero cost* inside another object through the *empty base optimization*, which is why using `std::less<int>` as a container's comparator adds nothing to the container's size. A stateless lambda enjoys the same benefit; a function *pointer* does not (it is a stored address).

**They enable inlining.** Because each is a distinct, statically known type, an algorithm templated on the comparator or operation knows exactly which `operator()` runs and can inline it, often eliminating the call entirely. This is the same performance advantage a function object has over a function pointer, and it applies equally to these library-supplied objects.



## Two spellings: fixed-type versus transparent (`<>`)

Each operator function object can be written in two ways, and the difference is important.

**Fixed type** — `std::less<int>` — has an `operator()` that takes two `int`s. It works only for that one operand type.

**Transparent** — `std::less<>` (which names `std::less<void>`) — has a *templated* `operator()` that accepts operands of *any* types for which the operator is valid, deducing them per call. The `<>` form became available in C++14, when the template parameter of these objects gained a default of `void`.

```cpp
std::less<int> homogeneous;   // operator()(int, int)
std::less<>    transparent;   // template operator()(T&&, U&&)
```

The transparent form is generally preferable in generic code and in containers with non-trivial keys, for two reasons developed in the next section: it avoids constructing temporary objects during lookup, and it permits *heterogeneous* comparison between related types. The fixed-type form remains appropriate when a single, specific operand type is intended and the extra genericity is unwanted.



## Transparent comparators and heterogeneous lookup

A transparent comparator carries a nested tag type named `is_transparent`. The tag holds no data; its sole purpose is to *signal* to the sorted associative containers (`std::set`, `std::map`, and their `multi` variants) that heterogeneous lookup is permitted. The presence of the tag can be detected with a small trait:

```cpp
#include <type_traits>
template<class T, class = void> constexpr bool is_transparent_v = false;
template<class T> constexpr bool
    is_transparent_v<T, std::void_t<typename T::is_transparent>> = true;

is_transparent_v<std::less<int>>;   // false — fixed type, no tag
is_transparent_v<std::less<>>;      // true  — transparent
```

When a container is given a transparent comparator, its `find`, `count`, `contains`, `lower_bound`, and related member functions gain a *template* overload that accepts a key of a different-but-comparable type — without first converting it to the container's key type. The practical payoff is efficiency: searching a `std::set<std::string>` with a `const char*` or a `std::string_view` no longer constructs a temporary `std::string`.

```cpp
#include <set>
#include <string>
#include <string_view>

std::set<std::string, std::less<>> trans{"apple", "pear", "kiwi"};

std::string_view sv = "apple";
trans.find(sv) != trans.end();   // true — searched with a string_view, no temporary string
trans.contains("kiwi");          // true — searched with a const char*, no temporary string
```

The distinction is not merely about avoiding a copy; it can be the difference between compiling and not compiling. A `std::string_view` does **not** implicitly convert to a `std::string`, so a *non-transparent* set — whose `find` takes `const std::string&` — cannot be searched with a `string_view` at all:

```cpp
std::set<std::string> homo{"apple"};   // default std::less<std::string>: NOT transparent
std::string_view sv = "apple";
// homo.find(sv);   // ill-formed: no matching find(string_view), and no string_view -> string
```

The same applies to `std::map` with a transparent comparator, which is the common case for a map keyed by strings that must be looked up by `string_view` or literal:

```cpp
#include <map>
std::map<std::string, int, std::less<>> m{{"one", 1}, {"two", 2}};
m.find(std::string_view{"two"})->second;   // 2 — heterogeneous key, no temporary
```

For containers whose key type is expensive to construct, declaring the comparator as the transparent `std::less<>` is the recommended default precisely because of these two benefits. (Note that this concerns the *ordered* containers. The unordered containers achieve heterogeneous lookup through a different mechanism involving transparent hash and equality types, available from C++20, and outside the scope of this file.)



## `std::identity` (C++20)

`std::identity` is the function object that returns its argument unchanged:

```cpp
#include <functional>
std::identity{}(42);   // 42
```

Trivial as it appears, it has a specific and important role: it is the **default projection** throughout the C++20 ranges library. A ranges algorithm applies a projection to each element before inspecting it, and passing `{}` (or omitting the projection) supplies `std::identity`, meaning "use the element itself":

```cpp
#include <algorithm>
#include <vector>
std::vector<int> nums{3, 1, 2};
std::ranges::sort(nums, std::less{}, std::identity{});   // project each element to itself
// nums == {1, 2, 3}
```

Naming `std::identity` explicitly is rarely necessary — the default handles it — but it is the value that fills the projection slot, and it composes cleanly wherever a "do nothing" transformation is required.



## `std::hash` and specializing it (C++11)

The `std::hash<Key>` is the standard function object that maps a value to a `std::size_t` hash. It is what the unordered containers (`std::unordered_set`, `std::unordered_map`) use to place elements in buckets. The library provides specializations for the built-in types, pointers, and common standard types such as `std::string`:

```cpp
#include <functional>
#include <string>

std::hash<int> hi;
hi(42) == hi(42);          // true — deterministic within a single program run

std::hash<std::string> hs;
hs("hello");               // some size_t value
```

Two properties matter. A hash is *deterministic within one execution* (the same input yields the same hash while the program runs) but is **not** guaranteed to be stable across runs or across implementations, so hash values must never be persisted or relied upon for anything but in-memory bucketing. And a hash need only be *consistent with equality*: equal keys must hash equally, while unequal keys may collide.

To make a user-defined type usable as a key in an unordered container, `std::hash` is specialized for it, and the type must also provide `operator==`:

```cpp
#include <unordered_set>

struct Point {
    int x, y;
    bool operator==(const Point&) const = default;   // required for unordered lookup
};

template<>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const noexcept {
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);          // a simple combiner (illustrative, not cryptographic)
    }
};

std::unordered_set<Point> pts{ {1, 2}, {3, 4}, {1, 2} };
pts.size();                 // 2 — the duplicate {1,2} collapses
pts.contains(Point{3, 4});  // true
```

The combiner above is intentionally simple; production code often uses a stronger mix to reduce collisions, but the shape — hash each member with `std::hash` and combine — is the standard pattern.
