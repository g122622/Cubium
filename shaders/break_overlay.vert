#version 450

// 方块破坏效果顶点着色器
// 用于渲染挖掘过程中的碎裂纹理覆盖层

// 顶点输入 - 简化的顶点格式（只需要位置和UV）
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

// 输出到片段着色器
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out float fragViewDistance;

// 描述符集 0 - 相机UBO
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} camera;

// 推送常量 - 方块位置
// 破坏阶段 UV 已在 CPU 端烘进顶点（inTexCoord 即绝对 UV），不再需要 damageStage。
layout(push_constant) uniform PushConstants {
    vec3 blockPos;  // 方块在世界空间中的位置
    float pad;      // 对齐填充（与 CPU 端 16 字节布局匹配）
} pc;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // 方块顶点相对于方块位置的偏移
    // inPosition 已经是相对于方块中心的顶点（0-1范围）
    vec3 worldPos = inPosition + pc.blockPos;

    // 计算视图空间位置用于正确的距离计算
    vec4 viewPos = camera.view * vec4(worldPos, 1.0);

    gl_Position = camera.projection * viewPos;
    fragTexCoord = inTexCoord;
    fragWorldPos = worldPos;
    // 使用视图空间距离（到相机的实际距离）
    fragViewDistance = length(viewPos.xyz);
}
