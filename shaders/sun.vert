#version 450

// 太阳顶点着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染太阳四边形，尺寸 30.0，位于天空球上

// 顶点输入 - 只有位置（单位四边形 [-1, 1]）
layout(location = 0) in vec3 inPosition;

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;

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
};

// 计算太阳方向向量
// 参考 DimensionType.calculateCelestialAngle()
vec3 calculateSunDirection(float angle) {
    // angle 范围 [0, 1]，其中：
    // 0.0 = 正午（太阳最高）
    // 0.25 = 日落
    // 0.5 = 午夜
    // 0.75 = 日出

    float radians = angle * 6.28318530718; // 2 * PI
    float y = cos(radians);
    float xz = sin(radians);

    // 太阳沿 X 轴移动
    return normalize(vec3(xz, y, 0.0));
}

void main() {
    // 太阳尺寸常量。
    // MC 1.16.5 中太阳尺寸为 30.0 像素，位于 Y=100 处
    // 由于我们使用 SKY_CLIP_SCALE 缩放矩阵，这里需要调整
    const float SUN_SIZE = 6.0;

    // 计算太阳方向
    vec3 sunDir = calculateSunDirection(sky.celestialAngle);

    // 构建太阳四边形的朝向基向量
    // 使用世界空间的 Y 轴作为参考构建正交基
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, sunDir));

    // 处理太阳接近天顶/天底时的退化情况
    // 当 sunDir 与 up 平行时，cross 结果接近零向量
    if (length(right) < 0.001) {
        // 使用备用向量
        right = normalize(cross(vec3(0.0, 0.0, 1.0), sunDir));
        if (length(right) < 0.001) {
            right = vec3(1.0, 0.0, 0.0);
        }
    }

    vec3 forward = normalize(cross(sunDir, right));

    // 计算太阳四边形的世界空间位置
    // MC 1.16.5 使用固定距离 100.0
    // 由于视图投影矩阵已移除平移分量，太阳始终在固定方向
    const float SUN_DISTANCE = 100.0;
    vec3 sunCenter = sunDir * SUN_DISTANCE;

    // 根据输入顶点位置计算四边形角点
    // inPosition.xy 在 [-1, 1] 范围内
    vec3 worldPos = sunCenter +
        right * inPosition.x * SUN_SIZE +
        forward * inPosition.y * SUN_SIZE;

    // 变换到裁剪空间
    // 矩阵已移除平移分量，太阳方向相对于相机固定
    gl_Position = pc.viewProjection * vec4(worldPos, 1.0);

    // 计算纹理坐标
    // 将 [-1, 1] 映射到 [0, 1]
    fragTexCoord = (inPosition.xy + 1.0) * 0.5;

    // 翻转 Y 坐标以匹配纹理
    fragTexCoord.y = 1.0 - fragTexCoord.y;
}
