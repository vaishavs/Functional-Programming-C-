/* Returning a function pointer */
#include <iostream>

// Sample functions to return
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

// 1. Using a type alias (Cleaner)
using MathOp = int(*)(int, int);
MathOp get_operation(char op)
{
    if (op == '*') return multiply;
    return add;
}

// 2. Using trailing return type (No alias needed)
auto get_op_modern(char op) -> int(*)(int, int)
{
    return (op == '*') ? multiply : add;
}

int main() {
    auto func = get_operation('*'); 
    std::cout << "Result: " << func(10, 5) << std::endl; // Outputs 50

    func = get_op_modern('+'); 
    std::cout << "Result: " << func(10, 5) << std::endl; // Outputs 15
    
    return 0;
}
