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

#include "LadderBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

LadderBlock::LadderBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器（HORIZONTAL_FACING 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 创建各方向的形状
    // 梯子形状：非常薄的板，厚度约1像素
    m_shapes[static_cast<size_t>(Direction::North)] = CollisionShape::box(0.0f, 0.0f, 15.0f / 16.0f, 1.0f, 1.0f, 1.0f);
    m_shapes[static_cast<size_t>(Direction::South)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f / 16.0f);
    m_shapes[static_cast<size_t>(Direction::West)] = CollisionShape::box(15.0f / 16.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    m_shapes[static_cast<size_t>(Direction::East)] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f / 16.0f, 1.0f, 1.0f);
}

BlockState LadderBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据点击的面确定朝向
    Direction facing = context.getClickedFace();

    // 只能放在侧面上
    if (facing == Direction::Down || facing == Direction::Up) {
        // 如果点击的是上下面，使用玩家的朝向
        facing = context.horizontalDirection();
    }

    // 确保是水平方向
    if (facing == Direction::Down || facing == Direction::Up) {
        facing = Direction::North;
    }

    // 检查是否含水
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 梯子朝向是附着面的反方向
    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing))
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool LadderBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 梯子需要附着在固体方块的侧面
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    // 获取梯子背面的方块位置（朝向的反方向）
    BlockPos attachPos = pos.offset(Directions::opposite(facing));
    const BlockState* attachState = world.getBlockState(attachPos);

    if (attachState == nullptr) {
        return false;
    }

    // 检查背面方块的朝向梯子的面是否为实体面
    return attachState->isSolidSide(world, attachPos, facing);
}

BlockState LadderBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 如果梯子背面方块被移除，则移除梯子
    Direction ladderFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (facing == Directions::opposite(ladderFacing)) {
        // 背面方块发生变化，检查是否仍然可以附着
        BlockPos attachPos = currentPos.offset(Directions::opposite(ladderFacing));
        const BlockState* attachState = world.getBlockState(attachPos);

        if (attachState == nullptr || !attachState->isSolidSide(world, attachPos, ladderFacing)) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const BlockState& LadderBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& LadderBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& LadderBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    size_t index = static_cast<size_t>(facing);

    if (index < Directions::COUNT && Directions::isHorizontal(facing)) {
        return m_shapes[index];
    }

    return m_shapes[static_cast<size_t>(Direction::North)];
}

const CollisionShape& LadderBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 梯子没有碰撞箱
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* LadderBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
