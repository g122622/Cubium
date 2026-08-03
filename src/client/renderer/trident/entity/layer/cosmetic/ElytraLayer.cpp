/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ElytraLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::layer::cosmetic {

namespace {
constexpr f32 ELYTRA_WIDTH = 10.0f / 16.0f;  // 单边鞘翅宽度
constexpr f32 ELYTRA_HEIGHT = 20.0f / 16.0f; // 鞘翅高度
constexpr i32 SPREAD_ANGLE_BUCKETS = 18;     // 展开角度分桶数

// 鞘翅默认角度
constexpr f32 DEFAULT_X_ANGLE = 0.2617994f;    // ~15度
constexpr f32 DEFAULT_Z_ANGLE = -0.2617994f;   // ~-15度
constexpr f32 GLIDING_X_ANGLE = 0.34906584f;   // ~20度
constexpr f32 GLIDING_Z_ANGLE = -1.5707963f;   // ~-90度
constexpr f32 CROUCHING_X_ANGLE = 0.6981317f;  // ~40度
constexpr f32 CROUCHING_Z_ANGLE = -0.7853982f; // ~-45度
} // namespace

template <typename TEntity>
void ElytraLayer<TEntity>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 检查是否有鞘翅纹理
    if (!shouldRender(entity)) {
        return;
    }

    // 计算鞘翅展开角度
    f32 spreadAngle = _calculateElytraAngle(entity, context, static_cast<f32>(context.partialTicks));

    // 获取或创建鞘翅网格
    pipeline::EntityMesh* mesh = _getOrCreateElytraMesh(spreadAngle, pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算鞘翅变换矩阵
    std::array<f64, 16> elytraTransform;
    elytraTransform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 鞘翅附着在背部
    elytraTransform[3] = 0.0;                       // X
    elytraTransform[7] = 1.0 - ELYTRA_HEIGHT * 0.3; // Y - 上部附着点
    elytraTransform[11] = 0.125;                    // Z - 向后偏移

    // 获取实体的世界位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 使用实体的受伤时间
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    }

    pipeline.drawMesh(
        cmd, *mesh, elytraTransform, entityPos, 1.0, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);
}

template <typename TEntity>
bool ElytraLayer<TEntity>::shouldRender(const TEntity& entity) const
{
    // 检查：
    // 1. 胸甲槽装备了鞘翅物品（Items.ELYTRA）
    // 2. 有鞘翅或披风纹理

    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        // 检查胸甲槽
        const auto& chest = entity.getEquipment(::mc::EquipmentSlot::Chest);
        if (chest.isEmpty()) {
            return false;
        }

        // 检查物品是否为鞘翅
        if (chest.getItem() != ::mc::Items::ELYTRA) {
            return false;
        }

        return m_customElytraRegion != nullptr || m_capeRegion != nullptr;
    } else {
        // 非 LivingEntity 类型，仅检查纹理
        return m_customElytraRegion != nullptr || m_capeRegion != nullptr;
    }
}

template <typename TEntity>
f32 ElytraLayer<TEntity>::_calculateElytraAngle(
    TEntity& entity, const mc::client::renderer::entity::core::AnimationContext& context, f32 partialTicks) const
{
    // 鞘翅角度取决于：
    // 1. isElytraFlying() - 是否在滑翔
    // 2. isCrouching() - 是否蹲伏
    // 3. 速度向量 - 滑翔时的俯仰角

    // 默认角度
    f32 angleZ = DEFAULT_Z_ANGLE; // ~-15度

    bool isGliding = false;
    bool isCrouching = false;

    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        // 检查是否正在鞘翅飞行
        isGliding = entity.isElytraFlying();

        // 检查是否蹲伏
        isCrouching = context.isSneaking;

        if (isGliding) {
            // 获取速度向量
            auto velocity = entity.velocity();
            f64 motionY = static_cast<f64>(velocity.y);

            f32 f4 = 1.0f;
            if (motionY < 0.0) {
                // 计算速度向量长度
                f64 speed = std::sqrt(static_cast<f64>(velocity.x) * static_cast<f64>(velocity.x) +
                    static_cast<f64>(velocity.y) * static_cast<f64>(velocity.y) +
                    static_cast<f64>(velocity.z) * static_cast<f64>(velocity.z));
                if (speed > 0.0) {
                    f64 normalizedY = motionY / speed;
                    f4 = static_cast<f32>(1.0 - std::pow(-normalizedY, 1.5));
                }
            }

            // 插值角度
            angleZ = f4 * GLIDING_Z_ANGLE + (1.0f - f4) * DEFAULT_Z_ANGLE;
        } else if (isCrouching) {
            // 蹲伏时的角度
            angleZ = CROUCHING_Z_ANGLE;
        }

        // 注意：X 轴前倾角尚未接入当前网格生成流程，后续若补齐姿态细节需要一并接入。
        // 注意：平滑角度插值需要架构调整
        // 当前项目的 ClientEntity 已有 rotateElytraX/Y/Z 字段和 updateElytraAngles() 方法
        // 但渲染层使用的是 Player/LivingEntity 实体，不是 ClientEntity
        // 完整实现需要：在渲染流程中关联 ClientEntity 或在 Player 中添加这些字段
    }

    // 将角度转换为展开度数（用于网格生成）
    // X 角度控制前后倾斜，Z 角度控制左右展开
    // 这里返回 Z 角度的绝对值作为展开角度
    return std::abs(angleZ) * mc::math::RAD_TO_DEG;
}

template <typename TEntity>
void ElytraLayer<TEntity>::_buildElytraMesh(
    f32 spreadAngle, bool isLeftWing, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 鞘翅由两个翼组成，每个翼是一个梯形形状

    f32 halfHeight = ELYTRA_HEIGHT / 2.0f;

    // 将展开角度转换为弧度
    f32 spreadRad = mc::math::toRadians(spreadAngle);
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

template <typename TEntity>
pipeline::EntityMesh* ElytraLayer<TEntity>::_getOrCreateElytraMesh(f32 spreadAngle, pipeline::EntityPipeline& pipeline)
{
    // 将展开角度离散化到桶中
    i32 bucket = static_cast<i32>(spreadAngle / 5.0f * SPREAD_ANGLE_BUCKETS);
    bucket = mc::math::clamp(bucket, 0, SPREAD_ANGLE_BUCKETS - 1);

    auto it = m_elytraMeshCache.find(bucket);
    if (it != m_elytraMeshCache.end()) {
        return &it->second;
    }

    // 构建新的鞘翅网格（左右两个翼）
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    // 左翼
    _buildElytraMesh(spreadAngle, true, vertices, indices);

    // 右翼
    u32 leftVertexCount = static_cast<u32>(vertices.size());
    _buildElytraMesh(spreadAngle, false, vertices, indices);

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

template <typename TEntity>
void ElytraLayer<TEntity>::setElytraTexture(const TextureRegion* region)
{
    m_customElytraRegion = region;
}

template <typename TEntity>
void ElytraLayer<TEntity>::setCapeTexture(const TextureRegion* region)
{
    m_capeRegion = region;
}

// 显式实例化
template class ElytraLayer<::mc::LivingEntity>;
template class ElytraLayer<::mc::Player>;

} // namespace mc::client::renderer::entity::layer::cosmetic
