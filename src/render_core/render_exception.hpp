#pragma once
#include <exception>
#include <string>
#include <utility>
namespace render {
class RenderException : public std::exception {
    public:
        explicit RenderException(std::string message) : message_{std::move(message)} {}
        [[nodiscard]] auto what() const noexcept -> const char* override { return message_.c_str(); }

    private:
        std::string message_;
};
}  // namespace render