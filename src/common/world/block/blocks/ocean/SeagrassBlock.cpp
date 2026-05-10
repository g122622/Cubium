#include "SeagrassBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../fluid/FluidTags.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {
namespace blocks {

SeagrassBlock::SeagrassBlock(const BlockProperties& properties)
    : Block(properties) {

    // 海草没有特殊状态
    // 形状：小型水下植物
    m_shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
}

BlockState SeagrassBlock::getStateForPlacement(BlockItemUseContext& context) {
    MC_UNUSED(context);
    return defaultState();
}

bool SeagrassBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    // 检查下方支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr || !belowState->isSolid()) {
        return false;
    }

    // MC 1.16.5: 检查当前位置是否为水源方块（流体等级=8）
    // FluidState fluidstate = worldIn.getFluidState(pos);
    // return fluidstate.isTagged(FluidTags.WATER) && fluidstate.getLevel() == 8;
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 检查是否为水且为水源（level == 8）
    if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
        return false;
    }

    // 水源等级为8
    return fluidState->isSource();
}

const CollisionShape& SeagrassBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& SeagrassBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== IGrowable 接口实现 ==========

bool SeagrassBlock::canGrow(
    IBlockReader& world,
    const BlockPos& pos,
    const BlockState& state,
    bool isClientSide) const {

    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    // 检查上方是否有水源方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const fluid::FluidState* fluidState = world.getFluidState(abovePos);

    if (fluidState == nullptr || fluidState->isEmpty()) {
        return false;
    }

    // 上方必须是水源方块
    return fluidState->getFluid().isIn(fluid::FluidTags::WATER()) && fluidState->isSource();
}

bool SeagrassBlock::canUseBonemeal(
    IWorld& world,
    math::IRandom& random,
    const BlockPos& pos,
    const BlockState& state) const {

    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // MC 1.16.5: 海草使用骨粉总是有效（如果可以生长）
    return true;
}

void SeagrassBlock::grow(
    IWorld& world,
    math::IRandom& random,
    const BlockPos& pos,
    const BlockState& state) {

    MC_UNUSED(random);

    // 检查上方是否有水源方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const fluid::FluidState* fluidState = world.getFluidState(abovePos);

    if (fluidState == nullptr || fluidState->isEmpty()) {
        return;
    }

    if (!fluidState->getFluid().isIn(fluid::FluidTags::WATER()) || !fluidState->isSource()) {
        return;
    }

    // 获取高海草方块
    Block* tallSeagrassBlock = VanillaBlocks::TALL_SEAGRASS;
    if (tallSeagrassBlock == nullptr) {
        return;
    }

    // 设置下半部分
    const BlockState& lowerState = tallSeagrassBlock->defaultState()
        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
        .with(BlockStateProperties::WATERLOGGED(), true);

    // 设置上半部分
    const BlockState& upperState = tallSeagrassBlock->defaultState()
        .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper)
        .with(BlockStateProperties::WATERLOGGED(), true);

    // 放置高海草
    world.setBlockState(pos, &lowerState, 3);
    world.setBlockState(abovePos, &upperState, 3);
}

// ========== 流体状态 ==========

const fluid::FluidState* SeagrassBlock::getFluidState(const BlockState& state) const {
    MC_UNUSED(state);
    // MC 1.16.5: 海草始终返回静止水的流体状态
    // return Fluids.WATER.getStillFluidState(false);
    fluid::Fluid* waterFluid = waterloggable::getWaterFluid();
    if (waterFluid != nullptr) {
        return &waterFluid->defaultState();
    }
    return nullptr;
}

} // namespace blocks
} // namespace mc
