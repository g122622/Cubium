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
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

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

void AbstractBannerBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(stack);
    // 图案数据和自定义名称通过 BannerItem 的放置逻辑和方块实体NBT加载处理
}

// ========== StandingBannerBlock ==========

StandingBannerBlock::StandingBannerBlock(const BlockProperties& properties, DyeColor color)
    : AbstractBannerBlock(properties, color)
{
    // vanilla 1.21.11 BannerBlock 仅有 rotation 属性（无 waterlogged）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::ROTATION_0_15())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::ROTATION_0_15(), 0));

    // 旗杆碰撞形状：中心8x16x8像素
    m_shape = CollisionShape::box(4.0 / 16.0, 0.0, 4.0 / 16.0, 12.0 / 16.0, 1.0, 12.0 / 16.0);
}

BlockState StandingBannerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据玩家朝向计算旋转值（0-15，每22.5度）
    i32 rotation = static_cast<i32>(std::floor((180.0f + context.getPlayerYaw()) * 16.0f / 360.0f + 0.5f)) & 15;

    return defaultState().with(BlockStateProperties::ROTATION_0_15(), rotation);
}

bool StandingBannerBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 需要下方方块是实心的
    const BlockState* below = world.getBlockState(pos.down());
    return below != nullptr && below->isSolid();
}

BlockState StandingBannerBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    MC_UNUSED(world);

    // 下方方块被移除时旗帜变为空气
    if (facing == Direction::Down) {
        const BlockState* below = world.getBlockState(currentPos.down());
        if (below == nullptr || !below->isSolid()) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const BlockState& StandingBannerBlock::rotate(const BlockState& state, Rotation rotation) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::rotateRotation(currentRotation, rotation, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
}

const BlockState& StandingBannerBlock::mirror(const BlockState& state, Mirror mirror) const
{
    i32 currentRotation = state.get(BlockStateProperties::ROTATION_0_15());
    i32 newRotation = Directions::mirrorRotation(currentRotation, mirror, 16);
    return state.with(BlockStateProperties::ROTATION_0_15(), newRotation);
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
    // 【构造顺序约束】shape 容器必须在 createBlockState 之前填充。
    // 原因：createBlockState 会为每个 BlockState 调用 _cacheProperties，其中
    // getOpacity→propagatesSkylightDown→getOcclusionShape→getShape 会在 BlockState 构造期
    // （此时本构造函数体尚未执行到此处）回调 getShape。若 m_shapesByDirection 此时为空，
    // getShape 的 fallback（m_shapesByDirection.at(North)）会命中空 unordered_map 抛
    // std::out_of_range("invalid unordered_map<K, T> key")。对齐 PointedDripstoneBlock /
    // AmethystClusterBlock 的正确构造顺序（先填 shape 再 createBlockState）。

    // 各方向碰撞形状（贴墙薄板）
    // 旗帜面宽度16，高度12.5，厚度2
    m_shapesByDirection[Direction::North] = CollisionShape::box(0.0, 0.0, 14.0 / 16.0, 1.0, 12.5 / 16.0, 1.0);
    m_shapesByDirection[Direction::South] = CollisionShape::box(0.0, 0.0, 0.0, 1.0, 12.5 / 16.0, 2.0 / 16.0);
    m_shapesByDirection[Direction::West] = CollisionShape::box(14.0 / 16.0, 0.0, 0.0, 1.0, 12.5 / 16.0, 1.0);
    m_shapesByDirection[Direction::East] = CollisionShape::box(0.0, 0.0, 0.0, 2.0 / 16.0, 12.5 / 16.0, 1.0);

    // vanilla 1.21.11 WallBannerBlock 仅有 facing 属性（无 waterlogged）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

BlockState WallBannerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockPos pos = context.placementPos();

    // 遍历玩家视线方向，找到可以放置的墙面
    for (Direction direction : context.getNearestLookingDirections()) {
        if (!Directions::isHorizontal(direction)) {
            continue;
        }

        Direction facing = Directions::opposite(direction);
        BlockState state = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);

        // 使用 IBlockReader 接口检查是否可放置
        IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(context.getWorld()));
        if (isValidPosition(state, blockReader, pos)) {
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
    const BlockState* supportState = world.getBlockState(pos.offset(Directions::opposite(facing)));
    return supportState != nullptr && supportState->isSolid();
}

BlockState WallBannerBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    MC_UNUSED(world);

    // 支撑方块被移除时旗帜变为空气
    Direction bannerFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    if (facing == Directions::opposite(bannerFacing)) {
        const BlockState* supportState = world.getBlockState(currentPos.offset(Directions::opposite(bannerFacing)));
        if (supportState == nullptr || !supportState->isSolid()) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const BlockState& WallBannerBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& WallBannerBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    Direction newFacing = Directions::rotateDirection(facing, rot);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
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
