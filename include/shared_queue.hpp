#include <mutex>
#include <condition_variable>
#include <queue>

template <typename T> class Queue {
    private:
        std::queue<T> queue_;
        std::mutex mutex_;
        std::condition_variable cond_;

    public:
        T pop() {
            std::unique_lock<std::mutex> mlock(mutex_);

            while(queue_.empty()) {
                cond_.wait(mlock);
            }
            auto item = queue_.front();
            queue_.pop();
            return item;
        }

        void pop(T& item) {
            std::unique_lock<std::mutex> mlock(mutex_);

            while (queue_.empty()) {
                cond_.wait(mlock);
            }
            item = queue_.front();
            queue_.pop();
        }

        void push(const T& item) {
            std::unique_lock<std::mutex> mlock(mutex_);
            queue_.push(item);
            mlock.unlock();
            cond_.notify_one();
        }

        void push(T&& item) {
            std::unique_lock<std::mutex> mlock(mutex_);
            queue_.push(std::move(item));
            mlock.unlock();
            cond_.notify_one();
        }

        int size() {
            std::unique_lock<std::mutex> mlock(mutex_);
            size_t sz = queue_.size();
            mlock.unlock();
            cond_.notify_one();
            return sz;
        }
};