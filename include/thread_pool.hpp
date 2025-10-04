#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "block_queue.hpp"

class thread_pool {
private:
    std::vector<std::thread> workers;
    std::mutex mtx;
    block_queue<std::function<void()>> tasks_queue; // Task queue with a maximum size of 100
    
    int max_threads_num;
    int core_threads_num;
    int total_threads_num;
    int active_threads_num = 0;
    bool accept_new_tasks = true;
public:
    thread_pool(int core_threads_num, int max_threads_num, size_t queue_max_size=100);
    void worker_loop();
    void submit(std::function<void()> task);
    ~thread_pool();
};
