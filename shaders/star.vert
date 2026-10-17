#version 450

// 星星顶点着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染星星点精灵，使用固定种子生成的球面分布

// 顶点输入 - 只有位置（星星在单位球面上）
layout(location = 0) in vec3 inPosition;

// 输出到片段着色器
layout(location = 0) out float fragBrightness;

// 推送常量 - 视图投影矩阵
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
    float gl_PointSize;
};

// 计算星星旋转后的位置
// 星星随天球旋转，角度与天体角度相关
vec3 rotateStars(vec3 pos, float angle) {
    // 星星绕 Y 轴旋转
    float radians = angle * 6.28318530718; // 2 * PI
    float cosA = cos(radians);
    float sinA = sin(radians);

    return vec3(
        pos.x * cosA + pos.z * sinA,
        pos.y,
        -pos.x * sinA + pos.z * cosA
    );
}

void main() {
    // 星星位置已经在球面上，需要根据天体角度旋转
    // MC 中星星的位置是固定的，整个星空随时间旋转
    vec3 rotatedPos = rotateStars(inPosition, sky.celestialAngle);

    // 星星距离 100（与 MC 1.16.5 一致）
    const float STAR_DISTANCE = 100.0;
    vec3 worldPos = rotatedPos * STAR_DISTANCE;

    // 变换到裁剪空间
    // 矩阵已移除平移分量，星星方向相对于相机固定
    gl_Position = pc.viewProjection * vec4(worldPos, 1.0);

    // 星星亮度传递给片段着色器
    fragBrightness = sky.starBrightness;

    // 设置点大小
    // 基础大小 + 根据亮度的变化
    gl_PointSize = 2.0 + fragBrightness * 2.0;
}
