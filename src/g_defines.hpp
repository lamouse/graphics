#pragma once
#include <string>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cstring>
#define __FILENAME__ (::std::strrchr(__FILE__, '/') ? ::std::strrchr(__FILE__, '/') + 1 : __FILE__)
#define PRINT_DEBUG_MSG



#define DEFAULT_FORMAT vk::Format::eR8G8B8A8Srgb
#define image_path  ::std::string{"./images/"}
#define shader_path ::std::string{"./shader/"}
#define models_path ::std::string{"./models/"}

#define DETAIL_INFO(msg)    ::std::string("file: ")  + __FILENAME__ +                   \
                            ::std::string("\n\tFunc: ") + __PRETTY_FUNCTION__ +            \
                            ::std::string(" Line: ") + ::std::to_string(__LINE__) +     \
                            ::std::string("\n\tMessage: ")    + msg

namespace g{
struct SimplePushConstantData
{
    ::glm::mat4 transform{1.F};
    ::glm::vec3 color;
};

}