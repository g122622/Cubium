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

#include "HeldItemLayer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/item/ItemMeshBuilder.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::layer::equipment {

namespace {

// 手持物品网格缓存
std::unordered_map<u32, pipeline::EntityMesh> s_heldItemMeshCache;

} // anonymous namespace

template <typename TEntity, typename TModel>
void HeldItemLayer<TEntity, TModel>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 根据主手判断哪只手渲染哪个物品
    // 右手为主手时：右手渲染主手物品，左手渲染副手物品
    // 左手为主手时：右手渲染副手物品，左手渲染主手物品

    bool isRightHanded = true; // 默认右手为主手
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        isRightHanded = entity.isRightHanded();
    } else if constexpr (std::is_same_v<std::remove_cv_t<TEntity>, ::mc::client::ClientEntity>) {
        isRightHanded = entity.isRightHanded();
    }

    mc::Hand rightHandSlot = isRightHanded ? mc::Hand::MainHand : mc::Hand::OffHand;
    mc::Hand leftHandSlot = isRightHanded ? mc::Hand::OffHand : mc::Hand::MainHand;

    // 渲染右手物品（使用 ThirdPersonRightHand 变换）
    renderHandItemPipeline(entity, rightHandSlot, mc::HandSide::Right, cmd, context, pipeline);

    // 渲染左手物品（使用 ThirdPersonLeftHand 变换）
    renderHandItemPipeline(entity, leftHandSlot, mc::HandSide::Left, cmd, context, pipeline);
}

template <typename TEntity, typename TModel>
bool HeldItemLayer<TEntity, TModel>::shouldRender(const TEntity& entity) const
{
    // 检查是否有手持物品
    const ItemStack* mainHand = getHeldItem(entity, mc::Hand::MainHand);
    const ItemStack* offHand = getHeldItem(entity, mc::Hand::OffHand);

    if (mainHand && !mainHand->isEmpty()) return true;
    if (offHand && !offHand->isEmpty()) return true;

    return false;
}

template <typename TEntity, typename TModel>
void HeldItemLayer<TEntity, TModel>::renderHandItemPipeline(TEntity& entity,
    mc::Hand hand,
    mc::HandSide handSide,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    const ItemStack* item = getHeldItem(entity, hand);
    if (!item || item->isEmpty()) {
        return;
    }

    // 获取父模型
    TModel* model = getParentModel();

    // 计算手持物品的变换矩阵
    std::array<f64, 16> itemTransform;
    computeItemTransform(model, handSide, itemTransform);

    // 获取物品ID用于缓存
    u32 itemId = static_cast<u32>(item->getItem()->itemId());

    // 获取或创建物品网格
    auto it = s_heldItemMeshCache.find(itemId);
    if (it == s_heldItemMeshCache.end()) {
        // 构建物品网格
        auto transformType = (handSide == mc::HandSide::Right) ? item::ItemTransformType::ThirdPersonRightHand
                                                               : item::ItemTransformType::ThirdPersonLeftHand;

        auto [vertices, indices] = item::ItemMeshBuilder::buildHeldItemMesh(*item, transformType, true);

        if (vertices.empty() || indices.empty()) {
            return;
        }

        // 创建 GPU 网格
        auto result = pipeline.createMesh(vertices, indices);
        if (!result.success()) {
            spdlog::warn(
                "HeldItemLayer: Failed to create mesh for item {}", item->getItem()->itemLocation().toString());
            return;
        }

        s_heldItemMeshCache[itemId] = std::move(result.value());
        it = s_heldItemMeshCache.find(itemId);
    }

    if (it == s_heldItemMeshCache.end() || it->second.indexCount == 0) {
        return;
    }

    // 获取实体的世界位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

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
        cmd, it->second, itemTransform, entityPos, 1.0, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);
}

template <typename TEntity, typename TModel>
const ItemStack* HeldItemLayer<TEntity, TModel>::getHeldItem(const TEntity& entity, mc::Hand hand) const
{
    // 从实体获取手持物品
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        // LivingEntity 通过 EquipmentSlot::getEquipment 统一槽位访问
        using EquipmentSlot = ::mc::EquipmentSlot;
        auto slot = (hand == mc::Hand::MainHand) ? EquipmentSlot::MainHand : EquipmentSlot::OffHand;
        return &entity.getEquipment(slot);
    } else if constexpr (std::is_same_v<std::remove_cv_t<TEntity>, ::mc::client::ClientEntity>) {
        // ClientEntity 用具名访问器（避免引入 LivingEntity.hpp 重依赖）
        return (hand == mc::Hand::MainHand) ? entity.getMainHandItem() : entity.getOffHandItem();
    }
    return nullptr;
}

template <typename TEntity, typename TModel>
void HeldItemLayer<TEntity, TModel>::computeItemTransformStatic(
    const TModel* model, mc::HandSide handSide, std::array<f64, 16>& outMatrix)
{
    // 初始化为单位矩阵
    outMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 物品变换步骤：
    // 1. 首先调用 model.translateHand(side, matrixStack)
    //    这会将矩阵变换到手臂的局部坐标系（包含手臂的旋转点和旋转角度）
    // 2. 然后应用固定的物品变换：
    //    X 轴 -90° 旋转 + Y 轴 180° 旋转 + 手部偏移

    // 步骤 1：从模型获取手臂变换矩阵
    std::array<f64, 16> armMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    if (model) {
        // 调用模型的 translateHand 方法获取手臂变换
        // 这会返回手臂的旋转点和旋转角度的组合变换
        // 转换 HandSide：mc::HandSide -> model::HandSide
        model::HandSide modelHandSide =
            (handSide == mc::HandSide::Left) ? model::HandSide::Left : model::HandSide::Right;
        model->translateHand(modelHandSide, armMatrix);
    }

    // 步骤 2：应用物品固定变换（相对于手臂坐标系）
    // X 轴 -90° 旋转 + Y 轴 180° 旋转的组合
    // cos(-90°) = 0, sin(-90°) = -1
    // cos(180°) = -1, sin(180°) = 0

    // 组合旋转矩阵 R = Ry(180°) * Rx(-90°)
    // 简化后：
    // Rx(-90°): [1, 0, 0, 0; 0, 0, 1, 0; 0, -1, 0, 0; 0, 0, 0, 1]
    // Ry(180°): [-1, 0, 0, 0; 0, 1, 0, 0; 0, 0, -1, 0; 0, 0, 0, 1]
    // R = Ry * Rx:
    // [-1, 0, 0, 0]   [1, 0, 0, 0]   [-1, 0, 0, 0]
    // [ 0, 1, 0, 0] * [0, 0, 1, 0] = [ 0, 0, 1, 0]
    // [ 0, 0,-1, 0]   [0,-1, 0, 0]   [ 0, 1, 0, 0]
    // [ 0, 0, 0, 1]   [0, 0, 0, 1]   [ 0, 0, 0, 1]

    std::array<f64, 16> itemRotation = {
        -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 步骤 3：应用手部偏移
    // 右手时 x = +1/16, 左手时 x = -1/16
    bool isLeftHand = (handSide == mc::HandSide::Left);
    f64 xOffset = isLeftHand ? -1.0 / 16.0 : 1.0 / 16.0;
    f64 yOffset = 0.125;
    f64 zOffset = -0.625;

    // 构建偏移矩阵
    std::array<f64, 16> translation = {
        1.0, 0.0, 0.0, xOffset, 0.0, 1.0, 0.0, yOffset, 0.0, 0.0, 1.0, zOffset, 0.0, 0.0, 0.0, 1.0};

    // 组合所有变换：outMatrix = armMatrix * itemRotation * translation
    // 矩阵乘法顺序：从右到左应用

    // 首先计算 temp = itemRotation * translation
    std::array<f64, 16> temp = {0};
    for (i32 row = 0; row < 4; ++row) {
        for (i32 col = 0; col < 4; ++col) {
            f64 sum = 0.0;
            for (i32 k = 0; k < 4; ++k) {
                sum += itemRotation[row * 4 + k] * translation[k * 4 + col];
            }
            temp[row * 4 + col] = sum;
        }
    }

    // 然后计算 outMatrix = armMatrix * temp
    for (i32 row = 0; row < 4; ++row) {
        for (i32 col = 0; col < 4; ++col) {
            f64 sum = 0.0;
            for (i32 k = 0; k < 4; ++k) {
                sum += armMatrix[row * 4 + k] * temp[k * 4 + col];
            }
            outMatrix[row * 4 + col] = sum;
        }
    }

    // 手持物品变换已完成：
    // - 添加了模型引用和 translateHand 调用，物品跟随手臂动画
    // - BipedModel::translateHand 方法获取手臂的实际旋转角度
    // - 物品变换正确包含手臂的旋转点和旋转角度
}

template <typename TEntity, typename TModel>
void HeldItemLayer<TEntity, TModel>::computeItemTransform(
    const TModel* model, mc::HandSide handSide, std::array<f64, 16>& outMatrix)
{
    // 委托给静态方法
    computeItemTransformStatic(model, handSide, outMatrix);
}

// 显式实例化常用类型
template class HeldItemLayer<::mc::LivingEntity, model::BipedModel>;
template class HeldItemLayer<::mc::Player, model::BipedModel>;
template class HeldItemLayer<::mc::Player, model::player::PlayerModel>;
template class HeldItemLayer<::mc::client::ClientEntity, model::player::PlayerModel>;

} // namespace mc::client::renderer::entity::layer::equipment
