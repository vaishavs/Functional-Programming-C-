# Introducton to functional programming in C++
C++ had initially started out as an imperative programming language. That is, there were statements and variables, and the statements manipulated the variables to do a computation. In simple workds, there were a series of instructions on how to do a computation, much lke an IKEA manual. This is also how the CPU works, so it was only reasonable. In contrast to imperative programming, there is declarative programming, where the programmer only specifies the outcome, and the system figures out how to do best achieve it. And functional programming is a declarative programming that uses composed functions to achieve this.

For instance, consider a factorial of an integer: 

```
int fac(unsigned int n)
{
  return (n == 0) ? 1 : (n * fac(n-1));
 }
```
 And assuming we want to filter odd factorials and sort them in descending, in imperative programming, this takes up a lot of steps and intermediate states, making the code error-prone and hard to read and debug. Functional programming simplifies this by eliminating intermediate states. This also means there is a signiicant overhead on the hardware. Using functional programming is a trade-off between performance and correctness. Fortunately, C++ is not a pure functional language, so it has the best of both worlds. We can only use it where it makes sense.
 
## The building blocks 
The way functional programming is used in C++ is to write building blocks using regular imperative-style code, and compose those blocks using functional paradigms.  These building blocks are efficient, and the composed structure is easy. In C++, 3 things are composed in particular:
1. Algorithms
2.  Functions
3.  I/O
 
### Composing algorithms
Consider a problem statement, where we have to find the biggest magnitude of an odd integer in the list [3, 0, 2, -1, 5, -7, 8].  In the regular imperative programming, it would look something like this:
```
int biggest_odd_magnitude(auto&& rng)
{
   int candidate = -1;
  for(int x: rng) {
     int magnitude = std::abs(X);
     if(magnitude % 2 == 1) {
       candidate = (magnitude > candidate) ? magnitude :  candidate;
    }
   }
   return candidate;
 }
```
 
This code can be divided into 3 subproblems: 
i) Compute the magnitude of each element
ii) Keep only odd ones 
iii) Find the maximum
 
After composition, the code looks like:
``` 
namespace stdr = std::ranges;
namespace stdv = std::views;
 int biggest_odd_magnitude(auto&& rng)
{
   return stdr::max( // 3. Maximum
          rng
           | stdv::transform([](int x) {return std::abs(x); }) // 1. Compute the magnitudes
           | stdv::filter([](int x) {return x % 2 == 1; }) // 2. Keep the odd values   );
 }
```
A range refers to a pair of two iterators, the head and the tail of a sequence (or a sentinel in some cases).
Some common building blocks in C++ are:
1. Creating ranges: iota, generator
2.  Transforming ranges: transform, filter
3. Combining ranges: cartesian_product, zip, concat
4. Splitting ranges: split, chunk_by
5. joining ranges: join, join_with
 
Some commonly used algorithms are:
 1. Folds: max*/min*, cout*, all_of/any_of/none_of
 2. Searches: find*, search*
3. Sorting: *sort
 
Using them minimizes the use of the for loop.
 
### Composing functions
Consider an example that doubles numbers upto 500 using string input.  The exception is represented by std::unexpected.
Let's first define error types:
```
// Define the error type as a simple string for this example
 using Error = std::string;
 // Define a type alias for a result that is an int or an Error
 using ExpectedInt = std::expected<int, Error>;
```
And functions that might fail: 
```
// 1. A function that might fail (converts std::string to integer)
 ExpectedInt stringToInt(const std::string& s)
{
     int value;
     auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc()) {
         return value; // Success
     } else {
         return std::unexpected<Error>("Failed to convert string to int: " + s); // Error
     }
}

// 2. A function that doubles a value and might fail if the result is more than 1000
ExpectedInt multiplyByTwo(int value) {
     if (value > 1000) {
        return std::unexpected<Error>("Value is too large to multiply by two: " + std::to_string(value)); // Error
     }
     return value * 2; // Success
 }
 
// 3. A function to handle the final result or an error
void handleResult(ExpectedInt result) {
    if (result.has_value()) {
         std::cout << "Final result: " << result.value() << std::endl;
     } else {
         std::cout << "Operation failed: " << result.error() << std::endl;
    }
}
```
 
Now, the composed code looks like: 
```
// Input list
 std::string list[] = {  "123",  // Success
                        "abc",  // Failure: the input is not a number
                        "600" };  // Failure: the double of the input is more than 1000
 
for (int i=0; i<3; i++) {
  auto result = stringToInt(list[i])
                .and_then(multiplyByTwo) // Chained operation if previous one succeeded
                .or_else([](Error e) {   // Fallback if any previous operation failed
                  std::cerr << "Caught error in or_else: " << e << std::endl;
                   // The callable to or_else must return an std::expected of the same value type
                   return std::unexpected<Error>("Recovery failed");
                 };
  handleResult(result);
 }
```
The compositions used in this category are:
1. .transform ->  to process a value
2. .transform_error -> to process an error
3. .and_then -> tochain with another optional/failible function
4. .or_else -> to chain with an error handling function

Unfortunately, mixing different kinds of optionalness/failure is diffucult. But composing using .transform/.and_then minimizes the use of if.
 
### Composing I/O
Pure functions are those which always produce the same value for the same input without side-effects. This essentially means no global state, and no functions interacting with the outside world. The io_action represents the action of reading/writing variables. However, in C++, functions need not be pure, but it is still a good idea to separate I/O from computation, by composing actions and executing them later. Consider an example where an integer is read as an upper limit, the fibonacci numbers are computed, and the results are written, asynchronously.
```
namespace stdx = std::execution;
 stdx::sender auto async_read_int();
 stdx::sender auto async_write_int(int i);
 stdx::sender auto read_and_write(stdx::scheduler auto scheduler auto sched)
{
   return stdx::schedule(sched) // Schedules threads
          | stdx::let_value(async_read_int) // Reading the integer
          | stdx::then([](int i) {
           return fib(i); // Computes fibonacci
          })
          | stdx::let_value(async_write_int); // Writing the result
}
```
The above example uses multithreading for asynchronosity. The std::execution::schedule starts asynchronous work, std::execution::then maps values, and std::execution::let_value chains new senders. Together, they form a pipeline for asynchronous programming in C++26, resulting in composability similar to Haskell’s do notation or Scala’s flatMap.

Source: https://www.youtube.com/watch?v=lvlXgSK03D4 

Note: The examples should be compiled using the latest GCC compiler with latest std specified.
