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

#include "IInventory.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// IInventory 默认实现
// ============================================================================

bool IInventory::isUsableByPlayer(const Player& player) const
{
    // 默认实现：始终返回 true
    // 子类应重写此方法以检查距离
    (void)player;
    return true;
}

void IInventory::openInventory(Player& player)
{
    // 默认实现：空操作
    // 子类可以重写以实现打开计数、音效等功能
    (void)player;
}

void IInventory::closeInventory(Player& player)
{
    // 默认实现：空操作
    // 子类可以重写以实现关闭计数、物品返还等功能
    (void)player;
}

ItemStack IInventory::addItem(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return ItemStack();
    }

    ItemStack remaining = stack;

    // 首先尝试堆叠到已有物品
    for (i32 i = 0; i < getContainerSize() && !remaining.isEmpty(); ++i) {
        ItemStack existing = getItem(i);
        if (!existing.isEmpty() && existing.canStackWith(remaining)) {
            const i32 maxCount = std::min(getMaxStackSize(), existing.getMaxStackSize());
            const i32 space = maxCount - existing.getCount();

            if (space > 0) {
                const i32 toAdd = std::min(space, remaining.getCount());
                existing.grow(toAdd);
                remaining.shrink(toAdd);
                setItem(i, existing);
            }
        }
    }

    // 然后尝试放入空槽位
    for (i32 i = 0; i < getContainerSize() && !remaining.isEmpty(); ++i) {
        if (getItem(i).isEmpty() && canPlaceItem(i, remaining)) {
            const i32 maxCount = std::min(getMaxStackSize(), remaining.getMaxStackSize());
            const i32 toAdd = std::min(maxCount, remaining.getCount());

            setItem(i, remaining.split(toAdd));
        }
    }

    return remaining;
}

bool IInventory::canAddItem(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return true;
    }

    // 检查是否有空槽位或可堆叠物品
    for (i32 i = 0; i < getContainerSize(); ++i) {
        ItemStack existing = getItem(i);
        if (existing.isEmpty()) {
            // 空槽位
            if (canPlaceItem(i, stack)) {
                return true;
            }
        } else if (existing.canStackWith(stack)) {
            // 可堆叠物品
            const i32 maxCount = std::min(getMaxStackSize(), existing.getMaxStackSize());
            if (existing.getCount() < maxCount) {
                return true;
            }
        }
    }

    return false;
}

} // namespace mc
