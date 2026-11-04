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

#include "BigDripleafStemBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

BigDripleafStemBlock::BigDripleafStemBlock(const BlockProperties& properties)
    : Block(BlockProperties(properties).noCollision())
    // 原版大垂滴叶茎：box(0.3125, 0, 0.5625 -> 0.6875, 1, 0.9375)。
    // X 方向宽 6 像素居中（5..11），Z 方向偏南且更宽（9..15），并非关于中心对称。
    , m_shape(CollisionShape::box(0.3125f, 0.0f, 0.5625f, 0.6875f, 1.0f, 0.9375f))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
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
}

void BigDripleafStemBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState BigDripleafStemBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction horizontalFacing = context.horizontalDirection();
    BlockState state = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), horizontalFacing);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

bool BigDripleafStemBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 大滴叶茎可以存活当且仅当：
    // 1. 下方是另一个大滴叶茎 或 BIG_DRIPLEAF_PLACEABLE 标签中的方块
    // 2. 上方是另一个大滴叶茎 或 大滴叶（BigDripleafBlock）
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }
    bool validBelow = belowState->is(this) || BlockTags::BIG_DRIPLEAF_PLACEABLE().contains(*belowState);
    if (!validBelow) {
        return false;
    }

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr) {
        return false;
    }
    bool validAbove = aboveState->is(this) || aboveState->is(VanillaBlocks::BIG_DRIPLEAF);
    return validAbove;
}

BlockState BigDripleafStemBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 当上方或下方方块变化导致无法存活时，延迟1tick后销毁
    if ((facing == Direction::Down || facing == Direction::Up) &&
        !isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        world.tickManager().scheduleBlockTick(currentPos, *this, 1);
    }

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

void BigDripleafStemBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 防护检查：确认方块尚未被替换（TickManager 回调也会检查，此处双重保障）
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || !currentState->is(this)) {
        return;
    }

    // 延迟 tick 触发后，重新检查存活条件
    // 如果仍然无法存活，则销毁方块并掉落物品
    if (!isValidPosition(state, static_cast<IBlockReader&>(world), pos)) {
        // 原版 tick 中的 serverLevel.destroyBlock(pos, true)
        world.destroyBlock(pos, true);
    }
}

const CollisionShape& BigDripleafStemBlock::getShape(const BlockState& state) const
{
    // 原版茎的 getShape 随 FACING 绕 Y 轴旋转（`Shapes.rotateHorizontal`）。
    // 逐条按 forAllBoxes 实测（big_dripleaf_stem）：
    //
    //   north -> [0.3125,0,0.5625 -> 0.6875,1,0.9375]
    //   east  -> [0.0625,0,0.3125 -> 0.4375,1,0.6875]
    //   south -> [0.3125,0,0.0625 -> 0.6875,1,0.4375]
    //   west  -> [0.5625,0,0.3125 -> 0.9375,1,0.6875]
    //
    // 此前无条件返回 NORTH 的 `m_shape`，四个朝向里三个是错的——
    // 茎在朝东/南/西的大垂滴叶下方会明显错位。
    static const std::array<CollisionShape, 6> shapes = {
        CollisionShape::empty(),                                             // Down
        CollisionShape::empty(),                                             // Up
        CollisionShape::box(0.3125f, 0.0f, 0.5625f, 0.6875f, 1.0f, 0.9375f), // North
        CollisionShape::box(0.3125f, 0.0f, 0.0625f, 0.6875f, 1.0f, 0.4375f), // South
        CollisionShape::box(0.5625f, 0.0f, 0.3125f, 0.9375f, 1.0f, 0.6875f), // West
        CollisionShape::box(0.0625f, 0.0f, 0.3125f, 0.4375f, 1.0f, 0.6875f), // East
    };
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const auto index = static_cast<std::size_t>(facing);
    MC_ASSERT(index < shapes.size() && Directions::isHorizontal(facing));
    return shapes[index];
}

const fluid::FluidState* BigDripleafStemBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

const BlockState& BigDripleafStemBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BigDripleafStemBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

} // namespace blocks
} // namespace mc
