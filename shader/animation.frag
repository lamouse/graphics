#version 460 core

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 texCoords;

layout(set = 0, binding = 1) uniform sampler2D texture_diffuse;

void main(){
    outColor = texture(texture_diffuse, texCoords);
}
