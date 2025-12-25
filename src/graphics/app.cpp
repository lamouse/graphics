#include "app.hpp"

#include "graphic.hpp"
#include "resource/mesh_instance.hpp"
#include "effects/particle/particle.hpp"
#include "effects/light/point_light.hpp"
#include "effects/model/multi_mesh_model.hpp"
#include "common/settings.hpp"
#include "effects/cubemap/skybox.hpp"
#include "system/setting_ui.hpp"
#include "world/world.hpp"
#include <qcoreapplication.h>
#include <spdlog/spdlog.h>
#include "gui.hpp"
#include "input/input.hpp"
#include "input/mouse.h"
#include "system/pick_system.hpp"
#include <QTimer>

#include <tracy/Tracy.hpp>

namespace graphics {

App::App() {
    input_system_ = std::make_shared<input::InputSystem>(),
    qt_main_window = std::make_unique<QTWindow>(input_system_, sys_, 1920, 1080, "graphics");
}

App::~App() = default;

}  // namespace graphics
