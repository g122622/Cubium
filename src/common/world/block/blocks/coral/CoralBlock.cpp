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

#include "CoralBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] bool hasNearbyWater(mc::IWorld& world, const mc::BlockPos& pos)
{
    for (mc::Direction dir : {mc::Direction::North,
             mc::Direction::South,
             mc::Direction::East,
             mc::Direction::West,
             mc::Direction::Up,
             mc::Direction::Down}) {
        const mc::fluid::FluidState* fluidState = world.getFluidState(pos.offset(dir));
        if (mc::waterloggable::isWaterFluidState(fluidState)) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] const mc::BlockState* getDeadBlockState(mc::u32 deadBlockId)
{
    mc::Block* deadBlock = mc::BlockRegistry::instance().getBlock(deadBlockId);
    return deadBlock != nullptr ? &deadBlock->defaultState() : nullptr;
}

} // namespace

namespace mc {
namespace blocks {

// ========== CoralBlock ==========

CoralBlock::CoralBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState CoralBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState().with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState CoralBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    if (!hasNearbyWater(world, currentPos)) {
        if (const BlockState* deadState = getDeadBlockState(m_deadBlock); deadState != nullptr) {
            return *deadState;
        }
    }

    return state;
}

bool CoralBlock::isInWater(const BlockState& state) const
{
    return state.get(BlockStateProperties::WATERLOGGED());
}

bool CoralBlock::isWaterNearby(IWorld& world, const BlockPos& pos) const
{
    return hasNearbyWater(world, pos);
}

const CollisionShape& CoralBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CoralBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== CoralFanBlock ==========

CoralFanBlock::CoralFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock)
{

    // 创建状态容器
    // vanilla 1.21.11 地面珊瑚扇(CoralFanBlock)仅有 waterlogged，无 facing（朝向由墙扇 CoralWallFanBlock 持有）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState CoralFanBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState().with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CoralFanBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 与 vanilla BaseCoralPlantTypeBlock.canSurvive 一致：检查下方块顶面是否坚固
    return canAttachTo(world, pos, Direction::Down);
}

BlockState CoralFanBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 下方支撑失效则移除（vanilla updateShape: facing==DOWN && !canSurvive -> AIR）
    if (facing == Direction::Down && !canAttachTo(static_cast<IBlockReader&>(world), currentPos, Direction::Down)) {
        if (auto* airState = BlockRegistry::instance().airState()) {
            return *airState;
        }
    }

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    if (!hasNearbyWater(world, currentPos)) {
        if (const BlockState* deadState = getDeadBlockState(m_deadBlock); deadState != nullptr) {
            return *deadState;
        }
    }

    return state;
}

const BlockState& CoralFanBlock::rotate(const BlockState& state, Rotation rotation) const
{
    // 地面珊瑚扇无朝向属性，旋转不变
    MC_UNUSED(rotation);
    return state;
}

const BlockState& CoralFanBlock::mirror(const BlockState& state, Mirror mirror) const
{
    // 地面珊瑚扇无朝向属性，镜像不变
    MC_UNUSED(mirror);
    return state;
}

const CollisionShape& CoralFanBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 珊瑚扇是薄层
    static CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
    return shape;
}

bool CoralFanBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const
{
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos);

    if (adjState == nullptr) {
        return false;
    }

    return adjState->isSolidSide(world, adjPos, direction);
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CoralFanBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== CoralWallFanBlock ==========

CoralWallFanBlock::CoralWallFanBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock)
{

    // 创建状态容器
    // vanilla 1.21.11 墙珊瑚扇 facing 仅水平 4 向（north/south/east/west），不含 up/down。
    // 用 HORIZONTAL_FACING 对齐 vanilla 8 状态以通过 JavaBlockStateIdMap 映射。
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::WATERLOGGED())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

BlockState CoralWallFanBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // vanilla 墙珊瑚扇只能贴附在水平面上：facing 仅水平 4 向。
    // HORIZONTAL_FACING 不接受 up/down（with 会抛 invalid_argument），故点击地面/天花板时
    // 退化为 North（随后 isValidPosition 会因对面无支撑方块而拒绝放置）。
    Direction clickedFace = context.getClickedFace();
    Direction facing =
        (clickedFace == Direction::Up || clickedFace == Direction::Down) ? Direction::North : clickedFace;
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CoralWallFanBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return canAttachTo(world, pos, Directions::opposite(facing));
}

BlockState CoralWallFanBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    Direction attachDir = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (Directions::opposite(facing) == attachDir) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!canAttachTo(blockReader, currentPos, facing)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    if (!hasNearbyWater(world, currentPos)) {
        if (const BlockState* deadState = getDeadBlockState(m_deadBlock); deadState != nullptr) {
            return *deadState;
        }
    }

    return state;
}

const BlockState& CoralWallFanBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& CoralWallFanBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& CoralWallFanBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());

    // 根据朝向返回不同形状
    switch (facing) {
        case Direction::North: {
            static CollisionShape northShape = CollisionShape::box(0.0f, 0.0f, 0.9375f, 1.0f, 1.0f, 1.0f);
            return northShape;
        }
        case Direction::South: {
            static CollisionShape southShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0625f);
            return southShape;
        }
        case Direction::East: {
            static CollisionShape eastShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 0.0625f, 1.0f, 1.0f);
            return eastShape;
        }
        case Direction::West: {
            static CollisionShape westShape = CollisionShape::box(0.9375f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
            return westShape;
        }
        default: {
            static CollisionShape emptyShape = CollisionShape::empty();
            return emptyShape;
        }
    }
}

bool CoralWallFanBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const
{
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos);

    if (adjState == nullptr) {
        return false;
    }

    return adjState->isSolidSide(world, adjPos, direction);
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CoralWallFanBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== CoralBlockBlock ==========

CoralBlockBlock::CoralBlockBlock(CoralColor color, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
{
    // 珊瑚块没有状态属性
}

const CollisionShape& CoralBlockBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

} // namespace blocks
} // namespace mc
