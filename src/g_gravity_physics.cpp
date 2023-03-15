#include "g_gravity_physics.hpp"

namespace g
{

void GravityPhysics::update(::std::vector<GameObject> & objs, float dt, unsigned int substeps)
{
    const float stepDelta = dt / substeps;
    for(int i = 0; i < substeps; i++)
    {
        
    }
}


::glm::vec2 GravityPhysics::computeForce(GameObject& fromObj, GameObject& toObj)const
{
    auto offset = fromObj.transform.translation - toObj.transform.translation;
    float distanceSquared = ::glm::dot(offset, offset);
    if(::glm::abs(distanceSquared) < 1e-10f)
    {
        return {.0f, .0f};
    }

    return {.0,.0f};
}

}