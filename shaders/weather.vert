#version 450

// 天气顶点着色器

// 顶点输入
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inLightmap;

// Uniform 缓冲区
layout(set = 0, binding = 0) uniform WeatherUBO {
    mat4 projection;
    mat4 view;
    vec3 cameraPos;
    float partialTick;
    float rainStrength;
    float thunderStrength;
    float useLightmap;
} ubo;

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 fragLightmap;

void main() {
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragLightmap = inLightmap;

    // C++ 代码中顶点位置已经是相对于相机的（视图空间位置）
    // 所以只需要应用投影矩阵，不需要再乘以视图矩阵
    gl_Position = ubo.projection * vec4(inPosition, 1.0);
}
