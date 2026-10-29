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
#include "common/world/block/SupportType.hpp"
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
    // "候选全部不能存活"的放弃语义由 canPlaceAt 在放置状态计算之前判定（见其文档）。
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

bool LadderBlock::canPlaceAt(const BlockItemUseContext& context) const
{
    // 对齐 vanilla LadderBlock.getStateForPlacement 的两处"返回 null"：
    //   1. 未替换点击方块（贴在方块侧面）时，若点击位置内侧方块是朝向点击面的同种梯子则放弃
    //      （避免两把梯子在相邻面上背靠背重叠）；
    //   2. 按点击顺序为每个水平候选方向设置 FACING，首个能存活者成立；全部不能存活时放弃。
    const BlockPos pos = context.placementPos();
    const Direction clickedFace = context.getClickedFace();
    const IWorld& world = context.getWorld();

    if (!context.replacingClickedBlock()) {
        if (const BlockState* behindState = world.getBlockState(pos.offset(clickedFace, -1)); behindState != nullptr &&
            behindState->owner().blockLocation() == blockLocation() && Directions::isHorizontal(clickedFace) &&
            behindState->get(BlockStateProperties::HORIZONTAL_FACING()) == clickedFace) {
            return false;
        }
    }

    IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));
    for (Direction direction : context.getNearestLookingDirections()) {
        if (!Directions::isHorizontal(direction)) {
            continue;
        }
        const BlockState candidate =
            defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(direction));
        if (isValidPosition(candidate, blockReader, pos)) {
            return true;
        }
    }
    return false;
}

bool LadderBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 对齐 vanilla LadderBlock.canSurvive → canAttachTo：倚靠方块在"朝向梯子的那个面"上提供
    // SupportType.FULL 支撑（该面投影覆盖整个 1×1）。据此可精确区分"面完整但不可遮挡"（如玻璃，
    // 可挂梯子）与"面不完整"（如台阶、雪层，不可挂梯子）。
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    // 支撑方块在梯子的**背面**：vanilla LadderBlock.canSurvive 用
    // `pos.relative(direction.getOpposite())` 取附着位置，再用 facing 找支撑面。
    // 此处方向必须取 opposite(facing)：facing=east 的梯子背面是 west，支撑方块在
    // x-1 一侧。此前错写成 pos.offset(facing)（取 facing 方向前方的格子），等于把
    // 「背后有没有东西撑着」判成了「面前有没有东西挡着」，移除真正的背面支撑时梯子
    // 不会自毁（ladder_breaks_when_attached_block_removed 因此失败）。
    const Direction back = Directions::opposite(facing);
    const BlockPos attachPos = pos.offset(back);
    const BlockState* attachState = world.getBlockState(attachPos);

    if (attachState == nullptr) {
        return false;
    }

    // 支撑方块须在「朝向梯子」的那个面上提供完整支撑（即 facing 面）。
    return attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full);
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

    // 仅当所倚靠的那个方块本身发生变化时重新检查存活。
    // vanilla LadderBlock.updateShape 的判据是 `changedDir.getOpposite() == FACING`：原版的
    // changedDir 是「从变化位置指向自身」的方向，而本项目 updatePostPlacement 的 facing 参数
    // 是「从自身指向邻居」的方向（见 Block::updateNeighborStates 传 direction，
    // 与 vanilla updateShape 的入参约定正好相反）。因此本项目的等价判据是
    // `facing == FACING`，即变化来自梯子所倚靠的那一侧。
    //
    // 此前按原版写法比较 opposite(FACING)，等于在等「变化来自梯子正面」，导致附着方块被移除时
    // 梯子不会自毁（ladder_breaks_when_attached_block_removed 用例失败）。
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
