#include "thread_pool.hpp"

#include "../../defines.hpp"

ThreadPool::ThreadPool() {
    size_t size = THREAD_COUNT;
    for (size_t i = 0; i < size; ++i) {
        threads_.emplace_back([this] () {
            while (true) {
                auto task = tasks_.pop();
                if (!task) {
                    break;
                }
                (*task)();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
    joinAll();
}

void ThreadPool::shutdown() {
    if (is_running_) {
        is_running_ = false;
    }
    tasks_.stop();
}

void ThreadPool::joinAll()
{
    for (auto &t : threads_)
    {
        if (t.joinable())
            t.join();
    }
}
