#include <shared_queue.hpp>

template <typename T> T Queue::pop() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while(queue_.empty()) {
        cond_.wait(mlock);
    }

    auto item = queue_.front();
    queue_.pop();
    return item;
}

template <typename T> Queue::push(const T& item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
}
