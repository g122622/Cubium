#include "HeldBlockLayer.hpp"
#include "common/world/block/Block.hpp"

namespace mc::client::renderer::entity::layer::entity {

template<typename TEntity>
HeldBlockLayer<TEntity>::HeldBlockLayer()
    : core::LayerRenderer<TEntity>()
{
}

template<typename TEntity>
void HeldBlockLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // 获取末影人持有的方块
    // TODO: 从实体获取持有的方块状态
    // const auto& carriedBlock = entity.getCarriedBlock();
    // if (!carriedBlock.isAir()) {
    //     renderBlock(carriedBlock, 0.0f, 0.6875f, 0.0f, scale);
    // }

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
bool HeldBlockLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // TODO: 检查实体是否持有方块
    // return !entity.getCarriedBlock().isAir();
    (void)entity;
    return false;
}

template<typename TEntity>
void HeldBlockLayer<TEntity>::renderBlock(
    const mc::BlockState& blockState,
    f32 x, f32 y, f32 z,
    f32 scale)
{
    // TODO: 渲染方块模型
    // 需要获取方块模型并渲染
    (void)blockState;
    (void)x;
    (void)y;
    (void)z;
    (void)scale;
}

// 显式实例化
template class HeldBlockLayer<mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::entity
