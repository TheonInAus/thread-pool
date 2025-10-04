#pragma once
#include <deque>
#include <condition_variable>
#include <mutex>

template<typename T>
class block_queue {

public:
    block_queue(size_t max_size) : max_size(max_size) {}
    block_queue() : max_size(0) {} // Default max size is 100
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_not_full.wait(lock, [this]() { return queue.size() < this->max_size; });
        queue.push_back(item);
        cv_not_empty.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv_not_empty.wait(lock, [this]() { return queue.empty(); });
        T item = queue.front();
        queue.pop_front();
        cv_not_full.notify_one();
        return item;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    bool full() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size() >= max_size;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

    void resize(size_t new_max_size) {
        std::lock_guard<std::mutex> lock(mtx);
        max_size = new_max_size;
        cv_not_full.notify_all();
    }

private:
    std::deque<T> queue;
    std::mutex mtx;
    std::condition_variable cv_not_empty;
    std::condition_variable cv_not_full;
    size_t max_size;
};