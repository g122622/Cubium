#include "SaddleLayer.hpp"

namespace mc::client::renderer::entity::layer::entity {

template<typename TEntity>
void SaddleLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // TODO: 渲染鞍
    // 参考 MC 1.16.5 SaddleLayer.render()
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
bool SaddleLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // TODO: 检查实体是否有鞍
    (void)entity;
    return false;
}

// 显式实例化
template class SaddleLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::entity
