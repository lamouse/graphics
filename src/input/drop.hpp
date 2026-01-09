#pragma once
#include <queue>
#include <string>
#include <utility>
namespace graphics::input {

class FileDrop {
    public:
        FileDrop(std::string engine_name) : engine_name_(std::move(engine_name)) {}
        [[nodiscard]] auto empty() const -> bool { return file_list_.empty(); }
        void push(const std::string& filename) { file_list_.push(filename); }
        auto pop() -> std::string {
            if(file_list_.empty()){
                return "";
            }
            auto ret = file_list_.front();
            file_list_.pop();
            return ret;
        }

    private:
        std::queue<std::string> file_list_;
        std::string engine_name_;
};

}  // namespace graphics::input