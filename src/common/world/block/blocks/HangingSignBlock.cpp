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
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/SignBlock.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
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

const BlockState& CeilingHangingSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
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
    // 墙面悬挂告示牌各方向的形状。
    //
    // 对齐原版 `WallHangingSignBlock`：
    //
    //   SHAPES_PLANK = rotateHorizontalAxis(Block.column(16.0, 4.0, 14.0, 16.0));
    //   SHAPES       = rotateHorizontalAxis(Shapes.or(SHAPES_PLANK.get(Axis.Z),
    //                                                 Block.column(14.0, 2.0, 0.0, 10.0)));
    //
    // `Block.column(xzW, xzD, yMin, yMax)` 以方块中心为原点，因此：
    //   column(16.0, 4.0, 14.0, 16.0) -> box(0,  4, 14, 16, 16, 16)   顶部横板（Z 朝向）
    //   column(14.0, 2.0, 0.0, 10.0)  -> box(1,  0,  1, 15, 10, 15)   中心立柱
    //
    // 绕 Y 轴转 90° 得到 X 朝向的横板 box(14, 4, 0, 16, 16, 16)，中心立柱不变。
    //
    // 此前的实现写成满高（Y 0..1）、2 像素厚的薄板，与原版差异很大：
    // 原版的板只在 y 4..16、贴着墙面，同时中心立柱伸到 y 0..10，
    // 这使得 DOWN/UP 面都能覆盖中心 2×2 区域（`SupportType.CENTER` 判定为 true）。
    constexpr f32 P = 1.0f / 16.0f;
    // 中心立柱（X/Z 两个朝向共用）
    const CollisionShape pillar = CollisionShape::box(1.0f * P, 0.0f, 1.0f * P, 15.0f * P, 10.0f * P, 15.0f * P);
    // Z 朝向：横板贴在 z=14..16
    CollisionShape plankZ = CollisionShape::box(0.0f, 4.0f * P, 14.0f * P, 1.0f, 1.0f, 1.0f);
    plankZ = CollisionShape::combine(plankZ, pillar, CollisionShape::CombineOp::OR);
    // X 朝向：横板贴在 x=14..16
    CollisionShape plankX = CollisionShape::box(14.0f * P, 4.0f * P, 0.0f, 1.0f, 1.0f, 1.0f);
    plankX = CollisionShape::combine(plankX, pillar, CollisionShape::CombineOp::OR);

    m_shapesByDirection[Direction::North] = plankZ;
    m_shapesByDirection[Direction::South] = plankZ;
    m_shapesByDirection[Direction::East] = plankX;
    m_shapesByDirection[Direction::West] = plankX;

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
                                   .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
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
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction oppositeDir = Directions::opposite(facing);
    BlockPos adjPos = pos.offset(oppositeDir);
    const BlockState* adjState = world.getBlockState(adjPos);

    return adjState != nullptr && adjState->isSolidSide(world, adjPos, facing);
}

const BlockState& WallHangingSignBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& WallHangingSignBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& WallHangingSignBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
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
