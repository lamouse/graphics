#include "g_app.hpp"

namespace g{

void App::run(){
    while (!window.shuldClose()){
        glfwPollEvents();
    }
    
}
App::App()
{
 
}

App::~App()
{
}
}