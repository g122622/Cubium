#include "NyliumBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../core/Constants.hpp"
#include <spdlog/spdlog.h>

namespace mc::blocks {

// ============================================================================
// NyliumBlock 实现
// ============================================================================

NyliumBlock::NyliumBlock(BlockProperties properties)
    : Block(std::move(properties)) {
}

void NyliumBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random) {

    (void)random;  // 退化不需要随机数

    // 参考: MC 1.16.5 NyliumBlock.randomTick()
    // 如果位置不够暗，退化为下界岩

    if (!isDarkEnough(world, pos, state)) {
        const BlockState* netherrackState = &VanillaBlocks::NETHERRACK->defaultState();
        if (netherrackState != nullptr) {
            world.setBlock(pos, netherrackState);
        }
    }
}

bool NyliumBlock::isDarkEnough(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) {

    (void)state;  // 暂时未使用

    // 参考: MC 1.16.5 NyliumBlock.isDarkEnough()
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    // 计算光照
    u8 skyLight = world.getSkyLight(abovePos);
    u8 blockLight = world.getBlockLight(abovePos);

    // 考虑上方方块的遮挡
    u8 opacity = 0;
    if (aboveState != nullptr) {
        opacity = static_cast<u8>(aboveState->getOpacity());
    }

    // 简化计算：如果遮挡后光照仍然满亮度，则不够暗
    // 完整实现需要 LightEngine.func_215613_a
    u8 effectiveLight = 0;
    if (skyLight > opacity) {
        effectiveLight = skyLight - opacity;
    }
    effectiveLight = std::max(effectiveLight, blockLight);

    // 光照 < 15 时足够暗
    return effectiveLight < 15;
}

// ============================================================================
// MagmaBlock 实现
// ============================================================================

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

    (void)neighborBlock;
    (void)isMoving;

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

void MagmaBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 参考: MC 1.16.5 MagmaBlock.tick()
    // 在上方生成气泡柱
    (void)state;  // 暂时未使用

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
    (void)state;  // 暂时未使用

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty() &&
            fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
            // 产生气泡粒子
            // world.addParticle(ParticleTypes::LARGE_SMOKE, ...)
            // world.playSound(SoundEvents::BLOCK_FIRE_EXTINGUISH, ...)
            (void)random;  // 目前不需要随机数，后续粒子位置需要
            spdlog::debug("MagmaBlock at ({}, {}, {}): bubble effect in water", pos.x, pos.y, pos.z);
        }
    }
}

} // namespace mc::blocks
