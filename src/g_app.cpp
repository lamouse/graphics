#include "g_app.hpp"

namespace g{

void App::run(){
    while (!window.shuldClose()){
        glfwPollEvents();
    }
    
}
App::App()
{
    ::std::string full_path{"/Users/sora/project/cpp/test/xmake/graphics/src/shader/"};
    pipline = new Pipline{full_path + "simple_shader.vert.spv", 
            full_path + "simple_shader.frag.spv"};
}

App::~App()
{
    delete pipline;
}
}