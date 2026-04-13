#pragma once
#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

template <typename T>
class ThreadSafeQueue {
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool stop_ = false;

public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue();
    
    template <typename U>
    void enqueue(U&& value);
    
    std::optional<T> pop();

    bool empty() const;
    size_t size() const;
    void stop();
   
    const std::queue<T>& getOriginQueue() const { return queue_; };
};

template <typename T> 
template <typename U>
void ThreadSafeQueue<T>::enqueue(U&& value) {
    {
        std::scoped_lock lock(mtx_);
        if (stop_) {
            throw std::runtime_error("Queue is stopped");
        }

        queue_.push(std::forward<U>(value));
    }
    cv_.notify_one();
}

template <typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue() {
    stop();
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::pop() {
    std::unique_lock lock(mtx_);
    
    cv_.wait(lock, [&] () {
        return !queue_.empty() || stop_;
    });

    if (queue_.empty() && stop_) {
        return std::nullopt;
    }

    T res = std::move(queue_.front());
    queue_.pop();

    return res;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const {
    std::scoped_lock lock(mtx_);
    return queue_.empty();
}

template <typename T>
size_t ThreadSafeQueue<T>::size() const {
    std::scoped_lock lock(mtx_);
    return queue_.size();
}

template <typename T>
void ThreadSafeQueue<T>::stop() {
    {
        std::scoped_lock lock(mtx_);
        stop_ = true;
    }

    cv_.notify_all();
}
