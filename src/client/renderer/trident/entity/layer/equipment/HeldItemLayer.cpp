#include "HeldItemLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/Item.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::equipment {

template<typename TEntity>
void HeldItemLayer<TEntity>::renderPipeline(
    TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 渲染主手和副手物品
    renderHandItemPipeline(entity, HandSide::MainHand, cmd, context, pipeline);
    renderHandItemPipeline(entity, HandSide::OffHand, cmd, context, pipeline);
}

template<typename TEntity>
void HeldItemLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // CPU 路径 - 已废弃，仅调用 GPU 路径的空实现
    // 这个方法保留用于向后兼容
    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
    // 注意：CPU 路径不再实现，需要通过 GPU 管线渲染
}

template<typename TEntity>
bool HeldItemLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // 检查是否有手持物品
    const ItemStack* mainHand = getHeldItem(entity, HandSide::MainHand);
    const ItemStack* offHand = getHeldItem(entity, HandSide::OffHand);

    if (mainHand && !mainHand->isEmpty()) return true;
    if (offHand && !offHand->isEmpty()) return true;

    return false;
}

template<typename TEntity>
void HeldItemLayer<TEntity>::renderHandItemPipeline(
    TEntity& entity,
    HandSide hand,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    const ItemStack* item = getHeldItem(entity, hand);
    if (!item || item->isEmpty()) {
        return;
    }

    // 计算手持物品的变换矩阵
    std::array<f64, 16> itemTransform;
    computeItemTransform(
        hand,
        static_cast<f32>(context.limbSwing),
        static_cast<f32>(context.limbSwingAmount),
        static_cast<f32>(context.swingProgress),
        itemTransform
    );

    // TODO: 实现 3D 物品渲染
    // 需要 ItemMeshBuilder 来生成物品网格
    // 当前作为占位符，仅记录日志
    spdlog::trace("HeldItemLayer: Rendering item '{}' in {} hand (GPU pipeline)",
                  item->getItem()->itemLocation().toString(),
                  hand == HandSide::MainHand ? "main" : "off");

    (void)cmd;
    (void)pipeline;
    (void)itemTransform;
}

template<typename TEntity>
void HeldItemLayer<TEntity>::renderHandItem(
    TEntity& entity,
    HandSide hand,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 scale)
{
    // CPU 路径 - 已废弃
    const ItemStack* item = getHeldItem(entity, hand);
    if (!item || item->isEmpty()) {
        return;
    }

    // 仅作为占位符
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)scale;
}

template<typename TEntity>
const ItemStack* HeldItemLayer<TEntity>::getHeldItem(
    const TEntity& entity,
    HandSide hand) const
{
    // 从实体获取手持物品
    // LivingEntity 有 getEquipment 方法
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        using EquipmentSlot = ::mc::EquipmentSlot;
        auto slot = (hand == HandSide::MainHand) ? EquipmentSlot::MainHand : EquipmentSlot::OffHand;
        return &entity.getEquipment(slot);
    }
    return nullptr;
}

template<typename TEntity>
void HeldItemLayer<TEntity>::computeItemTransform(
    HandSide hand,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 swingProgress,
    std::array<f64, 16>& outMatrix)
{
    // 初始化为单位矩阵
    outMatrix = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 参考 MC 1.16.5 HeldItemLayer.applyItemTransform
    // 手持物品变换需要根据手臂位置和物品类型调整

    // 基础位置偏移（相对于手臂）
    f64 xOffset = (hand == HandSide::MainHand) ? 0.5625 : -0.5625;
    f64 yOffset = -0.25;  // 略低于手臂中心
    f64 zOffset = -0.1875;

    // 步态动画影响
    f64 armSwing = std::sin(static_cast<f64>(limbSwing) * 0.5) * static_cast<f64>(limbSwingAmount) * 0.5;
    yOffset += armSwing * 0.2;

    // 挥动手臂动画
    if (swingProgress > 0.0f) {
        // 攻击动画
        f64 swingAngle = 1.0 - static_cast<f64>(swingProgress);
        swingAngle = 1.0 - swingAngle * swingAngle * swingAngle;  // 缓动
        outMatrix[0] = std::cos(swingAngle * 3.14159 * 0.5);
        outMatrix[2] = std::sin(swingAngle * 3.14159 * 0.5);
        outMatrix[8] = -std::sin(swingAngle * 3.14159 * 0.5);
        outMatrix[10] = std::cos(swingAngle * 3.14159 * 0.5);
    }

    // 应用位置偏移
    outMatrix[3] = xOffset;
    outMatrix[7] = yOffset;
    outMatrix[11] = zOffset;

    // 缩放因子（物品显示大小）
    const f64 itemScale = 0.4;
    outMatrix[0] *= itemScale;
    outMatrix[5] = -itemScale;  // Y 翻转
    outMatrix[10] *= itemScale;
}

// 显式实例化常用类型
template class HeldItemLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::equipment
