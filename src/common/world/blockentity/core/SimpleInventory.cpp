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

#include "SimpleInventory.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerListener.hpp"
#include "common/item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

SimpleInventory::SimpleInventory(i32 size)
    : m_items(static_cast<std::size_t>(size))
{
    MC_ASSERT_RELEASE(size > 0 && "Inventory size must be positive");
}

SimpleInventory::SimpleInventory(i32 size, std::function<void()> onChanged)
    : m_items(static_cast<std::size_t>(size))
    , m_onChanged(std::move(onChanged))
{
    MC_ASSERT_RELEASE(size > 0 && "Inventory size must be positive");
}

SimpleInventory::SimpleInventory(SimpleInventory&& other) noexcept
    : m_items(std::move(other.m_items))
    , m_maxStackSize(other.m_maxStackSize)
    , m_onChanged(std::move(other.m_onChanged))
    , m_lootUnpackCallback(std::move(other.m_lootUnpackCallback))
    , m_listeners(std::move(other.m_listeners))
{
    // 重置源对象
    other.m_maxStackSize = mc::item::DEFAULT_MAX_STACK_SIZE;
    other.m_onChanged = nullptr;
    other.m_lootUnpackCallback = nullptr;
    other.m_listeners.clear();
}

SimpleInventory& SimpleInventory::operator=(SimpleInventory&& other) noexcept
{
    if (this != &other) {
        m_items = std::move(other.m_items);
        m_maxStackSize = other.m_maxStackSize;
        m_onChanged = std::move(other.m_onChanged);
        m_lootUnpackCallback = std::move(other.m_lootUnpackCallback);
        m_listeners = std::move(other.m_listeners);

        // 重置源对象
        other.m_maxStackSize = mc::item::DEFAULT_MAX_STACK_SIZE;
        other.m_onChanged = nullptr;
        other.m_lootUnpackCallback = nullptr;
        other.m_listeners.clear();
    }
    return *this;
}

bool SimpleInventory::isEmpty() const
{
    _unpackLoot();
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack SimpleInventory::getItem(i32 slot) const
{
    _unpackLoot();
    if (!_isValidSlot(slot)) {
        return ItemStack();
    }
    return m_items[static_cast<std::size_t>(slot)];
}

void SimpleInventory::setItem(i32 slot, const ItemStack& stack)
{
    _unpackLoot();
    MC_ASSERT_RELEASE(_isValidSlot(slot) && "Slot index out of bounds");
    const std::size_t slotIndex = static_cast<std::size_t>(slot);

    // 只在物品实际变化时触发回调
    if (m_items[slotIndex] == stack) {
        return;
    }

    m_items[slotIndex] = stack;
    _onChanged();
}

ItemStack SimpleInventory::removeItem(i32 slot, i32 count)
{
    _unpackLoot();
    if (!_isValidSlot(slot) || count <= 0) {
        return ItemStack();
    }

    ItemStack& stack = m_items[static_cast<std::size_t>(slot)];
    if (stack.isEmpty()) {
        return ItemStack();
    }

    // 限制取出数量不超过当前堆叠数量
    const i32 actualCount = std::min(count, stack.getCount());
    ItemStack result = stack.split(actualCount);

    // 如果堆叠变空，清空槽位
    if (stack.isEmpty()) {
        m_items[static_cast<std::size_t>(slot)] = ItemStack();
    }

    _onChanged();
    return result;
}

ItemStack SimpleInventory::removeItemNoUpdate(i32 slot)
{
    _unpackLoot();
    if (!_isValidSlot(slot)) {
        return ItemStack();
    }

    const std::size_t slotIndex = static_cast<std::size_t>(slot);
    ItemStack result = std::move(m_items[slotIndex]);
    m_items[slotIndex] = ItemStack();
    return result;
}

void SimpleInventory::clear()
{
    _unpackLoot();
    for (auto& item : m_items) {
        item = ItemStack();
    }
    _onChanged();
}

void SimpleInventory::setChanged()
{
    _onChanged();
}

bool SimpleInventory::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    if (!_isValidSlot(slot) || stack.isEmpty()) {
        return false;
    }

    const ItemStack& existing = m_items[static_cast<std::size_t>(slot)];
    if (existing.isEmpty()) {
        return true;
    }

    // 检查是否可以堆叠
    if (!existing.canStackWith(stack)) {
        return false;
    }

    // 检查堆叠数量限制
    const i32 maxCount = std::min(m_maxStackSize, existing.getMaxStackSize());
    return existing.getCount() + stack.getCount() <= maxCount;
}

ItemStack SimpleInventory::addItem(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return ItemStack();
    }

    ItemStack remaining = stack;

    // 首先尝试堆叠到已有物品
    for (std::size_t i = 0; i < m_items.size() && !remaining.isEmpty(); ++i) {
        ItemStack& existing = m_items[i];
        if (!existing.isEmpty() && existing.canStackWith(remaining)) {
            const i32 maxCount = std::min(m_maxStackSize, existing.getMaxStackSize());
            const i32 space = maxCount - existing.getCount();

            if (space > 0) {
                const i32 toAdd = std::min(space, remaining.getCount());
                existing.grow(toAdd);
                remaining.shrink(toAdd);
            }
        }
    }

    // 然后尝试放入空槽位
    for (std::size_t i = 0; i < m_items.size() && !remaining.isEmpty(); ++i) {
        if (m_items[i].isEmpty()) {
            const i32 maxCount = std::min(m_maxStackSize, remaining.getMaxStackSize());
            const i32 toAdd = std::min(maxCount, remaining.getCount());

            m_items[i] = remaining.split(toAdd);
        }
    }

    if (remaining.getCount() != stack.getCount()) {
        _onChanged();
    }

    return remaining;
}

bool SimpleInventory::canAddItem(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return true;
    }

    i32 remaining = stack.getCount();

    // 检查已有物品的可堆叠空间
    for (std::size_t i = 0; i < m_items.size() && remaining > 0; ++i) {
        const ItemStack& existing = m_items[i];
        if (!existing.isEmpty() && existing.canStackWith(stack)) {
            const i32 maxCount = std::min(m_maxStackSize, existing.getMaxStackSize());
            const i32 space = maxCount - existing.getCount();
            remaining -= std::min(space, remaining);
        }
    }

    // 检查空槽位
    for (std::size_t i = 0; i < m_items.size() && remaining > 0; ++i) {
        if (m_items[i].isEmpty()) {
            const i32 maxCount = std::min(m_maxStackSize, stack.getMaxStackSize());
            remaining -= maxCount;
        }
    }

    return remaining <= 0;
}

ItemStack SimpleInventory::extractItem(i32 slot)
{
    return removeItemNoUpdate(slot);
}

bool SimpleInventory::isSlotEmpty(i32 slot) const noexcept
{
    if (!_isValidSlot(slot)) {
        return true;
    }
    return m_items[static_cast<std::size_t>(slot)].isEmpty();
}

i32 SimpleInventory::getNonEmptySlotCount() const noexcept
{
    i32 count = 0;
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            ++count;
        }
    }
    return count;
}

void SimpleInventory::forEachItem(std::function<bool(i32 slot, const ItemStack& stack)> consumer) const
{
    for (std::size_t i = 0; i < m_items.size(); ++i) {
        if (!m_items[i].isEmpty()) {
            if (!consumer(static_cast<i32>(i), m_items[i])) {
                break;
            }
        }
    }
}

void SimpleInventory::_onChanged()
{
    if (m_onChanged) {
        m_onChanged();
    }
    // 通知所有注册的 ContainerListener
    for (auto* listener : m_listeners) {
        listener->containerChanged(*this);
    }
}

void SimpleInventory::_unpackLoot() const
{
    // 战利品表延迟填充回调：由 LootableContainerBlockEntity 子类注入。
    // 回调内部会调用 _unpackLootTable(nullptr)，使用 m_lootFilled 防止递归。
    if (m_lootUnpackCallback) {
        m_lootUnpackCallback();
    }
}

void SimpleInventory::addListener(ContainerListener* listener)
{
    if (listener != nullptr) {
        // 避免重复添加
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it == m_listeners.end()) {
            m_listeners.push_back(listener);
        }
    }
}

void SimpleInventory::removeListener(ContainerListener* listener)
{
    if (listener != nullptr) {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it != m_listeners.end()) {
            m_listeners.erase(it);
        }
    }
}

void SimpleInventory::load(const nlohmann::json& data)
{
    if (!data.is_array()) {
        return;
    }

    for (std::size_t i = 0; i < m_items.size(); ++i) {
        m_items[i] = ItemStack();
    }

    for (std::size_t i = 0; i < data.size() && i < m_items.size(); ++i) {
        const auto& itemJson = data[i];
        if (!itemJson.is_object() || itemJson.empty()) {
            continue;
        }

        auto stackResult = ItemStack::fromJson(itemJson);
        if (stackResult.success()) {
            m_items[i] = stackResult.value();
        }
    }
}

void SimpleInventory::save(nlohmann::json& data) const
{
    data = nlohmann::json::array();
    for (std::size_t i = 0; i < m_items.size(); ++i) {
        if (!m_items[i].isEmpty()) {
            data.push_back(m_items[i].toJson());
        } else {
            data.push_back(nlohmann::json::object());
        }
    }
}

} // namespace blockentity
} // namespace mc
