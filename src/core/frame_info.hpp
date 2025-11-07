#pragma once
#include "core/camera/camera.hpp"
#include "core/input.hpp"
#include "core/frontend/framebuffer_layout.hpp"

namespace graphics {
class ResourceManager;
}
namespace core {
struct FrameInfo {
        graphics::ResourceManager* resource_manager{};
        Camera* camera{};
        float frameTime{};
        InputState input_state;
        int window_width;
        int window_hight;
        void clean(){
                input_state = {};
        }
};
}  // namespace core