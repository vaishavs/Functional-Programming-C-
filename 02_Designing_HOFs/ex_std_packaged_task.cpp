// ===========================================================================
// std::packaged_task<Signature>   — C++11, header <future>
//
//   Build : g++ -std=c++17 -pthread -o pt std_packaged_task.cpp
//
// A type-erased callable wrapper that WIRES a callable to a std::future: when
// the task is invoked, whatever the callable returns (or throws) is delivered
// through the future's shared state, where another part of the program — often
// another thread — collects it. It is the bridge between the callable world and
// asynchronous results, and the building block of thread pools and schedulers.
//
// Properties at a glance
//   * owns its target; move-only (it owns a shared state), not copyable
//   * get_future() hands out the paired future ONCE; a second call throws
//     std::future_error(future_already_retrieved)
//   * invoking the task (operator()) runs the callable and makes the future
//     ready; a thrown exception is stored and re-thrown by future::get()
//   * reset() abandons the current shared state and installs a fresh one, so
//     the same task object can be re-armed and run again
//   * valid() reports whether a shared state is present
// ===========================================================================
#include <cassert>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

int times3(int x) { return x * 3; }

int main() {
    std::cout << std::boolalpha;

    // -- 1. The basic shape -------------------------------------------------
    // wrap a callable -> take its future -> invoke -> collect the result.
    {
        std::packaged_task<int(int)> task(times3);
        std::future<int> fut = task.get_future();   // pair the future first
        task(14);                                    // run: result stored in state
        int r = fut.get();                           // get() is ONE-SHOT: read once
        std::cout << "1. basic result            : " << r << "\n"; // 42
        assert(r == 42);
    }

    // -- 2. Any callable, and arguments -------------------------------------
    {
        std::packaged_task<int(int, int)> task([](int a, int b){ return a * b; });
        auto fut = task.get_future();
        task(6, 7);
        std::cout << "2. lambda with arguments   : " << fut.get() << "\n"; // 42
    }

    // -- 3. Exceptions travel through the future ----------------------------
    // If the task throws, the exception is captured in the shared state and
    // re-thrown when the future is read — so failures cross thread boundaries.
    {
        std::packaged_task<int()> task([]() -> int {
            throw std::runtime_error("boom");
        });
        auto fut = task.get_future();
        task();                                      // stores the exception
        std::string msg;
        try { fut.get(); } catch (const std::runtime_error& e) { msg = e.what(); }
        std::cout << "3. exception propagated    : " << msg << "\n"; // boom
        assert(msg == "boom");
    }

    // -- 4. Move-only: run the task on ANOTHER thread -----------------------
    // packaged_task cannot be copied, but it can be moved — into a std::thread,
    // which invokes it there while this thread waits on the future.
    {
        std::packaged_task<int(int)> task(times3);
        std::future<int> fut = task.get_future();
        std::thread worker(std::move(task), 14);     // task(14) runs on `worker`
        int result = fut.get();                       // blocks until ready
        worker.join();
        std::cout << "4. computed on a thread    : " << result << "\n"; // 42
    }

    // -- 5. get_future() is one-shot ----------------------------------------
    {
        std::packaged_task<int()> task([]{ return 1; });
        assert(task.valid());
        auto fut = task.get_future();
        bool threw = false;
        try { auto again = task.get_future(); (void)again; }
        catch (const std::future_error&) { threw = true; }  // future_already_retrieved
        task();
        std::cout << "5. second get_future threw : " << threw
                  << " (value " << fut.get() << ")\n";        // true (value 1)
    }

    // -- 6. reset(): re-arm and run the same task again ---------------------
    {
        std::packaged_task<int(int)> task(times3);
        auto f1 = task.get_future();
        task(10);
        int first = f1.get();                         // 30
        task.reset();                                 // NEW shared state
        auto f2 = task.get_future();                  // future for the new run
        task(20);
        int second = f2.get();                        // 60
        std::cout << "6. reused via reset()      : " << first << " then " << second << "\n";
        assert(first == 30 && second == 60);
    }

    // -- 7. Realistic use: a batch of tasks across threads ------------------
    // Enqueue several packaged_tasks, launch each on its own thread, then
    // collect every result in order through the stored futures.
    {
        std::vector<std::future<int>> futures;
        std::vector<std::thread>      threads;
        for (int i = 1; i <= 4; ++i) {
            std::packaged_task<int(int)> t([](int x){ return x * x; });
            futures.push_back(t.get_future());        // keep the future
            threads.emplace_back(std::move(t), i);    // run t(i) on a thread
        }
        for (auto& th : threads) th.join();
        std::cout << "7. batch results           :";
        int sum = 0;
        for (auto& f : futures) { int v = f.get(); sum += v; std::cout << " " << v; }
        std::cout << "  (sum=" << sum << ")\n";        // 1 4 9 16 (sum=30)
        assert(sum == 30);
    }

    // -- Notes --------------------------------------------------------------
    // packaged_task vs std::async: std::async both wraps AND schedules the
    // call for the caller; packaged_task only wraps — the program decides WHEN
    // and WHERE to invoke it (inline, on a chosen thread, from a work queue),
    // which is exactly what a custom scheduler or thread pool needs.
    // packaged_task vs move_only_function/std::function: those are general
    // callable stores with no result plumbing; packaged_task additionally binds
    // the single invocation's result (or exception) to a future.
    std::cout << "\ndone\n";
}
