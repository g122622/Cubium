#version 450

// 星星片段着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染星星点精灵

// 从顶点着色器输入
layout(location = 0) in float fragBrightness;

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
    // 点精灵坐标，(0.5, 0.5) 是中心
    vec2 coord = gl_PointCoord - 0.5;
    float dist = length(coord) * 2.0;

    // 圆形点精灵
    float alpha = 1.0 - smoothstep(0.3, 1.0, dist);

    // 如果太暗则丢弃
    if (alpha < 0.1 || fragBrightness < 0.01) {
        discard;
    }

    // 星星颜色 - 白色/淡蓝色
    vec3 starColor = vec3(1.0, 1.0, 0.95);

    // 应用亮度
    alpha *= fragBrightness;

    outColor = vec4(starColor * fragBrightness, alpha);
}
