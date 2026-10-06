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

#include "TestBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/interactive/TestBlockEntity.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

TestBlock::TestBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 测试方块持有 MODE 属性（start/log/fail/accept）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::TEST_BLOCK_MODE())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 对齐 vanilla：TestBlock 默认 MODE=FAIL
    setDefaultState(defaultState().with(BlockStateProperties::TEST_BLOCK_MODE(), BlockStateProperties::TestBlockMode::Fail));
}

std::unique_ptr<BlockEntity> TestBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::TestBlockEntity>(pos);
}

BlockState TestBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = defaultState();

    // 对齐 vanilla TestBlock#getStateForPlacement：从物品的 BLOCK_STATE 组件恢复 MODE。
    // 本项目物品组件以 BlockStateTag 子标签承载（见 BlockItem::applyBlockStateFromNBT）。
    const ItemStack& stack = context.getItemStack();
    if (const nlohmann::json* blockStateTag = stack.getChildTag("BlockStateTag")) {
        if (blockStateTag->is_object()) {
            auto it = blockStateTag->find("mode");
            if (it != blockStateTag->end() && it->is_string()) {
                auto parsed = BlockStateProperties::TEST_BLOCK_MODE().parse(it->get<std::string>());
                if (parsed.has_value()) {
                    state = state.with(BlockStateProperties::TEST_BLOCK_MODE(), parsed.value());
                }
            }
        }
    }

    return state;
}

BlockActionResult TestBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 对齐 vanilla TestBlock#useWithoutItem：无管理员权限返回 PASS；
    // 否则客户端打开测试方块界面（GUI 尚未实现，服务端返回 SUCCESS）。
    if (!player.canUseGameMasterBlocks()) {
        return ActionResultType::Pass;
    }

    // TODO: 打开测试方块界面（对齐 vanilla Player.openTestBlock → TestBlockEditScreen），
    //   当前仅返回成功，GUI 待命令方块界面体系接入后补全。
    return ActionResultType::Success;
}

void TestBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    // 对齐 vanilla TestBlock#tick：计划刻触发时重置测试方块实体
    if (BlockEntity* be = world.getBlockEntity(pos)) {
        if (auto* testBe = dynamic_cast<blockentity::TestBlockEntity*>(be)) {
            testBe->reset();
        }
    }
}

void TestBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    BlockEntity* be = world.getBlockEntity(pos);
    auto* testBe = dynamic_cast<blockentity::TestBlockEntity*>(be);
    if (testBe == nullptr) {
        return;
    }

    // 对齐 vanilla TestBlock#neighborChanged：START 模式不响应邻居信号（其输出由自身驱动）
    if (testBe->getMode() == BlockStateProperties::TestBlockMode::Start) {
        return;
    }

    const bool hasSignal = world::redstone::RedstonePower::isPowered(world, pos);
    const bool wasPowered = testBe->isPowered();
    if (hasSignal && !wasPowered) {
        testBe->setPowered(true);
        testBe->trigger();
    } else if (!hasSignal && wasPowered) {
        testBe->setPowered(false);
    }
}

i32 TestBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(side);

    // 对齐 vanilla TestBlock#getSignal：仅 START 模式输出，被充能时输出 15
    if (state.get(BlockStateProperties::TEST_BLOCK_MODE()) != BlockStateProperties::TestBlockMode::Start) {
        return 0;
    }

    if (const BlockEntity* be = world.getBlockEntity(pos)) {
        if (const auto* testBe = dynamic_cast<const blockentity::TestBlockEntity*>(be)) {
            return testBe->isPowered() ? 15 : 0;
        }
    }
    return 0;
}

} // namespace blocks
} // namespace mc
