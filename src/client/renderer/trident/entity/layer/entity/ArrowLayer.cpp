#include "ArrowLayer.hpp"

namespace mc::client::renderer::entity::layer::entity {

template<typename TEntity>
void ArrowLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // TODO: 渲染箭矢
    // 参考 MC 1.16.5 ArrowLayer.render()
    // 遍历实体身上附着的箭矢并渲染
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
bool ArrowLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // TODO: 检查实体是否有箭矢附着
    (void)entity;
    return false;
}

template<typename TEntity>
void ArrowLayer<TEntity>::renderArrow(f32 x, f32 y, f32 z, f32 yaw, f32 pitch, f32 scale) {
    (void)x;
    (void)y;
    (void)z;
    (void)yaw;
    (void)pitch;
    (void)scale;
}

// 显式实例化
template class ArrowLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::entity
