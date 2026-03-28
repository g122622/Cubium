#include "FlowerPotBlock.hpp"

namespace mc {
namespace blocks {

FlowerPotBlock::FlowerPotBlock(const BlockProperties& properties, u32 content)
    : Block(properties)
    , m_content(content)
{
    // 花盆形状：底部圆形 + 顶部边缘
    // 简化为单个盒子
    m_shape = CollisionShape::box(5.0f / 16.0f, 0.0f, 5.0f / 16.0f,
                                   11.0f / 16.0f, 6.0f / 16.0f, 11.0f / 16.0f);
    m_collisionShape = CollisionShape::box(5.0f / 16.0f, 0.0f, 5.0f / 16.0f,
                                            11.0f / 16.0f, 6.0f / 16.0f, 11.0f / 16.0f);
}

const CollisionShape& FlowerPotBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& FlowerPotBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_collisionShape;
}

bool FlowerPotBlock::isValidPosition(
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

BlockState FlowerPotBlock::updatePostPlacement(
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
