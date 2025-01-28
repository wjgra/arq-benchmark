#ifndef _UTIL_SAFE_QUEUE_HPP_
#define _UTIL_SAFE_QUEUE_HPP_

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace util {

template <typename T>
class SafeQueue {
public:
    // Add an element to the queue
    void push(T&& value)
    {
        std::unique_lock<std::mutex> lock(mut_);
        queue_.push(std::forward<T>(value));
        cv_.notify_one();
    }

    // Remove an element from the queue. If queue is empty, wait
    // until an element is added.
    T pop_wait()
    {
        std::unique_lock<std::mutex> lock(mut_);
        while (queue_.empty()) {
            cv_.wait(lock);
        }
        T val = queue_.front();
        queue_.pop();
        return val;
    }

    // Remove an element from the queue. If the queue is empty, return
    // std::nullopt.
    std::optional<T> try_pop()
    {
        std::unique_lock<std::mutex> lock(mut_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        else {
            T val = queue_.front();
            queue_.pop();
            return val;
        }
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> lock(mut_);
        return queue_.empty();
    }

    auto size() const
    {
        std::unique_lock<std::mutex> lock(mut_);
        return queue_.size();
    }

private:
    // Underlying queue data
    std::queue<T> queue_;
    // Mutex which must be held to modify the queue
    mutable std::mutex mut_;
    // CV to enforce wait when popping from empty queue
    std::condition_variable cv_;
};

} // namespace util

#endif