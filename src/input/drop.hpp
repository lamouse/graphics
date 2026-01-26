#pragma once
#include <queue>
#include <string>
#include <utility>
#include <mutex>
namespace graphics::input {

class FileDrop {
    public:
        FileDrop(std::string engine_name) : engine_name_(std::move(engine_name)) {}
        [[nodiscard]] auto empty() const -> bool {
            std::scoped_lock lock(mutex_);
            return file_list_.empty();
        }
        void push(const std::string& filename) {
            std::scoped_lock lock(mutex_);
            file_list_.push(filename);
        }
        void pop() {
            std::scoped_lock lock(mutex_);
            file_list_.pop();
        }
        [[nodiscard]] auto top() const -> std::string_view {
            std::scoped_lock lock(mutex_);
            if (file_list_.empty()) {
                return "";
            }
            return file_list_.front();
        }

    private:
        std::queue<std::string> file_list_;
        std::string engine_name_;
        mutable std::mutex mutex_;
};

}  // namespace graphics::input