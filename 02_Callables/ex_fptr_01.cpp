#include <iostream>

int add (int a, int b)
{
    return a+b;
}

typedef int (*fptr)(int, int);

int main()
{
    std::cout << "Function pointer" << std::endl;
    // 1. Declaration and Initialization
    int (*funcPtr)(int, int) = &add;
    // 2. Invocation: Call the function via the pointer
    int result = funcPtr(10, 5); // Equivalent to add(10, 5)
    std::cout << "result = " << result << std::endl;
    
    std:: cout << "Typedef function pointer" << std::endl;
    fptr f = add;
    std::cout << "result = " << f(50, 50) << std::endl;
    
    // Comapring function pointers
    if ((f == &add) && (f == funcPtr)) {
        std::cout << "fptr points to add()." << std::endl;
    }
    return 0;
}
