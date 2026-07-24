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

#include "PlayerEnderChestInventory.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/storage/EnderChestEntity.hpp"
#include "util/assert/AssertAll.hpp"

#include <algorithm>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

PlayerEnderChestInventory::PlayerEnderChestInventory()
{
    for (auto& item : m_items) {
        item = ItemStack::EMPTY;
    }
}

// ============================================================================
// IInventory 接口实现
// ============================================================================

bool PlayerEnderChestInventory::isEmpty() const
{
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack PlayerEnderChestInventory::getItem(i32 slot) const
{
    if (slot < 0 || slot >= ENDER_CHEST_SIZE) {
        return ItemStack::EMPTY;
    }
    return m_items[static_cast<size_t>(slot)];
}

void PlayerEnderChestInventory::setItem(i32 slot, const ItemStack& stack)
{
    MC_ASSERT_RELEASE(slot >= 0 && slot < ENDER_CHEST_SIZE && "Slot index out of bounds");
    m_items[static_cast<size_t>(slot)] = stack;
    setChanged();
}

ItemStack PlayerEnderChestInventory::removeItem(i32 slot, i32 count)
{
    if (slot < 0 || slot >= ENDER_CHEST_SIZE || count <= 0) {
        return ItemStack::EMPTY;
    }

    ItemStack& stack = m_items[static_cast<size_t>(slot)];
    if (stack.isEmpty()) {
        return ItemStack::EMPTY;
    }

    const i32 actualCount = std::min(count, stack.getCount());
    ItemStack result = stack.split(actualCount);

    if (stack.isEmpty()) {
        m_items[static_cast<size_t>(slot)] = ItemStack::EMPTY;
    }

    setChanged();
    return result;
}

ItemStack PlayerEnderChestInventory::removeItemNoUpdate(i32 slot)
{
    if (slot < 0 || slot >= ENDER_CHEST_SIZE) {
        return ItemStack::EMPTY;
    }

    const size_t idx = static_cast<size_t>(slot);
    ItemStack result = std::move(m_items[idx]);
    m_items[idx] = ItemStack::EMPTY;
    return result;
}

void PlayerEnderChestInventory::clear()
{
    for (auto& item : m_items) {
        item = ItemStack::EMPTY;
    }
    setChanged();
}

void PlayerEnderChestInventory::setChanged()
{
    if (m_onChanged) {
        m_onChanged();
    }
    // 通知所有注册的 ContainerListener
    for (auto* listener : m_listeners) {
        listener->containerChanged(*this);
    }
}

bool PlayerEnderChestInventory::isUsableByPlayer(const Player& player) const
{
    // 如果有关联的末影箱方块实体，检查距离
    if (m_activeChest != nullptr) {
        return m_activeChest->canPlayerAccess(const_cast<Player*>(&player));
    }
    // 没有关联的方块实体时，始终允许访问（用于命令等）
    return true;
}

// ============================================================================
// IInventory 打开/关闭重写
// ============================================================================

void PlayerEnderChestInventory::openInventory(Player& player)
{
    startOpen(player);
}

void PlayerEnderChestInventory::closeInventory(Player& player)
{
    stopOpen(player);
}

// ============================================================================
// 末影箱特有功能
// ============================================================================

bool PlayerEnderChestInventory::isActiveChestValid() const
{
    return m_activeChest != nullptr;
}

void PlayerEnderChestInventory::addListener(ContainerListener* listener)
{
    if (listener != nullptr) {
        // 避免重复添加
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it == m_listeners.end()) {
            m_listeners.push_back(listener);
        }
    }
}

void PlayerEnderChestInventory::removeListener(ContainerListener* listener)
{
    if (listener != nullptr) {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it != m_listeners.end()) {
            m_listeners.erase(it);
        }
    }
}

void PlayerEnderChestInventory::startOpen(Player& player)
{
    if (m_activeChest != nullptr) {
        m_activeChest->openContainer(&player);
    }
}

void PlayerEnderChestInventory::stopOpen(Player& player)
{
    if (m_activeChest != nullptr) {
        m_activeChest->closeContainer(&player);
    }
    m_activeChest = nullptr;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void PlayerEnderChestInventory::toNbt(nbt::tags::compound_tag& tag) const
{
    using namespace entity::serialization::nbt_keys;

    auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();
    for (i32 i = 0; i < ENDER_CHEST_SIZE; ++i) {
        const ItemStack& stack = m_items[static_cast<size_t>(i)];
        if (stack.isEmpty()) {
            continue;
        }
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(i));
        stack.toNbt(itemTag);
        itemsList->value.push_back(std::move(itemTag));
    }
    tag.value.emplace(ENDER_ITEMS, std::move(itemsList));
}

void PlayerEnderChestInventory::fromNbt(const nbt::tags::compound_tag& tag)
{
    using namespace entity::serialization::nbt_helper;
    using namespace entity::serialization::nbt_keys;

    // 清空所有槽位
    for (auto& item : m_items) {
        item = ItemStack::EMPTY;
    }

    if (const auto* itemsList = tryGetList(tag, ENDER_ITEMS)) {
        if (itemsList->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
            for (const auto& itemTag : compoundList.value) {
                i8 slotIndex = 0;
                if (auto slotOpt = tryGetByte(itemTag, "Slot")) {
                    slotIndex = *slotOpt;
                } else {
                    continue;
                }

                if (slotIndex < 0 || slotIndex >= ENDER_CHEST_SIZE) {
                    continue;
                }

                auto stackResult = ItemStack::fromNbt(itemTag);
                if (stackResult.success() && !stackResult.value().isEmpty()) {
                    m_items[static_cast<size_t>(slotIndex)] = std::move(stackResult.value());
                }
            }
        }
    }
}

} // namespace mc
