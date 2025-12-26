# std::function
The ```std::function``` (defined in the ```<functional>``` header) is a general-purpose polymorphic function wrapper that stores any functions, lambdas, functors, or member functions that matches a specific signature. It is versatile and safer than raw function pointers. If no target is present, the wrapper is "empty," and calling it throws a ```std::bad_function_call``` exception.
It is declared as:
```
std::function<return_type(parameter_types)> func_name;
```
A function is then assigned to it:
```
func_name = myFunction;
```
It is invoked like a regular function
```
func_name(/* ... */);
```

For example,
```
#include <iostream>
#include <functional>

int add(int a, int b) {
    return a + b;
}

int main() {
    // Using std::function
    std::function<int(int, int)> func = add;

    std::cout << "Using std::function: " << func(2, 3) << '\n';  // Output: Using std::function: 5

    return 0;
}
```
It is crucial to note that ```std::function``` can introduce noticeable performance penalties. To be able to hide the contained type and provide a common interface over all callable types, it uses a technique known as type erasure. Type erasure is usually based on virtual member function calls. Because virtual calls are resolved at runtime, the compiler cannot inline the call, and thus has limited optimization opportunities.

# std::function_ref
Starting from C++26, ```std::function_ref``` is a non-owning reference to a callable object. It acts as a type-erased "view" into a function, lambda, or functor. It does not store a copy of the callable; it only stores a reference to it. It is typically the size of two pointers, making it highly efficient to pass by value.

Unlike ```std::function```, it cannot allocate dynamic memory and store a copy of the function assigned to it. Because it is non-owning, the referred callable must outlive the reference.

Returning a ```std::function_ref``` from a function or storing it in a class member when it points to a local or temporary object will cause a dangling reference and undefined behavior.

The ```std::function_ref``` guarantees avoids memory allocation, making it unsuitable for owning complex, potentially large callable objects or stateful lambdas. If a ```std::function_ref``` is constructed from a temporary object (e.g., a stateless lambda), it results in undefined behavior when it is called because it will be referencing a dangling object.
```
// DANGER: UNDEFINED BEHAVIOR
void risky_func() {
    function_ref<void()> fr = []{ std::cout << "Dangling reference!"; }; // fr refers to a temporary
    fr(); // The temporary lambda is gone by this point
}
```
* Dangling References: When returning a lambda, local variables should not be captured by reference (```[&]```) if those variables will go out of scope after the function returns. They should always be captured by value (```[=]``` or ```[var]```) for returned lambdas.
* Return Type Deduction: ```auto``` return type requires the function definition to be visible at the call site.

A ```const std::function_ref``` can still invoke a mutable lambda because it does not own the state being mutated.
