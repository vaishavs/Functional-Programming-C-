// =============================================================================
// This program demonstrates the major categories of function objects found
// in the C++ Standard Library, most of which live in the <functional> header.
// It also includes two hand-written functors and a couple of lambda
// expressions so the built-in function objects can be compared against the
// two most common alternatives.
//
// Build with:   g++ -std=c++17 -Wall -Wextra -o ex_std_functors ex_std_functors.cpp
// Run with:     ./ex_std_functors
// =============================================================================

#include <algorithm>   // Supplies std::sort, std::transform, and std::count_if, which all accept function objects.
#include <functional>  // Supplies std::function, std::plus, std::bind, std::mem_fn, std::not_fn, std::hash, and more.
#include <iostream>    // Supplies std::cout, which is used throughout the program to display results.
#include <numeric>     // Supplies std::accumulate, which combines the elements of a range using a function object.
#include <string>      // Supplies std::string, which appears in the std::hash demonstration near the end.
#include <vector>      // Supplies std::vector, which stores the sample data used by most of the examples.

// A hand-written function object, often called a functor. Overloading
// operator() is precisely what makes an instance of this struct callable
// just like an ordinary function.
struct Square {
    // This call operator lets an instance of Square be invoked with a single integer argument.
    int operator()(int value) const {
        return value * value;
    }
};

// A second hand-written function object that keeps state between calls,
// something an ordinary free function cannot do without resorting to a
// global variable.
class RunningTotal {
public:
    // This call operator adds the given value to an internal accumulator and returns the updated total.
    int operator()(int value) {
        total_ += value;
        return total_;
    }

private:
    int total_ = 0; // Stores the cumulative sum across every call made so far.
};

// A small class used later to demonstrate std::mem_fn, which turns a member
// function into a standalone function object that takes the owning object
// as its first argument.
class Rectangle {
public:
    // The constructor stores the width and height that describe this rectangle.
    Rectangle(double width, double height) : width_(width), height_(height) {}

    // This member function calculates and returns the rectangle's area.
    double area() const { return width_ * height_; }

private:
    double width_;  // Stores the rectangle's width.
    double height_; // Stores the rectangle's height.
};

// An ordinary free function, included to show that std::function and
// std::not_fn both work with plain functions just as well as with functors.
bool isEven(int value) {
    return value % 2 == 0;
}

int main() {
    std::cout << std::boolalpha; // Configures std::cout to print bool values as "true" or "false" instead of 1 or 0.

    // -------------------------------------------------------------------
    // Arithmetic function objects.
    // std::plus, std::minus, std::multiplies, std::divides, std::modulus,
    // and std::negate each wrap one built-in arithmetic operator behind a
    // callable interface, which is convenient whenever an algorithm asks
    // for a function object rather than an operator symbol.
    // -------------------------------------------------------------------
    std::cout << "-- Arithmetic function objects --\n";
    std::cout << "std::plus<int>{}(4, 3)       = " << std::plus<int>{}(4, 3) << '\n';       // Adds the two operands together to produce 7.
    std::cout << "std::minus<int>{}(4, 3)      = " << std::minus<int>{}(4, 3) << '\n';      // Subtracts the second operand from the first to produce 1.
    std::cout << "std::multiplies<int>{}(4, 3) = " << std::multiplies<int>{}(4, 3) << '\n'; // Multiplies the two operands together to produce 12.
    std::cout << "std::divides<int>{}(12, 3)   = " << std::divides<int>{}(12, 3) << '\n';   // Divides the first operand by the second to produce 4.
    std::cout << "std::modulus<int>{}(10, 3)   = " << std::modulus<int>{}(10, 3) << '\n';   // Computes the remainder of the division to produce 1.
    std::cout << "std::negate<int>{}(5)        = " << std::negate<int>{}(5) << '\n';        // Returns the arithmetic negation of the single operand, producing -5.

    // Passing std::plus explicitly to std::accumulate makes the combining
    // operation visible at the call site instead of relying on a default.
    std::vector<int> values = {1, 2, 3, 4, 5}; // A small sample vector reused by several examples below.
    int sum = std::accumulate(values.begin(), values.end(), 0, std::plus<int>{}); // Folds the vector down to a single sum using std::plus as the combining step.
    std::cout << "Sum of {1,2,3,4,5} via std::accumulate: " << sum << "\n\n";

    // -------------------------------------------------------------------
    // Comparison function objects.
    // std::equal_to, std::not_equal_to, std::greater, std::less,
    // std::greater_equal, and std::less_equal are the building blocks that
    // ordered containers such as std::map and std::set use internally, and
    // they can also be supplied explicitly to algorithms like std::sort.
    // -------------------------------------------------------------------
    std::cout << "-- Comparison function objects --\n";
    std::cout << "std::equal_to<int>{}(5, 5)      = " << std::equal_to<int>{}(5, 5) << '\n';      // True, because 5 is equal to 5.
    std::cout << "std::not_equal_to<int>{}(5, 6)  = " << std::not_equal_to<int>{}(5, 6) << '\n';  // True, because 5 is not equal to 6.
    std::cout << "std::greater<int>{}(9, 4)       = " << std::greater<int>{}(9, 4) << '\n';       // True, because 9 is greater than 4.
    std::cout << "std::less<int>{}(9, 4)          = " << std::less<int>{}(9, 4) << '\n';          // False, because 9 is not less than 4.
    std::cout << "std::greater_equal<int>{}(4, 4) = " << std::greater_equal<int>{}(4, 4) << '\n'; // True, because 4 is greater than or equal to 4.
    std::cout << "std::less_equal<int>{}(4, 5)    = " << std::less_equal<int>{}(4, 5) << '\n';    // True, because 4 is less than or equal to 5.

    // std::greater is a common way to ask std::sort for descending order
    // instead of the ascending order it produces by default.
    std::vector<int> numbers = {5, 2, 8, 1, 9}; // An unsorted vector used to demonstrate sorting with a custom comparator.
    std::sort(numbers.begin(), numbers.end(), std::greater<int>{}); // Sorts the vector from largest to smallest using std::greater as the comparator.
    std::cout << "Sorted descending with std::greater: ";
    for (int n : numbers) {
        std::cout << n << ' '; // Prints each element of the now-descending vector, separated by a space.
    }
    std::cout << "\n\n";

    // -------------------------------------------------------------------
    // Logical function objects.
    // std::logical_and, std::logical_or, and std::logical_not mirror the
    // built-in &&, ||, and ! operators but package them as callable objects.
    // -------------------------------------------------------------------
    std::cout << "-- Logical function objects --\n";
    std::cout << "std::logical_and<bool>{}(true, false) = " << std::logical_and<bool>{}(true, false) << '\n'; // False, because both operands must be true for logical AND to succeed.
    std::cout << "std::logical_or<bool>{}(true, false)  = " << std::logical_or<bool>{}(true, false) << '\n';  // True, because at least one operand is true.
    std::cout << "std::logical_not<bool>{}(true)        = " << std::logical_not<bool>{}(true) << "\n\n";      // False, because logical NOT inverts a true value to false.

    // -------------------------------------------------------------------
    // Bitwise function objects.
    // std::bit_and, std::bit_or, std::bit_xor, and std::bit_not mirror the
    // built-in &, |, ^, and ~ operators respectively.
    // -------------------------------------------------------------------
    std::cout << "-- Bitwise function objects --\n";
    std::cout << "std::bit_and<unsigned>{}(0b1100, 0b1010) = " << std::bit_and<unsigned>{}(0b1100, 0b1010) << '\n'; // Performs a bitwise AND, producing 0b1000, which is 8 in decimal.
    std::cout << "std::bit_or<unsigned>{}(0b1100, 0b1010)  = " << std::bit_or<unsigned>{}(0b1100, 0b1010) << '\n';  // Performs a bitwise OR, producing 0b1110, which is 14 in decimal.
    std::cout << "std::bit_xor<unsigned>{}(0b1100, 0b1010) = " << std::bit_xor<unsigned>{}(0b1100, 0b1010) << '\n'; // Performs a bitwise XOR, producing 0b0110, which is 6 in decimal.
    std::cout << "std::bit_not<unsigned char>{}(0b00001100) = "
              << static_cast<int>(std::bit_not<unsigned char>{}(0b00001100)) << "\n\n"; // Flips every one of the 8 bits, turning 0b00001100 into 0b11110011, which is 243 in decimal.

    // -------------------------------------------------------------------
    // A custom function object: Square.
    // Because Square overloads operator(), it can be passed anywhere the
    // standard library expects a callable, exactly like the built-in
    // function objects demonstrated above.
    // -------------------------------------------------------------------
    std::cout << "-- Custom function object (Square) --\n";
    Square square; // Creates an instance of the custom Square function object.
    std::cout << "square(6) = " << square(6) << '\n'; // Invokes the Square functor directly, producing 36.

    std::vector<int> squaredValues(values.size()); // Allocates a destination vector with the same length as values.
    std::transform(values.begin(), values.end(), squaredValues.begin(), Square{}); // Applies the Square functor to every element of values, writing the results into squaredValues.
    std::cout << "Squared values: ";
    for (int n : squaredValues) {
        std::cout << n << ' '; // Prints each squared value in turn.
    }
    std::cout << "\n\n";

    // -------------------------------------------------------------------
    // A stateful custom function object: RunningTotal.
    // Unlike an ordinary function, this functor remembers information
    // between calls, which is only possible because it is backed by an
    // object with member data rather than a bare function.
    // -------------------------------------------------------------------
    std::cout << "-- Stateful function object (RunningTotal) --\n";
    RunningTotal runningTotal; // Creates an instance of the stateful RunningTotal function object.
    for (int n : values) {
        std::cout << "After adding " << n << ", running total = " << runningTotal(n) << '\n'; // Calls the functor and prints the updated cumulative total after each call.
    }
    std::cout << '\n';

    // -------------------------------------------------------------------
    // std::function.
    // This is a polymorphic wrapper that can hold any callable matching a
    // given signature, whether that callable is a free function, a functor,
    // or a lambda expression.
    // -------------------------------------------------------------------
    std::cout << "-- std::function --\n";
    std::function<int(int, int)> operation; // Declares a wrapper capable of holding any callable that takes two ints and returns an int.

    operation = std::plus<int>{}; // Stores the std::plus function object inside the wrapper.
    std::cout << "operation(3, 4) holding std::plus = " << operation(3, 4) << '\n'; // Invokes the wrapped std::plus functor, producing 7.

    operation = [](int a, int b) { return a * b; }; // Replaces the stored callable with a lambda that multiplies its two arguments.
    std::cout << "operation(3, 4) holding a lambda   = " << operation(3, 4) << "\n\n"; // Invokes the wrapped lambda, producing 12.

    // -------------------------------------------------------------------
    // std::bind.
    // This utility creates a new callable by fixing some of the arguments
    // of an existing callable, while leaving the remaining ones to be
    // supplied when the new callable is eventually invoked.
    // -------------------------------------------------------------------
    std::cout << "-- std::bind --\n";
    auto addTen = std::bind(std::plus<int>{}, std::placeholders::_1, 10); // Binds the second argument of std::plus to 10, leaving the first argument open via the placeholder.
    std::cout << "addTen(5) = " << addTen(5) << "\n\n"; // Supplies 5 as the remaining argument, producing 15.

    // -------------------------------------------------------------------
    // std::mem_fn.
    // This utility adapts a pointer to a member function into a function
    // object that takes the owning instance as its first argument.
    // -------------------------------------------------------------------
    std::cout << "-- std::mem_fn --\n";
    Rectangle rectangle(3.0, 4.0); // Creates a Rectangle with a width of 3 and a height of 4.
    auto getArea = std::mem_fn(&Rectangle::area); // Wraps the Rectangle::area member function so it can be invoked like an ordinary function.
    std::cout << "getArea(rectangle) = " << getArea(rectangle) << "\n\n"; // Calls the wrapped member function on rectangle, producing 12.

    // -------------------------------------------------------------------
    // std::not_fn (introduced in C++17).
    // This utility wraps an existing callable and returns a new function
    // object whose result is the logical negation of the original one.
    // -------------------------------------------------------------------
    std::cout << "-- std::not_fn --\n";
    auto isOdd = std::not_fn(isEven); // Builds a new function object that returns the opposite boolean result of isEven.
    std::cout << "isOdd(7) = " << isOdd(7) << '\n'; // Calls the negated function object, producing true because 7 is not even.

    int oddCount = std::count_if(values.begin(), values.end(), isOdd); // Counts how many elements of values satisfy the isOdd predicate.
    std::cout << "Odd numbers in {1,2,3,4,5}: " << oddCount << "\n\n"; // Reports the count, which should be 3 for the values 1, 3, and 5.

    // -------------------------------------------------------------------
    // std::hash.
    // This function object produces a hash value for its argument, and it
    // is exactly what unordered containers such as std::unordered_map use
    // internally to decide which bucket an element belongs to.
    // -------------------------------------------------------------------
    std::cout << "-- std::hash --\n";
    std::hash<std::string> stringHasher; // Creates a hash function object specialized for hashing std::string values.
    std::cout << "std::hash<std::string>{}(\"hello\") = " << stringHasher("hello") << "\n\n"; // Computes and prints an implementation-defined hash value for the string "hello".

    // -------------------------------------------------------------------
    // std::reference_wrapper, produced by std::ref and std::cref.
    // A reference wrapper lets a reference be copied and stored like a
    // value, which matters because std::bind and std::function normally
    // copy their arguments rather than capturing them by reference.
    // -------------------------------------------------------------------
    std::cout << "-- std::reference_wrapper (std::ref) --\n";
    int counter = 0; // A plain integer that will be modified indirectly through a reference wrapper.
    auto increment = [](int& n) { ++n; }; // A lambda that increments its argument through a reference.

    std::function<void()> incrementCounter = std::bind(increment, std::ref(counter)); // Binds increment to a reference wrapper around counter so the original variable is modified rather than a copy.
    incrementCounter(); // Invokes the bound callable, which increments counter through the reference wrapper.
    incrementCounter(); // Invokes it a second time, incrementing counter once more.
    std::cout << "counter after two increments via std::ref = " << counter << "\n\n"; // Confirms that counter itself changed, since the output is 2 rather than 0.

    // -------------------------------------------------------------------
    // Lambda expressions.
    // A lambda is not technically a standard library entity, but each one
    // compiles down to an unnamed class with its own operator(), which
    // makes a lambda a function object in exactly the same sense as
    // Square or RunningTotal above, just written inline and without a name.
    // -------------------------------------------------------------------
    std::cout << "-- Lambda expressions as function objects --\n";
    auto isGreaterThanThree = [](int n) { return n > 3; }; // Defines an inline function object that tests whether a value exceeds 3.
    int countAboveThree = std::count_if(values.begin(), values.end(), isGreaterThanThree); // Counts how many elements of values satisfy the lambda's condition.
    std::cout << "Values greater than 3 in {1,2,3,4,5}: " << countAboveThree << '\n'; // Reports the count, which should be 2 for the values 4 and 5.

    return 0; // Indicates that the program finished successfully.
}
