#pragma once
#include <thread>
#include <functional>
#include <vector>
#include <atomic>
#include <memory>

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
    auto shared_func = std::make_shared<std::decay_t<Func>>(std::forward<Func>(func));
    tasks_.enqueue([shared_func]() { (*shared_func)(); });
}