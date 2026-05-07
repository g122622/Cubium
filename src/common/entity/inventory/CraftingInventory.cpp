#include "entity/inventory/CraftingInventory.hpp"
#include "item/core/Item.hpp"
#include "network/packet/PacketSerializer.hpp"

namespace mc {

// ========== CraftingInventory ==========

CraftingInventory::CraftingInventory(i32 width, i32 height)
    : m_width(width)
    , m_height(height) {
    m_items.resize(width * height);
}

bool CraftingInventory::isEmpty() const {
    for (const ItemStack& stack : m_items) {
        if (!stack.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack CraftingInventory::getItem(i32 slot) const {
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return ItemStack();
    }
    return m_items[slot];
}

void CraftingInventory::setItem(i32 slot, const ItemStack& stack) {
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return;
    }
    m_items[slot] = stack;
    setChanged();
}

ItemStack CraftingInventory::removeItem(i32 slot, i32 count) {
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return ItemStack();
    }

    ItemStack& stack = m_items[slot];
    if (stack.isEmpty()) {
        return ItemStack();
    }

    ItemStack result = stack.split(count);
    if (!result.isEmpty()) {
        setChanged();
    }
    return result;
}

ItemStack CraftingInventory::removeItemNoUpdate(i32 slot) {
    if (slot < 0 || slot >= static_cast<i32>(m_items.size())) {
        return ItemStack();
    }

    ItemStack result = std::move(m_items[slot]);
    m_items[slot] = ItemStack();
    return result;
}

void CraftingInventory::clear() {
    for (ItemStack& stack : m_items) {
        stack = ItemStack();
    }
    setChanged();
}

void CraftingInventory::setChanged() {
    IInventory::setChanged();
    if (m_onContentChanged) {
        m_onContentChanged();
    }
}

void CraftingInventory::serialize(network::PacketSerializer& ser) const {
    ser.writeVarInt(static_cast<i32>(m_items.size()));
    for (const ItemStack& stack : m_items) {
        stack.serialize(ser);
    }
}

ItemStack CraftingInventory::getItemAt(i32 x, i32 y) const {
    i32 slot = posToSlot(x, y);
    if (slot < 0) {
        return ItemStack();
    }
    return m_items[slot];
}

void CraftingInventory::setItemAt(i32 x, i32 y, const ItemStack& stack) {
    i32 slot = posToSlot(x, y);
    if (slot < 0) {
        return;
    }
    m_items[slot] = stack;
    setChanged();
}

ItemStack CraftingInventory::removeItemAt(i32 x, i32 y, i32 count) {
    i32 slot = posToSlot(x, y);
    if (slot < 0) {
        return ItemStack();
    }
    return removeItem(slot, count);
}

i32 CraftingInventory::posToSlot(i32 x, i32 y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return -1;
    }
    return y * m_width + x;
}

bool CraftingInventory::slotToPos(i32 slot, i32& outX, i32& outY) const {
    if (slot < 0 || slot >= m_width * m_height) {
        return false;
    }
    outX = slot % m_width;
    outY = slot / m_width;
    return true;
}

void CraftingInventory::setItems(const std::vector<ItemStack>& items) {
    if (static_cast<i32>(items.size()) != m_width * m_height) {
        return; // 大小不匹配，忽略
    }
    m_items = items;
    setChanged();
}

bool CraftingInventory::isAllEmpty() const {
    return isEmpty();
}

bool CraftingInventory::getContentBounds(i32& outMinX, i32& outMinY,
                                          i32& outMaxX, i32& outMaxY) const {
    bool found = false;
    outMinX = m_width;
    outMinY = m_height;
    outMaxX = -1;
    outMaxY = -1;

    for (i32 y = 0; y < m_height; ++y) {
        for (i32 x = 0; x < m_width; ++x) {
            if (!getItemAt(x, y).isEmpty()) {
                found = true;
                outMinX = std::min(outMinX, x);
                outMinY = std::min(outMinY, y);
                outMaxX = std::max(outMaxX, x);
                outMaxY = std::max(outMaxY, y);
            }
        }
    }

    return found;
}

void CraftingInventory::fillStackedContents(std::unordered_map<i32, i32>& itemCounts) const {
    // 遍历所有物品，只计数"普通"物品（未损坏、未附魔、无自定义名称）
    // 参考: net.minecraft.item.crafting.RecipeItemHelper.accountPlainStack
    for (const ItemStack& stack : m_items) {
        if (stack.isEmpty()) {
            continue;
        }
        // 检查是否为"普通"物品
        // accountPlainStack: !stack.isDamaged() && !stack.isEnchanted() && !stack.hasDisplayName()
        if (!stack.isDamaged() && !stack.hasEnchantments() && !stack.hasDisplayName()) {
            // 获取物品ID
            const Item* item = stack.getItem();
            if (item != nullptr) {
                i32 itemId = static_cast<i32>(item->itemId());
                i32 count = stack.getCount();
                // 累加计数
                itemCounts[itemId] += count;
            }
        }
    }
}

// ========== CraftResultInventory ==========

ItemStack CraftResultInventory::getItem(i32 slot) const {
    // MC 1.16.5: CraftResultInventory 只有一个槽位，忽略 slot 参数
    // 参考: net.minecraft.inventory.CraftResultInventory.getStackInSlot
    (void)slot;
    return m_result;
}

void CraftResultInventory::setItem(i32 slot, const ItemStack& stack) {
    // MC 1.16.5: CraftResultInventory 只有一个槽位，忽略 slot 参数
    // 参考: net.minecraft.inventory.CraftResultInventory.setInventorySlotContents
    (void)slot;
    m_result = stack;
    setChanged();
}

ItemStack CraftResultInventory::removeItem(i32 slot, i32 count) {
    // MC 1.16.5: CraftResultInventory 的 removeItem 应该移除整个槽位的物品
    // 忽略 count 参数，直接返回整个结果物品
    // 参考: net.minecraft.inventory.CraftResultInventory.decrStackSize
    if (slot != 0 || m_result.isEmpty()) {
        return ItemStack();
    }

    // MC 1.16.5: 使用 getAndRemove 而不是 split
    ItemStack result = std::move(m_result);
    m_result = ItemStack();
    setChanged();
    return result;
    (void)count; // 忽略count参数
}

ItemStack CraftResultInventory::removeItemNoUpdate(i32 slot) {
    if (slot != 0) {
        return ItemStack();
    }
    ItemStack result = std::move(m_result);
    m_result = ItemStack();
    return result;
}

void CraftResultInventory::clear() {
    m_result = ItemStack();
    setChanged();
}

void CraftResultInventory::setChanged() {
    IInventory::setChanged();
}

void CraftResultInventory::serialize(network::PacketSerializer& ser) const {
    m_result.serialize(ser);
}

} // namespace mc
