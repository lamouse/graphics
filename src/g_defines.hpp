#pragma once
#include <string>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace g
{
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    #define shader_path ::std::string{"/Users/sora/project/cpp/test/xmake/graphics/shader/"}
    #define image_path  ::std::string{"/Users/sora/project/cpp/test/xmake/graphics/images/"}
#else
    #define shader_path ::std::string{"E:/project/cpp/graphics/shader/"}
    #define image_path  ::std::string{"E:/project/cpp/graphics/images/"}
    #define DEFAULT_FORMAT vk::Format::eR8G8B8A8Srgb
#endif


struct SimplePushConstantData
{
    ::glm::mat4 transform{1.f};
    ::glm::vec3 color;
};

}