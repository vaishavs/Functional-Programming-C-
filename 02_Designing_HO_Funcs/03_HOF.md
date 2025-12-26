# Higher order functions (HOFs)
Higher order functions are those which take one or more fucntions as arguments and/or return a function. Here, a function could be any callable entity, such as a subroutine, function pointer, lambda, etc.

## Functions taking another function(s)
For example, standard algorithms like ```std::sort``` take a function as one of the arguments.
```
std::sort(nums.begin(), nums.end(), [](int a, int b) {
    return a > b; // Custom descending order logic
});
```
Any callable entities that behave like functions, such as function pointers, lambdas, or ```std::function``` objects can be passed in.

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

## Implementation
The below table summarizes the different implementations of HOFs.
| Method | Description | Performance | Limitations |
| --------------- | --------------- | --------------- | -------
| Raw Function Pointers | Minimal overhead but cannot easily capture state (no closures). | High | A function needs to be there beforehand, not on the fly
| Raw Function References | Minimal overhead and non-owning | High | Cannot be re-assigned, can only bind to a standard global/static function or a capture-less lambda, cannot be null, limited lifetime safety, cannot create an array of raw function references (STL DS should be used)
| Templates | Takes a generic F parameter. Allows the compiler to inline the function. | High (Best) | Type deduction, static polymorphism
| Functor | Class wrapper to operator ```()``` | Context-dependent | Potential performance issues (if not inlined), state management and type deduction complexity, slicing issues (when returning as a base class object), lifetime management
| ```std::function``` | Provides a uniform interface for different callable types. | Medium (Overhead) | Lands in problems in cases of ```std::move``` only objects, const-correctness, and signature deduction at runtime for erased wrappers.
| ```std::function_ref``` | Type-erasing reference to a callable type | High | Limited lifetime safety, reference semantics enforced, used only for synchronous algorithms

**Note**

Synchronous algorithms are those functions that run from the begging to end in one go. Asynchronous algorithms are those which can pause, save the context, and resume later.
