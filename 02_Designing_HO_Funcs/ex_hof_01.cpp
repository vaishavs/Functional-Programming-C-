#include <iostream>
#include <functional>

// Pass in a function pointer
void invoke(int x, int (*func)(int)) {
    std::cout << func(x) << std::endl;
}
// Function to pass in
int square(int n) { return n * n; }

// Template with a callback
template<typename Func>
void execute(int x, Func callback) {
    callback(x);
}

// Pass in std::function
// Accepts any callable that takes two ints and returns an int
void compute(int a, int b, std::function<int(int, int)> operation) {
    std::cout << "Result: " << operation(a, b) << std::endl;
}

// Since C++26
// Best for immediate callbacks that don't need to be stored long-term
// void process(std::function_ref<int(int)> action) {
//     std::cout << action(x) << std::endl;;
// }


int main() {
    invoke(5, square); // Passes the address of square
    execute(42, [](int n) { std::cout << n << std::endl; }); // Passes a lambda
    compute(10, 5, [](int x, int y) { return x+y; }); 

    // process(5, square);
}
