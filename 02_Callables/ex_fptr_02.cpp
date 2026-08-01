#include <iostream>

class Calculator {
public:
    int add(int a, int b) { return a + b; }
    // ...
};

// Member function pointer
int (Calculator::*ptr)(int, int);

int main()
{
    Calculator calc;
    // Pointer to a member function of class Calculator
    //Equivalent to int (Calculator::*ptr)(int, int) = &Calculator::add;
    ptr = &Calculator::add;
    
    // Call using an object instance
    int res = (calc.*ptr)(5, 5); // Evaluates to 10
    std::cout << "res = " << res << std::endl;
    return 0;
}
