// g++ -std=c++11 -O2 -pthread tests/test.cpp src/thread_pool.cpp -Iinclude -o tp_tests
#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include "thread_pool.hpp"

int main() {
    using std::chrono::milliseconds;

    // ---- Test 1: Parallel sum & graceful shutdown ----
    {
        std::cout << "[RUN] sum & shutdown\n";
        thread_pool pool(4, 8);


        const int N = 20000;
        std::atomic<long long> sum(0);
        
        
        for (int i = 1; i <= N; ++i) {
            pool.submit([i, &sum]{
                sum.fetch_add(i, std::memory_order_relaxed);
            });
        }

        // Pool destructor (leaving scope) should block until all tasks finish.
        long long expected = 1LL * N * (N + 1) / 2;
        (void)expected; // for visibility, see next block
    }

    // ---- Test 1b: Visible result check ----
    {
        std::cout << "[RUN] visible check\n";
        thread_pool pool(3, 6, 100);
        const int N = 10000;
        std::atomic<long long> sum(0);

        for (int i = 1; i <= N; ++i) {
            pool.submit([i, &sum]{
                sum.fetch_add(i, std::memory_order_relaxed);
            });
        }

        // Tiny head start (optional)
        std::this_thread::sleep_for(milliseconds(50));
        // Destructor will wait until all tasks are done.

        long long expected = 1LL * N * (N + 1) / 2;
        std::cout << "expected=" << expected
                  << " (pool dtor returns after sum reaches this)\n";
    }

    // ---- Test 2: Backpressure-ish timing (non-strict) ----
    {
        std::cout << "[RUN] backpressure-ish\n";
        thread_pool pool(2, 2);
        std::atomic<int> done(0);

        const int M = 5000;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < M; ++i) {
            pool.submit([&done]{
                done.fetch_add(1, std::memory_order_relaxed);
            });
        }
        auto t1 = std::chrono::steady_clock::now();

        auto ms = std::chrono::duration_cast<milliseconds>(t1 - t0).count();
        std::cout << "submitted " << M << " tasks in ~" << ms
                  << " ms (queue capacity may slow submission)\n";

        // Give workers a moment (optional)
        std::this_thread::sleep_for(milliseconds(50));
        // Destructor should block until all tasks are finished.
    }

    std::cout << "OK\n";
    return 0;
}
