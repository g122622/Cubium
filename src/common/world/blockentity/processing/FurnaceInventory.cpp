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

#include "world/blockentity/processing/FurnaceInventory.hpp"
#include "util/assert/AssertAll.hpp"

#include <algorithm>

namespace mc {
namespace blockentity {

FurnaceInventory::FurnaceInventory()
    : m_items{}
{
    // 所有槽位初始化为空物品堆
}

FurnaceInventory::FurnaceInventory(std::function<void()> onChanged)
    : m_items{}
    , m_onChanged(std::move(onChanged))
{}

bool FurnaceInventory::isEmpty() const
{
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack FurnaceInventory::getItem(i32 slot) const
{
    if (!_isValidSlot(slot)) {
        return ItemStack();
    }
    return m_items[static_cast<std::size_t>(slot)];
}

void FurnaceInventory::setItem(i32 slot, const ItemStack& stack)
{
    MC_ASSERT(_isValidSlot(slot) && "Slot index out of bounds");
    m_items[static_cast<std::size_t>(slot)] = stack;
    _onChanged();
}

ItemStack FurnaceInventory::removeItem(i32 slot, i32 count)
{
    if (!_isValidSlot(slot) || count <= 0) {
        return ItemStack();
    }

    ItemStack& stack = m_items[static_cast<std::size_t>(slot)];
    if (stack.isEmpty()) {
        return ItemStack();
    }

    const i32 actualCount = std::min(count, stack.getCount());
    ItemStack result = stack.split(actualCount);

    if (stack.isEmpty()) {
        m_items[static_cast<std::size_t>(slot)] = ItemStack();
    }

    _onChanged();
    return result;
}

ItemStack FurnaceInventory::removeItemNoUpdate(i32 slot)
{
    if (!_isValidSlot(slot)) {
        return ItemStack();
    }

    const std::size_t slotIndex = static_cast<std::size_t>(slot);
    ItemStack result = std::move(m_items[slotIndex]);
    m_items[slotIndex] = ItemStack();
    return result;
}

void FurnaceInventory::clear()
{
    for (auto& item : m_items) {
        item = ItemStack();
    }
    _onChanged();
}

void FurnaceInventory::setChanged()
{
    _onChanged();
}

bool FurnaceInventory::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    if (!_isValidSlot(slot) || stack.isEmpty()) {
        return false;
    }

    if (slot == SLOT_OUTPUT) {
        return false;
    }

    const ItemStack& existing = m_items[static_cast<std::size_t>(slot)];
    if (existing.isEmpty()) {
        return true;
    }

    if (!existing.canStackWith(stack)) {
        return false;
    }

    const i32 maxCount = existing.getMaxStackSize();
    return existing.getCount() + stack.getCount() <= maxCount;
}

ItemStack FurnaceInventory::addToOutput(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return ItemStack();
    }

    ItemStack& output = m_items[static_cast<std::size_t>(SLOT_OUTPUT)];

    if (output.isEmpty()) {
        // 输出槽为空，直接放入
        output = stack;
        _onChanged();
        return ItemStack();
    }

    // 检查是否可以堆叠
    if (output.canStackWith(stack)) {
        const i32 maxCount = output.getMaxStackSize();
        const i32 space = maxCount - output.getCount();

        if (space > 0) {
            const i32 toAdd = std::min(space, stack.getCount());
            output.grow(toAdd);

            ItemStack remaining = stack;
            remaining.shrink(toAdd);
            _onChanged();
            return remaining;
        }
    }

    // 无法堆叠，返回原物品
    return stack;
}

bool FurnaceInventory::canAcceptOutput(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return true;
    }

    const ItemStack& output = m_items[static_cast<std::size_t>(SLOT_OUTPUT)];

    if (output.isEmpty()) {
        return true;
    }

    if (!output.canStackWith(stack)) {
        return false;
    }

    const i32 maxCount = output.getMaxStackSize();
    return output.getCount() + stack.getCount() <= maxCount;
}

void FurnaceInventory::_onChanged()
{
    if (m_onChanged) {
        m_onChanged();
    }
    // 通知所有注册的 ContainerListener
    for (auto* listener : m_listeners) {
        listener->containerChanged(*this);
    }
}

void FurnaceInventory::addListener(ContainerListener* listener)
{
    if (listener != nullptr) {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it == m_listeners.end()) {
            m_listeners.push_back(listener);
        }
    }
}

void FurnaceInventory::removeListener(ContainerListener* listener)
{
    if (listener != nullptr) {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it != m_listeners.end()) {
            m_listeners.erase(it);
        }
    }
}

} // namespace blockentity
} // namespace mc
