#include "PistonRenderer.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"

namespace mc::client::renderer::trident::blockentity {

PistonRenderer::PistonRenderer()
    : BlockEntityRenderer<mc::blockentity::PistonBlockEntity>()
    , m_helper()
{}

void PistonRenderer::render(const mc::blockentity::PistonBlockEntity& entity, f32 partialTick, u32 light)
{
    // MC 1.16.5 PistonTileEntityRenderer.render():
    // 如果方块状态为空气，不渲染
    // 如果动画完成，方块已经被替换，不需要渲染

    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr || pistonState->isAir()) {
        return;
    }

    // MC 1.16.5: 计算偏移
    // matrixStackIn.translate(
    //     (double)tileEntityIn.getOffsetX(partialTicks),
    //     (double)tileEntityIn.getOffsetY(partialTicks),
    //     (double)tileEntityIn.getOffsetZ(partialTicks));
    const f32 offsetX = entity.getOffsetX(partialTick);
    const f32 offsetY = entity.getOffsetY(partialTick);
    const f32 offsetZ = entity.getOffsetZ(partialTick);
    const f32 progress = entity.getProgress(partialTick);

    // MC 1.16.5: 获取运动方向的反向位置（用于活塞基座渲染）
    const Direction motionDir = entity.getMotionDirection();
    const Direction oppositeDir = Directions::opposite(motionDir);
    const BlockPos basePos = entity.getPos().offset(oppositeDir);

    // MC 1.16.5: 有三种渲染情况
    // 1. 方块是活塞头 (PistonHeadBlock)：设置 SHORT 属性并渲染
    // 2. 需要渲染活塞头且正在收回：渲染活塞头 + 活塞基座
    // 3. 普通情况：渲染被移动的方块

    // 检查是否是活塞头方块
    // MC 1.16.5: blockstate.isIn(Blocks.PISTON_HEAD)
    // 注意：当前方块注册表不完整，无法判断是否为活塞头
    // 完整实现需要检查 pistonState->getBlock() == Blocks::PISTON_HEAD
    const bool isPistonHead = false;

    if (isPistonHead) {
        // MC 1.16.5: 活塞头需要设置 SHORT 属性
        // blockstate = blockstate.with(PistonHeadBlock.SHORT, progress <= 0.5F);
        const bool isShort = progress <= 0.5f;
        (void)isShort;

        // 渲染活塞头
        renderMovingBlock(entity, offsetX, offsetY, offsetZ, light);
    } else if (entity.shouldRenderPistonHead() && !entity.isExtending()) {
        // MC 1.16.5: 收回时需要渲染活塞头
        // blockstate1 = Blocks.PISTON_HEAD.getDefaultState()
        //     .with(PistonHeadBlock.TYPE, isSticky ? STICKY : DEFAULT)
        //     .with(PistonHeadBlock.FACING, facing)
        //     .with(PistonHeadBlock.SHORT, progress >= 0.5F);

        const bool isShort = progress >= 0.5f;
        (void)isShort;

        // 渲染活塞头
        renderPistonHead(entity, progress, light);

        // MC 1.16.5: 然后渲染活塞基座（延伸状态）
        // matrixStackIn.pop();
        // matrixStackIn.push();
        // blockstate = blockstate.with(PistonBlock.EXTENDED, true);

        const BlockPos pistonBasePos = basePos.offset(motionDir);
        (void)pistonBasePos;
        // 完整实现需要渲染延伸状态的活塞基座
    } else {
        // MC 1.16.5: 普通情况，直接渲染被移动的方块
        renderMovingBlock(entity, offsetX, offsetY, offsetZ, light);
    }
}

void PistonRenderer::renderPistonHead(const mc::blockentity::PistonBlockEntity& entity, f32 progress, u32 light)
{
    // MC 1.16.5 PistonTileEntityRenderer:
    // 渲染活塞头（收回时需要单独渲染）
    //
    // 活塞头方块状态：
    // - TYPE: STICKY 或 DEFAULT（取决于活塞类型）
    // - FACING: 活塞朝向
    // - SHORT: progress >= 0.5F 时为 true

    const Direction facing = entity.getFacing();
    const BlockPos& pos = entity.getPos();

    // 计算活塞头位置偏移
    // MC 1.16.5: 使用 getExtendedProgress 计算延伸进度
    const f32 extendedProgress = entity.getExtendedProgress(progress);
    const f32 headOffsetX = static_cast<f32>(Directions::xOffset(facing)) * extendedProgress;
    const f32 headOffsetY = static_cast<f32>(Directions::yOffset(facing)) * extendedProgress;
    const f32 headOffsetZ = static_cast<f32>(Directions::zOffset(facing)) * extendedProgress;

    // 完整实现需要：
    // 1. 获取活塞头方块状态（根据是否粘性选择 STICKY 或 DEFAULT）
    // 2. 设置 FACING 和 SHORT 属性
    // 3. 使用 BlockModelCache 获取模型并渲染

    (void)pos;
    (void)headOffsetX;
    (void)headOffsetY;
    (void)headOffsetZ;
    (void)light;
}

void PistonRenderer::renderMovingBlock(
    const mc::blockentity::PistonBlockEntity& entity, f32 offsetX, f32 offsetY, f32 offsetZ, u32 light)
{
    const BlockState* pistonState = entity.getPistonState();
    if (pistonState == nullptr) {
        return;
    }

    const BlockPos& pos = entity.getPos();

    // MC 1.16.5: 渲染被移动的方块，应用偏移
    // 需要使用 BlockRendererDispatcher 渲染方块模型
    // 当前使用 helper 的占位实现
    if (!m_helper.renderBlockWithOffset(*pistonState, pos, offsetX, offsetY, offsetZ, light)) {
        // 占位实现，暂不处理
        (void)pos;
    }
}

} // namespace mc::client::renderer::trident::blockentity
