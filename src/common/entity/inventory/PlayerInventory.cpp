#include "PlayerInventory.hpp"
#include "../entities/player/Player.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// PlayerInventory
// ============================================================================

PlayerInventory::PlayerInventory(Player* player)
    : m_player(player)
    , m_selectedSlot(0)
{
    // 初始化所有槽位为空
    for (auto& item : m_items) {
        item = ItemStack::EMPTY;
    }
}

bool PlayerInventory::isEmpty() const {
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack PlayerInventory::getItem(i32 slot) const {
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return ItemStack::EMPTY;
    }
    return m_items[slot];
}

void PlayerInventory::setItem(i32 slot, const ItemStack& stack) {
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return;
    }
    m_items[slot] = stack;
    m_timesChanged++;
}

ItemStack PlayerInventory::removeItem(i32 slot, i32 count) {
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return ItemStack::EMPTY;
    }

    ItemStack& stack = m_items[slot];
    if (stack.isEmpty()) {
        return ItemStack::EMPTY;
    }

    if (count >= stack.getCount()) {
        // 移除整个堆
        ItemStack result = stack;
        m_items[slot] = ItemStack::EMPTY;
        m_timesChanged++;
        return result;
    }

    // 移除部分
    ItemStack result = stack.split(count);
    m_timesChanged++;
    return result;
}

ItemStack PlayerInventory::removeItemNoUpdate(i32 slot) {
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return ItemStack::EMPTY;
    }

    ItemStack result = m_items[slot];
    m_items[slot] = ItemStack::EMPTY;
    return result;
}

void PlayerInventory::clear() {
    for (auto& item : m_items) {
        item = ItemStack::EMPTY;
    }
    m_timesChanged++;
}

void PlayerInventory::setChanged() {
    m_timesChanged++;
}

void PlayerInventory::serialize(network::PacketSerializer& ser) const {
    // 写入选中的槽位
    ser.writeI32(m_selectedSlot);

    // 写入物品数量
    i32 itemCount = 0;
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            itemCount++;
        }
    }
    ser.writeI32(itemCount);

    // 写入非空物品
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        if (!m_items[i].isEmpty()) {
            ser.writeI32(i);  // 槽位索引
            m_items[i].serialize(ser);
        }
    }
}

Result<PlayerInventory> PlayerInventory::deserialize(network::PacketDeserializer& deser) {
    PlayerInventory inventory(nullptr);

    // 读取选中的槽位
    auto selectedResult = deser.readI32();
    if (selectedResult.failed()) {
        return selectedResult.error();
    }
    inventory.m_selectedSlot = std::clamp(selectedResult.value(), 0, static_cast<i32>(HOTBAR_SIZE - 1));

    // 读取物品数量
    auto countResult = deser.readI32();
    if (countResult.failed()) {
        return countResult.error();
    }
    i32 itemCount = countResult.value();

    // 读取物品
    for (i32 i = 0; i < itemCount; ++i) {
        auto slotResult = deser.readI32();
        if (slotResult.failed()) {
            return slotResult.error();
        }
        i32 slot = slotResult.value();

        if (slot < 0 || slot >= TOTAL_SIZE) {
            return Error(ErrorCode::OutOfRange, "Invalid inventory slot index");
        }

        auto stackResult = ItemStack::deserialize(deser);
        if (stackResult.failed()) {
            return stackResult.error();
        }
        inventory.m_items[slot] = stackResult.value();
    }

    return inventory;
}

// ============================================================================
// 快捷栏操作
// ============================================================================

void PlayerInventory::setSelectedSlot(i32 slot) {
    m_selectedSlot = std::clamp(slot, 0, static_cast<i32>(HOTBAR_SIZE - 1));
}

ItemStack PlayerInventory::getSelectedStack() const {
    if (m_selectedSlot < 0 || m_selectedSlot >= HOTBAR_SIZE) {
        return ItemStack::EMPTY;
    }
    return m_items[m_selectedSlot];
}

i32 PlayerInventory::getBestHotbarSlot() const {
    // 首先寻找空槽位
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        if (m_items[i].isEmpty()) {
            return i;
        }
    }

    // 没有空槽位，返回当前选中的槽位
    return m_selectedSlot;
}

// ============================================================================
// 物品添加
// ============================================================================

i32 PlayerInventory::add(ItemStack& stack) {
    if (stack.isEmpty()) {
        return 0;
    }

    i32 originalCount = stack.getCount();

    // 首先尝试合并到现有堆叠
    // 优先检查快捷栏
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        if (canMergeStacks(m_items[i], stack)) {
            i32 maxStack = std::min(m_items[i].getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - m_items[i].getCount();
            i32 toAdd = std::min(space, stack.getCount());
            m_items[i].grow(toAdd);
            stack.shrink(toAdd);
            if (stack.isEmpty()) {
                return originalCount;
            }
        }
    }

    // 检查主背包
    for (i32 i = InventorySlots::MAIN_START; i <= InventorySlots::MAIN_END; ++i) {
        if (canMergeStacks(m_items[i], stack)) {
            i32 maxStack = std::min(m_items[i].getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - m_items[i].getCount();
            i32 toAdd = std::min(space, stack.getCount());
            m_items[i].grow(toAdd);
            stack.shrink(toAdd);
            if (stack.isEmpty()) {
                return originalCount;
            }
        }
    }

    // 寻找空槽位
    // 优先检查快捷栏
    i32 emptySlot = -1;
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        if (m_items[i].isEmpty()) {
            emptySlot = i;
            break;
        }
    }

    // 然后检查主背包
    if (emptySlot == -1) {
        for (i32 i = InventorySlots::MAIN_START; i <= InventorySlots::MAIN_END; ++i) {
            if (m_items[i].isEmpty()) {
                emptySlot = i;
                break;
            }
        }
    }

    if (emptySlot != -1) {
        m_items[emptySlot] = stack;
        stack = ItemStack::EMPTY;
        return originalCount;
    }

    // 返回已添加的数量
    return originalCount - stack.getCount();
}

i32 PlayerInventory::addInRange(ItemStack& stack, i32 start, i32 end) {
    if (stack.isEmpty() || start > end) {
        return 0;
    }

    i32 originalCount = stack.getCount();

    // 先尝试合并
    for (i32 i = start; i <= end; ++i) {
        if (canMergeStacks(m_items[i], stack)) {
            i32 maxStack = std::min(m_items[i].getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - m_items[i].getCount();
            i32 toAdd = std::min(space, stack.getCount());
            m_items[i].grow(toAdd);
            stack.shrink(toAdd);
            if (stack.isEmpty()) {
                return originalCount;
            }
        }
    }

    // 再找空槽位
    for (i32 i = start; i <= end; ++i) {
        if (m_items[i].isEmpty()) {
            m_items[i] = stack;
            stack = ItemStack::EMPTY;
            return originalCount;
        }
    }

    return originalCount - stack.getCount();
}

ItemStack PlayerInventory::addItemCopy(const ItemStack& stack) const {
    PlayerInventory tempCopy(nullptr);
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        tempCopy.m_items[i] = m_items[i];
    }

    ItemStack copy = stack.copy();
    tempCopy.add(copy);
    return copy;  // 返回剩余的物品
}

// ============================================================================
// 物品查找
// ============================================================================

i32 PlayerInventory::getFirstEmptySlot() const {
    // 先检查快捷栏
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        if (m_items[i].isEmpty()) {
            return i;
        }
    }

    // 再检查主背包
    for (i32 i = InventorySlots::MAIN_START; i <= InventorySlots::MAIN_END; ++i) {
        if (m_items[i].isEmpty()) {
            return i;
        }
    }

    return -1;
}

i32 PlayerInventory::findSlot(const Item& item) const {
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        if (m_items[i].getItem() == &item) {
            return i;
        }
    }
    return -1;
}

i32 PlayerInventory::findSlotMatching(const ItemStack& stack) const {
    if (stack.isEmpty()) {
        return -1;
    }

    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        if (!m_items[i].isEmpty() && stacksEqualExact(stack, m_items[i])) {
            return i;
        }
    }
    return -1;
}

i32 PlayerInventory::findSlotMatchingInRange(const ItemStack& stack, i32 start, i32 end) const {
    if (stack.isEmpty()) {
        return -1;
    }

    for (i32 i = start; i <= end; ++i) {
        if (i >= 0 && i < TOTAL_SIZE && !m_items[i].isEmpty() && stacksEqualExact(stack, m_items[i])) {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// 槽位操作
// ============================================================================

void PlayerInventory::swapSlots(i32 slot1, i32 slot2) {
    if (slot1 < 0 || slot1 >= TOTAL_SIZE || slot2 < 0 || slot2 >= TOTAL_SIZE) {
        return;
    }

    ItemStack temp = m_items[slot1];
    m_items[slot1] = m_items[slot2];
    m_items[slot2] = temp;
    m_timesChanged++;
}

ItemStack PlayerInventory::placeItem(i32 slot, ItemStack stack) {
    if (slot < 0 || slot >= TOTAL_SIZE || stack.isEmpty()) {
        return stack;
    }

    ItemStack& existing = m_items[slot];

    if (existing.isEmpty()) {
        // 槽位为空，直接放入
        m_items[slot] = stack;
        return ItemStack::EMPTY;
    }

    if (stacksEqualExact(existing, stack)) {
        // 相同物品，尝试合并
        i32 maxStack = std::min(existing.getMaxStackSize(), getMaxStackSize());
        i32 space = maxStack - existing.getCount();

        if (space > 0) {
            i32 toAdd = std::min(space, stack.getCount());
            existing.grow(toAdd);
            stack.shrink(toAdd);
            m_timesChanged++;

            if (stack.isEmpty()) {
                return ItemStack::EMPTY;
            }
        }
        return stack;
    }

    // 不同物品，交换
    ItemStack result = existing;
    m_items[slot] = stack;
    m_timesChanged++;
    return result;
}

// ============================================================================
// 统计
// ============================================================================

i32 PlayerInventory::countItem(const Item& item) const {
    i32 total = 0;
    for (const auto& stack : m_items) {
        if (stack.getItem() == &item) {
            total += stack.getCount();
        }
    }
    return total;
}

bool PlayerInventory::hasItem(const Item& item) const {
    for (const auto& stack : m_items) {
        if (stack.getItem() == &item) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// 私有方法
// ============================================================================

bool PlayerInventory::canMergeStacks(const ItemStack& stack1, const ItemStack& stack2) const {
    if (stack1.isEmpty() || stack2.isEmpty()) {
        return false;
    }

    // 检查是否可以合并
    if (!stacksEqualExact(stack1, stack2)) {
        return false;
    }

    // 检查堆叠数是否已达上限
    i32 maxStack = std::min(stack1.getMaxStackSize(), getMaxStackSize());
    return stack1.getCount() < maxStack;
}

bool PlayerInventory::stacksEqualExact(const ItemStack& stack1, const ItemStack& stack2) const {
    if (stack1.isEmpty() && stack2.isEmpty()) {
        return true;
    }
    if (stack1.isEmpty() || stack2.isEmpty()) {
        return false;
    }

    // 检查物品类型和耐久度
    return stack1.isSameItem(stack2) && stack1.getDamage() == stack2.getDamage();
    // TODO: 还需要检查NBT标签
}

} // namespace mc
