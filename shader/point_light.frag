#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 1) in vec3 fragWorldPos; // ← 接收插值后的位置
layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[10];
  int numLights;
} ubo;

layout(push_constant) uniform Push {
  vec4 position;
  vec4 color;
  float radius;
} push;

const float M_PI = 3.1415926538;

void main() {
  float dis = sqrt(dot(fragOffset, fragOffset));
  if (dis >= 1.0) {
    discard;
  }
 // 例如：计算到光源的距离（虽然这里光源就是中心）
  float distToLight = length(fragWorldPos - push.position.xyz);
 // 示例：物理衰减
     float attenuation = 1.0 / (1.0 + distToLight * distToLight * 5); // 可调系数让衰减更柔和
  vec3 lightColor = push.color.xyz * push.color.w * attenuation;

  float alpha = attenuation; // 或者用更激进的 falloff 做 alpha
  outColor = vec4(lightColor, alpha);
}
