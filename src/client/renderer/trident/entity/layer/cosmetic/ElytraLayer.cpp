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
    // 参考 MC 1.16.5 ElytraLayer.render()
    // 纹理优先级：
    // 1. 自定义鞘翅纹理 (m_customElytraRegion)
    // 2. 披风纹理（如果玩家有披风）
    // 3. 默认鞘翅纹理

    if (!shouldRender(entity)) {
        return;
    }

    // TODO: 渲染鞘翅网格
    // 需要使用纹理区域
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
    // 检查是否装备了鞘翅
    // TODO: 检查玩家装备槽
    (void)entity;
    return m_customElytraRegion != nullptr || m_capeRegion != nullptr;
}

template<typename TEntity>
f32 ElytraLayer<TEntity>::calculateElytraAngle(TEntity& entity, f32 partialTicks) const {
    (void)entity;
    (void)partialTicks;
    return 0.0f;
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
