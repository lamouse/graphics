
#pragma once

#include "g_game_object.hpp"

namespace g
{

class GravityPhysics
{
private:


public:
    const float strengGravity;

    void update(::std::vector<GameObject> & objs, float dt, unsigned int substeps = 1);
    ::glm::vec2 computeForce(GameObject& fromObj, GameObject& toObj) const;
    GravityPhysics(float streng) : strengGravity(streng){};
    ~GravityPhysics(){};
};



}