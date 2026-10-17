#version 450

// 月亮顶点着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染月亮四边形，尺寸 20.0，位于天空球上
// 支持月相 (moon phases 0-7)

// 顶点输入 - 只有位置（单位四边形 [-1, 1]）
layout(location = 0) in vec3 inPosition;

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out flat int fragMoonPhase;

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

// 计算月亮方向向量
// 月亮与太阳相对，角度偏移 0.5
vec3 calculateMoonDirection(float angle) {
    // 月亮在太阳的对面
    float moonAngle = angle + 0.5;
    if (moonAngle >= 1.0) {
        moonAngle -= 1.0;
    }

    float radians = moonAngle * 6.28318530718; // 2 * PI
    float y = cos(radians);
    float xz = sin(radians);

    // 月亮沿 X 轴移动
    return normalize(vec3(xz, y, 0.0));
}

void main() {
    // 月亮尺寸常量（MC 1.16.5 中为 20.0）
    const float MOON_SIZE = 20.0;

    // 计算月亮方向
    vec3 moonDir = calculateMoonDirection(sky.celestialAngle);

    // 构建月亮四边形的朝向基向量
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, moonDir));

    // 处理月亮接近天顶/天底时的退化情况
    if (length(right) < 0.001) {
        right = normalize(cross(vec3(0.0, 0.0, 1.0), moonDir));
        if (length(right) < 0.001) {
            right = vec3(1.0, 0.0, 0.0);
        }
    }

    vec3 forward = normalize(cross(moonDir, right));

    // 计算月亮四边形的世界空间位置
    // MC 1.16.5 中月亮与太阳使用相同距离 100.0
    // 由于视图投影矩阵已移除平移分量，月亮始终在固定方向
    // 月亮方向已通过 calculateMoonDirection 计算为太阳对面
    const float MOON_DISTANCE = 100.0;
    vec3 moonCenter = moonDir * MOON_DISTANCE;

    // 根据输入顶点位置计算四边形角点
    vec3 worldPos = moonCenter +
        right * inPosition.x * MOON_SIZE +
        forward * inPosition.y * MOON_SIZE;

    // 变换到裁剪空间
    gl_Position = pc.viewProjection * vec4(worldPos, 1.0);

    // 计算纹理坐标 - 根据月相选择 UV 区域
    // MC 的月相纹理是 4x2 网格 (4 列 2 行)
    // 月相 0-3 在第一行，4-7 在第二行
    int phaseX = sky.moonPhase % 4;
    int phaseY = sky.moonPhase / 4;

    // 基础 UV 坐标 (将 [-1, 1] 映射到 [0, 1])
    vec2 baseUV = (inPosition.xy + 1.0) * 0.5;

    // 翻转 Y 坐标
    baseUV.y = 1.0 - baseUV.y;

    // 缩放到单个月相单元 (1/4 宽度, 1/2 高度)
    vec2 uv;
    uv.x = (float(phaseX) + baseUV.x) / 4.0;
    uv.y = (float(phaseY) + baseUV.y) / 2.0;

    fragTexCoord = uv;
    fragMoonPhase = sky.moonPhase;
}
