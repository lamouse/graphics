#include "app.hpp"

#include "config/window.h"
#include "g_defines.hpp"
#include "resource/image.hpp"
#include <spdlog/spdlog.h>
#include "render_core/render_vulkan/render_vulkan.hpp"
#include "glfw_window.hpp"
#include "sdl_window.hpp"
#include <thread>
#include "model.hpp"
#include "render_core/framebufferConfig.hpp"
#include "ui.hpp"

namespace graphics {
namespace {}  // namespace

void App::run() {
    auto* graphics = render_base->getGraphics();
    ::std::string s(image_path + "viking_room.png");
    ::std::string s2(image_path + "p1.jpg");
    resource::image::Image img(s);
    resource::image::Image img2(s2);
    auto model = graphics::Model::createFromFile("models/viking_room.obj");
    std::span<float> verticesSpan(reinterpret_cast<float*>(model->vertices_.data()),
                                  model->vertices_.size() * sizeof(Model::Vertex) / sizeof(float));
    auto debugInfo = graphics::ui::init_debug_info();
    render::frame::FramebufferConfig frames{.width = 1920, .height = 1080, .stride = 1920};
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    render::PipelineState pipeline_state;
    auto layout = window->getFramebufferLayout();
    pipeline_state.viewport.width = layout.screen.GetWidth();
    pipeline_state.viewport.height = layout.screen.GetHeight();
    pipeline_state.scissors.width = layout.screen.GetWidth();
    pipeline_state.scissors.height = layout.screen.GetHeight();
    render::GraphicsContext graphics_ctx{};
    graphics_ctx.image = img.getImageInfo();
    graphics_ctx.vertex = verticesSpan;
    graphics_ctx.indices = model->indices_;
    graphics_ctx.indices_size = model->indices_.size();
    graphics_ctx.index_format = render::IndexFormat::UnsignedShort;
    graphics_ctx.uniform_size = sizeof(render::UniformBufferObject);
    auto graphicId = graphics->addGraphicContext(graphics_ctx);
    graphics_ctx.image = img2.getImageInfo();
    auto graphicId2 = graphics->addGraphicContext(graphics_ctx);
    while (!window->shouldClose()) {
        window->pullEvents();
        if (window->IsMinimized()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        graphics->start();
        auto ubo = graphics::ui::get_uniform_buffer(debugInfo, window->getAspectRatio());
        graphics->bindUniformBuffer(graphicId, &ubo, sizeof(ubo));
        graphics->setPipelineState(pipeline_state);
        graphics->draw(graphicId);

        auto debug2 = debugInfo;
        debug2.look_x += 8.f;
        debug2.center_z += 2.f;
        auto ubo2 = graphics::ui::get_uniform_buffer(debug2, window->getAspectRatio());
        graphics->bindUniformBuffer(graphicId2, &ubo2, sizeof(ubo2));
        graphics->setPipelineState(pipeline_state);
        graphics->draw(graphicId2);

        graphics->end();
        auto& shader_notify = render_base->getShaderNotify();
        const int shaders_building = shader_notify.ShadersBuilding();
        if (shaders_building > 0) {
            window->setWindowTitle(fmt::format("Building {} shader(s)", shaders_building));
        } else {
            window->setWindowTitle("graphics");
        }
#if defined(USE_DEBUG_UI)
        auto imageId = graphics->getDrawImage();
        render_base->addImguiUI([&]() {
            graphics::ui::begin();
            graphics::ui::draw_docked_window();
            graphics::ui::draw_result(imageId, window->getAspectRatio());
            graphics::ui::main_ui();
            graphics::ui::uniform_ui(debugInfo);
            graphics::ui::pipeline_state(pipeline_state);
            graphics::ui::draw_setting();
            graphics::ui::end();
        });
#endif
        render_base->composite(std::span{&frames, 1});
    }
}

App::App(const g::Config& config) {
    auto window_config = config.getConfig<config::window::Window>();

#if defined(USE_GLFW)
    window = std::make_unique<ScreenWindow>(
        ScreenExtent{.width = window_config.width, .height = window_config.height},
        window_config.title);
#endif
#if defined(USE_SDL)
    window = std::make_unique<graphics::SDLWindow>(window_config.width, window_config.height,
                                                   window_config.title);
#endif
    render_base = std::make_unique<render::vulkan::RendererVulkan>(window.get());
}

App::~App(){};

}  // namespace graphics
