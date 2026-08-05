# Extending an adaptor
Internally, a Boost.Range adaptor is constructed by combining a custom iterator, a range that wraps those iterators, and a pipe operator `|`, which allows the `range | adaptor` syntax to be utilized.

The full machinery involves:
* holder types
* pipe operator overloading
* adapted iterator
* range concept compliance.

To illustrate this process, let us take a custom adaptor named `add_value` that takes a range of numbers and adds a specific constant to each element lazily as the range is traversed.

#### Step 1: Defining the Underlying Iterator
Because a range is defined by its iterators, an iterator must be written to wrap the original iterator and modify the behavior when dereferencing occurs. The `boost::iterator_adaptor` is utilized for this purpose. The boilerplate code (incrementing, comparing, etc.) is handled by this utility, so only the `dereference()` method needs to be overridden. (The `boost::iterator_facade` could also be used instead of `boost::iterator_adaptor`, depending on the use case).

```cpp
#include <boost/iterator/iterator_adaptor.hpp>

// Template on the underlying Iterator and the Type of value to add
template <class Iterator, class ValueType>
class add_value_iterator
    : public boost::iterator_adaptor<
        add_value_iterator<Iterator, ValueType>, // CRTP: Pass derived class
        Iterator,                                // Base iterator
        ValueType,                               // Value type
        boost::use_default,                      // Category (inherit from Base)
        ValueType                                // Reference type! (Return by value)
      >
{
public:
    // Default constructor is required for iterators
    add_value_iterator() {}

    // Constructor taking the base iterator and the value to add
    add_value_iterator(Iterator base, ValueType val)
        : add_value_iterator::iterator_adaptor_(base), m_val(val) {}

private:
    // Grant access to boost::iterator_adaptor for core operations
    friend class boost::iterator_core_access;

    ValueType m_val;

    // The dereference is intercepted here.
    ValueType dereference() const {
        return *(this->base()) + m_val;
    }
};
```
**Note:** The fifth template parameter for `iterator_adaptor` is set to `ValueType` rather than `ValueType&`. Because a new value is calculated on the fly (`*base + val`), it must be returned by value.

#### Step 2: Creating the Adaptor "Holder" (The Tag)
When the expression `rng | add_value(5)` is processed, `add_value(5)` is evaluated first. A temporary object needs to be returned to hold the state (the number `5`) until it can be bound to the range by the pipe operator.
```cpp
// The holder struct
template <typename ValueType>
struct add_value_holder {
    ValueType val;
    add_value_holder(ValueType v) : val(v) {}
};

// The generator function
template <typename ValueType>
add_value_holder<ValueType> add_value(ValueType v) {
    return add_value_holder<ValueType>(v);
}
```

#### Step 3: Overloading the Pipe Operator (operator `|`)
The interaction between a Range and the Holder is defined by overloading operator `|`. A range is taken on the left and an `add_value_holder` is taken on the right. A `boost::iterator_range`, populated with the custom iterators, is then returned.
```cpp
#include <boost/range/iterator_range.hpp>

// Non-const range version
template <typename SinglePassRange, typename ValueType>
inline boost::iterator_range<
    add_value_iterator<typename boost::range_iterator<SinglePassRange>::type, ValueType>
>
operator|(SinglePassRange& rng, const add_value_holder<ValueType>& holder) {
    
    typedef typename boost::range_iterator<SinglePassRange>::type iterator_t;
    typedef add_value_iterator<iterator_t, ValueType> custom_iter_t;

    return boost::make_iterator_range(
        custom_iter_t(boost::begin(rng), holder.val),
        custom_iter_t(boost::end(rng), holder.val)
    );
}

// Const range version
template <typename SinglePassRange, typename ValueType>
inline boost::iterator_range<
    add_value_iterator<typename boost::range_iterator<const SinglePassRange>::type, ValueType>
>
operator|(const SinglePassRange& rng, const add_value_holder<ValueType>& holder) {
    
    typedef typename boost::range_iterator<const SinglePassRange>::type iterator_t;
    typedef add_value_iterator<iterator_t, ValueType> custom_iter_t;

    return boost::make_iterator_range(
        custom_iter_t(boost::begin(rng), holder.val),
        custom_iter_t(boost::end(rng), holder.val)
    );
}
```

#### Step 4: Verifying the Implementation
The custom adaptor can now be used in the same manner as `boost::adaptors::transformed`.
```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // The custom adaptor is applied!
    auto adapted_range = numbers | add_value(10);

    for (int n : adapted_range) {
        std::cout << n << " ";
    }
    // Output: 11 12 13 14 15 

    std::cout << "\n";
    
    // The original range remains unmodified
    for (int n : numbers) {
         std::cout << n << " ";
    }
    // Output: 1 2 3 4 5

    return 0;
}
```

The full implementation looks like this:
```cpp
#include <iostream>
#include <vector>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/iterator_range.hpp>

// ==========================================================
// 1. THE ITERATOR
// ==========================================================
template <typename Iterator, typename Value>
class scaled_iterator
    : public boost::iterator_adaptor<
        scaled_iterator<Iterator, Value>, // Derived (The CRTP part)
        Iterator,                         // Base iterator
        Value,                            // Value type
        boost::use_default,               // Category
        Value                             // Reference (Return by value for math)
      >
{
public:
    // Default constructor
    scaled_iterator() : scaled_iterator::iterator_adaptor_(), m_scale(1) {}

    // Constructor with base iterator and scale factor
    scaled_iterator(Iterator it, Value scale)
        : scaled_iterator::iterator_adaptor_(it), m_scale(scale) {}

private:
    friend class boost::iterator_core_access;

    // The logic that resolves the "expected class-name" issue 
    // by being correctly nested in the class body.
    Value dereference() const {
        return (*this->base()) * m_scale;
    }

    Value m_scale;
};

// ==========================================================
// 2. THE ADAPTOR TAG & PIPE OPERATOR
// ==========================================================
template <typename Value>
struct scale_holder {
    Value scale;
    scale_holder(Value v) : scale(v) {}
};

// Generator function
template <typename Value>
scale_holder<Value> scaled_by(Value v) {
    return scale_holder<Value>(v);
}

// Pipe operator for ranges
template <typename Range, typename Value>
auto operator|(Range& rng, const scale_holder<Value>& holder) {
    using iter_t = typename boost::range_iterator<Range>::type;
    using custom_iter_t = scaled_iterator<iter_t, Value>;

    return boost::make_iterator_range(
        custom_iter_t(boost::begin(rng), holder.scale),
        custom_iter_t(boost::end(rng), holder.scale)
    );
}

// Pipe operator for const/r-value ranges
template <typename Range, typename Value>
auto operator|(const Range& rng, const scale_holder<Value>& holder) {
    using iter_t = typename boost::range_iterator<const Range>::type;
    using custom_iter_t = scaled_iterator<iter_t, Value>;

    return boost::make_iterator_range(
        custom_iter_t(boost::begin(rng), holder.scale),
        custom_iter_t(boost::end(rng), holder.scale)
    );
}

// ==========================================================
// 3. USAGE
// ==========================================================
int main() {
    std::vector<int> data = {1, 2, 3, 4, 5};

    // Using the "scaled_by" adaptor
    auto result = data | scaled_by(10);

    for (int val : result) {
        std::cout << val << " "; // Output: 10 20 30 40 50
    }

    return 0;
}
```

Sources:
* [Extending Boost.Range](https://www.boost.org/doc/libs/latest/libs/range/doc/html/range/reference/extending.html)
* [Using boost::range](https://www.boost.org/doc/libs/latest/libs/numeric/odeint/doc/html/boost_numeric_odeint/odeint_in_detail/using_boost__range.html)
* [Boost Range for humans](https://www.caichinger.com/boost-range/index.html)
