#include "thread_pool.hpp"
#include "block_queue.hpp"
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>

thread_pool::thread_pool(int core_threads_num, int max_threads_num, size_t queue_max_size)
{
    this->tasks_queue.resize(queue_max_size); // Task queue with a maximum size of 100
    this->core_threads_num = core_threads_num >= 0 ? core_threads_num : 0;
    this->total_threads_num = this->core_threads_num;
    this->max_threads_num = max_threads_num >= this->core_threads_num ? max_threads_num : this->core_threads_num;
    
    this->workers.reserve(max_threads_num);
    for (int i = 0; i < core_threads_num; i++) {
        this->workers.emplace_back(&thread_pool::worker_loop, this);
    }
}

void thread_pool::worker_loop() {
    while(this->accept_new_tasks) {
        // Get a task from the queue
        auto task = this->tasks_queue.pop();

        // Notify the main thread that a worker thread has started
        std::unique_lock<std::mutex> lk(this->mtx);
        this->active_threads_num++;
        lk.unlock();
        // --------------------------
        
        // Execute the task
        try {
            task();
        } catch(...) {
            std::cout<<"Exception caught in thread pool worker loop"<<std::endl;
        };
        // --------------------------

        // Notify the main thread that a worker thread has finished
        lk.lock();
        this->active_threads_num--;
    }
}

void thread_pool::submit(std::function<void()> task) {
    std::unique_lock<std::mutex> lk(this->mtx);
    if (!this->accept_new_tasks) {
        throw std::runtime_error("Thread pool is not accepting new tasks");
    }
    lk.unlock();

    this->tasks_queue.push(task);

    // If all core threasds are busy and there are more tasks than active threads, create a new thread if we haven't reached max_threads
    lk.lock();
    if (this->active_threads_num >= this->core_threads_num && !this->tasks_queue.empty() && this->total_threads_num < this->max_threads_num) {
        this->workers.emplace_back(&thread_pool::worker_loop, this);
    }
}

thread_pool::~thread_pool() {
    // Stop accepting new tasks
    std::unique_lock<std::mutex> lk(this->mtx);
    this->accept_new_tasks = false;
    lk.unlock();

    // Wait for all tasks to be processed
    while (!this->tasks_queue.empty() || this->active_threads_num > 0) {
        std::this_thread::yield();
    }

    // Join all worker threads
    for (auto& worker : this->workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}