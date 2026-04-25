#include "WolfCollarLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::entity {

namespace {
    // 项圈颜色 RGB 值（MC 1.16.5 DyeColor）
    const Vector3f COLLAR_COLORS[16] = {
        Vector3f(1.0f, 1.0f, 1.0f),       // 白色 (0)
        Vector3f(0.85f, 0.5f, 0.2f),      // 橙色 (1)
        Vector3f(0.8f, 0.2f, 0.6f),       // 品红色 (2)
        Vector3f(0.2f, 0.6f, 0.9f),        // 淡蓝色 (3)
        Vector3f(0.9f, 0.9f, 0.2f),       // 黄色 (4)
        Vector3f(0.4f, 0.8f, 0.2f),       // 黄绿色 (5)
        Vector3f(1.0f, 0.5f, 0.7f),       // 粉红色 (6)
        Vector3f(0.3f, 0.3f, 0.3f),       // 灰色 (7)
        Vector3f(0.5f, 0.5f, 0.5f),       // 淡灰色 (8)
        Vector3f(0.2f, 0.4f, 0.6f),       // 青色 (9)
        Vector3f(0.5f, 0.2f, 0.8f),       // 紫色 (10)
        Vector3f(0.2f, 0.3f, 0.7f),       // 蓝色 (11)
        Vector3f(0.5f, 0.3f, 0.1f),       // 棕色 (12)
        Vector3f(0.2f, 0.5f, 0.2f),       // 绿色 (13)
        Vector3f(0.6f, 0.2f, 0.2f),       // 红色 (14)
        Vector3f(0.1f, 0.1f, 0.1f),       // 黑色 (15)
    };
}

// 静态成员定义
std::unique_ptr<pipeline::EntityMesh> WolfCollarLayer::s_collarMesh = nullptr;

void WolfCollarLayer::renderPipeline(
    ::mc::WolfEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 获取项圈颜色
    Vector3f color = getCollarColor(entity);

    // 获取或创建项圈网格
    pipeline::EntityMesh* mesh = getOrCreateCollarMesh(pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算项圈变换矩阵
    // 项圈位于狼的颈部
    std::array<f64, 16> collarTransform;
    collarTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 项圈位置（颈部）
    collarTransform[3] = 0.0;       // X
    collarTransform[7] = 0.5;       // Y - 颈部高度
    collarTransform[11] = 0.1;      // Z - 略微向前

    // 应用头部旋转（项圈跟随头部）
    f32 headYaw = static_cast<f32>(context.netHeadYaw);
    f32 yawRad = headYaw * 3.14159265f / 180.0f;
    f32 cosYaw = std::cos(yawRad);
    f32 sinYaw = std::sin(yawRad);

    // 轻微的头部旋转影响
    collarTransform[0] = cosYaw;
    collarTransform[2] = -sinYaw * 0.3;  // 轻微影响
    collarTransform[8] = sinYaw * 0.3;
    collarTransform[10] = cosYaw;

    // 获取实体位置
    Vector3f entityPos(
        static_cast<f32>(entity.x()),
        static_cast<f32>(entity.y()),
        static_cast<f32>(entity.z())
    );

    // 使用项圈颜色作为叠加颜色
    Vector4f overlayColor(color.x, color.y, color.z, 1.0f);

    // 使用实体的受伤时间
    f32 hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
    f32 deathTime = static_cast<f32>(entity.deathTime());

    pipeline.drawMesh(cmd, *mesh, collarTransform, entityPos, 1.0,
                      overlayColor, hurtTime, deathTime);

    spdlog::trace("WolfCollarLayer: Rendered collar with color ({}, {}, {})",
                  color.x, color.y, color.z);

    (void)context;
}

void WolfCollarLayer::render(
    ::mc::WolfEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // CPU 路径已废弃
    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

bool WolfCollarLayer::shouldRender(const ::mc::WolfEntity& entity) const {
    // 只有驯服的狼才显示项圈
    // TODO: 检查 entity.isTamed()
    (void)entity;
    return true; // 暂时返回 true
}

Vector3f WolfCollarLayer::getCollarColor(const ::mc::WolfEntity& entity) {
    // 获取狼的项圈颜色
    // TODO: 从实体获取项圈颜色
    // u8 colorIndex = entity.getCollarColor();
    // if (colorIndex < 16) {
    //     return COLLAR_COLORS[colorIndex];
    // }
    (void)entity;
    return COLLAR_COLORS[14]; // 默认红色
}

void WolfCollarLayer::buildCollarMesh(
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 参考 MC 1.16.5 的狼项圈模型
    // 项圈是一个简单的环形，围绕颈部

    vertices.clear();
    indices.clear();

    // 项圈尺寸
    constexpr f32 COLLAR_RADIUS = 0.15f;
    constexpr f32 COLLAR_HEIGHT = 0.1f;
    constexpr i32 SEGMENTS = 16;

    // 生成环形顶点
    for (i32 seg = 0; seg <= SEGMENTS; ++seg) {
        f32 angle = static_cast<f32>(seg) / static_cast<f32>(SEGMENTS) * 2.0f * 3.14159265f;
        f32 cosAngle = std::cos(angle);
        f32 sinAngle = std::sin(angle);

        f32 x = COLLAR_RADIUS * cosAngle;
        f32 z = COLLAR_RADIUS * sinAngle;

        // 外环 - ModelVertex(x, y, z, u, v, nx, ny, nz)
        vertices.push_back(model::ModelVertex(x, 0.0f, z,
                          static_cast<f32>(seg) / static_cast<f32>(SEGMENTS), 0.0f,
                          cosAngle, 0.0f, sinAngle));
        vertices.push_back(model::ModelVertex(x, COLLAR_HEIGHT, z,
                          static_cast<f32>(seg) / static_cast<f32>(SEGMENTS), 1.0f,
                          cosAngle, 0.0f, sinAngle));
    }

    // 生成索引
    for (i32 seg = 0; seg < SEGMENTS; ++seg) {
        u32 base = static_cast<u32>(seg * 2);

        // 外环
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 1);
        indices.push_back(base + 3);
        indices.push_back(base + 2);
    }
}

pipeline::EntityMesh* WolfCollarLayer::getOrCreateCollarMesh(pipeline::EntityPipeline& pipeline) {
    if (s_collarMesh && s_collarMesh->indexCount > 0) {
        return s_collarMesh.get();
    }

    // 构建项圈网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildCollarMesh(vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("WolfCollarLayer: Failed to create collar mesh");
        return nullptr;
    }

    s_collarMesh = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    return s_collarMesh.get();
}

} // namespace mc::client::renderer::entity::layer::entity
