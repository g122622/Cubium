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

#include "LanternBlock.hpp"

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
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

LanternBlock::LanternBlock(BlockProperties properties, u8 lightValue)
    : Block(std::move(properties))
    , m_lightValue(lightValue)
{
    // 创建状态容器（HANGING 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HANGING())
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
        defaultState().with(BlockStateProperties::HANGING(), false).with(BlockStateProperties::WATERLOGGED(), false));

    // 创建形状。原版：
    //
    //   SHAPE_STANDING = or(Block.column(4.0, 7.0, 9.0), Block.column(6.0, 0.0, 7.0));
    //   SHAPE_HANGING  = SHAPE_STANDING.move(0.0, 0.0625, 0.0).optimize();
    //
    // 展开成像素坐标：
    //   column(4.0, 7.0, 9.0) -> box(6, 7, 6, 10,  9, 10)   顶部小柱
    //   column(6.0, 0.0, 7.0) -> box(5, 0, 5, 11,  7, 11)   灯体
    //   合并外轮廓              box(5, 0, 5, 11,  9, 11)
    //   悬挂 = 站立整体上移 1 像素   box(5, 1, 5, 11, 10, 11)
    //
    // 注意必须用 fromPixelBox：`CollisionShape::box` 接收的是**归一化 0..1 坐标**，
    // 直接把像素值（5..11）传进去会得到远超方块的形状。
    m_standingShape = CollisionShape::combine(CollisionShape::fromPixelBox(6.0f, 7.0f, 6.0f, 10.0f, 9.0f, 10.0f),
        CollisionShape::fromPixelBox(5.0f, 0.0f, 5.0f, 11.0f, 7.0f, 11.0f),
        CollisionShape::CombineOp::OR);
    m_hangingShape = CollisionShape::combine(CollisionShape::fromPixelBox(6.0f, 8.0f, 6.0f, 10.0f, 10.0f, 10.0f),
        CollisionShape::fromPixelBox(5.0f, 1.0f, 5.0f, 11.0f, 8.0f, 11.0f),
        CollisionShape::CombineOp::OR);
}

BlockState LanternBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 与 MC 1.21.11 一致：遍历玩家视线方向，优先尝试悬挂（UP）或站立（DOWN）
    IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();
    const bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 先尝试玩家视线方向中可用的 Y 轴方向
    for (const Direction direction : context.getNearestLookingDirections()) {
        if (Directions::getAxis(direction) != Axis::Y) {
            continue;
        }
        const bool hanging = (direction == Direction::Up);
        BlockState candidate = defaultState()
                                   .with(BlockStateProperties::HANGING(), hanging)
                                   .with(BlockStateProperties::WATERLOGGED(), waterlogged);
        // canSurvive: canSupportCenter(world, pos.relative(supportDir), opposite(supportDir))
        const Direction supportDir = hanging ? Direction::Up : Direction::Down;
        if (Block::canSupportCenter(world, pos.offset(supportDir), Directions::opposite(supportDir))) {
            return candidate;
        }
    }

    // 默认状态（站立）——与 MC 返回 null 不同，本项目返回默认状态以避免放置时崩溃
    return defaultState()
        .with(BlockStateProperties::HANGING(), false)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool LanternBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 与 MC 1.21.11 LanternBlock.canSurvive 一致：
    //   Block.canSupportCenter(world, pos.relative(direction), direction.getOpposite())
    // 其中 direction = getConnectedDirection(state).opposite()
    // - HANGING=true: direction=UP，检查 pos.above() 的 DOWN 面
    // - HANGING=false: direction=DOWN，检查 pos.below() 的 UP 面
    const bool hanging = state.get(BlockStateProperties::HANGING());
    const Direction supportDir = hanging ? Direction::Up : Direction::Down;
    const BlockPos supportPos = pos.offset(supportDir);
    return Block::canSupportCenter(world, supportPos, Directions::opposite(supportDir));
}

BlockState LanternBlock::updatePostPlacement(const BlockState& state,
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

    // 与 MC 1.21.11 一致：支撑方向邻居变化且 canSurvive 失败时掉落为空气
    const bool hanging = state.get(BlockStateProperties::HANGING());
    const Direction supportDir = hanging ? Direction::Up : Direction::Down;

    if (facing == supportDir) {
        if (!Block::canSupportCenter(world, currentPos.offset(supportDir), Directions::opposite(supportDir))) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const CollisionShape& LanternBlock::getShape(const BlockState& state) const
{
    bool hanging = state.get(BlockStateProperties::HANGING());
    return hanging ? m_hangingShape : m_standingShape;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* LanternBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

bool LanternBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 原版显式返回 false：形状不完整不等于可以穿过。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

} // namespace blocks
} // namespace mc
