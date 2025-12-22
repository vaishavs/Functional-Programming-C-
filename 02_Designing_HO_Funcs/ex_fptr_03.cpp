/* Passing function pointers as callbacks */
#include <iostream>

// Standard functions
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

// Function taking a function pointer as a parameter
void process(int x, int y, int (*operation)(int, int)) {
    std::cout << "Result: " << operation(x, y) << std::endl;
}

int main() {
    process(10, 5, add);      // Pass add as a callback
    process(10, 5, multiply); // Pass multiply as a callback
    return 0;
}
