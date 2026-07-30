/*
Bug 1 (Memory Management in Lambda): 
  The lambda function incorrectly captures the local variable sum by reference (&sum) rather than by value (sum) in the second transform_vector call, 
  leading to undefined behavior as the reference becomes dangling.
Bug 2 (Function Signature Mismatch): 
  The function transform_vector is designed to take a unary function (one argument), but the main function attempts to pass a binary (two-argument) function to it, 
  which will cause a compilation error or unexpected behavior if a compatible overload is forced.
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

// A buggy higher-order function that applies a transformation to a vector.
// The transformation function should take a single element (int) and return a new element (int).
void transform_vector(std::vector<int>& data, std::function<int(int)> transform_func) {
    // This loop modifies the vector in-place.
    for (int i = 0; i < data.size(); ++i) {
        data[i] = transform_func(data[i]);
    }
}

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // --- First Usage (Correct, but simple) ---
    // Double each number in the vector.
    transform_vector(numbers, [](int n) {
        return n * 2;
    });

    std::cout << "Doubled numbers: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << "\n"; // Output: Doubled numbers: 2 4 6 8 10

    // --- Second Usage (BUG 1: Dangling reference in lambda capture) ---
    int sum = 10;
    // The capture [&sum] is dangerous if the lambda outlives 'sum' (it doesn't here, but it's poor practice)
    // More importantly, the intent of 'sum' is unclear here. Let's assume the user meant to add 10 to each.
    // The logic is flawed: it keeps adding to the original 'sum' variable, creating an escalating result.
    transform_vector(numbers, [&sum](int n) {
        sum += n; // This line has side effects on the captured variable 'sum' in an unexpected way
        return n + sum;
    });
    // The above call results in unpredictable/escalating sums added to each element, depending on the loop order and state of 'sum'.

    std::cout << "Numbers after buggy second transform: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << "\n";

    // --- Third Usage (BUG 2: Function signature mismatch) ---
    // This part will cause a *compilation error* because the signature
    // `transform_vector` expects a unary function (int -> int), but we
    // try to pass a binary function (int, int -> int).

    auto buggy_binary_op = [](int a, int b) {
        return a * b;
    };
    // This line would fail to compile:
    // transform_vector(numbers, buggy_binary_op);

    return 0;
}
