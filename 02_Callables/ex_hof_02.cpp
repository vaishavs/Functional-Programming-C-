/* This program demonstrates returning a function. */
#include <functional>
#include <iostream>

// Functor class
struct Multiplier {
    int factor;
    int operator()(int val) const { return val * factor; }
};

// Returns a std::function that takes an int and returns an int
std::function<int(int)> adder(int factor) {
    return [factor](int x) { return x + factor; }; // Returns a lambda
}

// Returns a functor
auto multiply(int x) {
    return Multiplier{std::move(x)}; 
}


// Sample function to return
int add(int a, int b) {
    return a+b;
}

// Return a function pointer to add
int (*getOperation())(int, int) {
    return add;
}
/*
// Alt 1:
// Returns a pointer to a function taking (int, int) and returning int
auto getOperation() -> int(*)(int, int) {
    return add;
}

// Alt 2:
using MathFunc = int(*)(int, int);
MathFunc getOperation() {
    return add;
}
 */

// Return a function reference to add
int(&getOp())(int, int) {
    return add;
}
// Alt definitions are similar to that of function pointer


// Since C++26
// Safe because 'add' has static storage duration
// std::function_ref<int(int, int)> getGlobalOp() {
//     return add; 
// }

/*
// DANGEROUS: Returns a reference to a temporary lambda
std::function_ref<int(int)> getBadOp() {
    auto local_lambda = [](int x) { return x * 2; };
    return local_lambda; // ERROR: local_lambda is destroyed here!
}
 */

int main() {
    auto addIt = adder(5);
    std::cout << addIt(5) << std::endl; // Outputs: 10

    auto myOp = getOperation(); // Returns pointer to add
    std::cout << myOp(10, 5) << std::endl; // Outputs: 15

    auto addOp = getOp();
    std::cout << addOp(10, 15) << std::endl; // Outputs: 25

    auto mul = multiply(5);
    std::cout << mul(10) << std::endl; // Outputs: 50

    // Since C++26
    // auto op = getGlobalOp();
    // std::cout << op(10, 20); // Outputs: 30
    // auto op = getBadOp(); 
    // op(5); // UNDEFINED BEHAVIOR: accessing a destroyed object
}
