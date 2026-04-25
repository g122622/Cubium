#include "CapeLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::cosmetic {

namespace {
    constexpr f32 PI = 3.14159265f;
    constexpr f32 CAPE_WIDTH = 10.0f / 16.0f;   // 斗篷宽度（世界单位）
    constexpr f32 CAPE_HEIGHT = 16.0f / 16.0f;  // 斗篷高度
    constexpr i32 SWING_ANGLE_BUCKETS = 36;     // 摆动角度分桶数（每10度一个桶）
}

void CapeLayer::renderPipeline(
    ::mc::Player& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 检查是否有斗篷纹理
    if (!m_customCapeRegion) {
        return;
    }

    // 计算斗篷摆动角度
    f32 swingAngle = calculateCapeSwing(entity, static_cast<f32>(context.partialTicks));

    // 获取或创建斗篷网格
    pipeline::EntityMesh* mesh = getOrCreateCapeMesh(swingAngle, pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算斗篷变换矩阵
    std::array<f64, 16> capeTransform;
    capeTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 斗篷附着在身体背部
    // 位置在身体中心偏后
    capeTransform[3] = 0.0;           // X
    capeTransform[7] = 1.0 - CAPE_HEIGHT / 2.0;  // Y - 斗篷中心在身体上方
    capeTransform[11] = 0.125;        // Z - 略微向后偏移

    // 获取实体的世界位置
    Vector3f entityPos(
        static_cast<f32>(entity.x()),
        static_cast<f32>(entity.y()),
        static_cast<f32>(entity.z())
    );

    // 使用实体的受伤时间
    f32 hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
    f32 deathTime = static_cast<f32>(entity.deathTime());

    pipeline.drawMesh(cmd, *mesh, capeTransform, entityPos, 1.0,
                      Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);

    spdlog::trace("CapeLayer: Rendered cape with swing angle {:.1f}", swingAngle);
}

void CapeLayer::render(
    ::mc::Player& entity,
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

bool CapeLayer::shouldRender(const ::mc::Player& entity) const {
    (void)entity;
    // 检查玩家是否有斗篷纹理
    // 在完整实现中，还需要检查：
    // 1. 玩家档案中是否有 capeUrl
    // 2. 是否在观察者模式（不可见）
    return m_customCapeRegion != nullptr;
}

f32 CapeLayer::calculateCapeSwing(::mc::Player& entity, f32 partialTicks) const {
    // 参考 MC 1.16.5 CapeLayer 中的摆动计算
    // 基于玩家移动和视角旋转计算斗篷摆动

    f64 prevX = static_cast<f64>(entity.prevX());
    f64 prevZ = static_cast<f64>(entity.prevZ());
    f64 x = static_cast<f64>(entity.x());
    f64 z = static_cast<f64>(entity.z());

    // 计算移动向量
    f32 moveX = static_cast<f32>(x - prevX);
    f32 moveZ = static_cast<f32>(z - prevZ);

    // 计算移动距离平方
    f32 moveSq = moveX * moveX + moveZ * moveZ;

    // 基于时间和移动计算摆动
    f32 time = static_cast<f32>(entity.ticksExisted()) + partialTicks;
    f32 baseSwing = std::sin(time * 0.067f) * 0.5f;  // 基础摆动

    // 移动时增加摆动幅度
    f32 moveSwing = std::sqrt(moveSq) * 0.3f;

    // 限制最大摆动角度
    f32 totalSwing = baseSwing + moveSwing;
    totalSwing = std::min(totalSwing, 25.0f);  // 最大 25 度

    // 考虑玩家朝向
    f32 yaw = static_cast<f32>(entity.yaw());
    f32 prevYaw = static_cast<f32>(entity.prevYaw());
    f32 yawDiff = yaw - prevYaw;

    // 转向时斗篷会甩动
    f32 turnSwing = yawDiff * 0.5f;
    totalSwing += std::abs(turnSwing);

    return totalSwing;
}

void CapeLayer::buildCapeMesh(
    f32 swingAngle,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 参考 MC 1.16.5 CapeModel
    // 斗篷是一个简单的矩形，带有摆动动画
    // 摆动使斗篷底部向后倾斜

    f32 halfWidth = CAPE_WIDTH / 2.0f;
    f32 halfHeight = CAPE_HEIGHT / 2.0f;

    // 将摆动角度转换为弧度
    f32 swingRad = swingAngle * PI / 180.0f;
    f32 cosSwing = std::cos(swingRad);
    f32 sinSwing = std::sin(swingRad);

    // 斗篷顶点
    // 斗篷顶部固定在身体上，底部随摆动倾斜
    // 顶点格式: position xyz, texcoord uv, normal xyz, color rgba

    // 顶部两点（固定）
    f32 topY = halfHeight;
    f32 bottomY = -halfHeight;

    // 底部两点随摆动偏移
    f32 bottomZ = sinSwing * CAPE_HEIGHT;
    f32 bottomYOffset = (1.0f - cosSwing) * CAPE_HEIGHT * 0.5f;

    // UV 坐标（斗篷纹理在皮肤纹理中）
    f32 u0 = 0.0f;
    f32 u1 = 1.0f;
    f32 v0 = 0.0f;
    f32 v1 = 1.0f;

    // 构建顶点（双面渲染）
    // 正面
    vertices.push_back(model::ModelVertex(-halfWidth, topY, 0.02f, u0, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(halfWidth, topY, 0.02f, u1, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(halfWidth, bottomY + bottomYOffset, bottomZ + 0.02f, u1, v1, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(-halfWidth, bottomY + bottomYOffset, bottomZ + 0.02f, u0, v1, 0.0f, 0.0f, 1.0f));

    // 反面
    vertices.push_back(model::ModelVertex(-halfWidth, topY, -0.02f, u0, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(halfWidth, topY, -0.02f, u1, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(halfWidth, bottomY + bottomYOffset, bottomZ - 0.02f, u1, v1, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(-halfWidth, bottomY + bottomYOffset, bottomZ - 0.02f, u0, v1, 0.0f, 0.0f, -1.0f));

    // 索引（两个三角形组成一个四边形，正反两面）
    // 正面
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    // 反面
    indices.push_back(4);
    indices.push_back(6);
    indices.push_back(5);
    indices.push_back(4);
    indices.push_back(7);
    indices.push_back(6);
}

pipeline::EntityMesh* CapeLayer::getOrCreateCapeMesh(
    f32 swingAngle,
    pipeline::EntityPipeline& pipeline)
{
    // 将摆动角度离散化到桶中
    i32 bucket = static_cast<i32>(swingAngle / 10.0f * SWING_ANGLE_BUCKETS);
    bucket = std::max(0, std::min(SWING_ANGLE_BUCKETS - 1, bucket));

    auto it = m_capeMeshCache.find(bucket);
    if (it != m_capeMeshCache.end()) {
        return &it->second;
    }

    // 构建新的斗篷网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildCapeMesh(swingAngle, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("CapeLayer: Failed to create cape mesh for angle {:.1f}", swingAngle);
        return nullptr;
    }

    m_capeMeshCache[bucket] = std::move(result.value());
    return &m_capeMeshCache[bucket];
}

} // namespace mc::client::renderer::entity::layer::cosmetic
