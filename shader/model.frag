#version 450

// 输入：插值后的顶点颜色、世界坐标位置、法线
layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragTexCoord;  //

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

struct SpotLight {
    vec4 position; //w outerCutOff
    vec4 color; // w 是强度
    vec4 direction; //w cutOff
};

// 光照相关的 Uniform Buffer
struct DirLight  {
    vec4 direction; // w 忽略
    vec4 color;    // w 是强度
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor; // w 是强度
    DirLight dirLight;
    SpotLight spotLight;
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
// 统一 Blinn-Phong 光照计算
vec3 CalcBlinnPhongLight(vec3 lightColor, float intensity,
                         vec3 lightDir, vec3 normal,
                         vec3 viewDir,
                         vec3 texDiffuse, vec3 texSpecular);
// function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffTex, vec3 specTex);

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffTex, vec3 specTex);

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseTex, vec3 specularTex);

void main() {

    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);

    vec3 ambientColor = texture(ambientSampler, fragTexCoord).rgb;
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;
    vec3 specTex = vec3(texture(specularSampler, fragTexCoord).rgb);

    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    for (int i = 0; i < ubo.numLights; i++) {
         specularLight += CalcPointLight(ubo.pointLights[i], surfaceNormal,
                            fragPosWorld, viewDirection, texColor, specTex);
    }

    vec3 dirLight = CalcDirLight(ubo.dirLight, surfaceNormal, viewDirection, texColor, specTex);
    vec3 ambient = ambientLight * material.ambient * texColor;

    vec3 calcLight = CalcSpotLight(ubo.spotLight, surfaceNormal, fragPosWorld, viewDirection, texColor, specTex);
    //最终颜色 = (环境 + 漫反射 + 高光) * 贴图
    vec3 finalColor = dirLight + ambient + specularLight + calcLight;

    outColor = vec4(finalColor, 1.0);
}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 diffTex, vec3 specTex)
{
    vec3 lightColor = light.color.xyz * light.color.w;
    vec3 lightDir = normalize(-light.direction.xyz);

    vec3 blinnLight = CalcBlinnPhongLight(light.color.xyz, light.color.w, lightDir,
        normal, viewDir, diffTex, specTex);
    // combine results
    vec3 dirLight = lightColor * material.ambient * diffTex;
       return dirLight + blinnLight;
}

// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffTex, vec3 specTex)
{
    vec3 lightToPos = light.position.xyz - fragPos;
    float distance = length(lightToPos);
    vec3 directionToLight = lightToPos / distance;
    float attenuation = 1.0 / (distance * distance + 1e-6); // 衰减

    return CalcBlinnPhongLight(light.color.xyz, light.color.w * attenuation, directionToLight,
        normal, viewDir, diffTex, specTex);
}

// 统一 Blinn-Phong 光照计算
vec3 CalcBlinnPhongLight(vec3 lightColor, float intensity,
                         vec3 lightDir, vec3 normal,
                         vec3 viewDir,
                         vec3 texDiffuse, vec3 texSpecular)
{
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightColor * intensity * material.diffuse * diff * texDiffuse;

    // 高光 (Blinn-Phong)
    vec3 halfAngle = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfAngle), 0.0), material.shininess);
    vec3 specular = lightColor * intensity * material.specular * spec * texSpecular;

    return diffuse + specular;
}

// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseTex, vec3 specularTex)
{
    if(light.position.w == 0){
        return vec3(0.0);
    }
    vec3 lightColor = light.color.xyz * light.color.w;
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (distance * distance + 1e-6);
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon = light.direction.w - light.position.w;
    float intensity = clamp((theta -light.position.w) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = material.ambient * diffuseTex;
    vec3 diffuse = lightColor * material.diffuse * diff * diffuseTex;
    vec3 specular = lightColor * material.specular * spec * specularTex;
    ambient *= light.color.w * attenuation * intensity;
    diffuse *= light.color.w * attenuation * intensity;
    specular *= light.color.w * attenuation * intensity;
    return (ambient + diffuse + specular);
}
