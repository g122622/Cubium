#version 450

// 区块顶点着色器

// 顶点输入 - 与Vertex结构匹配
layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec2 inTexCoord;
layout(location = 5) in vec4 inColor;
layout(location = 6) in uint inLight;

// 输出到片段着色器
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out float fragSkyLight;
layout(location = 4) out vec3 fragWorldPos;
layout(location = 5) out float fragViewDistance;
layout(location = 6) out float fragBlockLight;

// 推送常量 - 模型矩阵
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 chunkRelativeOffset;
} pc;

// 描述符集 0 - 相机UBO
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 在 CPU 侧预先计算区块相对相机偏移。顶点位置是区块局部坐标（0~256），与 f32 推送常量
    // chunkRelativeOffset 相加得到相对相机的世界坐标，避免大世界坐标精度问题。
    vec3 relativePos = inPosition + pc.chunkRelativeOffset.xyz;

    mat4 viewNoTranslation = camera.view;
    viewNoTranslation[3] = vec4(0.0, 0.0, 0.0, 1.0);

    gl_Position = camera.projection * viewNoTranslation * pc.model * vec4(relativePos, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragSkyLight = float((inLight >> 4) & 0xFu) / 15.0;
    fragBlockLight = float(inLight & 0xFu) / 15.0;

    // 输出世界坐标和视图距离（用于雾效果）
    fragWorldPos = relativePos;
    fragViewDistance = length(relativePos);
}
