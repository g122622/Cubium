#include "MagmaBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include <spdlog/spdlog.h>

namespace mc::blocks {

MagmaBlock::MagmaBlock(BlockProperties properties)
    : Block(std::move(properties)) {
}

void MagmaBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 参考: MC 1.16.5 MagmaBlock.onBlockAdded()
    // 调度 tick 以检查气泡柱
    Block& block = const_cast<Block&>(state.getBlock());
    world.tickManager().scheduleBlockTick(pos, block, 20);
}

void MagmaBlock::neighborChanged(
    IWorld& world,
    const BlockPos& pos,
    Block& neighborBlock,
    const BlockPos& neighborPos,
    bool isMoving) {

    MC_UNUSED(neighborBlock);
    MC_UNUSED(isMoving);

    // 参考: MC 1.16.5 MagmaBlock.updatePostPlacement()
    // 当上方有水时调度 tick
    if (neighborPos.x == pos.x && neighborPos.y == pos.y + 1 && neighborPos.z == pos.z) {
        const BlockState* aboveState = world.getBlockState(neighborPos);
        if (aboveState != nullptr) {
            const fluid::FluidState* fluidState = aboveState->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty() &&
                fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                // 需要调度 tick，使用 MagmaBlock 自身
                world.tickManager().scheduleBlockTick(pos, *const_cast<MagmaBlock*>(this), 20);
            }
        }
    }
}

void MagmaBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 参考: MC 1.16.5 MagmaBlock.tick()
    // 在上方生成气泡柱
    MC_UNUSED(state);  // 暂时未使用
    MC_UNUSED(random);

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty() &&
            fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
            // TODO: 当添加气泡柱方块后，这里应该生成气泡柱
            // BubbleColumnBlock.placeBubbleColumn(world, abovePos, true);
            spdlog::debug("MagmaBlock at ({}, {}, {}): would create bubble column", pos.x, pos.y, pos.z);
        }
    }
}

void MagmaBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random) {

    // 参考: MC 1.16.5 MagmaBlock.randomTick()
    // 在水中产生气泡效果
    MC_UNUSED(state);  // 暂时未使用

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty() &&
            fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
            // 产生气泡粒子
            // world.addParticle(ParticleTypes::LARGE_SMOKE, ...)
            // world.playSound(SoundEvents::BLOCK_FIRE_EXTINGUISH, ...)
            MC_UNUSED(random);  // 目前不需要随机数，后续粒子位置需要
            spdlog::debug("MagmaBlock at ({}, {}, {}): bubble effect in water", pos.x, pos.y, pos.z);
        }
    }
}

} // namespace mc::blocks
