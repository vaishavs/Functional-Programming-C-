// 03_comp_io.cpp
/* NOTE: Avaiable since C++26 */
#include <iostream>
#include <execution>
#include <thread>
 
using namespace std::execution;
int main() 
{
  // Create a scheduler (system thread pool) 
  auto sched = system_thread_pool{}.get_scheduler();

  // Build a pipeline 
  auto s =  schedule(sched) // Start work on the scheduler 
          | let_value([](int x) { // let_value: chain a new sender based on result 
            std::cout << "Step 2: got " << x << "\n";  // Return a new sender that does more work
            return just(x + 1); 
          })
          | then([](int y) { // then: final transformation 
            std::cout << "Step 3: final value " << y << "\n"; 
            return y * 2; 
          });
 
  // Run synchronously 
  auto result = this_thread::sync_wait(s);
  std::cout << "sync_wait result: " << *result << "\n"; 

  return 0;
}
