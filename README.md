# Thread Pool (C++11)

A tiny C++11 thread pool with a bounded blocking queue.  
Features: task submission via `std::function`, backpressure (bounded queue), optional dynamic scaling (core/max), exception-safe workers, and graceful shutdown (poison pills).

---

## Layout

```
.
├── include
│   ├── block_queue.hpp
│   └── thread_pool.hpp
├── src
│   └── thread_pool.cpp
└── tests
    └── test.cpp
```

---

## Requirements

- A C++11 compiler (g++/clang++)
- POSIX threads (Linux/macOS: `-pthread`)

---

## Build & Run Tests

From the project root:

```bash
g++ -std=c++11 -O2 -pthread   -Iinclude   tests/test.cpp src/thread_pool.cpp   -o tp_tests

./tp_tests
```

Expected tail of output (timings may vary):

```
[RUN] sum & shutdown
Thread pool destructor called
Destructor: Waiting for all tasks to be processed...
Destructor: All tasks processed. Shutting down worker threads...
Destructor: Poison pills sent. Joining worker threads...
Destructor: All worker threads joined. Thread pool destroyed.
[RUN] visible check
expected=50005000 (pool dtor returns after sum reaches this)
Thread pool destructor called
Destructor: Waiting for all tasks to be processed...
Destructor: All tasks processed. Shutting down worker threads...
Destructor: Poison pills sent. Joining worker threads...
Destructor: All worker threads joined. Thread pool destroyed.
[RUN] backpressure-ish
submitted 5000 tasks in ~2 ms (queue capacity may slow submission)
Thread pool destructor called
Destructor: Waiting for all tasks to be processed...
Destructor: All tasks processed. Shutting down worker threads...
Destructor: Poison pills sent. Joining worker threads...
Destructor: All worker threads joined. Thread pool destroyed.
OK
```

```bash
g++ -std=c++11 -O2 -pthread   -Iinclude   tests/test_scale.cpp src/thread_pool.cpp   -o test_scale

./test_scale
```

Expected tail of output (timings may vary):

```
[RUN] dynamic scaling when core threads are busy
Probe started after ~1 ms: scaling likely WORKED (extra worker handled probe while cores were busy)
OK
Thread pool destructor called
Destructor: Waiting for all tasks to be processed...
Destructor: All tasks processed. Shutting down worker threads...
Destructor: Poison pills sent. Joining worker threads...
Destructor: All worker threads joined. Thread pool destroyed.
```

---

## Common Issues

- **chrono literals error** (`invalid suffix 'ms'`) We’re on C++11. Ensure `tests/test.cpp` uses:

  ```cpp
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  ```

- **Hangs on shutdown** The pool uses poison pills to wake workers; make sure you’re using the provided `thread_pool.cpp`.

---

## Optional: Debug/Sanitizers

Address/UB Sanitizers help catch memory bugs quickly (clang++/g++ on Linux/macOS):

```bash
clang++ -std=c++11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer   -Iinclude tests/test.cpp src/thread_pool.cpp   -pthread -o tp_tests_asan

./tp_tests_asan
```

---

## Using in Your Project

Include headers and compile/link `src/thread_pool.cpp` with your sources.  
Minimal example:

```cpp
#include "thread_pool.hpp"
#include <atomic>
#include <iostream>

int main() {
    thread_pool pool(4, 8);             // core=4, max=8 (queue cap uses default)
    std::atomic<int> sum{0};

    for (int i = 0; i < 10000; ++i) {
        pool.submit([&]{ sum.fetch_add(1, std::memory_order_relaxed); });
    }

    // Pool destructor waits for tasks to finish & joins workers.
    std::cout << "Done\n";
    return 0;
}
```

---

## License

MIT (or your choice)
