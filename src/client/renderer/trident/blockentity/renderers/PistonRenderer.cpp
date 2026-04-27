#include "PistonRenderer.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"
#include "common/world/block/BlockState.hpp"
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
    // 获取插值后的进度
    const f32 progress = entity.getProgress(partialTick);

    // 如果动画完成，不渲染
    if (entity.isComplete()) {
        return;
    }

    // 获取被移动的方块状态
    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr) {
        return;
    }

    // 获取偏移量
    const f32 offsetX = entity.getOffsetX(partialTick);
    const f32 offsetY = entity.getOffsetY(partialTick);
    const f32 offsetZ = entity.getOffsetZ(partialTick);

    // 渲染被移动的方块
    renderMovingBlock(entity, progress, light);

    // 如果需要渲染活塞头
    if (entity.shouldRenderPistonHead()) {
        renderPistonHead(entity, progress, light);
    }
}

void PistonRenderer::renderPistonHead(
    const mc::blockentity::PistonBlockEntity& entity,
    f32 progress,
    u32 light)
{
    // TODO: 实现活塞臂渲染
    // 需要渲染活塞臂模型（Piston Head）
    // 活塞臂的位置根据进度和方向计算

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
    f32 progress,
    u32 light)
{
    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr) {
        return;
    }

    const BlockPos& pos = entity.getPos();

    // 计算移动偏移
    const f32 offsetX = entity.getOffsetX(progress);
    const f32 offsetY = entity.getOffsetY(progress);
    const f32 offsetZ = entity.getOffsetZ(progress);

    // 渲染被移动的方块
    if (!m_helper.renderBlockWithOffset(*pistonState, pos, offsetX, offsetY, offsetZ, light)) {
        spdlog::trace("PistonRenderer: Failed to render moving block at ({}, {}, {})",
                      pos.x, pos.y, pos.z);
    }
}

} // namespace mc::client::renderer::trident::blockentity
