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

#include "StructureBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/special/StructureBlockEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== StructureBlock ==========

StructureBlock::StructureBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器，添加 MODE 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::STRUCTURE_MODE())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：DATA 模式
    setDefaultState(defaultState().with(BlockStateProperties::STRUCTURE_MODE(), Mode::Data));
}

StructureBlock::Mode StructureBlock::getMode(const BlockState& state) const
{
    return state.get(BlockStateProperties::STRUCTURE_MODE());
}

BlockState StructureBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 放置时默认为 DATA 模式
    MC_UNUSED(context);
    return defaultState();
}

std::unique_ptr<BlockEntity> StructureBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::StructureBlockEntity>(pos);
}

void StructureBlock::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    Block::onBlockPlacedBy(world, pos, state, stack);
    // TODO: 对齐 vanilla StructureBlock.setPlacedBy 的记录作者语义（放置者写入实体 author 字段）。
    //   本项目方块放置回调不携带放置者实体，需先扩展 Block::onBlockPlacedBy 签名后再接入。
    MC_UNUSED(pos);
}

BlockActionResult StructureBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    MC_UNUSED(state);
    MC_UNUSED(hit);

    // 对齐 vanilla StructureBlockEntity.usedBy：无实体或无管理员权限返回 PASS。
    auto* entity = dynamic_cast<blockentity::StructureBlockEntity*>(world.getBlockEntity(pos));
    if (entity == nullptr || !player.canUseGameMasterBlocks()) {
        return ActionResultType::Pass;
    }

    // 对齐 vanilla：客户端据下发的方块实体数据自行打开编辑界面（无独立 GUI 包）。
    player.openStructureBlock(pos);
    return ActionResultType::Success;
}

} // namespace blocks
} // namespace mc
