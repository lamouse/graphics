#ifndef G_CONTEXT_HPP
#define G_CONTEXT_HPP
#include <GLFW/glfw3.h>

#include <memory>

#include "g_defines.hpp"

namespace g {

class Context final {
    private:
        static ScreenExtent extent_;
        static bool windowIsResize;

    public:
        static void setExtent(const ScreenExtent& extent) {
            extent_ = extent;
            windowIsResize = true;
        }
        static void resetWindowResize() { windowIsResize = false; }
        static auto isWindowResize() -> bool { return windowIsResize; }
        static auto getExtent() -> ScreenExtent { return extent_; }
        static void waitWindowEvents() { glfwWaitEvents(); }
};

}  // namespace g

#endif