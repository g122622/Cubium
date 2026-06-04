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

#include "BannerBlock.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../IBlockReader.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../fluid/FluidState.hpp"
#include "../../BlockItemUseContext.hpp"
#include "../../BlockState.hpp"
#include "../../BlockStateProperties.hpp"
#include "../../VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ========== AbstractBannerBlock ==========

AbstractBannerBlock::AbstractBannerBlock(const BlockProperties& properties, DyeColor color)
    : Block(properties)
    , m_color(color)
{}

std::unique_ptr<BlockEntity> AbstractBannerBlock::createBlockEntity(const BlockPos& pos)
{
    auto entity = std::make_unique<blockentity::BannerEntity>(pos);
    entity->setBaseColor(m_color);
    return entity;
}

void AbstractBannerBlock::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 图案数据和自定义名称通过 BannerItem 的放置逻辑和方块实体NBT加载处理
}

const fluid::FluidState* AbstractBannerBlock::getFluidState(const BlockState& state) const
{
    if (isWaterlogged(state)) {
        return &fluid::FluidState::WATER;
    }
    return &fluid::FluidState::EMPTY;
}

// ========== StandingBannerBlock ==========

StandingBannerBlock::StandingBannerBlock(const BlockProperties& properties, DyeColor color)
    : AbstractBannerBlock(properties, color)
{
    // 旗杆碰撞形状：中心8x16x8像素
    m_shape = CollisionShape::createBox(4.0, 0.0, 4.0, 12.0, 16.0, 12.0);
}

BlockState StandingBannerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    auto* world = context.getWorld();
    const auto& pos = context.getPos();

    // 根据玩家朝向计算旋转值（0-15，每22.5度）
    i32 rotation = static_cast<i32>(math::floor((180.0f + context.getPlacementYaw()) * 16.0f / 360.0f + 0.5f)) & 15;

    BlockState state = defaultState()
                           .with(BlockStateProperties::ROTATION_0_15(), rotation)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    // 检查含水状态
    if (world != nullptr) {
        auto* fluidState = world->getFluidState(pos);
        if (fluidState != nullptr && fluidState->isWater()) {
            state = state.with(BlockStateProperties::WATERLOGGED(), true);
        }
    }

    return state;
}

bool StandingBannerBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 需要下方方块是实心的
    const BlockState& below = world.getBlockState(pos.down());
    return below.isSolid();
}

BlockState StandingBannerBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 下方方块被移除时旗帜变为空气
    if (facing == Direction::Down && !isValidPosition(state, world, currentPos)) {
        return VanillaBlocks::AIR->defaultState();
    }

    // 处理含水状态更新
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        world.scheduleFluidTick(currentPos, *world.getFluidState(currentPos));
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

const BlockState& StandingBannerBlock::rotate(const BlockState& state, Rotation rotation) const
{
    return state.with(
        BlockStateProperties::ROTATION_0_15(), rotation.rotate(state.get(BlockStateProperties::ROTATION_0_15()), 16));
}

const BlockState& StandingBannerBlock::mirror(const BlockState& state, Mirror mirror) const
{
    return state.with(BlockStateProperties::ROTATION_0_15(),
        mirror.mirrorRotation(state.get(BlockStateProperties::ROTATION_0_15()), 16));
}

const CollisionShape& StandingBannerBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

// ========== WallBannerBlock ==========

WallBannerBlock::WallBannerBlock(const BlockProperties& properties, DyeColor color)
    : AbstractBannerBlock(properties, color)
{
    // 各方向碰撞形状（贴墙薄板）
    // 旗帜面宽度16，高度12.5，厚度2
    m_shapesByDirection[Direction::North] = CollisionShape::createBox(0.0, 0.0, 14.0, 16.0, 12.5, 16.0);
    m_shapesByDirection[Direction::South] = CollisionShape::createBox(0.0, 0.0, 0.0, 16.0, 12.5, 2.0);
    m_shapesByDirection[Direction::West] = CollisionShape::createBox(14.0, 0.0, 0.0, 16.0, 12.5, 16.0);
    m_shapesByDirection[Direction::East] = CollisionShape::createBox(0.0, 0.0, 0.0, 2.0, 12.5, 16.0);
}

BlockState WallBannerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    auto* world = context.getWorld();
    const auto& pos = context.getPos();

    // 遍历玩家视线方向，找到可以放置的墙面
    for (Direction direction : context.getNearestLookingDirections()) {
        if (!direction.isHorizontal()) {
            continue;
        }

        Direction facing = direction.getOpposite();
        BlockState state = defaultState()
                               .with(BlockStateProperties::HORIZONTAL_FACING(), facing)
                               .with(BlockStateProperties::WATERLOGGED(), false);

        if (state.isValidPosition(*world, pos)) {
            // 检查含水状态
            auto* fluidState = world->getFluidState(pos);
            if (fluidState != nullptr && fluidState->isWater()) {
                state = state.with(BlockStateProperties::WATERLOGGED(), true);
            }
            return state;
        }
    }

    // 无法放置时返回默认状态（调用者会检查isValidPosition）
    return defaultState();
}

bool WallBannerBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 检查面向方向的反方向是否有实心方块支撑
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const BlockState& supportState = world.getBlockState(pos.offset(facing.getOpposite()));
    return supportState.isSolid();
}

BlockState WallBannerBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 支撑方块被移除时旗帜变为空气
    Direction bannerFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (facing == bannerFacing.getOpposite() && !isValidPosition(state, world, currentPos)) {
        return VanillaBlocks::AIR->defaultState();
    }

    // 处理含水状态更新
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        world.scheduleFluidTick(currentPos, *world.getFluidState(currentPos));
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

const BlockState& WallBannerBlock::rotate(const BlockState& state, Rotation rotation) const
{
    return state.with(BlockStateProperties::HORIZONTAL_FACING(),
        rotation.rotate(state.get(BlockStateProperties::HORIZONTAL_FACING())));
}

const BlockState& WallBannerBlock::mirror(const BlockState& state, Mirror mirror) const
{
    return state.with(
        BlockStateProperties::HORIZONTAL_FACING(), mirror.mirror(state.get(BlockStateProperties::HORIZONTAL_FACING())));
}

const CollisionShape& WallBannerBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    // 默认返回北方向形状
    return m_shapesByDirection.at(Direction::North);
}

} // namespace blocks
} // namespace mc
