#version 450

// 实体顶点着色器

// 顶点输入 - 与ModelVertex结构匹配
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

// 输出到片段着色器
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out float fragLight;
layout(location = 4) out vec2 fragOverlayUV;

// 推送常量 - 模型矩阵
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 entityPos;      // 实体世界位置
    float scale;         // 缩放因子
    vec4 overlayColor;   // 覆盖层颜色 (受伤闪烁/道德效果)
    float hurtTime;      // 受伤时间 (0-10)
    float deathTime;     // 死亡时间
    float _padding0;
    float _padding1;
} pc;

// 描述符集 0 - 相机UBO
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

// 描述符集 1 - 光照UBO（需与 UniformManager::LightingUBO 保持一致）
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

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 计算世界位置：先应用模型旋转/局部变换，再加实体世界坐标
    vec3 localPos = vec3(pc.model * vec4(inPosition * pc.scale, 1.0));
    vec3 worldPos = localPos + pc.entityPos;

    gl_Position = camera.viewProjection * vec4(worldPos, 1.0);

    // 变换法线
    mat3 normalMatrix = mat3(transpose(inverse(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);

    fragTexCoord = inTexCoord;
    fragColor = vec4(1.0);  // 白色，可以通过uniform覆盖

    // 计算光照：太阳/月亮方向漫反射 + 基础环境光
    vec3 sunDir = normalize(lighting.sunDirection);
    vec3 moonDir = normalize(lighting.moonDirection);

    float sunDiffuse = max(dot(fragNormal, sunDir), 0.0) * max(lighting.sunIntensity, 0.0);
    float moonDiffuse = max(dot(fragNormal, moonDir), 0.0) * max(lighting.moonIntensity, 0.0) * 0.35;

    float skyVisibility = clamp(lighting.sunIntensity + lighting.moonIntensity * 0.35, 0.0, 1.0);
    float ambient = 0.18 + 0.12 * skyVisibility;
    fragLight = clamp(ambient + sunDiffuse + moonDiffuse, 0.0, 1.0);

    // 计算覆盖层 UV（受伤闪烁/道德效果）
    // MC 1.16.5: U = hurtTime / 10.0 * 16.0, V = 0
    // 道德效果时 U 固定为 3.0
    fragOverlayUV = vec2(pc.hurtTime / 10.0 * 16.0, 0.0);
}
