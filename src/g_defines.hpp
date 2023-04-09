#pragma once
#include <string>
namespace g
{
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    #define shader_path ::std::string{"/Users/sora/project/cpp/test/xmake/graphics/shader/"}
    #define image_path  ::std::string{"/Users/sora/project/cpp/test/xmake/graphics/images/"}
#else
    #define shader_path ::std::string{"E:/project/cpp/graphics/shader/"}
    #define image_path  ::std::string{"E:/project/cpp/graphics/images/"}
    #define DEFAULT_FORMAT vk::Format::eB8G8R8A8Srgb
#endif
}