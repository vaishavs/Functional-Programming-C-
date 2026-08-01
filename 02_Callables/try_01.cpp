#include <iostream>

using FuncPtr = void(*)(int);

// 1 & 3. Free Function
void doubleIt(int n) { 
    std::cout << (n * 2) << "\n"; 
}

// 2, 4 & 5. All-in-One Class: Static Method + Functor + Surrogate Conversion
class MathHelper {
public:
    // 2. Static member function
    static void printSquare(int n) {
        std::cout << (n * n) << "\n";
    }

    // 4. Functor call operator
    void operator()(int n) const {
        std::cout << (n + 10) << "\n";
    }

    // 5. Conversion operator to function pointer
    operator FuncPtr() const {
        return printSquare;
    }
};

int main() {
    int val = 5;

    std::cout << "1. Free Function Pointer: ";
    // TODO 1: Declare function pointer 'fp' pointing to 'doubleIt', then call fp(val)
    // HINT 1: Syntax: void (*fp)(int) = &doubleIt;
    /* TODO 1 */


    std::cout << "2. Static Member Function Pointer: ";
    // TODO 2: Declare function pointer 'sfp' pointing to 'MathHelper::printSquare', then call sfp(val)
    // HINT 2: Syntax: void (*sfp)(int) = &MathHelper::printSquare;
    /* TODO 2 */


    std::cout << "3. Function Reference: ";
    // TODO 3: Declare function reference 'fr' referencing 'doubleIt', then call fr(val)
    // HINT 3: Syntax: void (&fr)(int) = doubleIt;
    /* TODO 3 */


    std::cout << "4. Functor Call: ";
    MathHelper helper;
    // TODO 4: Call 'helper' directly as a functor with 'val'
    // HINT 4: helper(val); (Invokes MathHelper::operator())
    /* TODO 4 */


    std::cout << "5. Surrogate Function Call / Implicit Conversion: ";
    // TODO 5: Assign 'helper' to a function pointer 'sfc' to trigger operator FuncPtr(), then call sfc(val)
    // HINT 5: FuncPtr sfc = helper; sfc(val);
    /* TODO 5 */


    std::cout << "6. Lambda: ";
    int bonus = 100;
    // TODO 6: Write an inline lambda capturing 'bonus' by value and call it with 'val'
    // HINT 6: auto myLambda = [bonus](int n) { std::cout << (n + bonus) << "\n"; }; myLambda(val);
    /* TODO 6 */


    return 0;
}
