# Callable entities in C++
There are different types of callable entities in C++:
1. Function-like macros
2. Global/Namespace/Member functions
3. Function pointers
4. References to functions
5. Functors
6. Lambdas (Since C++11)

Out of these, function pointers, function references, and functors are object types, i.e., they can be used like regular variables, pointers, and references. This allows the user to call functions dynamically at runtime, pass them as arguments to other functions (callbacks), or store them in arrays for complex logic.

## Function pointers
A function pointer is a variable that stores the memory address of a function. The signature of a function pointer must match the return type, calling convention, and parameter list of the function it points to.

### Global/Namespace function pointer
A global or namespace function pointer is declared as:
```
return_type (*funcPtr)(parameter_types);
```
The the address of a function is then assigned to it.
```
funcPtr = &myFuncName; // '&' is optional; 'funcPtr = myFuncName;' also works
```
Finally, the function is invoked via its pointer:
```
auto result = funcPtr(/* ... */);
```

For example, consider a function
```
int add (int a, int b)
{
    return a+b;
}
```
Its function pointer would be:
```
// 1. Declaration: return_type (*pointer_name)(parameter_types);
int (*funcPtr)(int, int);

// 2. Initialization: Assign the address of a function
funcPtr = &add; // '&' is optional; 'funcPtr = add;' also works

// 3. Invocation: Call the function via the pointer
int result = funcPtr(10, 5); // Equivalent to add(10, 5)
```

Function pointers can also be used with ```typedef```.
```
typedef int (*funcPtr)(int, int);
// ...
funcPtr f = add; // OR, funcPtr f = &add;
int res = f(10, 5);
```

### Member function pointers
Function pointers to member functions are declared in the following manner:
```
ReturnType (ClassName::*ptrName)(Params) = &ClassName::Member;
```
And the invocation is done using ```.*``` or ```->*``` operator.
For example:
```
class Calculator {
public:
    int add(int a, int b) { return a + b; }
    // ...
};

// Function pointer
int (Calculator::*ptr)(int, int);

int main() {
    Calculator calc;
    // Pointer to a member function of class Calculator
    //Equivalent to int (Calculator::*ptr)(int, int) = &Calculator::add;
    ptr = &Calculator::add;
    
    // Call using an object instance
    int res = (calc.*ptr)(5, 5); 
    return 0;
}
```

### Operations allowed
1. Assign an address of a function
2. Compare 2 function pointers
3. Call a function using the pointer
4. Pass the function pointer as an argument to another function
5. Return a function pointer
6. Store them in arrays

## Function references
Just as there is a pointer to a function, an alias (reference) can be created for a function name.

**Syntax**
```
return_type (&referenceName)(param_list)
```
For example:
```
void greet()
{
    std::cout << "Hello!";
}

int main()
{
    void (&ref)() = greet; // ref is now an alias for greet
    ref();                 // Calls greet()
}
```
Once a reference is bound to an object (or function), it cannot be changed to refer to another one.

## Functors
Functors are smart function pointers in the sense that they have:
* Value semantics
* Added type safety
* Encapsulation for function parameters using templates and polymorphism
* Distiction between function pointers with identical signature
* State information

A functor is basically a class that overloads the function call operator (```()```), allowing an instance of a class to be called like a function. A functor's type is known at compile-time, so compilers can often inline the function logic directly into the calling code. This makes functors generally faster than function pointers.
For example:
```
#include <iostream>

// A sample function
int add(int a, int b) { return a+b; }

// A functor
class AdderFunctor {
public:
    int operator() (int a, int b) { return a + b; }
} aF;

int main()
{
    int x = 5, y = 7;
    
    int z = add(x, y);
    std::cout << "z = " << z << std::endl;

    int zf = aF(x, y); // aF.operator()(x,y)
    std::cout << "zf = " << zf << std::endl;
}
```
Functors store internal variables (member data) initialized via a constructor. This allows the object to "remember" data between calls without using global or static variables, hence ensuring state persistence.
The C++ Standard Library provides various built-in functors for common operations in the <functional> header: 
* Arithmetic: ```std::plus```, ```std::minus```, ```std::multiplies```, ```std::divides```, ```std::modulus```.
* Relational: ```std::equal_to```, ```std::not_equal_to```, ```std::greater```, ```std::less```, ```std::greater_equal```, ```std::less_equal```.
* Logical: ```std::logical_and```, ```std::logical_or```, ```std::logical_not```. 


## Modern C++ Alternatives (2025 Context)
While raw function pointers are efficient, modern C++ (C++11 and later) provides more flexible alternatives: 
* ```std::function```: A type-safe wrapper that can store function pointers, lambdas, or functors.
* Lambdas: Anonymous functions that can be passed directly to other functions without declaring a named function (internally converted to a functor by the compiler).
* ```std::invoke```: A universal way to call any callable (added in C++17) that simplifies the syntax for member function pointers.
* ```std::function_fref```: A type-safe function reference

Source: https://www.youtube.com/watch?v=i7-jWzWOBbk&t=79s
