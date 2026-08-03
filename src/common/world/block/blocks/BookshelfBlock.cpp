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

#include "BookshelfBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/EnchantingTableEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

BookshelfBlock::BookshelfBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 书架没有状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));
}

// ========== 放置和移除 ==========

void BookshelfBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    _notifyNearbyEnchantingTables(world, pos);
}

void BookshelfBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    _notifyNearbyEnchantingTables(world, pos);
}

// ========== 私有方法 ==========

/*static*/ void BookshelfBlock::_notifyNearbyEnchantingTables(IWorld& world, const BlockPos& pos)
{
    // 书架可以影响附魔台的范围：水平2格、垂直0-1格
    // 因此需要通知以书架为中心，水平2格、垂直1格范围内的附魔台
    // 附魔台的有效书架偏移范围是 x∈[-2,2], y∈[0,1], z∈[-2,2]
    // 反过来，书架能影响的附魔台范围也是 x∈[-2,2], y∈[-1,0], z∈[-2,2]
    // 这里遍历更大的范围以确保不遗漏
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                BlockPos tablePos(pos.x + dx, pos.y + dy, pos.z + dz);

                BlockEntity* blockEntity = world.getBlockEntity(tablePos);
                if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::EnchantingTable) {
                    auto* enchantingTable = static_cast<blockentity::EnchantingTableEntity*>(blockEntity);
                    enchantingTable->recalculateEnchantPower(world);
                }
            }
        }
    }
}

} // namespace blocks
} // namespace mc
