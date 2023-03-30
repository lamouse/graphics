#include "g_app.hpp"
#include "g_pipeline.hpp"
#include "g_render_system.hpp"
namespace g{

void App::run(){

    RenderSystem renderSystem(render.getRenderPass());
    renderSystem.createUniformBuffers(2);
    Camera camera;
    //auto gameObj = GameObject::createGameObject();

    while (!window.shuldClose()){
        glfwPollEvents();
        if(render.beginFrame())
        {
            render.beginSwapchainRenderPass();
            renderSystem.renderGameObject(gameObjects, render.getCurrentFrameIndex(), render.getCurrentCommadBuffer(), render.extentAspectRation());
            render.endSwapchainRenderPass();
            render.endFrame();
        }
    }
    Device::getInstance().getVKDevice().waitIdle();
}

std::unique_ptr<Model> createCubeModel(::glm::vec3 offset) {
  std::vector<Model::Vertex> vertices{
 
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
  };
//   for (auto& v : vertices) {
//     v.position += offset;
//   }
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };
  return std::make_unique<Model>(vertices, indices);
}

void App::loadGameObjects()
{
    ::std::shared_ptr<Model> model = createCubeModel({.0f, .0f, .0f});
    auto cube = GameObject::createGameObject();
    cube.model = model;
    // cube.transform.translation = {0.f,.0f, .5f};
    // cube.transform.scale = {.5f, .5f, .5f};
    gameObjects.push_back(std::move(cube));
}

App::App()
{
    loadGameObjects();
}

App::~App()
{
}
}