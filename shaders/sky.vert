#version 450

// 天空穹顶顶点着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染一个位于 Y=16 的平面网格，范围 -384 到 +384

// 顶点输入 - 只有位置
layout(location = 0) in vec3 inPosition;

// 输出到片段着色器
layout(location = 0) out vec3 fragWorldPos;

// 推送常量 - 视图投影矩阵（已经过缩放处理）
layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
} pc;

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

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 天空网格位于 Y=16 平面
    // 顶点位置就是世界位置
    fragWorldPos = inPosition;

    // 使用线性裁剪映射（linear clip mapping）
    // 矩阵已经应用了 SKY_CLIP_SCALE 缩放
    gl_Position = pc.viewProjection * vec4(inPosition, 1.0);

    // 强制 z = w，使深度为 1.0（远平面）
    gl_Position.z = gl_Position.w;
}
