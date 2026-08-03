/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/PlantType.hpp"

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
    // 委托给下方方块的 canSustainPlant 方法
    // 如果下方方块支持此植物的类型（通过 IPlantable 接口），则返回 true
    //
    // 注意：此实现替代了旧版的 groundState.getMaterial().isSolid() 检查。
    // 旧版允许植物放置在任意固体方块上（包括石头），但 MC 原版 VegetationBlock.mayPlaceOn()
    // 只允许放置在 DIRT 标签方块和耕地上。新的委托模式与原版行为一致。
    // 如果某些自定义植物需要更宽松的放置条件，可以重写 canSustain() 方法。
    const Block& groundBlock = groundState.getBlock();
    return groundBlock.canSustainPlant(groundState, static_cast<IBlockReader&>(world), groundPos, Direction::Up, *this);
}

// ========== IPlantable 接口实现 ==========

PlantType BushBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认植物类型为平原（大多数花草、树苗等）
    // 子类可重写返回其他类型
    return PlantType::Plains;
}

const BlockState& BushBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

} // namespace blocks
} // namespace mc
