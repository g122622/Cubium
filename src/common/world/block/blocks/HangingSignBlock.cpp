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

#include "HangingSignBlock.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/Direction.hpp"
#include "../../IWorld.hpp"
#include "../../blockentity/interactive/SignEntity.hpp"
#include "../WaterLoggableHelpers.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/SignBlock.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== CeilingHangingSignBlock ==========

CeilingHangingSignBlock::CeilingHangingSignBlock(const BlockProperties& properties, WoodType woodType)
    : AbstractSignBlock(properties, woodType)
    , m_shape(CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 1.0f, 0.75f))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ROTATION_0_15())
            .add(BlockStateProperties::ATTACHED())
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
            .with(BlockStateProperties::ROTATION_0_15(), 0)
            .with(BlockStateProperties::ATTACHED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState CeilingHangingSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    f32 yaw = context.getPlayerYaw();
    i32 rotation = static_cast<i32>(std::floor((180.0f + yaw) * 16.0f / 360.0f + 0.5f)) & 15;

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 检查上方是否有另一个天花板悬挂告示牌（形成链条连接）
    bool attached = false;
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr) {
        const Block* aboveBlock = &aboveState->getBlock();
        // 如果上方也是天花板悬挂告示牌，则当前为连接状态
        if (dynamic_cast<const CeilingHangingSignBlock*>(aboveBlock) != nullptr) {
            attached = true;
        }
    }

    return defaultState()
        .with(BlockStateProperties::ROTATION_0_15(), rotation)
        .with(BlockStateProperties::ATTACHED(), attached)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CeilingHangingSignBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 需要上方有固体支撑
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    return aboveState != nullptr && aboveState->isSolidSide(world, abovePos, Direction::Down);
}

const BlockState& CeilingHangingSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const CollisionShape& CeilingHangingSignBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

void CeilingHangingSignBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
}

const ResourceLocation& CeilingHangingSignBlock::getWaxedInteractFailSound() const
{
    // 悬挂告示牌返回专属音效
    // 对应 MC Java HangingSignBlockEntity.getSignInteractionFailedSoundEvent()
    return SoundEvents::BLOCK_HANGING_SIGN_WAXED_INTERACT_FAIL;
}

// ========== WallHangingSignBlock ==========

WallHangingSignBlock::WallHangingSignBlock(const BlockProperties& properties, WoodType woodType)
    : AbstractSignBlock(properties, woodType)
{
    // 墙面悬挂告示牌各方向的碰撞形状（贴在墙面的薄板，比普通墙面告示牌更宽更矮）
    m_shapesByDirection[Direction::North] = CollisionShape::box(0.0f, 0.0f, 0.875f, 1.0f, 1.0f, 1.0f);
    m_shapesByDirection[Direction::South] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.125f);
    m_shapesByDirection[Direction::East] = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.125f, 1.0f, 1.0f);
    m_shapesByDirection[Direction::West] = CollisionShape::box(0.875f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
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
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState WallHangingSignBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = waterloggable::isWaterFluidState(fluidState);

    for (Direction dir : context.getNearestLookingDirections()) {
        if (Directions::isHorizontal(dir)) {
            Direction facing = Directions::opposite(dir);
            BlockState state = defaultState()
                                   .with(BlockStateProperties::FACING(), facing)
                                   .with(BlockStateProperties::WATERLOGGED(), waterlogged);

            IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world));
            if (isValidPosition(state, blockReader, pos)) {
                return state;
            }
        }
    }

    return defaultState();
}

bool WallHangingSignBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction oppositeDir = Directions::opposite(facing);
    BlockPos adjPos = pos.offset(oppositeDir);
    const BlockState* adjState = world.getBlockState(adjPos);

    return adjState != nullptr && adjState->isSolidSide(world, adjPos, facing);
}

const BlockState& WallHangingSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const BlockState& WallHangingSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::FACING(), newFacing);
}

const CollisionShape& WallHangingSignBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

void WallHangingSignBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
}

const ResourceLocation& WallHangingSignBlock::getWaxedInteractFailSound() const
{
    // 墙面悬挂告示牌返回专属音效（与天花板悬挂告示牌相同）
    // 对应 MC Java HangingSignBlockEntity.getSignInteractionFailedSoundEvent()
    return SoundEvents::BLOCK_HANGING_SIGN_WAXED_INTERACT_FAIL;
}

} // namespace blocks
} // namespace mc
