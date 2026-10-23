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

#include "CreakingHeartBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

CreakingHeartBlock::CreakingHeartBlock(const BlockProperties& properties)
    : RotatedPillarBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AXIS())
            .add(BlockStateProperties::CREAKING_HEART_STATE())
            .add(BlockStateProperties::NATURAL())
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
            .with(BlockStateProperties::AXIS(), Axis::Y)
            .with(BlockStateProperties::CREAKING_HEART_STATE(), BlockStateProperties::CreakingHeartState::Uprooted)
            .with(BlockStateProperties::NATURAL(), false));
}

void CreakingHeartBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState CreakingHeartBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 对齐 vanilla CreakingHeartBlock.getStateForPlacement（CreakingHeartBlock.java:144-147）：
    //   AXIS = clickedFace.getAxis()（点击面的轴向，同原木 pillar 放置语义）。
    //   随后调 updateState：若 hasRequiredLogs（轴线两端均为 pale_oak_logs 且 AXIS 一致）
    //   且 STATE==UPROOTED → 按 CREAKING_ACTIVE 环境属性设 AWAKE/DORMANT；否则保持 UPROOTED。
    //   NATURAL 保持默认 false（玩家放置非自然生成，仅自然生成结构设 true）。
    const Axis axis = Directions::getAxis(context.getClickedFace());
    const BlockState placed =
        defaultState()
            .with(BlockStateProperties::AXIS(), axis)
            .with(BlockStateProperties::CREAKING_HEART_STATE(), BlockStateProperties::CreakingHeartState::Uprooted)
            .with(BlockStateProperties::NATURAL(), false);

    // 注意：vanilla 在此以 placementPos 调 updateState（用已放置位置查询两端原木）。
    return updateState(placed, context.world(), context.placementPos());
}

BlockState CreakingHeartBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 对齐 vanilla CreakingHeartBlock.updateShape（:94）：任一方向邻居变化即调度 1 刻后的检查，
    //   由 tick 重解析状态（两端原木被破坏或补齐时切换 UPROOTED↔DORMANT/AWAKE）。
    world.tickManager().scheduleBlockTick(currentPos, *this, 1);
    return state;
}

void CreakingHeartBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 对齐 vanilla CreakingHeartBlock.tick（:99-104）：重解析状态，变化则写回。
    const BlockState newState = updateState(state, world, pos);
    if (newState != state) {
        state = newState;
        world.setBlockState(pos, &state, world::BlockUpdateFlags::UPDATE_ALL);
    }
}

bool CreakingHeartBlock::hasRequiredLogs(const BlockState& state, IWorld& world, const BlockPos& pos)
{
    // 对齐 vanilla hasRequiredLogs（:119-130）：沿 AXIS 正负两向各取一块，须均在 PALE_OAK_LOGS
    //   标签内且其 AXIS 与该轴一致；两端任一不满足即返回 false。
    const Axis axis = state.get(BlockStateProperties::AXIS());
    Direction neg = Direction::Down;
    Direction pos_ = Direction::Up;
    Directions::getAxisDirections(axis, neg, pos_);

    for (const Direction dir : {neg, pos_}) {
        const BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighbor = world.getBlockState(neighborPos);
        if (neighbor == nullptr || !BlockTags::PALE_OAK_LOGS().contains(*neighbor)) {
            return false;
        }
        // 邻居须持有 AXIS 属性且与本方块同轴（非 pillar 方块不含 AXIS，视为不满足）。
        const auto* axisProp = &BlockStateProperties::AXIS();
        if (!neighbor->hasProperty(*axisProp) || neighbor->get(*axisProp) != axis) {
            return false;
        }
    }
    return true;
}

BlockState CreakingHeartBlock::updateState(const BlockState& state, IWorld& world, const BlockPos& pos)
{
    // 对齐 vanilla updateState（:106-117）：两端原木齐全且当前为 UPROOTED 时，按 CREAKING_ACTIVE
    //   置 AWAKE（活跃）或 DORMANT（休眠）；否则状态不变。
    const bool logsOk = hasRequiredLogs(state, world, pos);
    const bool isUprooted =
        state.get(BlockStateProperties::CREAKING_HEART_STATE()) == BlockStateProperties::CreakingHeartState::Uprooted;
    if (!logsOk || !isUprooted) {
        return state;
    }

    const auto newCreakingState = world.isCreakingActive(pos) ? BlockStateProperties::CreakingHeartState::Awake
                                                              : BlockStateProperties::CreakingHeartState::Dormant;
    return state.with(BlockStateProperties::CREAKING_HEART_STATE(), newCreakingState);
}

i32 CreakingHeartBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    auto creakingState = state.get(BlockStateProperties::CREAKING_HEART_STATE());
    switch (creakingState) {
        case BlockStateProperties::CreakingHeartState::Uprooted:
            return 0;
        case BlockStateProperties::CreakingHeartState::Dormant:
            return 1;
        case BlockStateProperties::CreakingHeartState::Awake:
            return 2;
        default:
            return 0;
    }
}

} // namespace blocks
} // namespace mc
