#pragma once
#include "core/camera/camera.hpp"
#include "core/input.hpp"

namespace graphics {
class ResourceManager;
}
namespace core {
struct FrameInfo {
        graphics::ResourceManager* resource_manager{};
        Camera* camera{};
        float frameTime{};
        float durationTime{};
        std::optional<InputState> input_event;
        int window_width{};
        int window_hight{};
};
}  // namespace core