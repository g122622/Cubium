#version 450

// 太阳片段着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染太阳圆盘，使用程序化圆盘而非纹理

// 从顶点着色器输入
layout(location = 0) in vec2 fragTexCoord;

// 输出颜色
layout(location = 0) out vec4 outColor;

// 描述符集 0 - Uniform 缓冲区
layout(set = 0, binding = 0) uniform SkyUBO {
    vec4 skyColor;
    vec4 fogColor;
    vec4 sunriseColor;
    vec4 sunriseDirection;
    vec4 cameraForward;
    float celestialAngle;
    float starBrightness;
    int moonPhase;
    float padding;
} sky;

void main() {
    // Java 版太阳为“方形太阳贴图”。
    // 这里用程序化方形 + 轻微边缘暖色渐变模拟。
    vec2 centered = abs(fragTexCoord - 0.5) * 2.0;
    float edge = max(centered.x, centered.y);

    vec3 inner = vec3(1.00, 0.98, 0.88);
    vec3 edgeColor = vec3(1.00, 0.90, 0.62);
    vec3 color = mix(inner, edgeColor, smoothstep(0.25, 1.0, edge));

    // 保持太阳不透明，避免出现“缺角/咬掉”视觉瑕疵。
    outColor = vec4(color, 1.0);
}
