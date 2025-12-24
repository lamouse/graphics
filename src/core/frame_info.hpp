#pragma once
#include "core/camera/camera.hpp"
#include <chrono>

namespace graphics {
class ResourceManager;
}
namespace core {
struct FrameTimeData {
        double duration;  // 总时间（秒）
        double frame;     // 帧间隔（秒）
};

class FrameTime {
    public:
        auto get() -> FrameTimeData {
            const auto currentTime = std::chrono::steady_clock::now();

            if (!m_isInitialized) {
                m_startTime = m_lastTime = currentTime;
                m_isInitialized = true;
            }

            const double total = std::chrono::duration<double>(currentTime - m_startTime).count();
            const double delta = std::chrono::duration<double>(currentTime - m_lastTime).count();

            m_lastTime = currentTime;

            return {.duration = total, .frame = delta};
        }

        void reset() { m_isInitialized = false; }

    private:
        bool m_isInitialized = false;
        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastTime;
};
struct FrameInfo {
        graphics::ResourceManager* resource_manager{};
        Camera* camera{};
        FrameTimeData frame_time{};
        int window_width{};
        int window_hight{};
};
}  // namespace core