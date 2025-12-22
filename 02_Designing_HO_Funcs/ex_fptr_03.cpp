#include <iostream>

// Sample functions to return
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

// Return function pointer using a type alias (Cleaner)
using MathOp = int(*)(int, int);
MathOp get_operation(char op) {
    if (op == '*') return multiply;
    return add;
}
// Return function pointer using trailing return type (No alias needed)
auto get_op_modern(char op) -> int(*)(int, int) {
    return (op == '*') ? multiply : add;
}

// Function taking a function pointer as a parameter
void process(int x, int y, int (*operation)(int, int)) {
    std::cout << "Result: " << operation(x, y) << std::endl;
}

int main()
{
    /* Return a function pointer */
    auto func = get_operation('*');
    std::cout << "Result: " << func(10, 5) << std::endl; // Outputs 50
    func = get_op_modern('+'); 
    std::cout << "Result: " << func(10, 5) << std::endl; // Outputs 15
    
    /* Jump table, aka array of function pointers */
    int (*ops[2])(int, int) = { add, multiply };
    
    int result = ops[0](10,20); // Calls add
    std::cout << "Result: " << result << std::endl; // Outputs 30
    result = ops[1](10, 20); // Calls multiply
    std::cout << "Result: " << result << std::endl; // Outputs 200

    /* Callback functions */
    process(10, 5, add);      // Pass add as a callback
    process(10, 5, multiply); // Pass multiply as a callback

    return 0;
}
