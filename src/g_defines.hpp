#pragma once
#include <cstring>
#include <string>

#define __FILENAME__ (::std::strrchr(__FILE__, '/') ? ::std::strrchr(__FILE__, '/') + 1 : __FILE__)

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define image_path ::std::string{"./images/"}
#define shader_path ::std::string{"./shader/"}
#define models_path ::std::string{"./models/"}

#define DETAIL_INFO(msg)                                                                  \
    ::std::string("(") + __FILENAME__ + ::std::string(":") + ::std::to_string(__LINE__) + \
        ::std::string(" ") + __PRETTY_FUNCTION__ + ::std::string("): ") + msg

namespace graphics {
struct ScreenExtent {
        int width;
        int height;
};
}  // namespace g
