#pragma once
#include <cstring>
#include <string>
#include <vector>


#define __FILENAME__ (::std::strrchr(__FILE__, '/') ? ::std::strrchr(__FILE__, '/') + 1 : __FILE__)

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define DEFAULT_FORMAT vk::Format::eR8G8B8A8Srgb
#define image_path \
    ::std::string { "./images/" }
#define shader_path \
    ::std::string { "./shader/" }
#define models_path \
    ::std::string { "./models/" }

#define DETAIL_INFO(msg)                                                                                       \
    ::std::string("(") + __FILENAME__ + ::std::string(":") + ::std::to_string(__LINE__) + ::std::string(" ") + \
        __PRETTY_FUNCTION__ + ::std::string("): ") + msg

namespace g {

using size_type = ::std::vector<int>::size_type;

}  // namespace g