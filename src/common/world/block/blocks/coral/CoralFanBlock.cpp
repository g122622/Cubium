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

#include "common/world/block/blocks/coral/CoralFanBlock.hpp"

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/support/SupportType.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：原版珊瑚扇（含水下形态）的默认状态是 waterlogged=true。
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), true));
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
    // 原版地面珊瑚扇：box(0.125, 0, 0.125 -> 0.875, 0.25, 0.875)，即 column(12, 0, 4)——
    // 宽 12 像素居中、高 4 像素。此前写成整格宽、1 像素厚，面积和厚度都不对。
    static CollisionShape shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.25f, 0.875f);
    return shape;
}

bool CoralFanBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const
{
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos);

    if (adjState == nullptr) {
        return false;
    }

    // 对应原版 CoralFanBlock 的 canSurvive：
    //   world.getBlockState(pos.below()).isFaceSturdy(world, pos.below(), Direction.UP)
    //
    // 注意 `direction` 是「从珊瑚指向底座」的方向（下方珊瑚传入 Down），而底座朝向珊瑚的
    // 那一面是它的反方向（即 UP）。这里原先直接传 `direction`，于是检查成了底座的**下表面**，
    // 上下半砖的结果完全颠倒（由差分能力 block_support 查出）。
    //
    // 另外原先用 isSolidSide，它只判「是固体且有碰撞」而不看面是否铺满整格，
    // 于是箱子、漏斗这类侧面不满的方块也被当成能附着珊瑚。
    const Direction supportingFace = Directions::opposite(direction);
    return adjState->isFaceSturdy(static_cast<IWorld&>(world), adjPos, supportingFace, SupportType::Full);
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CoralFanBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
