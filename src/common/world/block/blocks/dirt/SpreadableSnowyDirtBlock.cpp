#include "SpreadableSnowyDirtBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../core/Constants.hpp"

namespace mc::blocks {

// ============================================================================
// SpreadableSnowyDirtBlock 实现
// ============================================================================

SpreadableSnowyDirtBlock::SpreadableSnowyDirtBlock(BlockProperties properties)
    : Block(std::move(properties)) {
}

void SpreadableSnowyDirtBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random) {

    // 参考: MC 1.16.5 SpreadableSnowyDirtBlock.randomTick()

    // 检查是否满足蔓延条件
    if (!isSnowyConditions(world, pos, state)) {
        // 不满足条件，退化成泥土
        const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
        if (dirtState != nullptr) {
            world.setBlockState(pos, dirtState);
        }
    } else {
        // 满足条件，尝试向周围蔓延
        // 需要 pos.up() 的光照 >= 9
        u8 skyLight = world.getSkyLight(pos.x, pos.y + 1, pos.z);
        u8 blockLight = world.getBlockLight(pos.x, pos.y + 1, pos.z);
        u8 lightLevel = std::max(skyLight, blockLight);

        if (lightLevel >= 9) {
            const BlockState* defaultState = &getDefaultState();
            if (defaultState == nullptr) {
                return;
            }

            // 尝试向4个随机位置的泥土蔓延
            for (i32 i = 0; i < 4; ++i) {
                i32 dx = random.nextInt(3) - 1;  // -1, 0, 1
                i32 dy = random.nextInt(5) - 3;  // -3, -2, -1, 0, 1
                i32 dz = random.nextInt(3) - 1;  // -1, 0, 1

                BlockPos targetPos(pos.x + dx, pos.y + dy, pos.z + dz);

                // 检查目标位置是否为泥土
                const BlockState* targetState = world.getBlockState(targetPos);
                if (targetState == nullptr || targetState->blockId() != VanillaBlocks::DIRT->blockId()) {
                    continue;
                }

                // 检查目标位置是否满足蔓延条件
                if (isSnowyAndNotUnderwater(world, targetPos, *defaultState)) {
                    // 检查目标位置上方是否有雪
                    const BlockState* aboveState = world.getBlockState(
                        targetPos.x, targetPos.y + 1, targetPos.z);
                    bool hasSnow = aboveState != nullptr &&
                                   aboveState->blockId() == VanillaBlocks::SNOW->blockId();

                    // 设置 SNOWY 属性（如果存在）
                    const BlockState* newState = defaultState;
                    // TODO: 当添加 SNOWY 属性支持时更新这里
                    // newState = defaultState->with(SNOWY, hasSnow);

                    world.setBlockState(targetPos, newState);
                }
            }
        }
    }
}

bool SpreadableSnowyDirtBlock::isSnowyConditions(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) {

    (void)state;  // 暂时未使用

    // 参考: MC 1.16.5 SpreadableSnowyDirtBlock.isSnowyConditions()
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr) {
        return true;  // 上方为空气，满足条件
    }

    // 检查是否为雪层且层数为1
    if (aboveState->blockId() == VanillaBlocks::SNOW->blockId()) {
        // 检查层数是否为1
        // TODO: 当添加雪层属性时检查 LAYERS 属性
        // 暂时假设满足条件
        return true;
    }

    // 检查上方是否有流体
    const fluid::FluidState* fluidState = aboveState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->getLevel() == 8) {
        return false;  // 上方有完整水源，不满足条件
    }

    // 检查光照
    // 使用简化版本：如果上方方块不透明，则不满足条件
    // 完整实现需要计算光照
    u8 skyLight = world.getSkyLight(abovePos);
    u8 blockLight = world.getBlockLight(abovePos);
    u8 lightLevel = std::max(skyLight, blockLight);

    return lightLevel < 15;  // 不是完全被阻挡就满足条件
}

bool SpreadableSnowyDirtBlock::isSnowyAndNotUnderwater(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) {

    // 参考: MC 1.16.5 SpreadableSnowyDirtBlock.isSnowyAndNotUnderwater()

    if (!isSnowyConditions(world, pos, state)) {
        return false;
    }

    // 检查上方是否有水
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty()) {
            return false;  // 上方有流体
        }
    }

    return true;
}

// ============================================================================
// GrassBlock 实现
// ============================================================================

GrassBlock::GrassBlock(BlockProperties properties)
    : SpreadableSnowyDirtBlock(std::move(properties)) {
}

// ============================================================================
// MyceliumBlock 实现
// ============================================================================

MyceliumBlock::MyceliumBlock(BlockProperties properties)
    : SpreadableSnowyDirtBlock(std::move(properties)) {
}

} // namespace mc::blocks
