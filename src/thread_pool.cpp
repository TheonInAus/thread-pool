#include "thread_pool.hpp"
#include "block_queue.hpp"
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>

thread_pool::thread_pool(int core_threads_num, int max_threads_num, int queue_max_size)
{
    this->accept_new_tasks = true;
    this->active_threads_num = 0;
    this->max_queue_size = queue_max_size > 0 ? queue_max_size : 100;
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
    while(true) {
        // Get a task from the queue
        auto task = this->tasks_queue.pop();
        
        if (!task) {
            return;
        }
        // Notify the main thread that a worker thread has started
        {
            std::unique_lock<std::mutex> lk(this->mtx);
            this->active_threads_num++;
        }
        // --------------------------
        
        // Execute the task
        try {
            task();
        } catch(...) {
            std::cout<<"Exception caught in thread pool worker loop"<<std::endl;
        };
        // --------------------------

        // Notify the main thread that a worker thread has finished
        {
            std::unique_lock<std::mutex> lk(this->mtx);
            this->active_threads_num--;
        }
        if (this->accept_new_tasks) {
        }
    }
    return;
}

void thread_pool::submit(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lk(this->mtx);
        
        
        if (this->accept_new_tasks == false) {
            throw std::runtime_error("Thread pool is not accepting new tasks");
        }
    }

    this->tasks_queue.push(task);
    // If all core threasds are busy and there are more tasks than active threads, create a new thread if we haven't reached max_threads
    {
        std::unique_lock<std::mutex> lk(this->mtx);
        if (this->active_threads_num >= this->core_threads_num && !this->tasks_queue.empty() && this->total_threads_num < this->max_threads_num) {
            this->workers.emplace_back(&thread_pool::worker_loop, this);
        }
    }
}

thread_pool::~thread_pool() {
    std::cout<<"Thread pool destructor called\n";
    // Stop accepting new tasks
    {
        std::unique_lock<std::mutex> lk(this->mtx);
        this->accept_new_tasks = false;
    }
    // Wait for all tasks to be processed
    std::cout<<"Destructor: Waiting for all tasks to be processed...\n";
    while (!this->tasks_queue.empty() || this->active_threads_num > 0) {
        std::this_thread::yield();
    }

    std::cout<<"Destructor: All tasks processed. Shutting down worker threads...\n";
    // Send a poison pill to each worker thread to signal them to exit
    for (int i = 0; i < this->max_queue_size; ++i) {
        tasks_queue.push(std::function<void()>{}); // ← 毒丸
    }

    std::cout<<"Destructor: Poison pills sent. Joining worker threads...\n";
    // Join all worker threads
    for (auto& worker : this->workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    std::cout<<"Destructor: All worker threads joined. Thread pool destroyed.\n";
}