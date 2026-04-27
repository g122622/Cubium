#include "PistonRenderer.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"
#include "common/world/block/Block.hpp"
#include "common/util/Direction.hpp"
#include "client/resource/BlockModelCache.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::blockentity {

PistonRenderer::PistonRenderer()
    : BlockEntityRenderer<mc::blockentity::PistonBlockEntity>()
    , m_helper() {
}

void PistonRenderer::render(
    const mc::blockentity::PistonBlockEntity& entity,
    f32 partialTick,
    u32 light)
{
    // MC 1.16.5: 如果动画完成，不渲染移动中的活塞方块
    // 注意：完成后的方块状态已经由世界设置，不需要渲染器处理
    if (entity.isComplete()) {
        return;
    }

    // 获取被移动的方块状态
    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr) {
        return;
    }

    // MC 1.16.5: 直接使用 partialTick 计算偏移
    // getOffsetX/Y/Z 内部会调用 getProgress(partialTick)
    const f32 offsetX = entity.getOffsetX(partialTick);
    const f32 offsetY = entity.getOffsetY(partialTick);
    const f32 offsetZ = entity.getOffsetZ(partialTick);

    // 渲染被移动的方块
    renderMovingBlock(entity, offsetX, offsetY, offsetZ, light);

    // 如果需要渲染活塞头
    if (entity.shouldRenderPistonHead()) {
        const f32 progress = entity.getProgress(partialTick);
        renderPistonHead(entity, progress, light);
    }
}

void PistonRenderer::renderPistonHead(
    const mc::blockentity::PistonBlockEntity& entity,
    f32 progress,
    u32 light)
{
    // TODO: 实现活塞臂渲染
    // MC 1.16.5 PistonTileEntityRenderer:
    // 当收回时（!isExtending），需要渲染活塞头
    // 活塞头状态需要设置 SHORT 属性（progress <= 0.5F 时为 true）

    const Direction facing = entity.getFacing();
    const BlockPos& pos = entity.getPos();

    // 计算活塞臂位置
    const f32 extendedProgress = entity.getExtendedProgress(progress);
    const f32 headOffsetX = static_cast<f32>(Directions::xOffset(facing)) * extendedProgress;
    const f32 headOffsetY = static_cast<f32>(Directions::yOffset(facing)) * extendedProgress;
    const f32 headOffsetZ = static_cast<f32>(Directions::zOffset(facing)) * extendedProgress;

    // 渲染活塞臂
    // 需要获取活塞臂的模型（piston_head 或 sticky_piston_head）
    // 然后应用偏移和旋转

    // 暂时使用占位实现
    (void)pos;
    (void)headOffsetX;
    (void)headOffsetY;
    (void)headOffsetZ;
    (void)light;
}

void PistonRenderer::renderMovingBlock(
    const mc::blockentity::PistonBlockEntity& entity,
    f32 offsetX,
    f32 offsetY,
    f32 offsetZ,
    u32 light)
{
    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr) {
        return;
    }

    const BlockPos& pos = entity.getPos();

    // MC 1.16.5: 渲染被移动的方块，应用偏移
    if (!m_helper.renderBlockWithOffset(*pistonState, pos, offsetX, offsetY, offsetZ, light)) {
        spdlog::trace("PistonRenderer: Failed to render moving block at ({}, {}, {})",
                      pos.x, pos.y, pos.z);
    }
}

} // namespace mc::client::renderer::trident::blockentity
