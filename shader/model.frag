#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
  bool hasColor = length(fragColor) > 0.0;
    bool hasTexCoord = length(fragTexCoord) > 0.0; // 简单判断纹理坐标是否有效

    if (hasColor && hasTexCoord) {
        // 既有颜色又有纹理：混合
        vec4 texColor = texture(texSampler, fragTexCoord);
        outColor = vec4(fragColor * texColor.rgb, texColor.a);
    } else if (hasColor) {
        // 只有颜色：直接使用颜色
        outColor = vec4(fragColor, 1.0);
    } else if (hasTexCoord) {
        // 只有纹理：采样纹理
        outColor = texture(texSampler, fragTexCoord);
    } else {
        // 默认颜色（防止全黑）
        outColor = vec4(1.0, 0.0, 1.0, 1.0); // 洋红色表示错误
    }
}