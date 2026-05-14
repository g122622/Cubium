#include "CarpetBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../VanillaBlocks.hpp"

namespace mc {
namespace blocks {

CarpetBlock::CarpetBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 地毯高度为1像素（1/16格）
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f / 16.0f, 1.0f);
}

const CollisionShape& CarpetBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& CarpetBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 地毯没有碰撞箱（可以穿过）
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool CarpetBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 参考 MC 1.16.5: CarpetBlock.isValidPosition
    // 地毯需要放置在非空气方块上方
    const BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    // 检查下方是否为空气方块
    return belowState != nullptr && !belowState->isAir();
}

BlockState CarpetBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    // 参考 MC 1.16.5: CarpetBlock.updatePostPlacement
    // 如果下方方块被移除，则移除地毯
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
