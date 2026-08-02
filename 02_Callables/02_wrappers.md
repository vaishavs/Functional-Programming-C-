# Type-Erased Callable Wrappers
*Type erasure* is the technique of hiding a value's concrete type behind a fixed interface, storing it in a way that later code can use without knowing what it originally was. A type-erased callable wrapper stores whatever callable it is given, remembers how to invoke it, and presents a single call signature `R(Args...)`.

But because the concrete callable's size is not known at the wrapper's compile time, the wrapper may need to **allocate** on the heap to store it — although implementations apply a *small buffer optimization* (SBO) that stores small callables inline and avoids allocation for them.

And since the call goes through an indirection (a virtual-like dispatch to the erased callable), it is typically **not inlined**, unlike a direct call on a known function-object type. Type-erased wrappers therefore buy uniformity and storage at the cost of possible allocation and lost inlining, and belong at API and storage boundaries rather than in hot inner loops.

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

#### Pitfalls
* **Using it as a plain by-value callback parameter.** A function that only *calls* the callback during its own execution, and never stores it, still pays for a copy — and possibly a heap allocation — on every single invocation:
  ```cpp
  // Needlessly forces a copy (and possibly an allocation) of the callback
  // on every call, even though the callback is only used right here.
  void for_each_item(const std::vector<int>& items, std::function<void(int)> callback) {
      for (int i : items) callback(i);
  }
  ```
  When the callable is only needed for the duration of the call, `std::function_ref` (below) is the non-owning, allocation-free alternative.
* **Comparing two `std::function` objects with `==`.** `std::function` only defines `operator==` against `nullptr`, not against another `std::function` — there is no general way to decide whether two arbitrary erased callables are "the same" one:
  ```cpp
  std::function<void()> f1 = [] {};
  std::function<void()> f2 = [] {};
  if (f1 == f2) { /* ... */ }   // does not compile: no operator== between two std::function objects
  ```
* **Assuming that owning the callable makes the whole call safe.** `std::function` owns a copy of the *closure object*, but if that closure captured something *by reference*, the reference itself is still just a reference — nothing about `std::function` extends the lifetime of what it points to:
  ```cpp
  std::function<void()> make_printer() {
      int local = 42;
      return [&local]() { std::cout << local; };   // captures local BY REFERENCE
  }   // local's storage ends here; the compiler does not warn about this

  auto printer = make_printer();
  printer();   // undefined behavior: reads a destroyed stack variable
  ```
* **Assuming the small buffer optimization is guaranteed.** The standard does not mandate an SBO or a minimum inline size; whether a given lambda's capture avoids allocation is implementation-defined and differs between libstdc++, libc++, and MSVC's STL. Code that depends on "this capture is definitely small enough to stay inline" is leaning on a detail the standard leaves unspecified.

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

A ```const std::function_ref``` can still invoke a mutable lambda because it does not own the state being mutated.

The `std::function_ref` is the right choice for a **function parameter** that will call the supplied callable *during the call* and not retain it afterwards — the overwhelmingly common case for callback parameters. It replaces the frequent misuse of `std::function` as a by-value parameter (which needlessly copies and may allocate) with a zero-overhead view. Because it does not extend lifetime, it must never be stored beyond the referenced callable's lifetime; for storage, an owning wrapper is required.

#### Pitfalls
The dangling-temporary example above is the most common mistake, but the same non-owning nature causes trouble in two other recurring shapes:
* **Storing it as a class member.** A `std::function_ref` data member looks like an ordinary callback field, but it only ever refers to whatever was passed in at the time it was set — it does not keep that callable alive:
  ```cpp
  class Widget {
      std::function_ref<void()> callback_;   // non-owning: dangerous as a stored member
  public:
      void set_callback(std::function_ref<void()> cb) { callback_ = cb; }
      void fire() { callback_(); }
  };

  void setup(Widget& w) {
      int local = 10;
      w.set_callback([&local] { std::cout << local; });   // a temporary lambda referring to a local
  }   // both the lambda and 'local' are gone by the time setup() returns

  Widget w;
  setup(w);
  w.fire();   // undefined behavior: callback_ refers to objects that no longer exist
  ```
* **Using it for deferred or asynchronous work.** Anything that queues a callable to run later — a task queue, an event system, a coroutine continuation — needs to *own* that callable, not merely view it:
  ```cpp
  // BAD: none of these views are guaranteed to still be valid when run_all() executes.
  class TaskQueue {
      std::vector<std::function_ref<void()>> tasks_;
  public:
      void enqueue(std::function_ref<void()> t) { tasks_.push_back(t); }
      void run_all() { for (auto t : tasks_) t(); }
  };
  ```
  For storage or deferred invocation, reach for `std::function` or `std::move_only_function` instead — the wrappers below actually own their target.

As a rule of thumb: if the callable's lifetime is guaranteed to be "at least as long as this one function call," `std::function_ref` is safe and cheap; the moment that guarantee is unclear, it is the wrong tool.

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

Once moved, the original `std::move_only_function` object enters an empty state. **Unlike `std::function`, invoking an empty `std::move_only_function` is undefined behavior, not a guaranteed thrown exception** — there is no `std::bad_function_call` safety net here, so it is entirely the caller's responsibility to know the wrapper is still populated before calling it. This distinction is easy to miss precisely because `std::function` trained everyone to expect a clean, catchable exception in this situation.

The `std::move_only_function` also has a cleaner, more expressive signature template than `std::function`: the signature may carry `const`, `&`/`&&`, and `noexcept` qualifiers, which constrain *how* the wrapper may be called and are enforced by the type. For example, `std::move_only_function<int(int) const>` permits invocation on a `const` wrapper, and `std::move_only_function<int(int) noexcept>` promises the call will not throw. It deliberately provides **no** `target`/`target_type` inspection, which lets implementations be leaner. When a callable does not need to be copied — the common case for one-shot work items, deferred actions, and task queues — `std::move_only_function` is the more efficient and more precise choice.

#### Pitfalls
* **Trying to copy it.** `std::move_only_function` has no copy constructor, so anything that copies by value — including `push_back` on a `const&`, or simply assigning one variable to another — fails to compile:
  ```cpp
  std::move_only_function<void()> task = [] { /* ... */ };
  std::vector<std::move_only_function<void()>> tasks;
  tasks.push_back(task);              // error: use of deleted copy constructor
  tasks.push_back(std::move(task));   // OK: moves the target in
  ```
* **Calling it after it has been moved from.** As noted above, this is *not* a safe, checked failure the way it is for `std::function`:
  ```cpp
  std::move_only_function<int(int)> original = [](int x) { return x * 2; };
  std::move_only_function<int(int)> moved_to = std::move(original);

  moved_to(5);     // fine: 10
  original(5);     // undefined behavior -- original is now empty, and there is no exception to catch
  ```
  Treat a moved-from `std::move_only_function` the way you would a moved-from `std::unique_ptr`: assume it is empty, and do not call it.
* **Expecting `target()`/`target_type()` like `std::function`.** These were deliberately left out, so trying to inspect the erased type does not compile:
  ```cpp
  std::move_only_function<void()> f = [] {};
  auto* p = f.target<void(*)()>();   // error: 'class std::move_only_function<void()>' has no member named 'target'
  ```
* **Declaring a qualifier the assigned callable can't actually satisfy.** Because the signature's `const`/`noexcept`/ref-qualifiers are enforced by the type, a mismatched callable is rejected at the assignment itself, not discovered later at the call site:
  ```cpp
  int x = 0;
  std::move_only_function<void() const> f = [x]() mutable { x++; };
  // error: a mutable lambda cannot satisfy a const-qualified signature

  std::move_only_function<void() noexcept> g = [] { throw std::runtime_error("oops"); };
  // error: a potentially-throwing lambda cannot satisfy a noexcept-qualified signature
  ```

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

#### Pitfalls
* **Trying to store a move-only capture in it.** Being copyable is not optional — like `std::function`, `std::copyable_function` requires its target to be `CopyConstructible`, so a lambda that captured a `std::unique_ptr` by move is rejected exactly where `std::move_only_function` would have accepted it:
  ```cpp
  auto owned = std::make_unique<int>(42);
  std::copyable_function<int()> f = [p = std::move(owned)]() { return *p; };
  // error: the lambda's captured unique_ptr makes it move-only,
  // but std::copyable_function requires a copyable target
  ```
  If the captured state must be move-only, `std::move_only_function` is the correct wrapper, not this one.
* **Getting a qualifier wrong, the same way as `std::move_only_function`.** `std::copyable_function` enforces `const`/`noexcept`/ref-qualifiers just as strictly as `std::move_only_function` does (see its Pitfalls above) — declaring a `noexcept` signature and assigning a callable that can throw is rejected at compile time, not left to be discovered at runtime.
* **Assuming it removes every cost of `std::function`, not just the const-correctness ones.** `std::copyable_function` fixes the qualifier and mutable-lambda-through-a-const-wrapper problems shown above, but it is still a type-erased, copyable wrapper: a large capture can still trigger a heap allocation, and a call still goes through an indirection. Reaching for it does not, by itself, make code allocation-free or inlinable — for that, `std::function_ref` (for non-owning calls) or a template parameter are still the tools to reach for.
* **Assuming it is available wherever `std::function` is.** It is a C++26 facility requiring a very recent toolchain (GCC16, per the introduction above). Code that must still build with C++20/23 compilers cannot use it yet — `std::function` remains the portable, if less precise, choice until then.


Sources:

* https://en.cppreference.com/cpp
* https://medium.com/@sgn00/diving-into-std-function-d342e4b58ea7
* https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p0792r10.html
* https://towardsdev.com/std-move-only-function-cpp23-callable-wrapper-no-copy-369e79e5baa0
* https://www.sandordargo.com/blog/2026/05/20/cpp26-copyable-function
