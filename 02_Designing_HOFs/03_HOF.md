# Higher order functions (HOFs)
Higher order functions are those which take one or more fucntions as arguments and/or return a function. Here, a function could be any callable entity, such as a subroutine, function pointer, lambd[...]

## Functions taking another function(s)
For example, standard algorithms like ```std::sort``` take a function as one of the arguments.
```cpp
std::sort(nums.begin(), nums.end(), [](int a, int b) {
    return a > b; // Custom descending order logic
});
```
Any callable entities that behave like functions, such as function pointers, lambdas, or ```std::function``` objects can be passed in.

## Functions returning another function
Higher order functions can generate new functions on the fly using lambdas or ```std::function``` with ```auto``` return type.
```cpp
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
| --- | --- | --- | --- |
| **Pointers** — raw [function pointers](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#function-pointers), pointers to member functions, poin[...]
| [Raw Function References](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#function-references) | Minimal overhead and non-owning | High | Canno[...]
| Templates | Takes a generic `F` parameter. Allows the compiler to inline the function. | High (Best) | Type deduction, static polymorphism |
| [Functors](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#functors) — plain, and the virtual (abstract-base) variant | Class with `operator([...]
| Lambdas | Compiler-generated closure object (a functor); captures state; usually consumed via a template `F` or stored in a wrapper. | High (inlined via templates) | Unique unnamed type — need[...]
| Standard function objects (`std::less<>`, `std::plus<>`, `std::identity`, …) | Library functors for built-in operations; transparent (`<>`) forms accept mixed operand types. | High (inlined; e[...]
| **Type-erased wrappers** — [`std::function`](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HOFs/02_wrappers.md#stdfunction) (C++11), [`std::copyable_function`]([...]

A related category is the **adaptors that produce or transform** a callable to feed into one of the above:

| Method | Description | Performance | Limitations |
| --- | --- | --- | --- |
| `std::mem_fn` | Wraps a pointer-to-member into an ordinary callable (object-first). | High (thin, inlinable) | Wraps member pointers only; a lambda/projection often reads as well |
| `std::bind` (C++11) | Partial application with placeholders (`_1`, `_2`) and argument reordering. | Medium (eager arg copies; opaque type; often not inlined) | Eager-copy surprises (needs `std::[...]
| `std::bind_front` (C++20) / `std::bind_back` (C++23) | Bind leading / trailing arguments, no placeholders; forwards the rest. | High (light; concrete type, inlinable) | `bind_back` is C++23; for[...]
| `std::not_fn` (C++17) | Returns the logical negation of a predicate. | High (thin wrapper) | Predicates only; replaced the removed `not1`/`not2` |
| `std::reference_wrapper` (`std::ref`/`std::cref`) | Passes a callable by reference through copying interfaces; forwards the call to the referent. | High (no copy of the callable) | Referent must[...]

**Performance legend** (relative): *High (Best)* = static dispatch, fully inlinable; *High* = cheap/thin to pass, but often an indirect (non-inlined) call; *Context-dependent* = inlines when the c[...]

One mechanism sits underneath the pointer and lambda rows rather than earning its own: a **surrogate call function** — an object with a conversion-to-function-pointer is callable and can be pass[...]

**Note**

Synchronous algorithms are those functions that run from the begging to end in one go. Asynchronous algorithms are those which can pause, save the context, and resume later.

Reference: https://www.youtube.com/watch?v=EbnRt-omrFY&pp=ygUaaGlnaGVyIG9yZGVyIGZ1bmN0aW9ucyBjKys%3D
