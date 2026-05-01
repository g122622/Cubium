#include "FallingBlock.hpp"
#include "../BlockRegistry.hpp"
#include "../../IWorld.hpp"
#include "../../tick/manager/TickManager.hpp"
#include "../../../entity/entities/misc/MiscEntities.hpp"
#include "../../../core/Constants.hpp"
#include "../Material.hpp"

namespace mc {
namespace blocks {

FallingBlock::FallingBlock(const BlockProperties& properties)
    : Block(properties) {
}

void FallingBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);
    world.tickManager().scheduleBlockTick(pos, *this, getFallDelay(), world::tick::TickPriority::Normal);
}

void FallingBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                   Block& neighborBlock, const BlockPos& neighborPos,
                                   bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);
    world.tickManager().scheduleBlockTick(pos, *this, getFallDelay(), world::tick::TickPriority::Normal);
}

BlockState FallingBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {
    // 参考: net.minecraft.block.FallingBlock#updatePostPlacement
    // 当邻居更新时也调度 tick
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    world.tickManager().scheduleBlockTick(currentPos, *this, getFallDelay(), world::tick::TickPriority::Normal);
    return state;
}

void FallingBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    MC_UNUSED(state);

    if (pos.y <= world::MIN_BUILD_HEIGHT) {
        return;
    }

    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || currentState->isAir() || !currentState->is(this)) {
        return;
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (!canFallThrough(belowState)) {
        return;
    }

    const BlockState* airState = BlockRegistry::instance().airState();
    if (airState == nullptr) {
        return;
    }

    if (!world.setBlockState(pos, airState, 3)) {
        return;
    }

    auto fallingEntity = std::make_unique<entity::FallingBlockEntity>();
    fallingEntity->setPosition(
        static_cast<f32>(pos.x) + 0.5f,
        static_cast<f32>(pos.y),
        static_cast<f32>(pos.z) + 0.5f);
    fallingEntity->setVelocity(0.0f, 0.0f, 0.0f);
    fallingEntity->setBlockId(currentState->blockId());
    fallingEntity->setFallStartPos(static_cast<f64>(pos.y));

    // 调用开始下落回调
    onStartFalling(world, pos, *fallingEntity);

    const EntityId entityId = world.spawnEntity(std::move(fallingEntity));
    if (entityId == 0) {
        world.setBlockState(pos, currentState, 3);
    }
}

// 参考: net.minecraft.block.FallingBlock#canFallThrough
bool FallingBlock::canFallThrough(const BlockState* state) {
    // 空气可穿透
    if (state == nullptr || state->isAir()) {
        return true;
    }

    // 液体可穿透
    if (state->getMaterial().isLiquid()) {
        return true;
    }

    // 可替换材质可穿透（如火把、草等）
    if (state->getMaterial().isReplaceable()) {
        return true;
    }

    // TODO: 添加火焰标签检查
    // if (state->is(BlockTags::FIRE)) { return true; }

    // 不阻挡移动的方块可穿透
    return !state->blocksMovement();
}

} // namespace blocks
} // namespace mc
