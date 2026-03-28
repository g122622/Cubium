#include "CarpetBlock.hpp"

namespace mc {
namespace blocks {

CarpetBlock::CarpetBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 地毯高度为1像素（1/16格）
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f / 16.0f, 1.0f);
}

const CollisionShape& CarpetBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& CarpetBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 地毯没有碰撞箱（可以穿过）
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool CarpetBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 检查下方是否有固体方块
    return true;
}

BlockState CarpetBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);
    // TODO: 检查下方方块是否仍然存在
    return state;
}

} // namespace blocks
} // namespace mc
