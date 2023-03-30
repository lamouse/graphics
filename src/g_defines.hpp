#pragma once
#include <string>
namespace g
{
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    #define shader_path ::std::string{"/Users/sora/project/cpp/test/xmake/graphics/src/shader/"}
    #define image_path  ::std::string{"/Users/sora/project/cpp/test/xmake/graphics/src/images/"}
#else
    #define shader_path ::std::string{"E:/project/cpp/graphics/src/shader/"}
    #define image_path  ::std::string{"E:/project/cpp/graphics/src/images/"}
#endif
}