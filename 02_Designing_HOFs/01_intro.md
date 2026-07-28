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
Every function occupies a location in the compiled program, so it has an address. A function pointer is a pointer whose pointed-to type is a function type. It is a variable that stores the memory address of a function. The signature of a function pointer must match the return type, calling convention, and parameter list of the function it points to.

### Global/Namespace function pointer
A global or namespace function pointer is declared as:
```cpp
return_type (*funcPtr)(parameter_types);
```
The the address of a function is then assigned to it.
```cpp
funcPtr = &myFuncName; // '&' is optional; 'funcPtr = myFuncName;' also works
```
Finally, the function is invoked via its pointer:
```cpp
auto result = funcPtr(/* ... */);
```

In almost every context, the name of a function is implicitly converted to a pointer to that function. This is called function-to-pointer decay. It is why the address-of operator `&` is usually optional when taking a function's address, and why a function name can be assigned directly to a function pointer.

For example, consider a function
```cpp
int add (int a, int b)
{
    return a+b;
}
```
Its function pointer would be:
```cpp
// 1. Declaration: return_type (*pointer_name)(parameter_types);
int (*funcPtr)(int, int);

// 2. Initialization: Assign the address of a function
funcPtr = &add; // '&' is optional; 'funcPtr = add;' also works

// 3. Invocation: Call the function via the pointer
int result = funcPtr(10, 5); // Equivalent to add(10, 5)
```

Function pointers can also be used with ```typedef```.
```cpp
typedef int (*funcPtr)(int, int);
// ...
funcPtr f = add; // OR, funcPtr f = &add;
int res = f(10, 5);
```

### Member function pointers
For non-static member functions, the address is not an ordinary function pointer — it is a pointer to member, which requires the `&Class::member` form and cannot be stored in a variable. Unlike a free function, whose name decays to a pointer, a member function name does not decay. The address must be taken explicitly with `&`, and the name must be qualified with the class:
```cpp
ReturnType (ClassName::*ptrName)(Params) = &ClassName::Member;
```
And the invocation is done using ```.*``` or ```->*``` operator.
For example:
```cpp
class Calculator {
public:
    int base;
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

    // Using the .* and ->* operators
    Calc c{/*base=*/100};
    Calc* pc = &c;
    
    (c.*pAdd)(5);     // 105 — object .*  member-pointer, then call
    (pc->*pAdd)(7);   // 107 — pointer ->* member-pointer, then call
}
```
The parentheses around `c.*pAdd` are mandatory. The call operator `()` binds more tightly than `.*` and `->*`, so without the paranthesis, the statement would not make sense. The cv- and ref-qualifiers of a member function are part of the pointer type, so a pointer to a `const` member has a different type from a pointer to a non-const member:
```
struct Calc {
    int base;
    int addc(int x) const { return base + x; }
};

int (Calc::*pAddc)(int) const = &Calc::addc;   // note the trailing const
(c.*pAddc)(3);   // 103
```
A `int (Calc::*)(int)` and a `int (Calc::*)(int) const` **cannot** be assigned to one another.

A pointer to member function that refers to a virtual member still dispatches at run time to the correct override for the object it is called on. The pointer names the virtual member; the object determines which override runs:
```
struct Calc  { int base; virtual int scale(int x) const { return base * x; }
               virtual ~Calc() = default; };
struct Calc2 : Calc { int scale(int x) const override { return base * x * 10; } };

int (Calc::*pScale)(int) const = &Calc::scale;

Calc2 d{/*base=*/2};
Calc&  ref = d;
(ref.*pScale)(5);   // 100 — Calc2::scale runs (2 * 5 * 10), not Calc::scale
```
This is an important guarantee: a pointer to member is polymorphic-aware. Storing `&Calc::scale` and calling it on a `Calc2` invokes `Calc2::scale`, exactly as a direct virtual call would. Virtual functions therefore add no special case here; they work through the same pointer-to-member machinery.

Because the `.*`/`->*` syntax is awkward and precedence-prone, generic code — and much ordinary code — calls a pointer to member through `std::invoke`, which accepts the member pointer followed by the object and the arguments. The `std::invoke` handles all the variants uniformly.
```
std::invoke(pAdd, c, 9);                 // 109  == (c.*pAdd)(9)
std::invoke(&Calc::add, Calc{50}, 1);    // 51   — object may be a temporary
```

A pointer to member function is typically larger than a data pointer, because on common ABIs, it must encode enough information to handle virtual members and multiple inheritance. The exact size is implementation-defined. Two further consequences matter in practice:
* a pointer to member cannot be converted to `void*` or to any ordinary pointer type, and it should not be assumed to fit in a machine word.
* a pointer to a member of a base class converts implicitly to a pointer to a member of a derived class, because a derived object also has that base member. The direction is base-to-derived, which is the opposite of the base/derived direction for ordinary object pointers, and reflects that every derived object contains the base's members.

Since `noexcept` is part of a function's type, the address of a `noexcept` function has a noexcept-qualified pointer type. The conversion rule between the two is directional:
```
void hello() noexcept { /* ... */ }
static_assert(std::is_same_v<decltype(&hello), void(*)() noexcept>);  // holds

void (*plain)() = hello;   // OK: noexcept pointer → plain pointer (widening the promise)
```
The allowed direction is `noexcept`-pointer to plain-pointer: assigning a stronger promise where a weaker one is expected is always safe. The reverse — storing a possibly-throwing function where a `noexcept` one is required — is ill-formed, because it would silently break the guarantee. Keeping this direction straight avoids a class of subtle template errors.

### Operations allowed
1. Assign an address of a function
2. Compare 2 function pointers
3. Call a function using the pointer
4. Pass the function pointer as an argument to another function
5. Return a function pointer
6. Store them in arrays

#### Surrogate Call Functions
In C++, **surrogate call functions** are implicit entities *synthesized by the compiler* during overload resolution. The mechanism activates when an object of class type is invoked using function call syntax, but instead of possessing a direct `operator()`, the class provides a user-defined conversion operator that yields a function pointer or a function reference.

Consider a class that provides only a conversion to a function pointer:
```
struct Handler {
    using Fp = int(*)(int);
    operator Fp() const { return [](int x){ return x + 1; }; }   // conversion to fn pointer
    // note: there is no operator() here
};

Handler h;
h(41);   // 42
```
The call `h(41)` type-checks even though `Handler` has no call operator. The compiler sees the conversion to `int(*)(int)`, synthesizes a surrogate call function of the form "take a `Handler` and an `int`, convert the Handler to `int(*)(int)`, and call it with the `int`", and uses that to evaluate `h(41)`. Effectively, the call becomes `(h.operator Fp())(41)`.

This is a somewhat exotic capability to build deliberately, but it exists so that the language can define, uniformly, what it means to call any object — including those whose "callability" is expressed as a convertibility to a function.

When evaluating a function call expression `object(arguments)`, the compiler constructs a candidate set for overload resolution, which includes:

* Standard member `operator()` functions.
* **Surrogate call functions** derived from applicable conversion operators.

For every conversion operator yielding a function pointer or function reference type with a specific signature, a corresponding "surrogate" call function is implicitly generated. If overload resolution selects the surrogate, execution involves two steps:

1. Invoking the conversion operator to obtain the function pointer.
2. Invoking the underlying function using the obtained pointer.

A class may offer several conversions to different function-pointer types. The compiler then synthesizes one surrogate call function per conversion, and ordinary overload resolution selects among them based on the call arguments — the same resolution process that chooses among overloaded functions.

```
using FpI = long(*)(int);
using FpD = long(*)(double);

struct Multi {
    operator FpI() const { return [](int)    -> long { return 1; }; }
    operator FpD() const { return [](double) -> long { return 2; }; }
};

Multi m;
m(10);    // 1 — an int argument makes the FpI surrogate the best match
m(3.5);   // 2 — a double argument selects the FpD surrogate
```
Each surrogate participates as a candidate; the one whose parameter best matches the argument wins. If both an `operator()` and one or more conversions are present, all of them compete together, and an unbreakable tie is an ambiguity error — the usual overload-resolution outcome.

Surrogate call functions can also participate in standard overload resolution alongside `operator()` members. The compiler evaluates the arguments against all available options to determine the best match.

```cpp
#include <iostream>

void fallback_function(double) {
    std::cout << "Surrogate invoked (double).\n";
}

struct OverloadDemo {
    // Standard function call operator
    void operator()(int) const {
        std::cout << "Member operator() invoked (int).\n";
    }

    // Conversion yielding a surrogate call function candidate
    operator void(*)(double)() const {
        return fallback_function;
    }
};

int main() {
    OverloadDemo demo;

    demo(10);    // Exact match for int: Invokes operator()(int)
    demo(3.14);  // Exact match for double: Invokes surrogate function via conversion

    return 0;
}

```

## Function references
Just as there is a pointer to a function, an alias (reference) can be created for a function name. The function name binds directly to the reference rather than decaying to a pointer.

**Syntax**
```cpp
return_type (&referenceName)(param_list)
```
The `&` goes next to the name, wrapped in parentheses to bind tighter than the return type.

For example:
```cpp
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

A function reference must be initialized when declared (there is no "null" reference), and like all references it cannot be uninitialized or rebound to a different function afterward. Once a reference is bound to an object (or function), it cannot be changed to refer to another one.

Taking the address of a function reference gives you a pointer to the underlying function.
```cpp
int (*p)(int, int) = &fref;        // p points to add
```
The call syntax of a function reference is identical to that of a function pointer — `fref(...)`, `fptr(...)`, and `(*fptr)(...)` all work — but the difference is that a reference binds to a function without that decay.

Function references show up most often in template deduction. Passing a function by reference deduces the function type; passing by value decays it to a pointer:
```cpp
template <typename T> void byRef(T& f);   // T = int(int,int), f is a function reference
template <typename T> void byVal(T  f);   // T = int(*)(int,int), f is a function pointer

byRef(add);   // f is a reference to add
byVal(add);   // f is a pointer to add
```
The most common place a reference to a function shows up — often without the author intending it — is template argument deduction. When a function name is passed to a forwarding parameter `F&&`, the deduced type is a reference to function, not a function pointer:
```
template<class F>
void probe(F&&);      // when called as probe(add), F deduces to int(&)(int, int)
```
For `probe(add)`, `F` is a reference type whose referent is a function type. This is why generic code that forwards callables must be written to accept function references gracefully; facilities such as `std::invoke` and `std::function` are specified to handle a referent function correctly. A related rule, reference collapsing, governs what happens when references stack during deduction: `T(& &)`, `T(& &&)`, and `T(&& &)` all collapse to the lvalue reference `T(&)`, and only `T(&& &&)` yields an rvalue reference. 

## Functors
A functor is basically a class that overloads the function call operator (`()`), allowing an instance of a class to be called like a function. A functor's type is known at compile-time, so compilers can often inline the function logic directly into the calling code. This makes functors generally faster than function pointers.
For example:
```cpp
#include <iostream>

// A functor
class AdderFunctor {
public:
    int operator() (int a, int b) { return a + b; }
} aF;

int main()
{
    int x = 5, y = 7;
    
    int zf = aF(x, y); // aF.operator()(x,y)
    std::cout << "zf = " << zf << std::endl;
}
```
There are three big advantages over a regular function or a function pointer: 
* They can carry state. A function pointer is just an address; a functor is an object with member variables, so it can remember things between calls or be configured at construction.
* They're cheap to inline. Each functor is a distinct type, so when you pass it to a template the compiler knows exactly which `operator()` to call and can inline it. A function pointer is opaque at the call site and usually cannot be inlined.
* They integrate with the STL.

State doesn't have to be mutable — it can be constructed on the go.
Consider an example:
```cpp
class MultiplyBy {
    int factor;
public:
    explicit MultiplyBy(int f) : factor(f) {}
    int operator()(int x) const { return x * factor; }
};

MultiplyBy times3(3);
times3(10);   // 30
```
Now, `times3` behaves like a specialized `multiply-by-3` function. This creates a family of functions parameterized by factor. In other words, a factory of functions can be created by configuring an additional member of a functor.

A functor can also change its state after multiple calls, the way a function changes global or static variables to record state changes.
```cpp
struct Accumulator {
    int sum = 0;
    int operator()(int x) {   // note: NOT const, it mutates
        sum += x;
        return sum;
    }
};

Accumulator acc;
acc(10);   // 10
acc(20);   // 30
acc(5);    // 35  — state persists in acc.sum
```

The C++ Standard Library provides various built-in functors for common operations in the ```<functional>``` header: 
* Arithmetic: ```std::plus```, ```std::minus```, ```std::multiplies```, ```std::divides```, ```std::modulus```.
* Relational: ```std::equal_to```, ```std::not_equal_to```, ```std::greater```, ```std::less```, ```std::greater_equal```, ```std::less_equal```.
* Logical: ```std::logical_and```, ```std::logical_or```, ```std::logical_not```. 

One interesting modern C++ feature is that Lambdas *are* functors. The compiler generates an anonymous functor (a "closure type") when it encounters a lambda. The capture list becomes member variables, the body becomes the `operator()` body, and the call operator is const by default.

That is,
```cpp
int threshold = 10;
auto pred = [threshold](int x) { return x > threshold; };
```

is conceptually equivalent to:
```cpp
class __anonymous {
    int threshold;
public:
    explicit __anonymous(int t) : threshold(t) {}
    bool operator()(int x) const { return x > threshold; }
};
auto pred = __anonymous(threshold);
```

Adding mutable makes it non-const so the body can modify captured-by-value members:
```cpp
auto counter = [n = 0]() mutable { return ++n; };   // mutable: can change n
counter();  // 1
counter();  // 2
```
That's exactly the `Accumulator` pattern from earlier, written compactly. Captures by reference (`[&x]`) become reference members.


A functor can also be made generic to support any data type:
```cpp
struct Print {
    template <typename T>
    void operator()(const T& x) const {
        std::cout << x << '\n';
    }
};

Print p;
p(42);        // works
p("hello");   // works
p(3.14);      // works
```

Its equivalent lambda is:
```cpp
auto print = [](const auto& x) { std::cout << x << '\n'; };
```

To evaluate a functor at compile-time, `operator()` can be constexpr and marked `noexcept`. For example:
```cpp
struct Square {
    constexpr int operator()(int x) const noexcept { return x * x; }
};
constexpr int nine = Square{}(3);   // evaluated at compile time
```

## Modern C++ Alternatives (2025 Context)
While raw function objects are efficient, modern C++ (C++11 and later) provides more flexible alternatives: 
* ```std::function```: A type-safe wrapper that can store function pointers, lambdas, or functors.
* Lambdas: Anonymous functions that can be passed directly to other functions without declaring a named function (internally converted to a functor by the compiler).
* ```std::invoke```: A universal way to call any callable (added in C++17) that simplifies the syntax for member function pointers.
* ```std::function_fref```: A type-safe function reference

Reference: https://www.youtube.com/watch?v=i7-jWzWOBbk&t=79s
