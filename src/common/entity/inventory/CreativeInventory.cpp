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

#include "CreativeInventory.hpp"
#include "PlayerInventory.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/StringUtils.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace mc {

namespace {

[[nodiscard]] std::string buildSearchKey(const ItemStack& stack)
{
    std::string searchKey = util::toLowerAscii(stack.getDisplayName()->getUnformattedText());

    if (const auto* item = stack.getItem(); item != nullptr) {
        searchKey.push_back(' ');
        searchKey += util::toLowerAscii(item->itemLocation().toString());
    }

    return searchKey;
}

} // namespace

std::vector<CreativeInventoryEntry> buildCreativePaletteEntries()
{
    std::vector<CreativeInventoryEntry> entries;

    Item::forEachItem([&entries](Item& item) {
        const i32 stackSize =
            item.isDamageable() ? 1 : std::clamp(item.maxStackSize(), 1, mc::item::DEFAULT_MAX_STACK_SIZE);
        CreativeInventoryEntry entry{ItemStack(item, stackSize), {}};
        entry.searchKey = buildSearchKey(entry.stack);
        entries.push_back(std::move(entry));
    });

    std::sort(
        entries.begin(), entries.end(), [](const CreativeInventoryEntry& left, const CreativeInventoryEntry& right) {
            const auto* leftItem = left.stack.getItem();
            const auto* rightItem = right.stack.getItem();

            const bool leftIsBlock = leftItem != nullptr && BlockItemRegistry::instance().isBlockItem(leftItem);
            const bool rightIsBlock = rightItem != nullptr && BlockItemRegistry::instance().isBlockItem(rightItem);
            if (leftIsBlock != rightIsBlock) {
                return leftIsBlock > rightIsBlock;
            }

            const std::string leftId = leftItem != nullptr ? leftItem->itemLocation().toString() : std::string();
            const std::string rightId = rightItem != nullptr ? rightItem->itemLocation().toString() : std::string();
            if (leftId != rightId) {
                return leftId < rightId;
            }

            auto leftName = left.stack.getDisplayName();
            auto rightName = right.stack.getDisplayName();
            return (leftName ? leftName->getUnformattedText() : std::string()) <
                (rightName ? rightName->getUnformattedText() : std::string());
        });

    return entries;
}

void fillCreativeModeInventory(PlayerInventory& inventory)
{
    inventory.clear();

    i32 slot = 0;
    Item* craftingTableItem = ItemRegistry::instance().getItem(ResourceLocation("minecraft:crafting_table"));
    BlockItem* craftingTableBlockItem = dynamic_cast<BlockItem*>(craftingTableItem);
    if (craftingTableBlockItem != nullptr && slot < PlayerInventory::TOTAL_SIZE) {
        inventory.setItem(slot, ItemStack(*craftingTableBlockItem, mc::item::DEFAULT_MAX_STACK_SIZE));
        ++slot;
    }

    std::vector<const BlockItem*> blockItems;
    blockItems.reserve(BlockItemRegistry::instance().size());
    BlockItemRegistry::instance().forEachBlockItem(
        [&blockItems](const BlockItem& item) { blockItems.push_back(&item); });

    std::sort(blockItems.begin(), blockItems.end(), [](const BlockItem* left, const BlockItem* right) {
        return left->itemLocation().toString() < right->itemLocation().toString();
    });

    for (const BlockItem* blockItem : blockItems) {
        if (blockItem == nullptr) {
            continue;
        }

        if (craftingTableBlockItem != nullptr && blockItem == craftingTableBlockItem) {
            continue;
        }

        if (slot >= PlayerInventory::TOTAL_SIZE) {
            break;
        }

        inventory.setItem(slot, ItemStack(*blockItem, mc::item::DEFAULT_MAX_STACK_SIZE));
        ++slot;
    }
}

} // namespace mc