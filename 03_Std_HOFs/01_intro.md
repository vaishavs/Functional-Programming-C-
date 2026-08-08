# Components of standard HOFs
The implementation of standard higher order functions in C++ is based on the following components:
## Callable entities
These are the function types such as function pointers, functors, lambdas, etc., that are passed around (see [03_HOF.md](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/03_HOF.md)). They are used to provide specialized functional logic, such as:
* Predicate: A function that takes one or two arguments and returns a bool.
* Generator: A function that takes no arguments and returns values to fill a range.
* Operator: A function that produces specific results based on the inputs provided.
* Execution policy: A function that makes use of multi-core processors. (Since C++17)
## Execution Contexts
These are the containers or ranges on which various operations are performed. They are:
* Containers → `std::vector`, `std::list`, `std::map`, etc.
* Iterators → Mechanism to access elements of a container/range, e.g., `std::iterator`, etc.
* Ranges → More expressive composition of HOFs that eliminate the need to manually pass begin/end iterators. (Since C++20)
* Seed values → Initial values needed as starting point for certain callables, e.g., for reductions (`std::accumulate` needs a starting value).
## Algorithms/wrappers
Algorithms are standard higher-order functions that accept callables and apply them on containers/ranges. These are the wrappers that define the logic for how an operation should be executed. They apply the callable on the data. They can be categorized in the following manner:
| Category |	Typical Logic Type|	Purpose	|Example Algorithm| Signature Expectation |
|--------- | ------------------ | ------- | --------------- | --------------------- |
| Non-Modifying |	Predicate(Unary/Binary) |	Searching & Validation |	`find_if`, `all_of` | `(T) -> bool` |
| Modifying |	Unary/Binary Op |	Transformation |	`transform`, `generate` | `(T) -> U` |
| Sorting |	Comparator (Binary predicate) |	Custom Ordering |	`sort`, `stable_sort` | `(Acc, T) -> Acc` |
| Numeric |	Binary Op |	Accumulation/Reduction |	`accumulate`, `reduce` | `(T, T) -> bool` |
| Parallel |	Execution Policy |	Threading/Concurrency | `seq`, `par`, `par_unseq` |	`() -> T` |

To illustrate diagrammatically:

[![Copilot-20260214-125707.png](https://i.postimg.cc/RVWj6PPY/Copilot-20260214-125707.png)](https://postimg.cc/D8KCRc3Q)
# Workflow
A standard HOF typically follows this structure:
```
algorithm(execution_policy?, range_begin, range_end, seed?, callable);
```
For example:
```cpp
int sum = std::accumulate(
    vec.begin(),
    vec.end(),
    0,
    [](int a, int b) { return a + b; }
);
```
Here:
* Execution context → `vec.begin()`, `vec.end()`
* Seed value → `0`
* Callable (operator) → `[](int a, int b) { return a + b; }`
* Algorithm (HOF) → `std::accumulate`

Standard HOFs in C++ are built on:
* Behavior (Callable entities): Defines what operation should be performed.
* Data (Containers, iterators, ranges): Defines where the operation is applied.
* Control (Execution policy & seed values): Defines how and from what starting point the operation is executed (see [Control parameters](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/03_Std_HOFs/02_note.md#14-control-parameters)).

Implementing a standard higher function involves the following steps:
### Step 1: Define a callable
The first step is to define a callable that contains what the desired logic. It should define what operation should be performed. This essentially involves creating a lambda, functor, or a function pointer.
### Step 2: Choose an algorithm
The next step is to select an algorithm from the standard library that suits our purpose. This step defines how the callable will be applied.
### Step 3: Provide a data/range
The execution context, i.e., the data needs to be supplied, on which the algorithm would operate. This step involves supplying a container or iterator range.

[![Copilot-20260214-144124.png](https://i.postimg.cc/xTP95pGX/Copilot-20260214-144124.png)](https://postimg.cc/VddyLRVw)

