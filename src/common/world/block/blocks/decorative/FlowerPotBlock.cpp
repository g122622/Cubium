#include "FlowerPotBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../VanillaBlocks.hpp"

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
    // 参考 MC 1.16.5: FlowerPotBlock 继承自 Block，默认 isValidPosition 返回 true
    // 但实际上 updatePostPlacement 会检查下方是否有支撑
    // 花盆可以放置在任何完整方块上，不需要特别检查
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
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    // 参考 MC 1.16.5: FlowerPotBlock.updatePostPlacement
    // 如果下方方块被移除，则移除花盆
    if (facing == Direction::Down) {
        const BlockPos belowPos = currentPos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || belowState->isAir()) {
            return VanillaBlocks::AIR->defaultState();
        }
    }
    return state;
}

} // namespace blocks
} // namespace mc
