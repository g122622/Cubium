#include "EnergyGlintLayer.hpp"

namespace mc::client::renderer::entity::layer::effect {

template<typename TEntity>
void EnergyGlintLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // TODO: 渲染附魔光效
    // 参考 MC 1.16.5 EnergyLayer.render()
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
bool EnergyGlintLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // TODO: 检查是否有附魔物品
    (void)entity;
    return false;
}

template<typename TEntity>
f32 EnergyGlintLayer<TEntity>::calculateGlintOffset(f32 ageInTicks) const {
    // 光效滚动速度
    return ageInTicks * 0.01f;
}

// 显式实例化
template class EnergyGlintLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::effect
