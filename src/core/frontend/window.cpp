#include "window.hpp"
#include <cmath>
namespace core::frontend {
auto BaseWindow::getAspectRatio() const -> float {
    auto ratio = static_cast<float>(frame_buffer_layout.screen.GetWidth()) /
                 static_cast<float>(frame_buffer_layout.screen.GetHeight());

    return std::abs(ratio);
}
}  // namespace core::frontend