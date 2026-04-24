#include "HeldItemLayer.hpp"
#include "common/entity/core/LivingEntity.hpp"

namespace mc::client::renderer::entity::layer::equipment {

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
    // 参考 MC 1.16.5 HeldItemLayer.render()
    // 渲染主手和副手物品
    renderHandItem(entity, HandSide::MainHand, limbSwing, limbSwingAmount, partialTicks, scale);
    renderHandItem(entity, HandSide::OffHand, limbSwing, limbSwingAmount, partialTicks, scale);

    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
}

template<typename TEntity>
bool HeldItemLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // 检查是否有手持物品
    const ItemStack* mainHand = getHeldItem(entity, HandSide::MainHand);
    const ItemStack* offHand = getHeldItem(entity, HandSide::OffHand);
    return mainHand != nullptr || offHand != nullptr;
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
    const ItemStack* item = getHeldItem(entity, hand);
    if (!item) {
        return;
    }

    // TODO: 实际渲染物品
    // 需要获取 ItemRenderer 并渲染物品模型
    applyItemTransform(hand, limbSwing, limbSwingAmount);

    (void)entity;
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
void HeldItemLayer<TEntity>::applyItemTransform(
    HandSide hand,
    f32 limbSwing,
    f32 limbSwingAmount)
{
    // 应用物品变换
    // 根据手臂姿态和物品类型调整变换矩阵
    (void)hand;
    (void)limbSwing;
    (void)limbSwingAmount;
}

// 显式实例化常用类型
template class HeldItemLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::equipment
