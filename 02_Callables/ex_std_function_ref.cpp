/* Since C++26 */
#include <iostream>
#include <functional>

// 1. Free Function
void print_message(const std::string& msg) {
    std::cout << "Free function msg: " << msg << std::endl;
}

// 2. Functor (Function Object)
class MessageFunctor {
public:
    void operator()(const std::string& msg) const { // Overload the function call operator
        std::cout << "Functor msg: " << msg << std::endl;
    }
};

// Function that accepts a std::function_ref to a callable with a specific signature
void call_wrapper(std::function_ref<void(const std::string&)> func_ref) {
    func_ref("Hello, world!");
}

int main() {
    // --- Using a function pointer ---
    // The function pointer is implicitly convertible to std::function_ref
    void (*func_ptr)(const std::string&) = &print_message;
    call_wrapper(func_ptr); // The function reference is non-owning

    // --- Using a function ---
    // The free function is implicitly convertible to std::function_ref
    call_wrapper(print_message);

    // --- Using a raw function reference ---
    // The function reference is implicitly convertible to std::function_ref
    void (&func_ref)(const std::string&) = print_message;
    call_wrapper(func_ref);

    // --- Using a functor (function object) ---
    // The functor object (an lvalue) is implicitly convertible to std::function_ref
    MessageFunctor my_functor;
    call_wrapper(my_functor);
    
    // --- Using a lambda (which is a form of a functor) that outlives std::function_ref ---
    auto lambda_func = [](const std::string& msg) {
        std::cout << "Lambda msg: " << msg << std::endl;
    };
    call_wrapper(lambda_func);
    
    return 0;
}
