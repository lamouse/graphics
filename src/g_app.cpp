#include "g_app.hpp"
#include "g_pipeline.hpp"
#include "g_render_system.hpp"
#include "g_defines.hpp"
#include "g_context.hpp"
#include "gui_util.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
namespace g{

void App::run(){

    RenderProcesser render(Context::Instance().device());
    RenderSystem renderSystem(render.getRenderPass());
    renderSystem.createUniformBuffers(2);
    //auto gameObj = GameObject::createGameObject();
    ::std::string s(image_path + "viking_room.png");
    resource::image::Image img(s);
    resource::image::ImageTexture imageTexture{Context::Instance().device(), img, DEFAULT_FORMAT};

    while (!window.shuldClose()){
        glfwPollEvents();
        if(render.beginFrame())
        {
            render.beginSwapchainRenderPass();
            renderSystem.renderGameObject(gameObjects, render.getCurrentFrameIndex(), render.getCurrentCommadBuffer(), render.extentAspectRation(), imageTexture);
            render.endSwapchainRenderPass();
            render.endFrame();
        }
    }

    Context::Instance().device().logicalDevice().waitIdle();
}

std::unique_ptr<Model> createCubeModel(::glm::vec3 offset) {


    ::tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (models_path + "viking_room.obj") .c_str())) {
        throw std::runtime_error(warn + err);
    }
    std::vector<Model::Vertex> vertices;
    std::vector<uint16_t> indices;
   
   ::std::unordered_map<Model::Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Model::Vertex vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
     
  return std::make_unique<Model>(vertices, indices, Context::Instance().device());
}

void App::loadGameObjects()
{
    ::std::shared_ptr<Model> model = createCubeModel({.0f, .0f, .0f});
    auto cube = GameObject::createGameObject();
    cube.model = model;
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