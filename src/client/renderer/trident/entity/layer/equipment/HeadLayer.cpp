#include "HeadLayer.hpp"

namespace mc::client::renderer::entity::layer::equipment {

template<typename TEntity>
void HeadLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // TODO: 获取头部装备槽的物品并渲染
    // 参考 MC 1.16.5 HeadLayer.render()
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
bool HeadLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // TODO: 检查头部装备槽是否有物品
    (void)entity;
    return false;
}

template<typename TEntity>
void HeadLayer<TEntity>::renderHeadItem(
    TEntity& entity,
    const ItemStack& itemStack,
    f32 headYaw,
    f32 headPitch,
    f32 scale)
{
    (void)entity;
    (void)itemStack;
    (void)headYaw;
    (void)headPitch;
    (void)scale;
}

// 显式实例化常用类型
template class HeadLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::equipment
