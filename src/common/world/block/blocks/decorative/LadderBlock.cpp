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
    // 遍历 context.getNearestLookingDirections() 中"从放置位置出发、经该方向可达点击方块"的方向：
    //   - 水平方向：FACING = 其 Opposite（倚靠触发放置的那个面）；
    //   - 竖直方向与指向放置位置自身的方向：取玩家水平朝向对应的水平面
    //     （原版此处用 FACING = direction.getOpposite() = Up/Down 构造非法状态、canSurvive 必假，等价跳过）。
    // 首个能存活的候选即为结果。关系为"放置位置相对点击方块的位移"，故用点击位置而非
    // 被替换的点击方块本身（后者在最高台阶、草丛等场景与放置位置重合，虽仍可作为方向锚点）。
    // TODO: 当候选全部不能存活时，原版返回 null 放弃放置；本项目 Block::getStateForPlacement
    //   返回 BlockState 值，只能回退默认状态，由 BlockItem::canPlace 拒绝放置（结果一致，
    //   但放置判定路径与方块侧不一致）。待该接口可表达"不可放置"后对齐。
    const IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();

    Direction fallback = Directions::opposite(context.getClickedFace());
    if (!Directions::isHorizontal(fallback)) {
        fallback = context.horizontalDirection();
    }
    if (!Directions::isHorizontal(fallback)) {
        fallback = Direction::North;
    }

    BlockState candidate = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), fallback);
    bool resolved = false;
    for (Direction direction : context.getNearestLookingDirections()) {
        // 仅接受从放置位置出发、经 direction 可达点击方块（或点击位置本身即放置位置）的方向
        if (pos.offset(direction) != context.getBlockPos() && pos != context.getBlockPos()) {
            continue;
        }
        const Direction facing =
            Directions::isHorizontal(direction) ? Directions::opposite(direction) : context.horizontalDirection();
        candidate = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        if (isValidPosition(candidate, const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world)), pos)) {
            resolved = true;
            break;
        }
    }
    if (!resolved) {
        candidate = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), fallback);
    }

    // 检查是否含水（等价原版 getFluidState(pos).getType() == Fluids.WATER）
    const bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);
    return candidate.with(BlockStateProperties::WATERLOGGED(), waterlogged);
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
