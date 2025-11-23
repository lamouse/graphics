#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO
{
	mat4 model;
	mat4 projection;
} ubo;

layout (location = 0) out vec3 outUVW;

void main()
{
	outUVW = inPos;
	// Convert cubemap coordinates into Vulkan coordinate space
	// Remove translation from view matrix
	mat4 viewMat = mat4(mat3(ubo.model));
	vec4 pos = ubo.projection * ubo.model * vec4(inPos, 1.0);
	//gl_Position = ubo.projection * viewMat * vec4(inPos.xyz, 1.0);
   gl_Position = pos.xyww;
}
