#include "SeagrassBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"

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

    if (belowState == nullptr) {
        return false;
    }

    // TODO: 需要检查当前位置是否为水源方块（level=8）
    // 参见 MC 1.16.5 SeaGrassBlock.isValidPosition
    // FluidState fluidstate = worldIn.getFluidState(pos);
    // return fluidstate.isTagged(FluidTags.WATER) && fluidstate.getLevel() == 8;

    return belowState->isSolid();
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

} // namespace blocks
} // namespace mc
