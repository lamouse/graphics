
#include "g_app.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include<glm/mat4x4.hpp>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace g;

int main(int argc, char** argv)
{

    g::App app;
    try{
        app.run();
    }catch(const ::std::exception &e){
        ::std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
