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
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
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
    // 层级关系：FACING 指向梯子所倚靠的墙体。原版从 getNearestLookingDirections() 中取水平方向 d，
    // 令 FACING = d.opposite，并以"canSurvive"判定附着。Cubium 的 placementPos 恒为点击面的相邻格，
    // 因此点击面的 Opposite 恰等于放置位置相对点击方块指向，即所需 FACING——直接由点击面推导即可；
    // 点击上/下面时视线最近方向为 Down（非水平），原版会继续尝试其余方向，此处按语义退化为
    // 玩家水平朝向（放置时 BlockItem::canPlace → isValidPosition 再行校验存活）。
    // 与原版仍有的偏差：可替换方块上的放置原版按视线方向而非点击面定朝向；
    // 视线方向与点击面不一致时，原版选的朝向可能不同。
    // TODO: 待 Block::getStateForPlacement 支持返回"不可放置"（如 std::optional<BlockState>）
    //   后，改为逐个尝试 context.getNearestLookingDirections() 中的水平方向 + isValidPosition，
    //   完全对齐原版的遍历语义。
    Direction facing = context.getClickedFace();
    if (facing == Direction::Down || facing == Direction::Up) {
        // 点击上下面时退化为玩家水平朝向
        facing = context.horizontalDirection();
    }
    // FACING 指向梯子所倚靠的墙体，即放置位置相对点击方块的方向（点击面的 Opposite）
    facing = Directions::opposite(facing);

    // 检查是否含水（等价原版 getFluidState(pos).getType() == Fluids.WATER）
    const IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();
    const bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool LadderBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 梯子需要附着在朝向反方向方块的相应面上，且该面为实体面（原版 canSurvive → canAttachTo
    //   → BlockState.isFaceSturdy(direction, SupportType.FULL)，direction 为朝向梯子的面）。
    // TODO: 待 VoxelShape::getFaceShape 修复面投影返回错误形状（完整方块取面后仍被判为投影不完整，
    //   使 SupportType 三档判定对完整方块一致返回 false）后，改用 isFaceSturdy(..., SupportType::Full)，
    //   以精确区分"面完整但不可遮挡"（如玻璃）与"面不完整"（如台阶、雪层）的方块。
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const BlockPos attachPos = pos.offset(facing);
    const BlockState* attachState = world.getBlockState(attachPos);

    if (attachState == nullptr) {
        return false;
    }

    return attachState->isSolidSide(world, attachPos, Directions::opposite(facing));
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

    // 仅当变化方向为梯子的 FACING（即所倚靠的方块本身发生变化）时重新检查存活
    const Direction ladderFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (facing == ladderFacing && !isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        return VanillaBlocks::AIR->defaultState();
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
