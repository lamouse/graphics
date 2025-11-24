#version 450

// 输入：插值后的顶点颜色、世界坐标位置、法线
layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragTexCoord;  // 新增：纹理坐标

// 输出：最终颜色
layout (location = 0) out vec4 outColor;

// 纹理采样器
layout(binding = 2) uniform sampler2D ambientSampler;
layout(binding = 3) uniform sampler2D texSampler;
layout(binding = 4) uniform sampler2D specularSampler;

// 光照相关的 Uniform Buffer
struct PointLight {
    vec4 position; // w 忽略
    vec4 color;    // w 是强度
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor; // w 是强度
    PointLight pointLights[10];
    int numLights;
} ubo;

layout(set = 0, binding = 1) uniform Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}material;


// 模型变换矩阵（来自 Push Constant）
layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;


void main() {

    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 lightToPos = light.position.xyz - fragPosWorld;
        float distance = length(lightToPos);
        vec3 directionToLight = lightToPos / distance;
        float attenuation = 1.0 / (distance * distance); // 衰减 = 1/r²

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0.0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        // 漫反射
        diffuseLight += intensity * cosAngIncidence * material.diffuse;

        // 高光（Blinn-Phong）
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = max(dot(surfaceNormal, halfAngle), 0.0);
        blinnTerm = pow(blinnTerm, material.shininess); // 可提取为 uniform 控制 shininess
        specularLight += intensity * blinnTerm * material.specular;
    }

    vec3 ambientColor = texture(ambientSampler, fragTexCoord).rgb;
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;
    vec3 specTex = vec3(texture(specularSampler, fragTexCoord).rgb);
    //最终颜色 = (环境 + 漫反射 + 高光) * 贴图
    vec3 finalColor = ambientLight * texColor * ambientColor +
                    diffuseLight * texColor +
                    specularLight * specTex;

    outColor = vec4(finalColor, 1.0);
}