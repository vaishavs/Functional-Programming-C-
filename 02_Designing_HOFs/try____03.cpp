/* ============================================================================
 *  C++ CALLABLES DEBUG CHALLENGE
 * ============================================================================
 *
 * INSTRUCTIONS:
 * 1. Read each scenario and its associated HINT block.
 * 2. Fix the broken code lines inside main() to make the file compile and run.
 * 3. Compare your fixes with the solution commentary at the bottom!
 * ============================================================================
 */

#include <iostream>

using FuncPtr = void(*)(int);

// Free Function
void doubleIt(int n) { 
    std::cout << (n * 2) << "\n"; 
}

// All-in-One Helper Class
class MathHelper {
public:
    // Static member function
    static void printSquare(int n) {
        std::cout << (n * n) << "\n";
    }

    // Functor call operator
    // ------------------------------------------------------------------------
    // HINT FOR SCENARIO 4: Look closely at the qualifier on this operator.
    // If a MathHelper object is marked 'const', can C++ call this member function?
    // ------------------------------------------------------------------------
    void operator()(int n) { // <--- BUG 4 IS HERE
        std::cout << (n + 10) << "\n";
    }

    // Conversion operator for surrogate call
    // ------------------------------------------------------------------------
    // HINT FOR SCENARIO 5: What does the 'explicit' keyword do?
    // Does it allow implicit conversions like: FuncPtr sfc = surrogateObj; ?
    // ------------------------------------------------------------------------
    explicit operator FuncPtr() const { // <--- BUG 5 IS HERE
        return printSquare;
    }
};

int main() {
    int val = 5;

    // ========================================================================
    // SCENARIO 1: Free Function Pointer Syntax
    // "The developer wants to store 'doubleIt' in a raw function pointer 'fp'."
    // ========================================================================
    /* 
     *  DETAILED HINT 1:
     * - Compiler error: 'fp' declared as function returning 'void*', not a pointer.
     * - WHY: C++ operator precedence evaluates '()' before '*'. Writing 
     *   'void *fp(int)' tells C++ you are prototyping a function named 'fp'
     *   that accepts an int and returns a void pointer (void*).
     * - FIX: Use grouping parentheses '(*fp)' to attach the asterisk to the 
     *   variable name: void (*fp)(int) = &doubleIt;
     */
    // void *fp(int) = &doubleIt; // <--- BROKEN LINE 1
    // fp(val);


    // ========================================================================
    // SCENARIO 2: Static Member Function Pointer
    // "The developer wants to take the address of the static member 'printSquare'."
    // ========================================================================
    /* 
     *  DETAILED HINT 2:
     * - Compiler error: cannot convert 'void(*)(int)' to 'void(MathHelper::*)(int)'.
     * - WHY: Member pointer syntax ('Class::*') is reserved for INSTANCE methods 
     *   because they carry an implicit 'this' pointer. Static functions do NOT 
     *   have a 'this' pointer—they live in class scope but behave like regular free functions.
     * - FIX: Remove 'MathHelper::' from the pointer type declaration:
     *   void (*sfp)(int) = &MathHelper::printSquare;
     */
    // void (MathHelper::*sfp)(int) = &MathHelper::printSquare; // <--- BROKEN LINE 2
    // sfp(val);


    // ========================================================================
    // SCENARIO 3: Function Reference Binding
    // "The developer wants to bind a function reference 'fr' to 'doubleIt'."
    // ========================================================================
    /* 
     *  DETAILED HINT 3:
     * - Compiler error: 'fr' declared as reference but not initialized.
     * - WHY: Function references follow the same lifetime rules as variable 
     *   references (int&). They are immutable aliases and MUST be bound to a 
     *   target function at the exact moment of declaration.
     * - FIX: Combine declaration and binding into one statement:
     *   void (&fr)(int) = doubleIt;
     */
    // void (&fr)(int);    // <--- BROKEN LINE 3a
    // fr = doubleIt;      // <--- BROKEN LINE 3b
    // fr(val);


    // ========================================================================
    // SCENARIO 4: Const-Correctness on Functors
    // "The developer passes a const instance of 'MathHelper' to a read-only context."
    // ========================================================================
    /* 
     *  DETAILED HINT 4:
     * - Compiler error: passing 'const MathHelper' as 'this' argument discards qualifiers.
     * - WHY: Invoking 'helperObj(val)' calls 'MathHelper::operator()(int)'. 
     *   Because 'helperObj' is 'const', C++ forbids calling any member function 
     *   that isn't explicitly promised not to modify the object state.
     * - FIX: Mark operator() as const inside the class definition:
     *   void operator()(int n) const { ... }
     */
    // const MathHelper helperObj;
    // helperObj(val); // <--- BROKEN LINE 4


    // ========================================================================
    // SCENARIO 5: Surrogate Conversion Operator
    // "The developer tries to implicitly trigger the conversion operator to get a FuncPtr."
    // ========================================================================
    /* 
     *  DETAILED HINT 5:
     * - Compiler error: no viable conversion from 'MathHelper' to 'FuncPtr'.
     * - WHY: 'operator FuncPtr()' is marked 'explicit' in the class definition. 
     *   The 'explicit' keyword prevents implicit assignments like 'FuncPtr sfc = obj;'.
     * - FIX: Either remove 'explicit' from the class definition OR perform an 
     *   explicit cast in main(): FuncPtr sfc = static_cast<FuncPtr>(surrogateObj);
     */
    // MathHelper surrogateObj;
    // FuncPtr sfc = surrogateObj; // <--- BROKEN LINE 5
    // sfc(val);


    // ========================================================================
    // SCENARIO 6: Lambda Mutability
    // "The developer writes a lambda capturing 'bonus' by value to accumulate a total."
    // ========================================================================
    /* 
     *  DETAILED HINT 6:
     * - Compiler error: cannot assign to a variable captured by copy in a non-mutable lambda.
     * - WHY: By default, value captures ('[bonus]') are stored as read-only (const) 
     *   members inside the compiler-generated lambda closure object.
     * - FIX: Add the 'mutable' keyword after the lambda parameter list:
     *   auto myLambda = [bonus](int n) mutable { bonus += n; ... };
     */
    // int bonus = 100;
    // auto myLambda = [bonus](int n) {
    //     bonus += n; // <--- BROKEN LINE 6
    //     std::cout << bonus << "\n";
    // };
    // myLambda(val);

    return 0;
}

/* ============================================================================
 *  FULL WORKING SOLUTION CODE (FOR REFERENCE)
 * ============================================================================
 *
 * void (*fp)(int) = &doubleIt;                         // Fix 1
 * void (*sfp)(int) = &MathHelper::printSquare;         // Fix 2
 * void (&fr)(int) = doubleIt;                          // Fix 3
 * 
 * const MathHelper helperObj;                          // Fix 4 (Needs 'const' on operator() in class)
 * helperObj(val);
 * 
 * MathHelper surrogateObj;                             // Fix 5 (Needs 'explicit' removed from class)
 * FuncPtr sfc = surrogateObj; 
 * 
 * auto myLambda = [bonus](int n) mutable { ... };      // Fix 6 (Needs 'mutable' keyword)
 * ============================================================================
 */
