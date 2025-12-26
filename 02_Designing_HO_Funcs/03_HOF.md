# Higher order functions (HOFs)
Higher order functions are those which take one or more fucntions as arguments and/or return a function. Here, a function could be any callable entity, such as a subroutine, function pointer, lambda, etc.

## Functions taking another function(s)
For example, standard algorithms like ```std::sort``` take a function as one of the arguments.
```
std::sort(nums.begin(), nums.end(), [](int a, int b) {
    return a > b; // Custom descending order logic
});
```

The below table summarizes the different implementations of HOFs.
| Method | Description | Performance | Limitations |
| --------------- | --------------- | --------------- | -------
| Templates | Takes a generic F parameter. Allows the compiler to inline the function. | High (Best) | Type deduction, static polymorphism
| ```std::function``` | Provides a uniform interface for different callable types. | Medium (Overhead) | Lands in problems in cases of ```std::move``` only objects, const-correctness, and signature deduction at runtime for erased wrappers.
| Function Pointers | Minimal overhead but cannot easily capture state (no closures). | High | A function needs to be there beforehand, not on the fly
| ```std::fucntion_ref``` | Type-erasing reference to a callable type | High | Limited lifetime or dangling reference, reference semantics enforced, used only for synchronous algorithms

**Note**

Synchronous algorithms are those functions that run from the begging to end in one go. Asynchronous algorithms are those which can pause, save the context, and resume later.

The ```std::function_ref``` guarantees trivial copyability and avoids memory allocation, making it unsuitable for owning complex, potentially large callable objects or stateful lambdas. It cannot make an internal copy of the function it is assigned to, unlike ```std::function```. If a ```std::function_ref``` is constructed from a temporary object (e.g., a stateless lambda), it results in undefined behavior when it is called because it will be referencing a dangling object. That is, reference semantics are enforced.
```
// DANGER: UNDEFINED BEHAVIOR
void risky_func() {
    function_ref<void()> fr = []{ std::cout << "Dangling reference!"; }; // fr refers to a temporary
    fr(); // The temporary lambda is gone by this point
}
```
The ```std::function_ref``` should be used only when it is certain the referenced callable will outlive the function_ref object itself. 

## Functions returning another function
Higher order functions can generate new functions on the fly using lambdas or ```std::function``` with ```auto``` return type.
```
// HOF that returns a new lambda function
auto createMultiplier(int factor) {
    return [factor](int x) {
        return x * factor;
    };
}
```
Any callable entities that behave like functions, such as function pointers, lambdas, or ```std::function``` objects can be returned.

Important Warnings
* Dangling References: When returning a lambda, ensure you do not capture local variables by reference (```[&]```) if those variables will go out of scope after the function returns. Always capture by value (```[=]``` or ```[var]```) for returned lambdas.
* Return Type Deduction: ```auto``` return type requires the function definition to be visible at the call site.
