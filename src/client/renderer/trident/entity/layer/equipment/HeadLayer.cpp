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

#include "HeadLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/item/ItemMeshBuilder.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>
#include <type_traits>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::equipment {

// 静态成员定义
template <typename TEntity, typename TModel>
std::unordered_map<u32, pipeline::EntityMesh> HeadLayer<TEntity, TModel>::s_headItemMeshCache;

template <typename TEntity, typename TModel>
void HeadLayer<TEntity, TModel>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    renderHeadItemPipeline(entity, cmd, context, pipeline);
}

template <typename TEntity, typename TModel>
bool HeadLayer<TEntity, TModel>::shouldRender(const TEntity& entity) const
{
    const ItemStack* headItem = getHeadItem(entity);
    return headItem && !headItem->isEmpty();
}

template <typename TEntity, typename TModel>
void HeadLayer<TEntity, TModel>::renderHeadItemPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    const ItemStack* item = getHeadItem(entity);
    if (!item || item->isEmpty()) {
        return;
    }

    // 获取父模型并使用头部部件定位
    std::array<f64, 16> headTransform;

    TModel* parentModel = getParentModel();
    if (parentModel) {
        // 获取头部部件变换
        auto headPart = parentModel->getModelHead();
        if (headPart) {
            headPart->getTransformMatrix(headTransform);
        } else {
            // 回退到硬编码变换
            computeHeadTransform(
                static_cast<f32>(context.netHeadYaw), static_cast<f32>(context.headPitch), headTransform);
        }
    } else {
        // 无父模型时使用硬编码变换
        computeHeadTransform(static_cast<f32>(context.netHeadYaw), static_cast<f32>(context.headPitch), headTransform);
    }

    // 获取物品ID用于缓存
    u32 itemId = static_cast<u32>(item->getItem()->itemId());

    // 获取或创建头部物品网格
    auto it = s_headItemMeshCache.find(itemId);
    if (it == s_headItemMeshCache.end()) {
        // 构建头部物品网格
        auto [vertices, indices] = item::ItemMeshBuilder::buildHeadMesh(*item);

        if (vertices.empty() || indices.empty()) {
            return;
        }

        // 创建 GPU 网格
        auto result = pipeline.createMesh(vertices, indices);
        if (!result.success()) {
            spdlog::warn("HeadLayer: Failed to create mesh for item {}", item->getItem()->itemLocation().toString());
            return;
        }

        s_headItemMeshCache[itemId] = std::move(result.value());
        it = s_headItemMeshCache.find(itemId);
    }

    if (it == s_headItemMeshCache.end() || it->second.indexCount == 0) {
        return;
    }

    // 获取实体的世界位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 绘制头部物品网格
    // 使用实体的 hurtTime 和 deathTime 来传递受伤效果
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    } else if constexpr (std::is_same_v<std::remove_cv_t<TEntity>, ::mc::client::ClientEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    }

    pipeline.drawMesh(
        cmd, it->second, headTransform, entityPos, 1.0, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);
}

template <typename TEntity, typename TModel>
const ItemStack* HeadLayer<TEntity, TModel>::getHeadItem(const TEntity& entity) const
{
    // 从实体获取头部装备
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        return &entity.getEquipment(::mc::EquipmentSlot::Head);
    } else if constexpr (std::is_same_v<std::remove_cv_t<TEntity>, ::mc::client::ClientEntity>) {
        // ClientEntity 用具名访问器
        return entity.getHeadArmor();
    }
    return nullptr;
}

template <typename TEntity, typename TModel>
void HeadLayer<TEntity, TModel>::computeHeadTransform(f32 headYaw, f32 headPitch, std::array<f64, 16>& outMatrix)
{
    // 初始化为单位矩阵
    outMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 头部物品跟随头部旋转
    f64 yawRad = static_cast<f64>(headYaw) * mc::math::PI_DOUBLE / 180.0;
    f64 pitchRad = static_cast<f64>(headPitch) * mc::math::PI_DOUBLE / 180.0;

    f64 cosYaw = std::cos(yawRad);
    f64 sinYaw = std::sin(yawRad);
    f64 cosPitch = std::cos(pitchRad);
    f64 sinPitch = std::sin(pitchRad);

    // 先绕 Y 轴旋转（偏航），然后绕 X 轴旋转（俯仰）
    outMatrix[0] = cosYaw;
    outMatrix[2] = -sinYaw;
    outMatrix[5] = cosPitch;
    outMatrix[6] = sinPitch * sinYaw;
    outMatrix[8] = sinYaw;
    outMatrix[9] = -sinPitch;
    outMatrix[10] = cosPitch * cosYaw;

    // 头部位置偏移（相对于实体原点）
    // 玩家模型的头部在 y=1.5 左右
    outMatrix[3] = 0.0;
    outMatrix[7] = 1.5; // 头部高度
    outMatrix[11] = 0.0;

    // 头部物品缩放
    const f64 headScale = 1.0;
    outMatrix[0] *= headScale;
    outMatrix[5] *= headScale;
    outMatrix[10] *= headScale;
}

// 显式实例化常用类型
template class HeadLayer<::mc::LivingEntity, ::mc::client::renderer::entity::model::BipedModel>;
template class HeadLayer<::mc::Player, ::mc::client::renderer::entity::model::player::PlayerModel>;
template class HeadLayer<::mc::client::ClientEntity, ::mc::client::renderer::entity::model::player::PlayerModel>;

} // namespace mc::client::renderer::entity::layer::equipment
