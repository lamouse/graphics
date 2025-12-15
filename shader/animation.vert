#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex;
layout(location = 3) in ivec4 boneIds;
layout(location = 4) in vec4 weights;

layout(set = 0, binding = 0) uniform UniformBuffer{
    mat4 projection;
    mat4 view;
    mat4 model;
}ubo;

const int MAX_BONES = 100;
const int MAX_INFLUENCE = 4;

layout(set = 0, binding = 1) uniform BonesMatrices{
    mat4 bones[MAX_BONES];
}finalBoneMatrices;


layout(location = 0) out vec2 texCoords;

void main(){
    vec4 totalPosition = vec4(0.0f);
    for(int i = 0; i < MAX_INFLUENCE; i++)
    {
        if(boneIds[i] == -1)
        {
            continue;
        }
        if(boneIds[i] >= MAX_BONES)
        {
            totalPosition = vec4(position, 1.0f);
            break;
        }

        vec4 localPosition = finalBoneMatrices.bones[boneIds[i]] * vec4(position, 1.0f);
        totalPosition += localPosition * weights[i];
        vec3 localNormal = mat3(finalBoneMatrices.bones[i]) * norm;
    }

    mat4 viewModel = ubo.view * ubo.model;
    gl_Position = ubo.projection * viewModel * totalPosition;
    texCoords = tex;
}
