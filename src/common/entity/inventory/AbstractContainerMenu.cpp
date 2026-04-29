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
    (void)player;

    // 特殊槽位索引：-999 表示点击了屏幕外部
    if (slotIndex == -999) {
        // 点击屏幕外部 - 丢弃鼠标物品
        if (clickType == ClickType::Throw) {
            ItemStack toDrop = m_carried.split(button == 1 ? m_carried.getCount() : 1);
            // TODO: 在世界中生成物品实体
            return m_carried;
        }
        return m_carried;
    }

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr) {
        return m_carried;
    }

    ItemStack slotStack = slot->getItem();

    switch (clickType) {
        case ClickType::Pick:
        case ClickType::PickSome:
            // 左键/右键拾取或放置
            return handleClickPick(*slot, slotIndex, slotStack, button);

        case ClickType::QuickMove:
            // Shift+点击快速移动
            return handleQuickMove(*slot, slotIndex, slotStack);

        case ClickType::Swap:
            // 数字键交换 (button = 0-8 对应快捷栏1-9, button = 40 对应副手)
            return handleSwap(*slot, slotIndex, slotStack, button);

        case ClickType::Clone:
            // 创造模式中键复制
            return handleClone(*slot, slotIndex, slotStack, player);

        case ClickType::Throw:
            // Q键丢弃
            return handleThrow(*slot, slotIndex, slotStack, button);

        case ClickType::QuickCraft:
            // 拖拽分发
            return handleQuickCraft(*slot, slotIndex, button);

        case ClickType::PickAll:
            // 双击拾取全部
            return handlePickupAll(*slot, slotIndex, slotStack);

        default:
            return m_carried;
    }
}

ItemStack AbstractContainerMenu::handleClickPick(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button) {
    // 左键
    if (button == 0) {
        if (m_carried.isEmpty()) {
            // 拾取整个槽位
            if (!slotStack.isEmpty() && slot.mayPickup(*m_playerInventory->getPlayer())) {
                m_carried = slot.remove(slotStack.getCount());
                slot.setChanged();
                notifySlotChanged(slotIndex, slot.getItem());
            }
        } else if (slotStack.isEmpty()) {
            // 放置物品到空槽位
            if (slot.mayPlace(m_carried)) {
                i32 toPlace = std::min(m_carried.getCount(), slot.getMaxStackSize(m_carried));
                slot.set(m_carried.split(toPlace));
                slot.setChanged();
                notifySlotChanged(slotIndex, slot.getItem());
            }
        } else if (m_carried.canMergeWith(slotStack)) {
            // 合并物品
            i32 space = slot.getMaxStackSize(m_carried) - slotStack.getCount();
            if (space > 0 && slot.mayPlace(m_carried)) {
                i32 toAdd = std::min(space, m_carried.getCount());
                ItemStack newStack = slotStack.copy();
                newStack.grow(toAdd);
                slot.set(newStack);
                m_carried.shrink(toAdd);
                slot.setChanged();
                notifySlotChanged(slotIndex, slot.getItem());
            }
        } else if (slot.mayPlace(m_carried) && m_carried.getCount() <= slot.getMaxStackSize(m_carried)) {
            // 交换物品
            slot.set(m_carried);
            m_carried = slotStack;
            slot.setChanged();
            notifySlotChanged(slotIndex, slot.getItem());
        }
    }
    // 右键
    else if (button == 1) {
        if (m_carried.isEmpty()) {
            // 拾取一半
            if (!slotStack.isEmpty() && slot.mayPickup(*m_playerInventory->getPlayer())) {
                i32 toTake = (slotStack.getCount() + 1) / 2;
                m_carried = slot.remove(toTake);
                slot.setChanged();
                notifySlotChanged(slotIndex, slot.getItem());
            }
        } else if (slotStack.isEmpty()) {
            // 放置一个物品
            if (slot.mayPlace(m_carried)) {
                ItemStack single = m_carried.split(1);
                slot.set(single);
                slot.setChanged();
                notifySlotChanged(slotIndex, slot.getItem());
            }
        } else if (m_carried.canMergeWith(slotStack)) {
            // 放置一个物品
            i32 space = slot.getMaxStackSize(m_carried) - slotStack.getCount();
            if (space > 0 && slot.mayPlace(m_carried)) {
                ItemStack newStack = slotStack.copy();
                newStack.grow(1);
                slot.set(newStack);
                m_carried.shrink(1);
                slot.setChanged();
                notifySlotChanged(slotIndex, slot.getItem());
            }
        } else if (slot.mayPlace(m_carried) && m_carried.getCount() == 1) {
            // 交换（右键只允许单个物品交换）
            slot.set(m_carried);
            m_carried = slotStack;
            slot.setChanged();
            notifySlotChanged(slotIndex, slot.getItem());
        }
    }

    return m_carried;
}

ItemStack AbstractContainerMenu::handleQuickMove(Slot& slot, i32 slotIndex, const ItemStack& slotStack) {
    if (slotStack.isEmpty()) {
        return m_carried;
    }

    ItemStack result = quickMoveStack(slotIndex, *m_playerInventory->getPlayer());
    if (!result.isEmpty()) {
        slot.setChanged();
        notifySlotChanged(slotIndex, slot.getItem());
    }
    return m_carried;
}

ItemStack AbstractContainerMenu::handleSwap(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button) {
    // button = 0-8 对应快捷栏槽位，button = 40 对应副手
    PlayerInventory* inv = m_playerInventory;
    if (!inv) {
        return m_carried;
    }

    i32 swapSlot = -1;
    if (button >= 0 && button <= 8) {
        swapSlot = button;  // 快捷栏
    } else if (button == 40) {
        swapSlot = InventorySlots::OFFHAND;  // 副手
    }

    if (swapSlot < 0) {
        return m_carried;
    }

    ItemStack swapStack = inv->getItem(swapSlot);

    // 检查是否可以交换
    if (slot.mayPlace(swapStack) && (swapStack.isEmpty() || slot.getMaxStackSize(swapStack) >= swapStack.getCount())) {
        if (swapStack.isEmpty() || slot.mayPickup(*m_playerInventory->getPlayer())) {
            // 执行交换
            slot.set(swapStack);
            inv->setItem(swapSlot, slotStack);
            slot.setChanged();
            notifySlotChanged(slotIndex, slotStack);
        }
    }

    return m_carried;
}

ItemStack AbstractContainerMenu::handleClone(Slot& slot, i32 slotIndex, const ItemStack& slotStack, Player& player) {
    // 创造模式中键复制 - 只在创造模式下可用
    // TODO: 检查玩家是否为创造模式
    (void)player;

    if (!slotStack.isEmpty() && m_carried.isEmpty()) {
        // 复制整组物品
        m_carried = slotStack.copy();
        m_carried.setCount(slotStack.getMaxStackSize());
    }

    return m_carried;
}

ItemStack AbstractContainerMenu::handleThrow(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button) {
    // Ctrl+点击丢弃全部 (button=1)，普通点击丢弃一个 (button=0)
    if (!slotStack.isEmpty()) {
        i32 toDrop = (button == 1) ? slotStack.getCount() : 1;
        ItemStack dropped = slot.remove(toDrop);
        slot.setChanged();
        notifySlotChanged(slotIndex, slot.getItem());
        // TODO: 在世界中生成物品实体
        (void)dropped;
    }

    return m_carried;
}

ItemStack AbstractContainerMenu::handleQuickCraft(Slot& slot, i32 slotIndex, i32 button) {
    // MC 1.16.5 拖拽分发状态机
    // button: 0=开始拖拽, 1=添加槽位, 2=结束拖拽
    // m_dragMode: 0=均匀分发(左键), 1=逐个分发(右键), 2=全部分发(中键)

    int prevDragEvent = m_dragEvent;
    m_dragEvent = getDragEvent(button);

    // 检查状态是否有效
    if ((prevDragEvent != 1 || m_dragEvent != 2) && prevDragEvent != m_dragEvent) {
        resetDrag();
    } else if (m_carried.isEmpty()) {
        resetDrag();
    } else if (m_dragEvent == 0) {
        // 开始拖拽 - 确定拖拽模式
        m_dragMode = extractDragMode(button);
        if (isValidDragMode(m_dragMode)) {
            m_dragEvent = 1;
            m_dragSlots.clear();
        } else {
            resetDrag();
        }
    } else if (m_dragEvent == 1) {
        // 添加槽位到拖拽列表
        ItemStack carried = m_carried;
        if (canDragIntoSlot(slot, carried) && slot.mayPlace(carried)) {
            if (m_dragMode == 2 || carried.getCount() > static_cast<i32>(m_dragSlots.size())) {
                m_dragSlots.push_back(slotIndex);
            }
        }
    } else if (m_dragEvent == 2) {
        // 结束拖拽 - 分发物品
        if (!m_dragSlots.empty()) {
            ItemStack toDistribute = m_carried.copy();
            int totalToDistribute = m_carried.getCount();

            // 首先计算每个槽位可以放入多少
            std::vector<std::pair<i32, i32>> slotAmounts; // slotIndex, amount
            for (i32 dragSlotIndex : m_dragSlots) {
                Slot* dragSlot = getSlot(dragSlotIndex);
                if (dragSlot == nullptr) continue;

                ItemStack existing = dragSlot->getItem();
                int maxStackSize = dragSlot->getMaxStackSize(toDistribute);
                int space = existing.isEmpty() ? maxStackSize : maxStackSize - existing.getCount();

                if (space > 0 && (existing.isEmpty() || existing.canMergeWith(toDistribute))) {
                    slotAmounts.push_back({dragSlotIndex, space});
                }
            }

            // 根据拖拽模式分发
            if (m_dragMode == 0) {
                // 均匀分发
                int slotsRemaining = static_cast<int>(slotAmounts.size());
                for (auto& [idx, space] : slotAmounts) {
                    if (totalToDistribute <= 0) break;

                    int perSlot = totalToDistribute / slotsRemaining;
                    if (perSlot == 0) perSlot = 1;
                    perSlot = std::min(perSlot, space);
                    perSlot = std::min(perSlot, totalToDistribute);

                    Slot* dragSlot = getSlot(idx);
                    if (dragSlot != nullptr) {
                        ItemStack existing = dragSlot->getItem();
                        if (existing.isEmpty()) {
                            dragSlot->set(toDistribute.split(perSlot));
                        } else {
                            existing.grow(perSlot);
                            dragSlot->set(existing);
                            toDistribute.shrink(perSlot);
                        }
                        dragSlot->setChanged();
                        notifySlotChanged(idx, dragSlot->getItem());
                    }
                    totalToDistribute -= perSlot;
                    slotsRemaining--;
                }
            } else if (m_dragMode == 1) {
                // 逐个分发 (右键拖拽)
                for (auto& [idx, space] : slotAmounts) {
                    if (totalToDistribute <= 0) break;

                    int amount = std::min(1, space);
                    amount = std::min(amount, totalToDistribute);

                    Slot* dragSlot = getSlot(idx);
                    if (dragSlot != nullptr) {
                        ItemStack existing = dragSlot->getItem();
                        if (existing.isEmpty()) {
                            ItemStack splitItem = toDistribute.split(amount);
                            dragSlot->set(splitItem);
                        } else {
                            existing.grow(amount);
                            dragSlot->set(existing);
                            toDistribute.shrink(amount);
                        }
                        dragSlot->setChanged();
                        notifySlotChanged(idx, dragSlot->getItem());
                    }
                    totalToDistribute -= amount;
                }
            } else if (m_dragMode == 2) {
                // 全部分发 (中键拖拽) - 尝试填满每个槽位
                for (auto& [idx, space] : slotAmounts) {
                    if (totalToDistribute <= 0) break;

                    int amount = std::min(space, totalToDistribute);

                    Slot* dragSlot = getSlot(idx);
                    if (dragSlot != nullptr) {
                        ItemStack existing = dragSlot->getItem();
                        if (existing.isEmpty()) {
                            dragSlot->set(toDistribute.split(amount));
                        } else {
                            existing.grow(amount);
                            dragSlot->set(existing);
                            toDistribute.shrink(amount);
                        }
                        dragSlot->setChanged();
                        notifySlotChanged(idx, dragSlot->getItem());
                    }
                    totalToDistribute -= amount;
                }
            }

            // 更新鼠标物品
            m_carried = toDistribute.isEmpty() ? ItemStack() : toDistribute;
        }
        resetDrag();
    } else {
        resetDrag();
    }

    (void)slot;
    return m_carried;
}

void AbstractContainerMenu::resetDrag() {
    m_dragEvent = 0;
    m_dragMode = 0;
    m_dragSlots.clear();
}

i32 AbstractContainerMenu::getDragEvent(i32 button) {
    // 从 button 中提取拖拽事件
    // MC 1.16.5: button 包含 dragMode (高位) 和 dragEvent (低位)
    return button & 0x3;
}

i32 AbstractContainerMenu::extractDragMode(i32 button) {
    // 从 button 中提取拖拽模式
    return (button >> 2) & 0x3;
}

bool AbstractContainerMenu::isValidDragMode(i32 dragMode) const {
    if (m_playerInventory == nullptr) return false;
    // 模式 2 (全部分发) 只在创造模式下有效
    // MC 1.16.5: 检查玩家是否为创造模式
    // TODO: 添加创造模式检查
    (void)dragMode;
    return true;
}

bool AbstractContainerMenu::canDragIntoSlot(Slot& slot, const ItemStack& stack) const {
    // 检查是否可以拖拽物品到该槽位
    if (slot.isEmpty()) {
        return slot.mayPlace(stack);
    }
    return slot.mayPlace(stack) && slot.getItem().canMergeWith(stack);
}

ItemStack AbstractContainerMenu::handlePickupAll(Slot& slot, i32 slotIndex, const ItemStack& slotStack) {
    // 双击拾取全部相同物品
    if (m_carried.isEmpty() && !slotStack.isEmpty()) {
        ItemStack toPickup = slotStack.copy();

        // 遍历所有槽位，合并相同物品
        for (i32 i = 0; i < getSlotCount(); ++i) {
            if (i == slotIndex) continue;

            Slot* otherSlot = getSlot(i);
            if (!otherSlot || otherSlot->isEmpty()) continue;

            ItemStack otherStack = otherSlot->getItem();
            if (toPickup.canMergeWith(otherStack) && otherSlot->mayPickup(*m_playerInventory->getPlayer())) {
                i32 space = toPickup.getMaxStackSize() - toPickup.getCount();
                if (space > 0) {
                    i32 toAdd = std::min(space, otherStack.getCount());
                    toPickup.grow(toAdd);
                    otherSlot->remove(toAdd);
                    otherSlot->setChanged();
                    notifySlotChanged(i, otherSlot->getItem());

                    if (toPickup.getCount() >= toPickup.getMaxStackSize()) {
                        break;
                    }
                }
            }
        }

        m_carried = toPickup;
        slot.remove(slotStack.getCount());
        slot.setChanged();
        notifySlotChanged(slotIndex, slot.getItem());
    }

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
