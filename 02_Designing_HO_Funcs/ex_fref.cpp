/* Function reference */
#include <iostream>

void greet() { std::cout << "Hello!"; }

int main() {
    void (&ref)() = greet; // ref is now an alias for greet
    ref();                 // Calls greet()
}
