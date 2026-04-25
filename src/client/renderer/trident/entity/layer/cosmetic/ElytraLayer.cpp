#include "ElytraLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::cosmetic {

namespace {
    constexpr f32 PI = 3.14159265f;
    constexpr f32 ELYTRA_WIDTH = 10.0f / 16.0f;   // 单边鞘翅宽度
    constexpr f32 ELYTRA_HEIGHT = 20.0f / 16.0f;  // 鞘翅高度
    constexpr i32 SPREAD_ANGLE_BUCKETS = 18;      // 展开角度分桶数
}

template<typename TEntity>
void ElytraLayer<TEntity>::renderPipeline(
    TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 检查是否有鞘翅纹理
    if (!shouldRender(entity)) {
        return;
    }

    // 计算鞘翅展开角度
    f32 spreadAngle = calculateElytraAngle(entity, static_cast<f32>(context.partialTicks));

    // 获取或创建鞘翅网格
    pipeline::EntityMesh* mesh = getOrCreateElytraMesh(spreadAngle, pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算鞘翅变换矩阵
    std::array<f64, 16> elytraTransform;
    elytraTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 鞘翅附着在背部
    elytraTransform[3] = 0.0;                             // X
    elytraTransform[7] = 1.0 - ELYTRA_HEIGHT * 0.3;       // Y - 上部附着点
    elytraTransform[11] = 0.1;                            // Z - 略微向后

    // 获取实体的世界位置
    Vector3f entityPos(
        static_cast<f32>(entity.x()),
        static_cast<f32>(entity.y()),
        static_cast<f32>(entity.z())
    );

    // 使用实体的受伤时间
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    }

    pipeline.drawMesh(cmd, *mesh, elytraTransform, entityPos, 1.0,
                      Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);

    spdlog::trace("ElytraLayer: Rendered elytra with spread angle {:.1f}", spreadAngle);
}

template<typename TEntity>
void ElytraLayer<TEntity>::render(
    TEntity& entity,
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

template<typename TEntity>
bool ElytraLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // 检查实体是否正在滑翔（装备了鞘翅且在飞行）
    // 完整实现需要检查：
    // 1. 胸甲槽是否装备了鞘翅物品
    // 2. 实体是否处于滑翔状态
    // 3. 是否有鞘翅或披风纹理

    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        // 检查是否装备鞘翅
        const auto& chest = entity.getEquipment(::mc::EquipmentSlot::Chest);
        // TODO: 检查物品是否为鞘翅
        if (!chest.isEmpty()) {
            return m_customElytraRegion != nullptr || m_capeRegion != nullptr;
        }
    }

    return m_customElytraRegion != nullptr || m_capeRegion != nullptr;
}

template<typename TEntity>
f32 ElytraLayer<TEntity>::calculateElytraAngle(TEntity& entity, f32 partialTicks) const {
    // 参考 MC 1.16.5 ElytraLayer
    // 鞘翅展开角度取决于：
    // 1. 是否在滑翔
    // 2. 滑翔时间和速度

    // 检查实体是否正在滑翔
    bool isGliding = false;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        // TODO: 检查 isGliding() 状态
        // 暂时根据 fall distance 估算
        isGliding = entity.fallDistance() > 1.0f;
    }

    if (!isGliding) {
        // 未滑翔时鞘翅收起在背上
        return 0.0f;
    }

    // 滑翔时鞘翅展开
    // 展开角度基于滑翔时间或速度
    f32 time = static_cast<f32>(entity.ticksExisted()) + partialTicks;

    // 基础展开角度约 60 度
    f32 baseSpread = 60.0f;

    // 添加轻微的摆动
    f32 wobble = std::sin(time * 0.5f) * 5.0f;

    return baseSpread + wobble;
}

template<typename TEntity>
void ElytraLayer<TEntity>::buildElytraMesh(
    f32 spreadAngle,
    bool isLeftWing,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 参考 MC 1.16.5 ElytraModel
    // 鞘翅由两个翼组成，每个翼是一个梯形形状

    f32 halfHeight = ELYTRA_HEIGHT / 2.0f;

    // 将展开角度转换为弧度
    f32 spreadRad = spreadAngle * PI / 180.0f;
    f32 cosSpread = std::cos(spreadRad);
    f32 sinSpread = std::sin(spreadRad);

    // 翼的展开方向
    f32 direction = isLeftWing ? -1.0f : 1.0f;

    // 鞘翅翼的顶点
    // 顶部附着在背部
    f32 topY = halfHeight;
    f32 bottomY = -halfHeight;

    // 展开后的翼端位置
    f32 wingTipX = direction * ELYTRA_WIDTH * cosSpread;
    f32 wingTipZ = ELYTRA_WIDTH * sinSpread;

    // UV 坐标
    f32 u0 = 0.0f;
    f32 u1 = 1.0f;
    f32 v0 = 0.0f;
    f32 v1 = 1.0f;

    // 构建顶点（单面渲染，因为背部看不到）
    // 从背部中心到翼端 - ModelVertex(x, y, z, u, v, nx, ny, nz)
    vertices.push_back(model::ModelVertex(0.0f, topY, 0.02f, u0, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(wingTipX * 0.8f, topY, wingTipZ * 0.5f + 0.02f, u1, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(wingTipX, bottomY, wingTipZ + 0.02f, u1, v1, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(0.0f, bottomY, 0.02f, u0, v1, 0.0f, 0.0f, 1.0f));

    // 索引
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);
}

template<typename TEntity>
pipeline::EntityMesh* ElytraLayer<TEntity>::getOrCreateElytraMesh(
    f32 spreadAngle,
    pipeline::EntityPipeline& pipeline)
{
    // 将展开角度离散化到桶中
    i32 bucket = static_cast<i32>(spreadAngle / 5.0f * SPREAD_ANGLE_BUCKETS);
    bucket = std::max(0, std::min(SPREAD_ANGLE_BUCKETS - 1, bucket));

    auto it = m_elytraMeshCache.find(bucket);
    if (it != m_elytraMeshCache.end()) {
        return &it->second;
    }

    // 构建新的鞘翅网格（左右两个翼）
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    // 左翼
    buildElytraMesh(spreadAngle, true, vertices, indices);

    // 右翼
    u32 leftVertexCount = static_cast<u32>(vertices.size());
    buildElytraMesh(spreadAngle, false, vertices, indices);

    // 调整右翼的索引
    for (size_t i = leftVertexCount; i < indices.size(); ++i) {
        indices[i] += leftVertexCount;
    }

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("ElytraLayer: Failed to create elytra mesh for angle {:.1f}", spreadAngle);
        return nullptr;
    }

    m_elytraMeshCache[bucket] = std::move(result.value());
    return &m_elytraMeshCache[bucket];
}

template<typename TEntity>
void ElytraLayer<TEntity>::setElytraTexture(const TextureRegion* region) {
    m_customElytraRegion = region;
}

template<typename TEntity>
void ElytraLayer<TEntity>::setCapeTexture(const TextureRegion* region) {
    m_capeRegion = region;
}

// 显式实例化
template class ElytraLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::cosmetic
