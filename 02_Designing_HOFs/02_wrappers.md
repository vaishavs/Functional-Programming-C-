# Type-Erased Callable Wrappers
*Type erasure* is the technique of hiding a value's concrete type behind a fixed interface, storing it in a way that later code can use without knowing what it originally was. A type-erased callable wrapper stores whatever callable it is given, remembers how to invoke it, and presents a single call signature `R(Args...)`.

This flexibility has a price that is important to state up front, because it is the central trade-off for the whole file. Because the concrete callable's size is not known at the wrapper's compile time, the wrapper may need to **allocate** on the heap to store it — although implementations apply a *small buffer optimization* (SBO) that stores small callables inline and avoids allocation for them.

And because the call goes through an indirection (a virtual-like dispatch to the erased callable), it is typically **not inlined**, unlike a direct call on a known function-object type. Type-erased wrappers therefore buy uniformity and storage at the cost of possible allocation and lost inlining, and belong at API and storage boundaries rather than in hot inner loops.

## std::function
The ```std::function``` (defined in the ```<functional>``` header) is a general-purpose polymorphic function wrapper that stores any functions, lambdas, functors, or member functions that matches a specific signature. It **owns** a copy of whatever callable it is assigned, is itself **copyable and assignable**, and can be **empty**. It is versatile and safer than raw function pointers. If no target is present, the wrapper is "empty," and calling it throws a ```std::bad_function_call``` exception.
It is declared as:
```cpp
std::function<return_type(parameter_types)> func_name;
```
A function is then assigned to it:
```cpp
func_name = myFunction;
```
It is invoked like a regular function
```cpp
func_name(/* ... */);
```

For example,
```cpp
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

Because it owns its target and is copyable, `std::function` can be stored in containers, held as a class member, and passed by value freely — the things a raw lambda cannot do.
#### Empty state and `bad_function_call`.
A default-constructed (or moved-from) `std::function` holds no target. Testing it in a boolean context reports emptiness; calling an empty `std::function` throws `std::bad_function_call`:

```cpp
std::function<int(int)> empty;
static_cast<bool>(empty);            // false
empty(1);                            // throws std::bad_function_call
```

#### Recovering the target (`target`, `target_type`).
`std::function` can report the `std::type_info` of its stored callable and hand back a typed pointer to it. These use RTTI:

```cpp
f.target_type() == typeid(int(*)(int));      // true, when f holds `triple`
if (auto p = f.target<int(*)(int)>()) (*p)(2); // 6 — recovered the underlying function pointer
```

Here, `target` returns `nullptr` if the requested type does not match, so it is a checked downcast of the erased callable. It requires RTTI to be enabled.

#### Cost profile
`std::function` copies its target (so a large captured state is copied with it), may allocate when the target does not fit the SBO, and dispatches indirectly. These costs
are the reason the C++23 and C++26 additions below exist.

## std::function_ref
Starting from C++26 and GCC16 toolchain, ```std::function_ref```, defined in ```<functional>``` header, is a non-owning reference to a callable object. It acts as a type-erased "view" into a function, lambda, or functor. It does not store a copy of the callable; it only stores a reference to it. It is the callable analogue of `std::string_view` — cheap to pass, but valid only while the referenced callable outlives it. It is typically the size of two pointers, making it highly efficient to pass by value. It is similar to a raw function pointer but more flexible.

Unlike ```std::function```, it cannot allocate dynamic memory and store a copy of the function assigned to it. Because it is non-owning, the referred callable must outlive the reference.

Returning a ```std::function_ref``` from a function or storing it in a class member when it points to a local or temporary object will cause a dangling reference and undefined behavior.

The syntax for ```std::function_ref``` follows the standard function signature template format:
```cpp
#include <functional>

// Basic signature
std::function_ref<void(int)> func;
// const or noexcept qualifiers can be used inside template arguments
```
Function pointers, free functions, raw function references, lambdas, and functors can be passed to a ```std::function_ref```. The ```std::function_ref``` can be used in a manner similar to ```std::function```. But unlike ```std::function```, a ```std::function_ref``` cannot be empty. It must be initialized with a valid callable upon creation.

For example,
```cpp
#include <iostream>
#include <functional> // Required for std::function_ref (C++26)

// A function that takes a non-owning reference to a callable with the signature void(int)
void call_function_ref(std::function_ref<void(int)> f) {
    std::cout << "Calling the passed function_ref with argument 42" << std::endl;
    f(42); // Invoke the referenced callable
}

int main() {
    // Pass a lambda function (must ensure lambda lifetime covers the call)
    // The lambda captures 'x' by reference. The lifetime of the lambda expression 
    // is extended until the end of the `call_function_ref` function call.
    int x = 10;
    auto lambda = [&](int value) { 
        x += value;
        std::cout << "Lambda called, x is now: " << x << std::endl;
    };
    call_function_ref(lambda); 
    std::cout << "After call, x is: " << x << std::endl;

    return 0;
}
```

The ```std::function_ref``` guarantees avoids memory allocation, making it unsuitable for owning complex, potentially large callable objects or stateful lambdas. If a ```std::function_ref``` is constructed from a temporary object (e.g., a stateless lambda), it results in undefined behavior when it is called because it will be referencing a dangling object.
```cpp
// DANGER: UNDEFINED BEHAVIOR
void risky_func() {
    function_ref<void()> fr = []{ std::cout << "Dangling reference!"; }; // fr refers to a temporary
    fr(); // The temporary lambda is gone by this point
}
```
* Dangling References: When returning a lambda, local variables should not be captured by reference (```[&]```) if those variables will go out of scope after the function returns. They should always be captured by value (```[=]``` or ```[var]```) for returned lambdas.
* Return Type Deduction: ```auto``` return type requires the function definition to be visible at the call site.

A ```const std::function_ref``` can still invoke a mutable lambda because it does not own the state being mutated.

The `std::function_ref` is the right choice for a **function parameter** that will call the supplied callable *during the call* and not retain it afterwards — the overwhelmingly common case for callback parameters. It replaces the frequent misuse of `std::function` as a by-value parameter (which needlessly copies and may allocate) with a zero-overhead view. Because it does not extend lifetime, it must never be stored beyond the referenced callable's lifetime; for storage, an owning wrapper is required.

## `std::move_only_function`
Many useful callables cannot be copied — for example a lambda that has captured a `std::unique_ptr` by move. The `std::function` requires a *copyable* target and so rejects them. The `std::move_only_function` (since C++23) removes that requirement: it is **move-only** and can therefore hold move-only callables.

```cpp
auto res = std::make_unique<int>(100);
std::move_only_function<int(int)> mof =
    [r = std::move(res)](int x){ return *r + x; };   // captures a unique_ptr by move
mof(5);                                               // 105

std::move_only_function<int(int)> mof2 = std::move(mof);  // movable
mof2(7);                                                   // 107
// mof2 = mof;   // would not compile: no copy
```

Once moved, the original `std::move_only_function` object enters an empty state. Attempting to invoke an empty wrapper throws a `std::bad_function_call` exception.

The `std::move_only_function` also has a cleaner, more expressive signature template than `std::function`: the signature may carry `const`, `&`/`&&`, and `noexcept` qualifiers, which constrain *how* the wrapper may be called and are enforced by the type. For example, `std::move_only_function<int(int) const>` permits invocation on a `const` wrapper, and `std::move_only_function<int(int) noexcept>` promises the call will not throw. It deliberately provides **no** `target`/`target_type` inspection, which lets implementations be leaner. When a callable does not need to be copied — the common case for one-shot work items, deferred actions, and task queues — `std::move_only_function` is the more efficient and more precise choice.

## `std::copyable_function`

The `std::copyable_function` is also a C++26 facility, and is supported by GCC16 toolchain.

The `std::copyable_function` is a copyable type-erased wrapper with the improved, qualifier-aware signature design of `std::move_only_function`, specified with cleaner semantics than the original `std::function`. It serves the exact same purpose, but it fixes long-standing design flaws regarding const-correctness and qualifiers.

Upon creating a `std::function`, the corresponding `operator()` is marked `const`, meaning invocation is permitted even if the `std::function` object is `const`. However, `std::function` permits storing a *mutable* lambda inside, allowing a `const` wrapper to modify underlying state:

```cpp
int x = 0;
// A mutable lambda modifying 'x'
std::function<void()> f = [x]() mutable { 
    x++; 
}; 

const auto& const_f = f;
const_f(); // THIS COMPILES! A const object modifying internal state.

```

Additionally, `std::function` cannot specify whether a callable must be `noexcept`, nor can reference qualifiers (like `&` or `&&`) be specified.
`std::copyable_function` enforces strict cv-qualifiers (`const`, `volatile`), reference qualifiers (`&`, `&&`), and `noexcept` specifications.

To mandate that a function is safe to call as `const`, explicit declaration as `std::copyable_function<void() const>` is required. Passing a mutable lambda to a `const`-qualified wrapper causes a compiler rejection.

```cpp
#include <functional>
#include <iostream>

int main() {
    int x = 0;

    // 1. A const-qualified copyable_function
    std::copyable_function<void() const> safe_func = [x]() {
        std::cout << "Only reading x: " << x << "\n";
    };
    safe_func(); // Works!

    // 2. Compilation failure! 
    // Binding a mutable lambda to a const copyable_function is prohibited.
    /*
    std::copyable_function<void() const> bad_func = [x]() mutable {
        x++;
    };
    */

    // 3. To allow mutation, remove 'const' from the signature
    std::copyable_function<void()> mutable_func = [x]() mutable {
        x++;
        std::cout << "Modified x: " << x << "\n";
    };
    
    // As the name implies, copying is supported:
    auto func_copy = mutable_func; 
    func_copy();
    
    return 0;
}

```

The intent is that new code preferring a copyable wrapper reach for `std::copyable_function`, leaving `std::function` in place for compatibility. Until C++26 is available, `std::function` remains the copyable option.

## `std::packaged_task`

The `std::packaged_task` wraps a callable so that invoking it delivers the result (or exception) through a `std::future`. It is the bridge between the callable world and asynchronous results.

```cpp
std::packaged_task<int(int)> task(triple);   // wrap a callable
std::future<int> fut = task.get_future();    // obtain the future before running
task(14);                                     // run: computes 42, stored in the shared state
fut.get();                                    // 42 — retrieved (would block until ready)
```

The task can be moved to another thread and run there, with the originating code retrieving the result via the future. `std::packaged_task` is move-only (it owns a shared state) and, like the others, is a function object at heart. It is used to build thread pools and task schedulers, where work is enqueued as tasks and results collected through futures.

The lifecycle of a `std::packaged_task` involves four distinct phases:

* **Initialization:** The template is instantiated with a specific function signature and bound to a callable object.
* **Future Extraction:** Invoking the `.get_future()` method yields a `std::future` linked directly to the task's shared state.
* **Execution:** The task must be manually invoked via `operator()`. Execution typically occurs on a separate thread or within a custom thread pool.
* **Retrieval:** The associated `std::future` retrieves the computed result via `.get()`, blocking the current thread until the execution concludes.

Here is a complete example:
```cpp
#include <iostream>
#include <future>
#include <thread>

// A simple function designated for asynchronous execution
int compute_square(int x) {
    return x * x;
}

int main() {
    // Wrap the function in a packaged_task
    std::packaged_task<int(int)> task(compute_square);

    // Extract the future to retrieve the result later
    std::future<int> result_future = task.get_future();

    // Move the task to a separate thread for execution
    std::thread task_thread(std::move(task), 10);

    // ... concurrent operations can occur here ...

    // Retrieve the result (blocks until the calculation finishes)
    int result = result_future.get();
    std::cout << "Computed result: " << result << "\n";

    // Synchronize the thread
    task_thread.join();

    return 0;
}

```

> **Note on move semantics:** `std::packaged_task` cannot be copied. Moving the object via `std::move()` is mandatory when transferring the task to a new thread.


