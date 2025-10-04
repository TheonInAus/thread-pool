# Thread Pool (C++17, header-only)

A small, production‑style thread pool suitable for interviews and real use.

## Highlights

- **C++17 header-only** (`include/thread_pool.hpp`), no deps
- **std::future** interface: `auto fut = pool.submit(f, args...)`
- **Graceful stop** in destructor; no task loss
- **Optional bounded queue** to provide backpressure
- **Utility**: `parallel_for(first, last, fn)` with simple block partitioning
- **Exception-safe**: exceptions propagate via `future::get()`

## Build & Run

```bash
mkdir -p build && cd build
cmake .. && cmake --build . -j
./demo
./smoke # tiny sanity test
```

## API

```cpp
namespace tp {
class thread_pool {
public:
explicit thread_pool(std::size_t workers = std::thread::hardware_concurrency(),
std::size_t max_queue = 0 /*0 = unbounded*/);


template <class F, class... Args>
auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;


void stop() noexcept; // graceful: drain and join
std::size_t size() const noexcept;


template <class Index, class Func>
void parallel_for(Index first, Index last, Func fn);
};
}
```

## Design Notes

- **Queue**: `std::deque<std::function<void()>>` for FIFO fairness and cache friendliness.
- **Bounded mode**: when `max_queue>0`, producers block on `cv_not_full_` (classic producer/consumer), preventing OOM under load.
- **Exception handling**: tasks run inside try/catch; user-visible exceptions rethrow from `future::get()`.
- **Shutdown**: `stop_` flag + drain existing tasks; workers exit when queue empty.

## What interviewers may ask (with talking points)

1. **Why futures vs. callbacks?** Futures integrate with standard C++; exceptions/values travel through `get()`.
2. **Why condition variables, not busy-wait?** Avoid CPU spin; CVs scale and are power-efficient.
3. **Can we add task prioritization?** Replace deque with a priority queue, add a `submit(priority, f)` overload.
4. **How to avoid head-of-line blocking?** Use per-thread deques + work-stealing (Chase-Lev) for throughput; here we keep simplicity.
5. **How to support stop-now?** Track a `force_stop_` flag; cancel remaining tasks or return `std::future` with broken-promise error.
6. **ABA / lost wakeups?** Guarded by mutex + CV with predicates; no ABA on queue ops.
7. **Thread affinity?** Add pinning via OS APIs if needed for cache locality.

## Bench idea (optional)

- Compare sequential vs. `parallel_for` on filling a large vector. Use `std::chrono`.

## License

MIT
