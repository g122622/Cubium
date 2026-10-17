#version 450

// 月亮片段着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染月亮圆盘，使用程序化月相而非纹理

// 从顶点着色器输入
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in flat int fragMoonPhase;

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
    // 月相纹理 UV 已经由顶点着色器计算好
    // 这里需要根据月相来渲染月亮的外观

    // 将 UV 转换回相对于当前月相的局部坐标
    // 月相单元内: [0, 0.25] x [0, 0.5] 或 [0, 0.25] x [0.5, 1.0]
    vec2 localUV = fragTexCoord;
    localUV.x = fract(localUV.x * 4.0); // 4 列
    localUV.y = fract(localUV.y * 2.0);  // 2 行

    // 计算到中心的距离
    vec2 center = localUV - 0.5;
    float dist = length(center) * 2.0;

    // 圆盘半径
    float radius = 0.45;
    float edgeSoftness = 0.05;

    // 圆盘形状
    float alpha = 1.0 - smoothstep(radius - edgeSoftness, radius, dist);

    // 如果完全透明则丢弃
    if (alpha < 0.01) {
        discard;
    }

    // 月亮基础颜色 - 灰白色
    vec3 moonColor = vec3(0.95, 0.95, 0.9);

    // 月相效果 - 简化版本
    // 实际 MC 中月相纹理定义了明暗区域
    // 这里用简单的阴影模拟

    // 月相 0: 满月 (全部亮)
    // 月相 1: 盈凸月 (左侧微暗)
    // 月相 2: 上弦月 (右半亮)
    // 月相 3: 盈月 (右侧大部分暗)
    // 月相 4: 新月 (几乎全暗)
    // 月相 5: 亏月 (左侧大部分暗)
    // 月相 6: 下弦月 (左半亮)
    // 月相 7: 亏凸月 (右侧微暗)

    float shadowX = 0.0;
    float shadowWidth = 0.0;
    bool shadowRight = false;

    switch (fragMoonPhase) {
        case 0: // 满月
            break;
        case 1: // 盈凸月
            shadowRight = false;
            shadowX = -0.15;
            shadowWidth = 0.3;
            break;
        case 2: // 上弦月
            shadowRight = false;
            shadowX = 0.0;
            shadowWidth = 0.5;
            break;
        case 3: // 盈月
            shadowRight = false;
            shadowX = 0.15;
            shadowWidth = 0.7;
            break;
        case 4: // 新月
            shadowX = 0.0;
            shadowWidth = 1.0;
            break;
        case 5: // 亏月
            shadowRight = true;
            shadowX = -0.15;
            shadowWidth = 0.7;
            break;
        case 6: // 下弦月
            shadowRight = true;
            shadowX = 0.0;
            shadowWidth = 0.5;
            break;
        case 7: // 亏凸月
            shadowRight = true;
            shadowX = 0.15;
            shadowWidth = 0.3;
            break;
    }

    // 应用阴影
    if (shadowWidth > 0.0) {
        float shadowDist;
        if (shadowRight) {
            shadowDist = localUV.x - 0.5 + shadowX;
        } else {
            shadowDist = 0.5 - localUV.x - shadowX;
        }

        // 椭圆阴影模拟月相
        float shadowFactor = smoothstep(-0.1, shadowWidth * 0.5, shadowDist);

        // 保留圆盘形状内的阴影
        if (dist < radius) {
            alpha *= (1.0 - shadowFactor * 0.95);
        }
    }

    // 新月几乎不可见
    if (fragMoonPhase == 4) {
        alpha *= 0.1;
    }

    if (alpha < 0.01) {
        discard;
    }

    outColor = vec4(moonColor, alpha);
}
