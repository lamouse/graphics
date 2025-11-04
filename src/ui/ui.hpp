#pragma once

#include <string>
#include <glm/glm.hpp>

void DrawVec3ColorControl(const std::string& label, glm::vec3& value, const glm::vec3& resetValue,
                          float speed = 0.1F, float columnWidth = 120.0F);

void DrawFloatControl(const std::string& label, float& value, float speed = 0.1F,
                      float resetValue = 45.0F, float minValue = .0F, float maxValue = 0.F,
                      float labelWidth = 120.0F);
void HelpMarker(const char* desc);
namespace graphics {}