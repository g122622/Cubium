#version 450

// 实体片段着色器

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragLight;
layout(location = 4) in vec2 fragOverlayUV;

layout(location = 0) out vec4 outColor;

// 推送常量 - 与顶点着色器保持一致
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 entityPos;
    float scale;
    vec4 overlayColor;   // 覆盖层颜色 (受伤闪烁/道德效果)
    float hurtTime;      // 受伤时间 (0-10)
    float deathTime;     // 死亡时间
    float _padding0;
    float _padding1;
} pc;

// 描述符集 1 - 纹理采样器
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// 是否应用受伤效果
bool shouldApplyHurtEffect() {
    return pc.hurtTime > 0.0;
}

// 是否应用死亡效果
bool shouldApplyDeathEffect() {
    return pc.deathTime > 0.0;
}

// 计算受伤闪烁强度
float computeHurtFlashIntensity() {
    if (pc.hurtTime <= 0.0) {
        return 0.0;
    }

    // 闪烁强度在受伤开始时最强，逐渐减弱
    // hurtTime 从 10 递减到 0
    float progress = 1.0 - (pc.hurtTime / 10.0);
    float intensity = 1.0 - progress;

    // 使用 sin 函数创建闪烁效果
    // MC 1.16.5 使用 sin(hurtTime * 0.3) 来创建闪烁
    float flash = sin(pc.hurtTime * 3.14159 * 0.3) * 0.5 + 0.5;

    return intensity * flash;
}

// 计算死亡淡出强度
float computeDeathFadeIntensity() {
    if (pc.deathTime <= 0.0) {
        return 0.0;
    }

    // 死亡时间最大 20 秒
    float progress = clamp(pc.deathTime / 20.0, 0.0, 1.0);
    return progress;
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // 如果纹理alpha太低，丢弃片段（透明）
    if (texColor.a < 0.1) {
        discard;
    }

    // 基础颜色
    vec3 color = texColor.rgb;
    float alpha = texColor.a;

    // 应用光照
    color *= fragLight;

    // 应用 fragColor 调制
    color *= fragColor.rgb;
    alpha *= fragColor.a;

    // 应用受伤闪烁效果
    if (shouldApplyHurtEffect()) {
        float hurtIntensity = computeHurtFlashIntensity();

        // 叠加红色闪烁
        // MC 1.16.5: 受伤时实体变红
        vec3 hurtColor = vec3(1.0, 0.0, 0.0);
        color = mix(color, hurtColor, hurtIntensity * 0.5);
    }

    // 应用死亡淡出效果
    if (shouldApplyDeathEffect()) {
        float deathIntensity = computeDeathFadeIntensity();

        // 死亡时逐渐变红然后消失
        // MC 1.16.5: 死亡动画
        vec3 deathColor = vec3(1.0, 0.0, 0.0);
        color = mix(color, deathColor, deathIntensity * 0.5);

        // 同时降低透明度
        alpha *= (1.0 - deathIntensity);
    }

    // 应用覆盖层颜色（如果有）
    if (pc.overlayColor.a > 0.0) {
        color = mix(color, pc.overlayColor.rgb, pc.overlayColor.a);
    }

    outColor = vec4(color, alpha);
}
