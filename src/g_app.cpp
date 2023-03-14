#include "g_app.hpp"
#include "g_render.hpp"
namespace g{

void App::run(){
    while (!window.shuldClose()){
        glfwPollEvents();
        RenderProcess::getInstance().render();
    }
    
}
App::App()
{
}

App::~App()
{
}
}