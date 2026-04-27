#include "BeaconRenderer.hpp"
#include "common/world/blockentity/processing/BeaconEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::trident::blockentity {

BeaconRenderer::BeaconRenderer()
    : BlockEntityRenderer<mc::blockentity::BeaconEntity>()
    , m_helper() {
}

void BeaconRenderer::render(
    const mc::blockentity::BeaconEntity& entity,
    f32 partialTick,
    u32 light)
{
    const BlockPos& pos = entity.getPos();

    // 渲染信标基座
    renderBeaconBase(pos, light);

    // 如果激活，渲染光束
    if (entity.isActive()) {
        // TODO: 需要从世界获取 gameTime
        // i64 gameTime = world.getGameTime();
        i64 gameTime = 0;
        renderBeam(pos, entity.getLevel(), gameTime, partialTick, light);
    }
}

void BeaconRenderer::renderBeaconBase(const BlockPos& pos, u32 light) {
    // TODO: 实现信标基座渲染
    // 信标基座使用普通的方块模型
    (void)pos;
    (void)light;
}

void BeaconRenderer::renderBeam(
    const BlockPos& pos,
    i32 level,
    i64 gameTime,
    f32 partialTick,
    u32 light)
{
    if (level <= 0) {
        return;
    }

    // 计算光束旋转角度
    const f32 rotation = calculateBeamRotation(gameTime, partialTick);

    // 光束高度取决于金字塔等级
    // MC中光束高度 = 等级 * 10 + 10，最高50格
    const f32 beamHeight = static_cast<f32>(level * 10 + 10);

    // TODO: 实现光束渲染
    // 光束使用半透明的发光纹理
    // 需要：
    // 1. 创建光束顶点数据（锥形或圆柱形）
    // 2. 应用旋转变换
    // 3. 设置半透明混合
    // 4. 渲染到正确的深度层

    (void)pos;
    (void)beamHeight;
    (void)rotation;
    (void)light;
}

f32 BeaconRenderer::calculateBeamRotation(i64 gameTime, f32 partialTick) const
{
    // MC 1.16.5 BeaconTileEntityRenderer:
    // float f = (float)Math.floorMod(totalWorldTime, 40L) + partialTicks;
    // matrixStackIn.rotate(Vector3f.YP.rotationDegrees(f * 2.25F - 45.0F));
    //
    // 光束每秒旋转 45 度（每 tick 2.25 度），周期 40 ticks
    // 角度范围：-45 度 到 +45 度

    const f32 f = static_cast<f32>(math::floorMod(gameTime, 40LL)) + partialTick;
    const f32 degrees = f * 2.25f - 45.0f;
    return math::toRadians(degrees);
}

} // namespace mc::client::renderer::trident::blockentity
