#include "BushBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

BushBlock::BushBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::fullBlock())
{}

// ========== 放置逻辑 ==========

BlockState BushBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool BushBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方方块是否可以支撑此植物
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    return canSustain(*belowState, world, belowPos);
}

BlockState BushBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 下方方块更新时检查是否仍可支撑
    if (facing == Direction::Down) {
        BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos);

        if (belowState == nullptr || !canSustain(*belowState, world, belowPos)) {
            // 无法支撑，破坏（返回空气）
            const BlockState* airState = BlockRegistry::instance().airState();
            if (airState != nullptr) {
                return *airState;
            }
        }
    }

    return state;
}

// ========== 形状 ==========

const CollisionShape& BushBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& BushBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 植物无碰撞
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& BushBlock::getOcclusionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 植物不遮挡光线
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== 保护方法 ==========

bool BushBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 默认实现：检查是否为草地或泥土
    const Material& material = groundState.getMaterial();

    // 检查是否为固体且可支撑植物
    // 实际 MC 中有更复杂的逻辑，包括 IPlantable 接口
    return material.isSolid() && !material.isLiquid();
}

} // namespace blocks
} // namespace mc
