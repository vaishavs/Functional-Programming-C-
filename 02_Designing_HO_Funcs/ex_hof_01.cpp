/* This program demonstrates passing a function to another function. */
#include <iostream>
#include <functional>

// Functor class
struct Multiplier {
    int factor;
    void operator()(int val) const { std::cout << val * factor << std::endl; }
};

// Pass in a function pointer
void invoke(int x, int (*func)(int)) {
    std::cout << func(x) << std::endl;
}

// Pass in a reference to a function
void call(int x, int (&func)(int)) {
    std::cout << func(x) << std::endl;
}

// Function to pass in
int square(int n) { return n * n; }

// Template with a callback
template<typename Func>
void execute(int x, Func callback) { // Accepts any callable entity with a matching signature
    callback(x);
}

// Pass in std::function
// Accepts any callable that takes two ints and returns an int
void compute(int a, int b, std::function<int(int, int)> operation) {
    std::cout << "Result: " << operation(a, b) << std::endl;
}

// Since C++26
// Pass in std::function_ref
// Best for immediate callbacks that don't need to be stored long-term
// void process(std::function_ref<int(int)> action) {
//     std::cout << action(x) << std::endl;;
// }


int main() {
    // Passes the address of square
    invoke(5, square);
    call(3, square);

    // Passes a lambda
    compute(10, 5, [](int x, int y) { return x+y; });

    // Passes the functor
    Multiplier triple{3};
    execute(10, triple);

    // Passes a reference to square; a  lambda can also be used.
    // process(5, square);
}
