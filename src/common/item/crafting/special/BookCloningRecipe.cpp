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

#include "item/crafting/special/BookCloningRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/Items.hpp"
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace crafting {

BookCloningRecipe::BookCloningRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool BookCloningRecipe::matches(const CraftingInventory& inventory) const
{
    bool hasWrittenBook = false;
    bool hasWritableBook = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (_isWrittenBook(stack)) {
            if (hasWrittenBook) {
                // 只能有一本成书
                return false;
            }
            hasWrittenBook = true;
            // 检查代数，最多复制到第二代
            if (_getGeneration(stack) >= 2) {
                return false;
            }
        } else if (_isWritableBook(stack)) {
            hasWritableBook = true;
        } else {
            // 有其他物品，不匹配
            return false;
        }
    }

    return hasWrittenBook && hasWritableBook;
}

ItemStack BookCloningRecipe::assemble(const CraftingInventory& inventory) const
{
    ItemStack writtenBook;
    i32 writableBookCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (_isWrittenBook(stack)) {
            writtenBook = stack;
        } else if (_isWritableBook(stack)) {
            writableBookCount += stack.getCount();
        }
    }

    if (writtenBook.isEmpty() || writableBookCount == 0) {
        return ItemStack::EMPTY;
    }

    // 检查代数
    i32 generation = _getGeneration(writtenBook);
    if (generation >= 2) {
        return ItemStack::EMPTY;
    }

    // 创建复制的书
    ItemStack result = writtenBook.copy();
    result.setCount(writableBookCount);
    _setGeneration(result, generation + 1);

    return result;
}

std::vector<ItemStack> BookCloningRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    std::vector<ItemStack> remaining(inventory.getContainerSize());

    // 保留原书（只有一本）
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (_isWrittenBook(stack)) {
            ItemStack originalBook = stack.copy();
            originalBook.setCount(1);
            remaining[i] = originalBook;
            break;
        }
    }

    return remaining;
}

bool BookCloningRecipe::_isWrittenBook(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    const Item* item = stack.getItem();
    return item == Items::WRITTEN_BOOK;
}

bool BookCloningRecipe::_isWritableBook(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    const Item* item = stack.getItem();
    return item == Items::WRITABLE_BOOK;
}

i32 BookCloningRecipe::_getGeneration(const ItemStack& stack)
{
    // 从 NBT 标签获取代数
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr || !tag->is_object()) {
        return 0;
    }

    auto it = tag->find("generation");
    if (it == tag->end() || !it->is_number()) {
        return 0;
    }

    return it->get<i32>();
}

void BookCloningRecipe::_setGeneration(ItemStack& stack, i32 generation)
{
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["generation"] = generation;
}

} // namespace crafting
} // namespace mc
