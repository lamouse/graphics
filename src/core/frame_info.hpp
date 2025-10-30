#pragma once
#include "core/camera/camera.hpp"
namespace core{
    struct FrameInfo{
        Camera* camera;
        float frameTime;
    };
}