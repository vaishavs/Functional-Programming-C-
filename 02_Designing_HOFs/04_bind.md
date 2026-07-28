# Binding function arguments
The ```std::bind``` is part of the ```<functional>``` library in C++. It allows us to bind one or more arguments to a function, creating new callable objects by pre-setting (binding) some or all arguments of an existing function. The function is invoked only when someone calls the function object returned by ```std::bind```.

### Syntax
```cpp
auto new_callable = std::bind(function, arguments...);
```

There are several ways in which ```std::bind``` is used.
### Placeholders
The ```std::bind``` takes a callable (function pointer, functor, or lambda) as its first argument, followed by the specific values to be "fixed". To leave some arguments "open" to be provided at the actual call time, ```std::placeholders::_1```, ```_2```, etc., are used. Placeholders are special objects used in ```std::bind``` to represent arguments provided later. By using a placeholder, we say: "Hey, there will be an argument, but it is not present right now!" The ```std::placeholders::_1``` represents the first parameter, ```std::placeholders::_2``` represents the second parameter, and so on.

```cpp
#include <iostream>
#include <functional>

int add(int a, int b) {
    return a + b;
}

int main() {
    // Using std::bind to create a new function that always adds 5
    auto add_five = std::bind(add, std::placeholders::_1, 5);
    std::cout << "3 + 5 = " << add_five(3) << '\n'; // Output: 3 + 5 = 8

    return 0;
}
```
In this example:
* ```add``` is a function that takes two ints and returns their sum.
* ```std::bind``` creates ```add_five``` by binding the second argument of ```add``` to ```5```.
* Calling ```add_five(3)``` results in ```add(3, 5)```, producing ```8```.

### Argument reordering and duplication
Placeholders can be used to change the order of its function parameters or repeat them.
```cpp
void divide(double num, double den) { std::cout << num / den << "\n"; }

int main() {
    using namespace std::placeholders;

    // REORDERING: _2 becomes the 1st param (num), _1 becomes the 2nd (den)
    auto reverse_div = std::bind(divide, _2, _1);
    reverse_div(2, 10); // Calls divide(10, 2) -> Output: 5

    // DUPLICATION: Pass the first input to both parameters
    auto square = std::bind(std::multiplies<int>(), _1, _1);
    std::cout << square(4) << "\n"; // Output: 16
}
```
### Binding Member Functions
When binding a member function, provide a pointer to the function and an instance (or pointer to an instance) of the class *must* be provided as the first argument after the function name.
```cpp
struct Calculator {
    void multiply(int a, int b) { std::cout << a * b; }
};

int main() {
    Calculator calc;
    // Bind member function to instance 'calc'
    auto bound_mem = std::bind(&Calculator::multiply, &calc, std::placeholders::_1, 10);
    
    bound_mem(7); // Outputs: 70
}
```
## Pitfalls
By default, ```std::bind``` copies the arguments provided at the time of binding. It can cause significant performance overhead when accidentally copying large objects like ```std::string``` or other container types.
Also, ```std::bind``` cannot automatically resolve which version of an overloaded function you want to use. This leads to complex compilation errors.
For example,
```cpp
void print(int x);
void print(double x);

// ERROR: Compiler doesn't know which 'print' to bind
auto f = std::bind(print, 10); 

// Lambda handles this easily:
auto l = [](int x) { print(x); };
```

If an attempt is made to bind move-only objects like ```std::move``` or ```std::unique_ptr```, and/or if they are used along with repeated placeholders, this leads to undefined behaviour, because ```std::bind``` tries to copy them internally. What happens is that the first parameter may successfully "steal" the resource, leaving its own place as well as the others with a null or empty object.
```cpp
#include <iostream>
#include <functional>
#include <memory>

void compare_ptrs(std::unique_ptr<int> p1, std::unique_ptr<int> p2) {
    if (p1 && p2) std::cout << "Both exist\n";
    else std::cout << "One or both are NULL\n";
}

void process_ptr(std::unique_ptr<int> p) { /* takes ownership */ }

int main() {
    auto ptr = std::make_unique<int>(10);

    // DANGER: This binder is move-only.
    auto binder = std::bind(process_ptr, std::move(ptr));

    // ERROR 1: This will not compile. std::function requires copy-ability.
    // std::function<void()> func = std::move(binder); 

    // ERROR 2: Even calling it directly fails because std::bind 
    // tries to COPY the unique_ptr into process_ptr's argument.
    // binder();

    // ERROR 3: _1 is used twice with a move-only object.
    auto repeat_binder = std::bind(compare_ptrs, _1, _1);

    auto my_ptr = std::make_unique<int>(100);

    // This will likely FAIL to compile or produce UNEXPECTED behavior.
    // std::bind tries to pass the same object to two parameters that 
    // both expect to take ownership (pass-by-value).
    repeat_binder(std::move(my_ptr));
}
```
If an invalid or null function pointer is bound, ```std::bind``` will not catch this at compile time. Instead, it will throw ```std::bad_function_call``` at runtime when the object is invoked.
```cpp
#include <iostream>
#include <functional>

void (*func_ptr)(int) = nullptr;
std::function<void()> func = std::bind(nullptr);

int main() {
    // SUCCESS: The binder is created even though func_ptr is null.
    auto binder = std::bind(func_ptr, 42);

    std::cout << "Binder created successfully.\n";

    // RUNTIME CRASH: Attempting to call the null pointer.
    // This typically results in a Segmentation Fault.
    binder();

    if (func) {
        // This block executes because std::function sees a valid callable OBJECT,
        // not realizing that the object internally holds a null pointer.
        func(); // CRASH
    }

    return 0;
}
```
The two lighter binders below cover the most common patterns without the pitfalls.
### `std::bind_front` (C++20)

The `std::bind_front(f, args...)` binds the *leading* arguments and forwards the rest in order — no placeholders, no reordering, and it forwards rather than eagerly decaying in the ways `bind` does:

```cpp
auto from100 = std::bind_front(sub, 100);   // fix the 1st argument to 100
from100(40);                                 // 60  (100 - 40)
```

This is the idiomatic modern binder for "fix the first few arguments", and a common way to bind a member function together with its object: `std::bind_front(&Widget::scaled, w)`.

### `std::bind_back` (Since C++23, GCC14)
`std::bind_back(f, args...)` is the mirror image: it binds the *trailing* arguments and forwards the leading ones:

```cpp
// Illustrative (C++23):
auto minus3 = std::bind_back(sub, 3);   // fix the last argument to 3
minus3(10);                              // 7   (10 - 3)
```

Together, `bind_front` and `bind_back` cover the overwhelming majority of real binding needs without placeholders.

Another way to overcome them is to bind by reference.

# Binding by reference
Binding by reference can be useful when the bound parameter needs to reflect any changes made to the original variable. This means that the bound function will use the current value of the variable when invoked, not the value it had when the function was created. To bind by reference, ```std::ref``` is used for non-const references and ```std::cref``` is used for const references. The ```std::ref``` and ```std::cref``` are helper functions defined in ```<functional>``` header that are used to generate a ```std::reference_wrapper```. They automatically convert to a raw reference (```T&```) when passed to a function that expects the underlying type.
```cpp
#include <iostream>
#include <functional>

int add(int a, int b) {
   return a + b;
}

int main() {
   int x = 5;
   auto add_ref = std::bind(add, std::ref(x), std::placeholders::_1);
   std::cout << "5 + 3 = " << add_ref(3) << '\n'; // Output: 5 + 3 = 8

   x = 10;
   std::cout << "10 + 2 = " << add_ref(2) << '\n'; // Output: 10 + 2 = 12

   const int y = 10;
   auto add_const_ref = std::bind(add, std::cref(y), std::placeholders::_1);
   std::cout << "10 + 2 = " << add_const_ref(2) << '\n'; // Output: 10 + 2 = 12

   return 0;
}
```
Wrapping an argument in ```std::ref``` or ```std::cref``` ensures the utility stores a reference instead of a local copy. 

Binding by reference is particularly useful for working with large data structures where copying them would be inefficient. C++ does not allow containers of raw references (e.g., ```std::vector<int&>```) because references are not objects. The ```std::reference_wrapper``` can be used to store rebindable references in a collection.
```cpp
 int main() {
   int a = 1, b = 2;
   std::vector<std::reference_wrapper<int>> vec;
   
   vec.push_back(std::ref(a));
   vec.push_back(std::ref(b));

   for(int& i : vec) i *= 10; // Modifies original a and b

   std::cout << a << " " << b; // Output: 10 20
}
```
Also, standard algorithms like ```std::for_each``` take functors by value. If a functor is to be used and then its modified state should be inspected afterward, ```std::reference_wrapper``` comes in handy. 
```cpp 
 struct Counter {
   int count = 0;
   void operator()(int) { count++; }
};

int main() {
   std::vector<int> v = {1, 2, 3};
   Counter c;
   std::for_each(v.begin(), v.end(), std::ref(c));
   std::cout << c.count; // Output: 3 (without std::ref, this would be 0)
}
```
## Pitfalls
The ```std::reference_wrapper``` cannot be used on rvalues (temporary objects) because it would lead to immediate dangling references. It is restricted to lvalues only. It does not extend the lifetime of the object it refers to. The developer must ensure the original object outlives the wrapper. 
```cpp
auto get_callback() {
    int x = 42;
    // DANGER: Binding a reference to a local variable 'x'
    // 'x' will be destroyed when this function returns.
    return std::bind([](int& val) { std::cout << val; }, std::ref(x));
}

int main() {
    auto cb = get_callback();
    cb(); // UNDEFINED BEHAVIOR: 'x' no longer exists.
}
```

However, unlike a raw reference, which is permanently bound to its initial object, ```std::reference_wrapper``` can be reassigned to point to a different object. This *may* lead to bugs where developers expect to update the value of the referred object but accidentally rebind the wrapper to a different instance.
```cpp
int a = 10, b = 20;
std::reference_wrapper<int> ref = a;

ref = b; // DANGER: This does NOT make 'a' equal 20. 
         // It makes 'ref' now point to 'b'.

std::cout << a; // Still prints 10
// Fix: To update the value, you must use .get(): ref.get() = b;
```

The ```std::reference_wrapper``` provides an implicit conversion operator to T&. This is convenient but creates ambiguity in template deduction or when using the auto keyword.
```cpp
int x = 5;
auto r = std::ref(x); 

// 'r' is NOT an int&, it is a std::reference_wrapper<int>.
// If you pass 'r' to a template function expecting T, T becomes reference_wrapper.
```

A ```std::reference_wrapper``` must always refer to a valid object upon initialization. However, it can be initialized from a dereferenced null pointer, which the compiler may not catch.
```cpp
int* ptr = nullptr;
// This might compile but causes a crash/UB at runtime
std::reference_wrapper<int> ref = *ptr; 
```
