// =============================================================================
// function_objects_exercise.cpp
//
// A hands-on TODO/DEBUG exercise covering the major categories of function
// objects found in the C++ Standard Library, most of which live in the
// <functional> header.
//
// This file contains thirteen short exercises, each tagged as either:
//   [TODO] - a piece of code is missing; read the Task and Hint above the
//            placeholder, then replace the placeholder with real code.
//   [BUG]  - a full line of code is already there, but it does not satisfy
//            the Task described above it; read the Task and Hint, compare
//            them against the code that follows, find the mismatch, and
//            fix it. The comment sitting on a [BUG] line only describes
//            what that line appears to be doing, not whether it is right,
//            so the comparison against the Task is left up to you.
//
// Every exercise reports its own outcome through a small checkResult()
// helper, so you can compile and run this file at any point, even before
// you have solved everything, to see exactly which exercises still need
// work and which ones are already correct.
//
// Note: until TODO 1 is completed, the compiler will warn that factorA and
// factorB are unused; that warning is expected, and it will disappear on
// its own once your fix for TODO 1 actually uses those two variables.
//
// Build with:   g++ -std=c++17 -Wall -Wextra -o function_objects_exercise function_objects_exercise.cpp
// Run with:     ./function_objects_exercise
// =============================================================================

#include <algorithm>   // Supplies std::sort, std::count_if, and std::is_sorted, which several exercises rely on.
#include <functional>  // Supplies std::function, std::plus, std::bind, std::mem_fn, std::not_fn, std::hash, and more.
#include <iostream>    // Supplies std::cout, which is used to print every exercise's result.
#include <string>      // Supplies std::string, used for labels and for the std::hash exercise.
#include <vector>      // Supplies std::vector, which stores the sample data used by several exercises.

// A small helper used throughout this exercise to report, in a single line,
// whether a computed value matches the value it is supposed to have. It
// never stops the program, so every exercise gets checked even if an
// earlier one is still broken, and it returns whether the check passed so
// that main() can keep a running tally.
template <typename T>
bool checkResult(const std::string& label, const T& actual, const T& expected) {
    const bool passed = (actual == expected); // True only when the computed value exactly matches what was expected.
    std::cout << label << " -> got: " << actual << ", expected: " << expected
              << (passed ? "  [OK]\n" : "  [NOT YET CORRECT]\n");
    return passed;
}

// -----------------------------------------------------------------------
// [TODO 5] Custom function object.
// Task: fix this function object so that calling it returns its argument
// cubed (the value multiplied by itself three times) instead of returning
// the argument unchanged.
// Hint: a function object becomes callable by overloading operator(); the
// fix belongs entirely inside that operator, not anywhere else.
// -----------------------------------------------------------------------
struct Cube {
    int operator()(int value) const {
        return value; // TODO: replace this placeholder with an expression that returns value cubed.
    }
};

// -----------------------------------------------------------------------
// [BUG 6] Stateful custom function object.
// Task: the very first call to this function object should return 1, the
// second call should return 2, and so on — each call should return one
// more than the call before it.
// Hint: look closely at whether the internal counter is incremented before
// or after its value is captured for the return statement below.
// -----------------------------------------------------------------------
class Counter {
public:
    int operator()() {
        int current = count_; // Remembers the count as it stands right now.
        count_++;              // Updates the count for next time this object is called.
        return current;        // Reports the count that was remembered a moment ago.
    }

private:
    int count_ = 0; // Tracks how many times this function object has been called so far.
};

// A small class used by the std::mem_fn exercise later in main(). It
// represents a rectangular plot of land with a fixed width and height.
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

// A small helper predicate used by the std::not_fn exercise later in
// main(). It reports whether a given integer is evenly divisible by three.
bool isMultipleOfThree(int n) {
    return n % 3 == 0;
}

int main() {
    std::cout << std::boolalpha; // Configures std::cout to print bool values as "true" or "false" instead of 1 or 0.
    std::cout << "Running function object exercises...\n\n";

    constexpr int totalExercises = 13; // The number of exercises in this file, used only for the summary at the end.
    int passedCount = 0; // Tracks how many exercises currently report a correct result.

    // =====================================================================
    // [TODO 1] Arithmetic function objects.
    // Task: compute the product of factorA and factorB using
    // std::multiplies<int>, storing the result in `product`.
    // Hint: a standard library arithmetic function object is constructed by
    // writing its type followed by curly braces, then called by placing the
    // operands in parentheses right after it, for example std::plus<int>{}(x, y)
    // adds x and y together.
    // =====================================================================
    int factorA = 6; // The first factor to multiply.
    int factorB = 7; // The second factor to multiply.
    int product = 0; // TODO: replace 0 with std::multiplies<int>{}(factorA, factorB).
    passedCount += checkResult("[TODO 1] factorA * factorB via std::multiplies", product, 42);

    // =====================================================================
    // [BUG 2] Comparison function objects.
    // Task: sort `scores` into ascending order (lowest score first).
    // Hint: two comparison function objects that are easy to mix up are
    // std::less, which orders smallest-to-largest, and std::greater, which
    // orders largest-to-smallest.
    // =====================================================================
    std::vector<int> scores = {88, 45, 92, 67, 71}; // Exam scores in the order they were recorded.
    std::sort(scores.begin(), scores.end(), std::greater<int>{}); // Sorts the scores using std::greater as the comparator.
    passedCount += checkResult("[BUG 2] scores sorted ascending", std::is_sorted(scores.begin(), scores.end()), true);

    // =====================================================================
    // [BUG 3] Logical function objects.
    // Task: accessGranted should be true whenever the user is an
    // administrator OR has a valid token — that is, true as long as at
    // least one of the two conditions holds.
    // Hint: two logical function objects that are easy to mix up are
    // std::logical_and, which requires every operand to be true, and
    // std::logical_or, which only requires at least one operand to be true.
    // =====================================================================
    bool isAdministrator = false; // Whether the current user has administrator privileges.
    bool hasValidToken = true;    // Whether the current user supplied a valid access token.
    bool accessGranted = std::logical_and<bool>{}(isAdministrator, hasValidToken); // Combines the two conditions using std::logical_and.
    passedCount += checkResult("[BUG 3] accessGranted (admin OR valid token)", accessGranted, true);

    // =====================================================================
    // [BUG 4] Bitwise function objects.
    // Task: combinedPermissions should contain every bit that is set in
    // either READ_PERMISSION or WRITE_PERMISSION — that is, the two flags
    // should be combined together, not restricted to only the bits they share.
    // Hint: two bitwise function objects that are easy to mix up are
    // std::bit_and, which keeps only the bits set in both operands, and
    // std::bit_or, which keeps every bit set in either operand.
    // =====================================================================
    constexpr unsigned READ_PERMISSION = 0b001;  // Represents permission to read a file.
    constexpr unsigned WRITE_PERMISSION = 0b010; // Represents permission to write to a file.
    unsigned combinedPermissions = std::bit_and<unsigned>{}(READ_PERMISSION, WRITE_PERMISSION); // Combines the two permission flags using std::bit_and.
    passedCount += checkResult("[BUG 4] combinedPermissions (read + write)", static_cast<int>(combinedPermissions), 0b011);

    // =====================================================================
    // [TODO 5] Custom function object (see the Cube struct defined above main).
    // Task: once Cube is fixed, calling it on 4 should return 64.
    // =====================================================================
    Cube cubeIt; // Creates an instance of the Cube function object.
    passedCount += checkResult("[TODO 5] cubeIt(4)", cubeIt(4), 64);

    // =====================================================================
    // [BUG 6] Stateful custom function object (see the Counter class
    // defined above main). Calling callTally three times in a row should
    // yield 1, then 2, then 3.
    // =====================================================================
    Counter callTally; // Creates an instance of the stateful Counter function object.
    callTally();                       // First call; its result is intentionally not checked here.
    callTally();                       // Second call; its result is intentionally not checked here.
    int thirdCallResult = callTally(); // Third call; this is the result being checked below.
    passedCount += checkResult("[BUG 6] callTally's third call result", thirdCallResult, 3);

    // =====================================================================
    // [BUG 7] std::function.
    // Task: after priceCalculator is reassigned to a new callable,
    // finalResult should reflect that NEW behavior applied to a price of 50.
    // Hint: once a std::function has been reassigned to hold a different
    // callable, any value calculated earlier with the OLD callable is now
    // out of date; think about whether the code actually calls
    // priceCalculator again after the reassignment.
    // =====================================================================
    std::function<int(int)> priceCalculator = [](int price) { return price * 2; }; // Doubles the price it is given.
    int firstResult = priceCalculator(50); // Calls priceCalculator while it still doubles its input.
    priceCalculator = [](int price) { return price + 10; }; // Reassigns priceCalculator to add a flat 10 instead of doubling.
    int finalResult = firstResult; // Uses firstResult as the value to report.
    passedCount += checkResult("[BUG 7] finalResult after reassigning priceCalculator", finalResult, 60);

    // =====================================================================
    // [BUG 8] std::bind.
    // Task: subtractFive should take one number and return that number
    // minus five (for example, subtractFive(20) should return 15).
    // Hint: std::minus<int>{}(a, b) computes a - b, not b - a; pay attention
    // to which value ends up in which position relative to the placeholder
    // in the std::bind call below.
    // =====================================================================
    auto subtractFive = std::bind(std::minus<int>{}, 5, std::placeholders::_1); // Binds std::minus with 5 in the first position and the placeholder in the second position.
    passedCount += checkResult("[BUG 8] subtractFive(20)", subtractFive(20), 15);

    // =====================================================================
    // [TODO 9] std::mem_fn.
    // Task: use std::mem_fn to wrap Rectangle's area() member function into
    // a standalone callable, then invoke that callable on `plot` to compute
    // its area, storing the result in `area`.
    // Hint: std::mem_fn takes a pointer to a member function, formed by
    // writing an ampersand immediately before the fully qualified member
    // function name, such as &ClassName::methodName.
    // =====================================================================
    Rectangle plot(6.0, 4.0); // Represents a rectangular plot of land 6 units wide and 4 units tall.
    double area = 0.0; // TODO: replace 0.0 with (std::mem_fn(&Rectangle::area))(plot).
    passedCount += checkResult("[TODO 9] area of plot via std::mem_fn", area, 24.0);

    // =====================================================================
    // [BUG 10] std::not_fn.
    // Task: notMultiplesOfThree should count how many bins in `warehouse`
    // are NOT evenly divisible by three.
    // Hint: std::not_fn wraps an existing predicate and flips its result;
    // consider whether that wrapper needs to appear somewhere in the
    // std::count_if call below for the count to reflect "not a multiple of
    // three" rather than "a multiple of three".
    // =====================================================================
    std::vector<int> warehouse = {1, 2, 3, 4, 5, 6, 7, 8, 9}; // Represents nine bins in a warehouse, numbered 1 through 9.
    int notMultiplesOfThree = std::count_if(warehouse.begin(), warehouse.end(), isMultipleOfThree); // Counts how many bins satisfy isMultipleOfThree.
    passedCount += checkResult("[BUG 10] bins that are NOT multiples of three", notMultiplesOfThree, 6);

    // =====================================================================
    // [TODO 11] std::hash.
    // Task: create a std::hash function object specialized for std::string,
    // then use it to compute the hash of the word "orbit" twice, storing
    // the results in hashFirstCall and hashSecondCall.
    // Hint: declare the function object with std::hash<std::string> hasher;
    // and then call it the same way you would call any other function
    // object, by writing hasher("orbit").
    // =====================================================================
    std::size_t hashFirstCall = 0;  // TODO: replace 0 with a std::hash<std::string> call on "orbit".
    std::size_t hashSecondCall = 1; // TODO: replace 1 with another std::hash<std::string> call on "orbit".
    passedCount += checkResult("[TODO 11] std::hash gives the same result for the same input", hashFirstCall == hashSecondCall, true);

    // =====================================================================
    // [BUG 12] std::reference_wrapper via std::ref.
    // Task: calling castVote should increase the ORIGINAL voteCount
    // variable declared below, so that after three calls, voteCount equals 3.
    // Hint: by default, std::bind stores its own private copy of each
    // argument it is given; wrapping an argument in std::ref tells
    // std::bind to store a reference to the original variable instead of a
    // copy of it. Consider whether voteCount is passed to std::bind as-is
    // or wrapped in std::ref below.
    // =====================================================================
    int voteCount = 0; // Represents the number of votes cast so far.
    auto incrementVote = [](int& n) { ++n; }; // A lambda that increments its argument through a reference.
    auto castVote = std::bind(incrementVote, voteCount); // Binds incrementVote together with voteCount so that castVote can be called with no further arguments.
    castVote(); // Casts one vote.
    castVote(); // Casts a second vote.
    castVote(); // Casts a third vote.
    passedCount += checkResult("[BUG 12] voteCount after three calls to castVote", voteCount, 3);

    // =====================================================================
    // [TODO 13] Lambda expressions as function objects.
    // Task: write a lambda that takes a single int and returns true when
    // that value is a multiple of five, then use it with std::count_if to
    // count how many multiples of five appear in `readings`, storing the
    // result in multiplesOfFive.
    // Hint: a lambda taking one int parameter and returning a bool looks
    // like [](int n) { return /* boolean expression involving n */; };
    // remember that the % operator gives the remainder of a division.
    // =====================================================================
    std::vector<int> readings = {5, 12, 15, 20, 23, 30, 41}; // A set of sample sensor readings.
    int multiplesOfFive = 0; // TODO: replace 0 with std::count_if(readings.begin(), readings.end(), <your lambda here>).
    passedCount += checkResult("[TODO 13] multiples of five in readings", multiplesOfFive, 4);

    // =====================================================================
    // Summary.
    // =====================================================================
    std::cout << "\n" << passedCount << " / " << totalExercises << " exercises currently correct.\n"; // Reports overall progress across every exercise above.
    std::cout << "Fix a TODO or BUG, then recompile and rerun to see your progress update.\n";

    return 0; // Indicates that the program finished successfully, regardless of how many exercises are still unsolved.
}
