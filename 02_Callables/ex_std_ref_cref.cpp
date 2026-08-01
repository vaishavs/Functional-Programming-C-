#include <iostream>
#include <functional> // Required for std::ref and std::cref
#include <thread>
#include <vector>
#include <string>

void worker(int& n, const std::string& s) {
    n += 10;
    std::cout << "Thread updating n to: " << n << " with msg: " << s << "\n";
}

int main() {
    int x = 5;
    std::string msg = "Process";

    // --- SUCCESS CASES ---

    // 1. std::thread: Forces reference passing to the thread's function.
    // Without std::ref, std::thread attempts to copy arguments into internal storage.
    std::thread t(worker, std::ref(x), std::cref(msg)); 
    t.join(); // x is now 15

    // 2. std::bind: Bound arguments are copied by default. std::ref prevents this.
    auto bound_inc = std::bind(worker, std::ref(x), std::cref(msg));
    bound_inc(); // x is now 25

    // 3. STL Containers: vector<int&> is illegal, but reference_wrapper is an object.
    std::vector<std::reference_wrapper<int>> vec;
    vec.push_back(std::ref(x));
    vec[0].get() += 5; // x is now 30

    // --- ERROR CASES (Compilation Failures) ---

    // 4. Mutation Error with std::cref
    // std::cref(x) creates a reference_wrapper<const int>.
    // This cannot bind to worker(int& n) because it is const.
    // std::thread t_err(worker, std::cref(x), std::cref(msg)); // COMPILE ERROR

    // 5. R-value (Temporary) Error
    // std::ref and std::cref only bind to L-values (named variables).
    // They cannot bind to temporaries like literals.
    // auto r_err = std::ref(100); // COMPILE ERROR

    // --- RUNTIME ERROR (Dangling Reference) ---

    // 6. Lifetime Issue
    // std::ref does not extend the life of the variable.
    std::reference_wrapper<int> dangling = [&]() {
        int temp = 42;
        return std::ref(temp); // DANGER: temp is destroyed here
    }();
    // std::cout << dangling.get(); // UNDEFINED BEHAVIOR

    std::cout << "Final x: " << x << std::endl;
    return 0;
}
