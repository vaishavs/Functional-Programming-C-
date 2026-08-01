/*
 * This program is designed as a C++ debugging exercise. It intentionally contains several 
 * compile-time errors, logical bugs, and undefined behavior to demonstrate common pitfalls in 
 * standard library algorithm usage, iterator invalidation, and lambda function structure. 
 * The original goal was to filter students by name, extract their grades, and compute an 
 * average, but the implementation is fundamentally broken. The comments below have been 
 * expanded to thoroughly explain why each bug occurs and how it impacts the compiler or 
 * the runtime environment.
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <iomanip>

int main() {
    // This data structure represents a student, containing their name as a string, followed by
    // a nested pair storing their age as an integer and their current grade as a double.
    std::vector<std::pair<std::string, std::pair<int, double>>> students = {
        {"Alice", {20, 85.5}},
        {"bob", {17, 92.0}},
        {"Amy", {22, 78.2}},
        {"Charlie", {19, 88.1}},
        {"Anna", {21, 95.3}},
        {"Diana", {16, 76.4}}
    };
    
    // The expected outcome of this program is to correctly calculate the average grade of the
    // students whose names begin with the capital letter 'A' (specifically Alice at 85.5, Amy
    // at 78.2, and Anna at 95.3). 85.5 + 78.2 + 95.3 = 259.0, and 259.0 / 3 = 86.333..., so the
    // expected result is approximately 86.33 - not exactly 86.3 as originally stated here.
    
    // BUG #1: The output vector named 'grades' is currently completely empty and has not been
    // allocated any space or given a back inserter to properly store incoming elements.
    // HINT: Because the vector has not been resized or initialized with default elements,
    // calling grades.begin() points to the exact same memory location as grades.end(),
    // meaning there is no valid memory range available to write to.
    std::vector<double> grades;
    
    // BUG #2: The std::copy_if function is being called with an incorrect signature because it
    // is strictly designed to accept only four arguments - true since std::copy_if was first
    // introduced in C++11, and not a C++20-specific rule as originally stated here - but here
    // it is erroneously being provided with five arguments.
    // BUG #3: The method 'startsWith' does not actually exist anywhere in the standard C++
    // string library; you would need to specifically use 'starts_with' if you are compiling
    // with C++20, or alternatively rely on the 'find' method.
    // BUG #4: An extra lambda function intended for transformation has been passed as an
    // argument, but std::copy_if only copies elements that successfully meet a condition and
    // does not possess the inherent capability to transform them during the copy process.
    // PROOFREADING NOTE (added): simply deleting this fifth argument would not be enough to fix
    // this call on its own. With four arguments, std::copy_if would still try to copy a
    // std::pair<std::string, std::pair<int, double>> (dereferenced from `students`) directly
    // into a std::vector<double>, which fails to compile ("cannot convert ... to 'double' in
    // assignment") because copy_if only ever filters - it never changes the element type.
    // Producing `grades` from `students` genuinely needs two separate steps: filter, then
    // transform (e.g. copy_if into a same-typed container via std::back_inserter, then
    // std::transform the filtered results into grades).
    std::copy_if(students.begin(), students.end(), grades.begin(), 
        [](const auto& s) { 
            // HINT: If you attempt to compile this specific block of code, the compiler will
            // generate a clear error explicitly stating that there is no member function named
            // 'startsWith' associated with the std::string class.
            return s.first.startsWith("A");  
        },                                  
        [](const auto& s) { 
            // HINT: The std::copy_if algorithm is strictly designed to copy elements exactly
            // as they are without modifying them, so providing a secondary transformation
            // lambda here is invalid and will break the build.
            return s.second.second;          
        });
    
    // CRASH #1 (HYPOTHETICAL - see PROOFREADING NOTE above): if BUG #2-4 above were fixed so
    // this line actually compiled, writing directly to grades.begin() without utilizing a
    // dynamic iterator like std::back_inserter would very likely cause a segmentation fault at
    // runtime, because the program would be dereferencing and writing through an iterator that
    // belongs to a completely empty vector. As the file actually stands, though, this never
    // happens: the compile-time errors above stop the program from building at all, so there is
    // no running executable in which this write could occur.
    
    // BUG #5: The local variable 'invalid_it' is being assigned the result of grades.end(),
    // but because the previous std::copy_if statement is broken and does not properly populate
    // the vector as intended, this iterator still simply points to the end of a totally
    // empty container.
    auto invalid_it = grades.end();
    
    // BUG #6: The secondary lambda function provided to the std::transform algorithm is
    // entirely missing a return statement. Because the lambda has no explicit return type, the
    // compiler deduces one from its body - and with no return statement anywhere in it, that
    // deduced type is void. This is a COMPILE-TIME error, not undefined behavior as originally
    // stated here: std::transform internally does `*d_first = unary_op(*first)`, and a void
    // result cannot be assigned to the double that d_first points at ("error: void value not
    // ignored as it ought to be").
    // HINT: the tell-tale compiler warning is not "not all control paths return a value" - GCC
    // instead flags the discarded expression itself, with "warning: statement has no effect
    // [-Wunused-value]" pointing directly at the `s.second.second * 2;` line below, since that
    // computed value is thrown away rather than returned.
    std::transform(students.begin(), students.end(), invalid_it,
        [](const auto& s) { 
            // HINT: look for "void value not ignored as it ought to be" in the compiler output -
            // it points straight at the missing return, one level up in std::transform itself.
            s.second.second * 2;  // This expression computes a value but never returns it, so
                                  // the lambda's return type is deduced as void. On its own that
                                  // is only a warning (statement has no effect); the hard error
                                  // appears where std::transform tries to use that void result.
        });
    
    // BUG #7 : grades.begin() and invalid_it are not "mismatched" or "disconnected" -
    // they are actually the same iterator value. Nothing anywhere in this file ever adds an
    // element to grades (no push_back, no resize, no insertion), so grades.size() stays 0 for
    // the entire program, meaning grades.begin() == grades.end() == invalid_it throughout.
    // std::sort(it, it) on two equal iterators is a perfectly valid, well-defined empty range:
    // verified with g++, it compiles, runs, and silently does nothing - it does not crash. The
    // real bug traces back to BUG #1: grades never has anything in it to sort in the first
    // place.
    // HINT: don't assume a suspicious-looking std::sort call is using a broken range - check
    // whether the two iterators are actually equal first. Here they are, and both come from the
    // same container (grades), so the "range" is technically valid; it is just empty.
    std::sort(grades.begin(), invalid_it);
    
    // BUG #8: Calling std::accumulate on an empty vector will technically execute safely and
    // silently return the initial fallback value of 0.0, but it completely defeats the
    // underlying purpose of the mathematical calculation since there is absolutely no actual
    // data to sum.
    // BUG #9: The mathematical logic inside the custom accumulation lambda inexplicably
    // multiplies every incoming grade by 1.1, which looks like a misguided attempt at applying
    // a generous grading curve but ultimately results in an entirely incorrect sum calculation
    // for the dataset.
    double sum = std::accumulate(grades.begin(), grades.end(), 
        0.0, [](double acc, double g) { 
            // HINT: If you are genuinely trying to calculate a standard mathematical sum
            // without introducing any artificial grade inflation to the students, the correct
            // logic inside the lambda should simply return the current accumulator value added
            // directly to the current grade.
            return acc + g * 1.1;  
        });
    
    // BUG #10: The final arithmetic average calculation erroneously divides the accumulated
    // sum by the total number of students present in the original roster (which is 6), rather
    // than properly dividing by the actual count of students who were successfully filtered
    // into the grades vector (which should theoretically be 3).
    // HINT: At this exact point in the program's execution, 'grades.size()' is unfortunately
    // still equal to 0, but even if the earlier code blocks were miraculously fixed to
    // populate the vector correctly, using 'students.size()' as the mathematical denominator
    // would still inevitably yield a drastically incorrect average.
    double avg = sum / students.size();
    
    // BUG #11: The std::for_each algorithm strictly mandates that its very first argument must
    // be an iterator marking the beginning of a sequence, but in this flawed implementation,
    // it is incorrectly being passed a standard output stream object instead. On top of that,
    // std::for_each takes three arguments (first, last, function), and only two are supplied
    // here - the "last" iterator is missing entirely, so this is as much an arity error as a
    // type error.
    // BUG #12 : the original claim here - that std::for_each's return value is being
    // misused in "this specific formatting context" - does not match this code: nothing ever
    // reads or prints for_each's return value anywhere. The real second problem with this
    // lambda is that its capture list is empty ([]), yet its body reads the outer local
    // variable `avg`, which is never captured. That alone is a separate compile error,
    // verified with g++: "error: 'avg' is not captured ... the lambda has no capture-default".
    // Fixing it needs an explicit capture, e.g. [avg] or [&].
    std::for_each(std::cout, [](double x) { 
        // HINT: The correct usage of std::for_each requires the programmer to provide a valid
        // half-open iterator range denoted conceptually by [first, last) followed by a function
        // to execute over that range, rather than trying to pass an output stream directly to
        // the algorithm's first parameter.
        std::cout << avg << std::endl;       
    });  // Because of the wrong argument count and type in the std::for_each call, and the
         // uncaptured 'avg' inside the lambda, this specific line of code will entirely fail to
         // compile and halt the build process. Note, too, that even if all of that were fixed,
         // this lambda ignores its own parameter 'x' and would print the same 'avg' value once
         // per element instead of each individual grade.
    
    // The floating-point variable 'avg' has been computed using deeply flawed logic and
    // entirely empty data structures - but it is not "garbage" in the sense of an
    // unpredictable, uninitialized value. Per BUG #8 above, std::accumulate on an empty range
    // deterministically returns exactly its init value (0.0), so sum == 0.0 and
    // avg == 0.0 / 6 == exactly 0.0 every single time this runs. This final output statement
    // will print a clean, reproducible - but completely meaningless - "Result: 0" to the
    // console.
    std::cout << "Result: " << avg << std::endl;
    return 0;
}
