#include <vector>
#include <algorithm>
#include <iostream>

// Functor to increment a number by a certain amount
template <typename T>
struct Incrementer {
    T amount;
    Incrementer(T a) : amount(a) {}
    T operator()(T n) { return n + amount; }
};

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5};
    // Use the functor to add 10 to every element
    std::transform(nums.begin(), nums.end(), nums.begin(), Incrementer<int>(10));
    for(auto n : nums) std::cout << n << " "; // Output: 11 12 13 14 15
    
    std::cout << std::endl;
    
    std::vector<char> chars = {'a', 'b', 'c'};
    // Use the functor to add 10 to every element
    std::transform(chars.begin(), chars.end(), chars.begin(), Incrementer<double>(10.0));
    for(auto n : chars) std::cout << n << " "; // Output: k l m
}
