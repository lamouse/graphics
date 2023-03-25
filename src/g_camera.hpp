#pragma


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace g
{

class Camera
{
private:
    ::glm::mat4 projectionMatrix{1.f};
    ::glm::mat4 viewMatrix{1.f};
public:
    void setOrthographicProjection(float left, float right, float top, float botton, float near, float far);
    void setPerspecitiveProjection(float fovy, float aspect, float near, float far);
    void setViewDirection(::glm::vec3 position, ::glm::vec3 direction, ::glm::vec3 up = ::glm::vec3{0.f, -1.f, 0.f});
    void setViewTarget(::glm::vec3 position, ::glm::vec3 target, ::glm::vec3 up = ::glm::vec3{0.f, -1.f, 0.f});
    void setViewYXZ(::glm::vec3 position, ::glm::vec3 ratation);
    const ::glm::mat4& getProjection(){return projectionMatrix;}
    const ::glm::mat4& getView(){return viewMatrix;}
};


}