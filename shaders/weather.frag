#version 450

// 天气片段着色器

// 从顶点着色器输入
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 fragLightmap;

// 纹理采样器
layout(set = 0, binding = 1) uniform sampler2D texSampler;
// 光照贴图（16×16，blockLight × skyLight 网格）
layout(set = 0, binding = 2) uniform sampler2D lightmapSampler;

layout(set = 0, binding = 0) uniform WeatherUBO {
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
    float partialTick;
    float rainStrength;
    float thunderStrength;
    float useLightmap;
} ubo;

// 输出颜色
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // 应用顶点颜色和透明度
    outColor = texColor * fragColor;

    // 如果 alpha 太低，丢弃片段
    if (outColor.a < 0.01) {
        discard;
    }

    // 应用光照：fragLightmap.x/y 为归一化 blockLight/skyLight (0..1)。
    // lightmap 已注入时采样 16×16 光照贴图取 RGB 光照颜色；否则回退标量 max。
    vec3 lightColor;
    if (ubo.useLightmap > 0.5) {
        lightColor = texture(lightmapSampler, fragLightmap).rgb;
    } else {
        lightColor = vec3(max(fragLightmap.x, fragLightmap.y));
    }
    outColor.rgb *= lightColor;
}
