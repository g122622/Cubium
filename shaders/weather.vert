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

    // C++ 端顶点位置已是世界坐标减相机平移 (worldPos - cameraPos)，
    // 仅保留相对相机的世界朝向（雨滴在世界空间始终垂直下落）。
    // 这里只取 view 的旋转部分（丢弃其平移列，否则会多减一次 cameraPos），
    // 使旋转相机时雨滴按世界垂直方向正确透视，而非钉在屏幕上。
    mat3 viewRotation = mat3(ubo.view);
    gl_Position = ubo.projection * mat4(viewRotation) * vec4(inPosition, 1.0);
}
