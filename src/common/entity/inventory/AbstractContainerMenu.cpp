#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/entities/player/Player.hpp"

namespace mc {

AbstractContainerMenu::AbstractContainerMenu(ContainerId id, PlayerInventory* playerInventory)
    : m_id(id)
    , m_playerInventory(playerInventory)
    , m_carried() {
}

Slot* AbstractContainerMenu::getSlot(i32 index) {
    if (index >= 0 && index < static_cast<i32>(m_slots.size())) {
        return m_slots[index].get();
    }
    return nullptr;
}

const Slot* AbstractContainerMenu::getSlot(i32 index) const {
    if (index >= 0 && index < static_cast<i32>(m_slots.size())) {
        return m_slots[index].get();
    }
    return nullptr;
}

i32 AbstractContainerMenu::addSlot(std::unique_ptr<Slot> slot) {
    i32 index = static_cast<i32>(m_slots.size());
    m_slots.push_back(std::move(slot));
    return index;
}

void AbstractContainerMenu::addPlayerInventorySlots(i32 startX, i32 startY) {
    m_playerInvStart = static_cast<i32>(m_slots.size());

    // 玩家主背包：3行9列（槽位 9-35 在 PlayerInventory 中）
    // 渲染顺序：从上到下，每行从左到右
    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            // PlayerInventory 中主背包从槽位 9 开始
            i32 playerSlotIndex = InventorySlots::MAIN_START + row * 9 + col;
            i32 posX = startX + col * 18;
            i32 posY = startY + row * 18;
            addSlot(std::make_unique<Slot>(m_playerInventory, playerSlotIndex, posX, posY));
        }
    }

    m_playerInvEnd = static_cast<i32>(m_slots.size()) - 1;
}

void AbstractContainerMenu::addPlayerHotbarSlots(i32 startX, i32 startY) {
    m_hotbarStart = static_cast<i32>(m_slots.size());

    // 快捷栏：1行9列（槽位 0-8 在 PlayerInventory 中）
    for (i32 col = 0; col < 9; ++col) {
        i32 posX = startX + col * 18;
        i32 posY = startY;
        addSlot(std::make_unique<Slot>(m_playerInventory, col, posX, posY));
    }

    m_hotbarEnd = static_cast<i32>(m_slots.size()) - 1;
}

void AbstractContainerMenu::addPlayerArmorSlots(i32 startX, i32 startY) {
    // 护甲槽位顺序（从上到下）：头盔、胸甲、护腿、靴子
    // 对应 PlayerInventory 的槽位 36-39
    constexpr i32 ARMOR_INDICES[] = {
        InventorySlots::ARMOR_HEAD,   // 36 头盔
        InventorySlots::ARMOR_CHEST,  // 37 胸甲
        InventorySlots::ARMOR_LEGS,   // 38 护腿
        InventorySlots::ARMOR_FEET    // 39 靴子
    };

    for (i32 i = 0; i < 4; ++i) {
        i32 posX = startX;
        i32 posY = startY + i * 18;
        addSlot(std::make_unique<ArmorSlot>(
            m_playerInventory,
            ARMOR_INDICES[i],
            posX, posY,
            static_cast<ArmorSlot::ArmorType>(i)
        ));
    }
}

void AbstractContainerMenu::addPlayerOffhandSlot(i32 x, i32 y) {
    // 副手槽对应 PlayerInventory 的槽位 40
    addSlot(std::make_unique<Slot>(m_playerInventory, InventorySlots::OFFHAND, x, y));
}

void AbstractContainerMenu::setCarriedItem(const ItemStack& stack) {
    m_carried = stack;
}

void AbstractContainerMenu::broadcastChanges() {
    for (i32 i = 0; i < static_cast<i32>(m_slots.size()); ++i) {
        const Slot* slot = m_slots[i].get();
        if (slot) {
            ItemStack stack = slot->getItem();
            notifySlotChanged(i, stack);
        }
    }
}

i32 AbstractContainerMenu::addListener(std::function<void(i32, ItemStack)> listener) {
    i32 id = m_nextListenerId++;
    m_listeners[id] = std::move(listener);
    return id;
}

void AbstractContainerMenu::removeListener(i32 listenerId) {
    m_listeners.erase(listenerId);
}

void AbstractContainerMenu::notifySlotChanged(i32 slotIndex, const ItemStack& stack) {
    for (auto& pair : m_listeners) {
        pair.second(slotIndex, stack);
    }
}

ItemStack AbstractContainerMenu::clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player) {
    (void)button;
    (void)clickType;
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr) {
        return ItemStack();
    }

    // 简化版实现：左键拾取/放置
    ItemStack slotStack = slot->getItem();
    if (m_carried.isEmpty()) {
        // 拾取物品
        if (!slotStack.isEmpty()) {
            m_carried = slotStack;
            slot->set(ItemStack());
        }
    } else {
        // 放置物品
        if (slotStack.isEmpty()) {
            slot->set(m_carried);
            m_carried = ItemStack();
        } else if (slotStack.isSameItem(m_carried)) {
            // 合并物品
            i32 space = slotStack.getMaxStackSize() - slotStack.getCount();
            i32 carriedCount = m_carried.getCount();
            i32 toAdd = (space < carriedCount) ? space : carriedCount;
            slotStack.grow(toAdd);
            slot->set(slotStack);
            m_carried.shrink(toAdd);
        }
    }

    broadcastChanges();
    return m_carried;
}

ItemStack AbstractContainerMenu::quickMoveStack(i32 slotIndex, Player& player) {
    (void)player;
    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack originalStack = slot->getItem();
    ItemStack resultStack = originalStack.copy();

    // 默认实现：在容器槽位和玩家背包之间移动
    // 子类应重写此方法以实现特定的快速移动逻辑
    if (slotIndex < m_playerInvStart) {
        // 从容器槽位移动到玩家背包
        if (!moveItemToRange(resultStack, m_playerInvStart, getSlotCount() - 1, true)) {
            return ItemStack();
        }
    } else {
        // 从玩家背包移动到容器槽位
        if (!moveItemToRange(resultStack, 0, m_playerInvStart - 1)) {
            return ItemStack();
        }
    }

    // 更新槽位
    if (resultStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->set(resultStack);
    }

    return originalStack;
}

bool AbstractContainerMenu::moveItemToRange(ItemStack& stack, i32 startIndex, i32 endIndex, bool reverse) {
    if (stack.isEmpty()) {
        return false;
    }

    bool moved = false;

    // 首先尝试合并到现有堆叠
    if (reverse) {
        for (i32 i = endIndex; i >= startIndex && !stack.isEmpty(); --i) {
            Slot* slot = getSlot(i);
            if (slot != nullptr && !slot->isEmpty()) {
                ItemStack existing = slot->getItem();
                if (existing.isSameItem(stack) && slot->mayPlace(stack)) {
                    i32 maxStack = slot->getMaxStackSize(stack);
                    i32 space = maxStack - existing.getCount();
                    if (space > 0) {
                        i32 toAdd = std::min(space, stack.getCount());
                        existing.grow(toAdd);
                        slot->set(existing);
                        stack.shrink(toAdd);
                        moved = true;
                    }
                }
            }
        }
    } else {
        for (i32 i = startIndex; i <= endIndex && !stack.isEmpty(); ++i) {
            Slot* slot = getSlot(i);
            if (slot != nullptr && !slot->isEmpty()) {
                ItemStack existing = slot->getItem();
                if (existing.isSameItem(stack) && slot->mayPlace(stack)) {
                    i32 maxStack = slot->getMaxStackSize(stack);
                    i32 space = maxStack - existing.getCount();
                    if (space > 0) {
                        i32 toAdd = std::min(space, stack.getCount());
                        existing.grow(toAdd);
                        slot->set(existing);
                        stack.shrink(toAdd);
                        moved = true;
                    }
                }
            }
        }
    }

    // 然后尝试放入空槽位
    if (!stack.isEmpty()) {
        if (reverse) {
            for (i32 i = endIndex; i >= startIndex && !stack.isEmpty(); --i) {
                Slot* slot = getSlot(i);
                if (slot != nullptr && slot->isEmpty() && slot->mayPlace(stack)) {
                    slot->set(stack);
                    stack = ItemStack();
                    moved = true;
                    break;
                }
            }
        } else {
            for (i32 i = startIndex; i <= endIndex && !stack.isEmpty(); ++i) {
                Slot* slot = getSlot(i);
                if (slot != nullptr && slot->isEmpty() && slot->mayPlace(stack)) {
                    slot->set(stack);
                    stack = ItemStack();
                    moved = true;
                    break;
                }
            }
        }
    }

    return moved;
}

void AbstractContainerMenu::removed(Player& player) {
    (void)player;
    if (!m_carried.isEmpty() && m_playerInventory) {
        m_carried = ItemStack();
    }
}

} // namespace mc
