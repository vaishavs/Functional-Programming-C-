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
