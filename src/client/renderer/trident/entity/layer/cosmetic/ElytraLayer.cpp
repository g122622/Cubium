#include "ElytraLayer.hpp"
#include "common/entity/core/LivingEntity.hpp"

namespace mc::client::renderer::entity::layer::cosmetic {

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
    // TODO: 渲染鞘翅
    // 参考 MC 1.16.5 ElytraLayer.render()
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
    // TODO: 检查是否装备了鞘翅
    (void)entity;
    return false;
}

template<typename TEntity>
f32 ElytraLayer<TEntity>::calculateElytraAngle(TEntity& entity, f32 partialTicks) const {
    (void)entity;
    (void)partialTicks;
    return 0.0f;
}

// 显式实例化
template class ElytraLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::cosmetic
