#pragma once

namespace g
{

class System
{
private:
    void createPipelineLayout();
    void createPipeline();
    void renderGameObjects();
public:
    System();
    ~System();
};

System::System()
{
}

System::~System()
{
}


}