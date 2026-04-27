#include "ChestRenderer.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::trident::blockentity {

ChestRenderer::ChestRenderer()
    : BlockEntityRenderer<mc::blockentity::ChestEntity>()
    , m_helper() {
}

void ChestRenderer::render(
    const mc::blockentity::ChestEntity& entity,
    f32 partialTick,
    u32 light)
{
    const BlockPos& pos = entity.getPos();

    // 获取插值后的盖子角度
    f32 lidAngle = getInterpolatedLidAngle(entity, partialTick);

    // 应用非线性缓动
    lidAngle = applyLidEasing(lidAngle);

    // 渲染箱体
    renderChestBody(pos, light);

    // 渲染盖子
    renderChestLid(pos, lidAngle, light);

    // 渲染锁扣
    renderChestLatch(pos, lidAngle, light);
}

f32 ChestRenderer::getInterpolatedLidAngle(
    const mc::blockentity::ChestEntity& entity,
    f32 partialTick) const
{
    const f32 prevAngle = entity.getPrevLidAngle();
    const f32 angle = entity.getLidAngle();
    return math::lerp(prevAngle, angle, partialTick);
}

f32 ChestRenderer::applyLidEasing(f32 angle) const
{
    // MC风格的三次缓动
    // angle = 1.0f - angle;
    // angle = 1.0f - angle * angle * angle;
    f32 eased = 1.0f - angle;
    eased = 1.0f - eased * eased * eased;
    return eased;
}

void ChestRenderer::renderChestBody(const BlockPos& pos, u32 light) {
    // TODO: 实现箱体渲染
    // 需要从BlockModelCache获取箱子模型
    (void)pos;
    (void)light;
}

void ChestRenderer::renderChestLid(const BlockPos& pos, f32 lidAngle, u32 light) {
    // TODO: 实现盖子渲染
    // 需要应用旋转变换（绕铰链轴旋转）
    (void)pos;
    (void)lidAngle;
    (void)light;
}

void ChestRenderer::renderChestLatch(const BlockPos& pos, f32 lidAngle, u32 light) {
    // TODO: 实现锁扣渲染
    // 锁扣跟随盖子旋转
    (void)pos;
    (void)lidAngle;
    (void)light;
}

} // namespace mc::client::renderer::trident::blockentity
