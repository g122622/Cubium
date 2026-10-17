#version 450

// 天空穹顶片段着色器
// 参考 MC 1.16.5 WorldRenderer.renderSky()
// 渲染天空颜色，在地平线附近应用雾颜色渐变

// 从顶点着色器输入
layout(location = 0) in vec3 fragWorldPos;

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
    vec3 viewDir = normalize(fragWorldPos);
    vec3 color = sky.skyColor.rgb;

    // 在地平线附近加入雾感（上下半球都生效）
    float horizonBand = 1.0 - smoothstep(0.0, 0.45, abs(viewDir.y));
    color = mix(color, sky.fogColor.rgb, horizonBand * 0.45);

    // 日出日落颜色（只在朝向日出/日落中心的一侧地平线出现）
    vec2 sunriseXZ = sky.sunriseDirection.xz;
    vec2 dirXZ = viewDir.xz;
    float sunriseLen = length(sunriseXZ);
    float dirLen = length(dirXZ);
    if (sky.sunriseColor.a > 0.0001 && sunriseLen > 0.0001 && dirLen > 0.0001) {
        vec2 sunriseN = sunriseXZ / sunriseLen;
        vec2 dirN = dirXZ / dirLen;

        // 上半天空扇形：地平线附近 + 朝向中心
        float towardSunrise = max(dot(dirN, sunriseN), 0.0);
        float upperMask = smoothstep(0.35, 0.0, abs(viewDir.y));
        float upperBlend = towardSunrise * upperMask * sky.sunriseColor.a;
        color = mix(color, sky.sunriseColor.rgb, upperBlend);

        // 下半天空填充：受“摄像机朝向与日出中心方向对齐度”影响
        vec2 cameraXZ = sky.cameraForward.xz;
        float camLen = length(cameraXZ);
        float cameraFacing = 0.0;
        if (camLen > 0.0001) {
            cameraFacing = max(dot(cameraXZ / camLen, sunriseN), 0.0);
        }

        float lowerMask = smoothstep(0.0, -0.55, viewDir.y);
        float lowerBlend = lowerMask * cameraFacing * sky.sunriseColor.a;
        color = mix(color, sky.sunriseColor.rgb, lowerBlend);
    }

    outColor = vec4(color, 1.0);
}
