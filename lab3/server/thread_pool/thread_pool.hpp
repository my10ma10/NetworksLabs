#pragma once
#include <thread>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>

#include "thread_safe_queue.hpp"

class ThreadPool {
    ThreadSafeQueue<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;

    std::atomic<bool> is_running_;

public:
    ThreadPool();
    ~ThreadPool();

    void shutdown();

    template <typename Func>
    void enqueueConnection(Func&& func);
    
    void joinAll();
};

template <typename Func>
void ThreadPool::enqueueConnection(Func&& func) {
    tasks_.enqueue(std::function<void()>(std::forward<Func>(func)));
}