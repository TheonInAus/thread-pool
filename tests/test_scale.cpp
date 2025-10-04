// tests/test_scale.cpp
// Build & run (from project root):
// g++ -std=c++11 -O2 -pthread -Iinclude tests/test_scale.cpp src/thread_pool.cpp -o test_scale && ./test_scale
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include "thread_pool.hpp"

int main() {
    using namespace std::chrono;
    using clk = steady_clock;

    std::cout << "[RUN] dynamic scaling when core threads are busy\n";

    // Adjust if your ctor is (core, max, queue_cap)
    // thread_pool pool(/*core*/2, /*max*/4, /*queue_cap*/100);
    thread_pool pool(/*core*/2, /*max*/4);

    // Step 1: occupy all core threads with long-running tasks
    const int core = 2;
    std::atomic<int> long_started(0);
    for (int i = 0; i < core; ++i) {
        pool.submit([&]{
            long_started.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(milliseconds(700)); // keep core threads busy
        });
    }

    // Wait a brief moment to ensure long tasks have started
    auto t0 = clk::now();
    while (long_started.load(std::memory_order_relaxed) < core &&
           duration_cast<milliseconds>(clk::now() - t0).count() < 100) {
        std::this_thread::sleep_for(milliseconds(1));
    }

    // Step 2: submit short probe tasks that should start quickly if the pool scales
    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> probe_started(0);
    time_point<clk> first_probe_start;
    bool recorded = false;

    auto probe = [&]{
        auto now = clk::now();
        {
            std::lock_guard<std::mutex> lk(m);
            if (!recorded) { first_probe_start = now; recorded = true; }
        }
        probe_started.fetch_add(1, std::memory_order_relaxed);
        cv.notify_all();
        // do a tiny bit of work
        std::this_thread::sleep_for(milliseconds(5));
    };

    // Submit two probe tasks; each submit may trigger an extra worker
    pool.submit(probe);
    pool.submit(probe);

    // Step 3: wait a short timeout to see if any probe starts before long tasks finish
    std::unique_lock<std::mutex> lk(m);
    bool ok = cv.wait_for(lk, milliseconds(300), [&]{ return probe_started.load() >= 1; });
    auto elapsed_ms = duration_cast<milliseconds>(first_probe_start - t0).count();

    if (ok) {
        std::cout << "Probe started after ~" << elapsed_ms
                  << " ms: scaling likely WORKED (extra worker handled probe while cores were busy)\n";
    } else {
        std::cout << "No probe started within 300 ms: scaling likely DID NOT occur (probes waited for cores)\n";
        std::cout << "This may mean dynamic scaling is disabled or conditions not met.\n";
        return 1; // treat as failure
    }

    std::cout << "OK\n";
    return 0;
}
