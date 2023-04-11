#pragma once
#include <string>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace g{


#define DEFAULT_FORMAT vk::Format::eR8G8B8A8Srgb
#define image_path  ::std::string{"./images/"}
#define shader_path ::std::string{"./shader/"}
#define models_path ::std::string{"./models/"}

struct SimplePushConstantData
{
    ::glm::mat4 transform{1.f};
    ::glm::vec3 color;
};

}