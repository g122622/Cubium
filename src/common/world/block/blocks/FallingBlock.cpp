#include "FallingBlock.hpp"
#include "../BlockRegistry.hpp"
#include "../../IWorld.hpp"
#include "../../../entity/entities/misc/MiscEntities.hpp"

namespace mc {
namespace blocks {

FallingBlock::FallingBlock(const BlockProperties& properties)
    : Block(properties) {
}

void FallingBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    MC_UNUSED(state);
    world.scheduleBlockTick(pos, *this, FALL_DELAY_TICKS, world::tick::TickPriority::Normal);
}

void FallingBlock::neighborChanged(IWorld& world, const BlockPos& pos,
                                   Block& neighborBlock, const BlockPos& neighborPos,
                                   bool isMoving) {
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);
    world.scheduleBlockTick(pos, *this, FALL_DELAY_TICKS, world::tick::TickPriority::Normal);
}

void FallingBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    MC_UNUSED(state);

    if (pos.y <= 0) {
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

    const EntityId entityId = world.spawnEntity(std::move(fallingEntity));
    if (entityId == 0) {
        world.setBlockState(pos, currentState, 3);
    }
}

bool FallingBlock::canFallThrough(const BlockState* state) const {
    if (state == nullptr || state->isAir()) {
        return true;
    }

    if (state->getMaterial().isLiquid()) {
        return true;
    }

    return !state->blocksMovement();
}

} // namespace blocks
} // namespace mc
