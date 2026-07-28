# Higher order functions (HOFs)
Higher order functions are those which take one or more fucntions as arguments and/or return a function. Here, a function could be any callable entity, such as a subroutine, function pointer, lambda, etc.

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
| **Pointers** — raw [function pointers](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#function-pointers), pointers to member functions, pointers to data members | A stored *address* selecting what to call (or read) at run time. **Function pointer** `R(*)(...)`: names a free/static function, called directly with `f(args)`; carries no state. **Pointer to member function** `R(C::*)(...)`: names a member without an object — supply the object via `std::invoke`/`std::mem_fn` (object first). **Pointer to data member** `T C::*`: names a field — INVOKE *reads* it (the idiomatic ranges projection `&T::field`). | High (thin), but an indirect call; the data-member form is just a field read | Function pointer: the target must exist beforehand (not on the fly), carries no state (no closures), and inhibits inlining. Member/data pointers: not callable with plain `f(args)`, need an object, not convertible to a plain function pointer, larger than a data pointer, awkward `.*`/`->*` syntax; data-member pointers are INVOKE-only, read-only, and take no arguments |
| [Raw Function References](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#function-references) | Minimal overhead and non-owning | High | Cannot be re-assigned, can only bind to a standard global/static function or a capture-less lambda, cannot be null, limited lifetime safety, cannot create an array of raw function references (STL DS should be used) |
| Templates | Takes a generic `F` parameter. Allows the compiler to inline the function. | High (Best) | Type deduction, static polymorphism |
| [Functors](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#functors) — plain, and the virtual (abstract-base) variant | Class with `operator()`. **Plain functor**: a concrete class carrying state; inlines when its type is visible (e.g. through a template `F`). **Virtual variant**: an abstract base with `virtual operator()`, called through a base pointer/reference for runtime polymorphism — the OOP alternative to type erasure. | Context-dependent (plain: High when inlined; virtual: Medium — virtual dispatch, not inlined) | Plain: potential perf loss if not inlined, state-management/type-deduction complexity, lifetime management. Virtual: requires inheritance and (usually) heap ownership, object slicing if held/returned by value, more boilerplate than a wrapper |
| Lambdas | Compiler-generated closure object (a functor); captures state; usually consumed via a template `F` or stored in a wrapper. | High (inlined via templates) | Unique unnamed type — needs `auto`/template or a wrapper to store; reference captures need lifetime care; only capture-less ones decay to a function pointer |
| Standard function objects (`std::less<>`, `std::plus<>`, `std::identity`, …) | Library functors for built-in operations; transparent (`<>`) forms accept mixed operand types. | High (inlined; empty types via EBO) | Fixed to built-in operations; transparent variants need C++14; `std::identity` needs C++20 |
| **Type-erased wrappers** — [`std::function`](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HOFs/02_wrappers.md#stdfunction) (C++11), [`std::copyable_function`](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HOFs/02_wrappers.md#stdcopyable_function) (C++26), [`std::move_only_function`](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HOFs/02_wrappers.md#stdmove_only_function) (C++23), [`std::function_ref`](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HOFs/02_wrappers.md#stdfunction_ref) (C++26), [`std::packaged_task`](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HOFs/02_wrappers.md#stdpackaged_task) (C++11) | One uniform, *named* type that holds or points at any callable of a fixed signature, hiding the concrete type. **Owning + copyable:** `std::function` and its C++26 redesign `std::copyable_function` (adds `const`/`&`/`&&`/`noexcept` signatures). **Owning + move-only:** `std::move_only_function` (holds move-only targets, e.g. a captured `unique_ptr`). **Non-owning view:** `std::function_ref`. **Result-to-future:** `std::packaged_task`. | Medium (erasure ⇒ indirect, non-inlined call; owning forms may heap-allocate; `function_ref` is lightest / non-owning) | Erasure overhead vs a template; empty-call differs — `std::function` throws `bad_function_call` but `move_only_function`/`copyable_function` are **UB** (guard with the bool test); `function_ref` owns nothing ⇒ referent must outlive it (synchronous use only); `move_only_function`/`packaged_task` can't be copied; `copyable_function`/`function_ref` need C++26; `packaged_task` is specialized (`get_future()` once, needs `<future>`) |

A related category is the **adaptors that produce or transform** a callable to feed into one of the above:

| Method | Description | Performance | Limitations |
| --- | --- | --- | --- |
| `std::bind` (C++11) | Partial application with placeholders (`_1`, `_2`) and argument reordering. | Medium (eager arg copies; opaque type; often not inlined) | Eager-copy surprises (needs `std::ref` for reference args); composes poorly; largely superseded by lambdas / `bind_front` |
| `std::bind_front` (C++20) / `std::bind_back` (C++23) | Bind leading / trailing arguments, no placeholders; forwards the rest. | High (light; concrete type, inlinable) | `bind_back` is C++23; for reordering use `std::bind` or a lambda |
| `std::not_fn` (C++17) | Returns the logical negation of a predicate. | High (thin wrapper) | Predicates only; replaced the removed `not1`/`not2` |
| `std::reference_wrapper` (`std::ref`/`std::cref`) | Passes a callable by reference through copying interfaces; forwards the call to the referent. | High (no copy of the callable) | Referent must outlive it (lifetime hazard); reference semantics |

**Performance legend** (relative): *High (Best)* = static dispatch, fully inlinable; *High* = cheap/thin to pass, but often an indirect (non-inlined) call; *Context-dependent* = inlines when the concrete type is visible to a template, adds overhead once type-erased; *Medium* = type erasure or virtual dispatch, with possible heap allocation and an indirect call.

One mechanism sits underneath the pointer and lambda rows rather than earning its own: a **surrogate call function** — an object with a conversion-to-function-pointer is callable and can be passed where a function pointer is expected; the everyday instance is the capture-less-lambda → function-pointer decay, an indirect (non-inlined) call.

**Note**

Synchronous algorithms are those functions that run from the begging to end in one go. Asynchronous algorithms are those which can pause, save the context, and resume later.

Reference: https://www.youtube.com/watch?v=EbnRt-omrFY&pp=ygUaaGlnaGVyIG9yZGVyIGZ1bmN0aW9ucyBjKys%3D
