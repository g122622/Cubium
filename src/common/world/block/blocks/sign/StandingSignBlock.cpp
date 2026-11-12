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

#include "common/world/block/blocks/sign/StandingSignBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include <cstddef>

namespace mc {
namespace blocks {

// ========== StandingSignBlock ==========

StandingSignBlock::StandingSignBlock(const BlockProperties& properties, const WoodType& woodType)
    : AbstractSignBlock(properties, woodType, woodType.soundType)
    , m_shape(CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f))
{

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ROTATION_0_15())
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

    setDefaultState(
        defaultState().with(BlockStateProperties::ROTATION_0_15(), 0).with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState StandingSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 根据玩家朝向计算旋转（16个方向）
    f32 yaw = context.getPlayerYaw();
    i32 rotation = static_cast<i32>(std::floor((180.0f + yaw) * 16.0f / 360.0f + 0.5f)) & 15;

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::ROTATION_0_15(), rotation)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool StandingSignBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    // 对应原版 StandingSignBlock#canSurvive：`world.getBlockState(pos.below()).isSolid()`。
    //
    // 这里原先用 `isSolidSide(world, below, UP)`，比原版严格一档：它要求下表面的碰撞形状
    // 铺满整格，而 `isSolid()` 只要求「有一点碰撞、不算流体、不是空气」。于是玻璃、苔藓块、
    // 箱子这类有碰撞但上表面不完整的方块，在原版能撑住告示牌、在 Cubium 撑不住
    // （由差分能力 block_support 查出）。
    MC_UNUSED(state);

    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    return belowState != nullptr && belowState->isSolid();
}

const BlockState& StandingSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& StandingSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const CollisionShape& StandingSignBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
