#version 450

// 区块片段着色器

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragSkyLight;
layout(location = 4) in vec3 fragWorldPos;
layout(location = 5) in float fragViewDistance;
layout(location = 6) in float fragBlockLight;

layout(location = 0) out vec4 outColor;

// 描述符集 1 - 纹理采样器
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// 描述符集 0 - 光照 UBO（与相机 UBO 同一个描述符集）
layout(set = 0, binding = 1) uniform LightingUBO {
    vec3 sunDirection;
    float sunIntensity;
    vec3 moonDirection;
    float moonIntensity;
    float dayTime;
    float gameTime;
    float _padding0;
    float _padding1;
} lighting;

// 描述符集 2 - 雾 UBO
layout(set = 2, binding = 0) uniform FogUBO {
    float fogStart;
    float fogEnd;
    float fogDensity;
    int fogMode;
    vec4 fogColor;
} fog;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // 透明像素裁剪，避免叶子等贴图产生黑边
    if (texColor.a < 0.1) {
        discard;
    }

    // 天空光昼夜衰减：白天受太阳主导，夜晚受月光主导
    float skyVisibility = clamp(lighting.sunIntensity + lighting.moonIntensity * 0.35, 0.0, 1.0);

    // Java 版逻辑近似：max(blockLight, skyLight - skyDarkening)
    // 这里将 skyDarkening 近似为 skyVisibility 缩放。
    float effectiveSkyLight = fragSkyLight * skyVisibility;
    float effectiveLight = max(fragBlockLight, effectiveSkyLight);

    // 避免树荫下出现纯黑块：仅在有天空光时给最低环境光，不影响纯地下区域
    float ambientFloor = (0.04 + 0.08 * skyVisibility) * step(0.001, fragSkyLight);
    float finalLight = max(effectiveLight, ambientFloor);

    vec3 finalColor = texColor.rgb * fragColor.rgb * finalLight;

    // 雾计算
    // fogMode: 0 = 禁用, 1 = 线性雾, 2 = 指数雾
    if (fog.fogMode == 1) {
        // 线性雾（陆地）
        float distance = fragViewDistance;
        float fogFactor = clamp((fog.fogEnd - distance) / (fog.fogEnd - fog.fogStart), 0.0, 1.0);
        finalColor = mix(fog.fogColor.rgb, finalColor, fogFactor);
    } else if (fog.fogMode == 2) {
        // 指数雾（水中/岩浆）
        float fogFactor = 1.0 - exp(-fog.fogDensity * fragViewDistance);
        finalColor = mix(fog.fogColor.rgb, finalColor, clamp(fogFactor, 0.0, 1.0));
    }

    outColor = vec4(finalColor, texColor.a * fragColor.a);
}
