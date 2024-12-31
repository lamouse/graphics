#pragma once
#include <condition_variable>
#include <stop_token>
namespace common::thread {
template <typename Condvar, typename Lock, typename Pred>
void condvarWait(Condvar& cv, std::unique_lock<Lock>& lk, std::stop_token token, Pred&& pred) {
    cv.wait(lk, token, std::forward<Pred>(pred));
}

class NotifyOnExit {
    public:
        NotifyOnExit(std::condition_variable_any& cv, std::unique_lock<std::mutex>& lock)
            : cv_(cv), lock_(lock) {}
        ~NotifyOnExit() { cv_.notify_one(); }

    private:
        std::condition_variable_any& cv_;
        std::unique_lock<std::mutex>& lock_;
};
}  // namespace common::thread