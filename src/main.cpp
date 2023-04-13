
#include "g_app.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <iostream>
#include <stdexcept>
#include <spdlog/spdlog.h>

using namespace std;
using namespace g;

int main(int argc, char** argv)
{

    ::spdlog::set_level(::spdlog::level::debug);
    g::App app;
    try{
        app.run();
    }catch(const ::std::exception &e){
        ::std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
