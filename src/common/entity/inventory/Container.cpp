#include "Container.hpp"
#include "PlayerInventory.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

namespace {
    constexpr i32 SLOT_SPACING = 18;  // 槽位间距（像素）
    constexpr i32 PLAYER_INV_X = 8;    // 玩家背包X起始位置
    constexpr i32 PLAYER_INV_Y = 84;   // 玩家背包Y起始位置
    constexpr i32 HOTBAR_Y = 142;      // 快捷栏Y位置
}

// ============================================================================
// Container 实现
// ============================================================================

Container::Container(ContainerType type, ContainerId id)
    : m_type(type)
    , m_id(id)
{
    m_slots.reserve(64);  // 预分配槽位空间
}

// ========== 槽位管理 ==========

i32 Container::addSlot(std::unique_ptr<Slot> slot) {
    if (!slot) {
        return INVALID_SLOT;
    }

    i32 index = static_cast<i32>(m_slots.size());
    if (index >= MAX_SLOTS) {
        return INVALID_SLOT;
    }

    m_slots.push_back(std::move(slot));
    return index;
}

SlotRange Container::addInventorySlots(IInventory* inventory, i32 start, i32 count, i32 x, i32 y) {
    if (!inventory || count <= 0) {
        return SlotRange(0, 0);
    }

    i32 slotStart = static_cast<i32>(m_slots.size());

    for (i32 i = 0; i < count; ++i) {
        i32 slotIndex = start + i;
        i32 xPos = x + (i % 9) * SLOT_SPACING;
        i32 yPos = y + (i / 9) * SLOT_SPACING;

        auto slot = std::make_unique<Slot>(inventory, slotIndex, xPos, yPos);
        addSlot(std::move(slot));
    }

    return SlotRange(slotStart, static_cast<i32>(m_slots.size()));
}

Slot* Container::getSlot(i32 index) {
    if (index < 0 || index >= static_cast<i32>(m_slots.size())) {
        return nullptr;
    }
    return m_slots[index].get();
}

const Slot* Container::getSlot(i32 index) const {
    if (index < 0 || index >= static_cast<i32>(m_slots.size())) {
        return nullptr;
    }
    return m_slots[index].get();
}

ItemStack Container::getSlotItem(i32 index) const {
    const Slot* slot = getSlot(index);
    if (!slot) {
        return ItemStack::EMPTY;
    }
    return slot->getItem();
}

// ========== 物品操作 ==========

ItemStack Container::clicked(i32 slotIndex, i32 button, ClickType clickType, ItemStack cursorItem) {
    // 无效槽位
    if (slotIndex < 0 || slotIndex >= static_cast<i32>(m_slots.size())) {
        // 点击外部区域 - 丢弃物品
        if (clickType == ClickType::Pick && !cursorItem.isEmpty()) {
            return ItemStack::EMPTY;
        }
        return cursorItem;
    }

    Slot* slot = m_slots[slotIndex].get();
    if (!slot) {
        return cursorItem;
    }

    switch (clickType) {
        case ClickType::Pick:
        case ClickType::PickAll:
            return handlePickClick(slotIndex, button, cursorItem);

        case ClickType::QuickMove:
            return handleQuickMoveClick(slotIndex, cursorItem);

        case ClickType::Throw:
            return handleThrowClick(slotIndex, button, cursorItem);

        case ClickType::Pickup:
            return handleDragClick(slotIndex, button, cursorItem);

        case ClickType::Clone:
            return handleCloneClick(slotIndex, cursorItem);

        case ClickType::Swap:
            return handleSwapClick(slotIndex, button, cursorItem);

        default:
            return cursorItem;
    }
}

ItemStack Container::quickMoveStack(i32 slotIndex, ItemStack cursorItem) {
    return handleQuickMoveClick(slotIndex, cursorItem);
}

bool Container::mergeItem(ItemStack& stack, i32 start, i32 end, bool reverse) {
    if (stack.isEmpty()) {
        return false;
    }

    bool merged = false;
    i32 maxStackSize = stack.getMaxStackSize();

    // 首先尝试合并到现有堆叠
    if (reverse) {
        for (i32 i = end - 1; i >= start && !stack.isEmpty(); --i) {
            Slot* slot = getSlot(i);
            if (!slot || !slot->mayPlace(stack)) {
                continue;
            }

            ItemStack slotStack = slot->getItem();
            if (!slotStack.isEmpty() && slotStack.canMergeWith(stack)) {
                i32 space = maxStackSize - slotStack.getCount();
                i32 toAdd = std::min(space, stack.getCount());
                if (toAdd > 0) {
                    slotStack.grow(toAdd);
                    slot->set(slotStack);
                    stack.shrink(toAdd);
                    merged = true;
                    setChanged();
                    if (m_onSlotChanged) {
                        m_onSlotChanged(i);
                    }
                }
            }
        }
    } else {
        for (i32 i = start; i < end && !stack.isEmpty(); ++i) {
            Slot* slot = getSlot(i);
            if (!slot || !slot->mayPlace(stack)) {
                continue;
            }

            ItemStack slotStack = slot->getItem();
            if (!slotStack.isEmpty() && slotStack.canMergeWith(stack)) {
                i32 space = maxStackSize - slotStack.getCount();
                i32 toAdd = std::min(space, stack.getCount());
                if (toAdd > 0) {
                    slotStack.grow(toAdd);
                    slot->set(slotStack);
                    stack.shrink(toAdd);
                    merged = true;
                    setChanged();
                    if (m_onSlotChanged) {
                        m_onSlotChanged(i);
                    }
                }
            }
        }
    }

    // 然后尝试放入空槽位
    if (!stack.isEmpty()) {
        if (reverse) {
            for (i32 i = end - 1; i >= start && !stack.isEmpty(); --i) {
                Slot* slot = getSlot(i);
                if (!slot || !slot->mayPlace(stack)) {
                    continue;
                }

                if (slot->isEmpty()) {
                    i32 toPlace = std::min(maxStackSize, stack.getCount());
                    slot->set(stack.split(toPlace));
                    merged = true;
                    setChanged();
                    if (m_onSlotChanged) {
                        m_onSlotChanged(i);
                    }
                }
            }
        } else {
            for (i32 i = start; i < end && !stack.isEmpty(); ++i) {
                Slot* slot = getSlot(i);
                if (!slot || !slot->mayPlace(stack)) {
                    continue;
                }

                if (slot->isEmpty()) {
                    i32 toPlace = std::min(maxStackSize, stack.getCount());
                    slot->set(stack.split(toPlace));
                    merged = true;
                    setChanged();
                    if (m_onSlotChanged) {
                        m_onSlotChanged(i);
                    }
                }
            }
        }
    }

    return merged;
}

// ========== 同步 ==========

std::vector<ItemStack> Container::getAllSlots() const {
    std::vector<ItemStack> items;
    items.reserve(m_slots.size());

    for (const auto& slot : m_slots) {
        items.push_back(slot ? slot->getItem() : ItemStack::EMPTY);
    }

    return items;
}

void Container::setAllSlots(const std::vector<ItemStack>& items) {
    size_t count = std::min(items.size(), m_slots.size());

    for (size_t i = 0; i < count; ++i) {
        if (m_slots[i]) {
            m_slots[i]->set(items[i]);
        }
    }

    setChanged();
}

void Container::serialize(network::PacketSerializer& ser) const {
    // 写入容器类型和ID
    ser.writeU8(static_cast<u8>(m_type));
    ser.writeU8(static_cast<ContainerIdU8>(m_id));

    // 写入槽位数量
    ser.writeVarUInt(static_cast<u32>(m_slots.size()));

    // 写入每个槽位的物品
    for (const auto& slot : m_slots) {
        if (slot) {
            slot->getItem().serialize(ser);
        } else {
            ItemStack::EMPTY.serialize(ser);
        }
    }

    // 写入槽位范围
    ser.writeI32(m_playerInventoryRange.start);
    ser.writeI32(m_playerInventoryRange.end);
    ser.writeI32(m_containerInventoryRange.start);
    ser.writeI32(m_containerInventoryRange.end);
}

Result<std::unique_ptr<Container>> Container::deserialize(network::PacketDeserializer& deser) {
    // 读取容器类型和ID
    auto typeResult = deser.readU8();
    if (typeResult.failed()) return typeResult.error();

    auto idResult = deser.readU8();
    if (idResult.failed()) return idResult.error();

    auto container = std::make_unique<Container>(
        static_cast<ContainerType>(typeResult.value()),
        idResult.value()
    );

    // 读取槽位数量
    auto countResult = deser.readVarUInt();
    if (countResult.failed()) return countResult.error();
    u32 slotCount = countResult.value();

    // 读取每个槽位的物品
    // 注意：这里只读取数据，槽位需要在构造时添加
    // 所以这个反序列化需要配合特定的容器类型实现
    for (u32 i = 0; i < slotCount; ++i) {
        auto stackResult = ItemStack::deserialize(deser);
        if (stackResult.failed()) return stackResult.error();
        // 物品暂存，等待子类处理
    }

    // 读取槽位范围
    auto playerStartResult = deser.readI32();
    if (playerStartResult.failed()) return playerStartResult.error();

    auto playerEndResult = deser.readI32();
    if (playerEndResult.failed()) return playerEndResult.error();

    auto containerStartResult = deser.readI32();
    if (containerStartResult.failed()) return containerStartResult.error();

    auto containerEndResult = deser.readI32();
    if (containerEndResult.failed()) return containerEndResult.error();

    container->m_playerInventoryRange = SlotRange(playerStartResult.value(), playerEndResult.value());
    container->m_containerInventoryRange = SlotRange(containerStartResult.value(), containerEndResult.value());

    return container;
}

// ========== 保护方法 ==========

ItemStack Container::doQuickMove(i32 slotIndex, ItemStack cursorItem) {
    // 默认实现：在玩家背包和容器之间移动
    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();

    // 判断从哪个方向移动
    bool fromContainer = m_containerInventoryRange.contains(slotIndex);

    SlotRange targetRange = fromContainer ? m_playerInventoryRange : m_containerInventoryRange;

    if (targetRange.size() == 0) {
        return cursorItem;
    }

    // 尝试合并到目标范围
    ItemStack toMove = slotStack.copy();
    if (mergeItem(toMove, targetRange.start, targetRange.end, true)) {
        slot->set(toMove.isEmpty() ? ItemStack::EMPTY : toMove);
        setChanged();
    }

    return cursorItem;
}

bool Container::canQuickMove(i32 slotIndex) const {
    const Slot* slot = getSlot(slotIndex);
    return slot && !slot->isEmpty();
}

// ========== 私有方法 ==========

ItemStack Container::handlePickClick(i32 slotIndex, i32 button, ItemStack cursorItem) {
    Slot* slot = getSlot(slotIndex);
    if (!slot) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();

    // 左键
    if (button == 0) {
        if (cursorItem.isEmpty()) {
            // 拾取整个槽位
            if (!slotStack.isEmpty()) {
                slot->set(ItemStack::EMPTY);
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
                return slotStack;
            }
        } else if (slotStack.isEmpty()) {
            // 放置物品到空槽位
            if (slot->mayPlace(cursorItem)) {
                i32 toPlace = std::min(cursorItem.getCount(), slot->getMaxStackSize(cursorItem));
                slot->set(cursorItem.split(toPlace));
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
            }
        } else if (cursorItem.canMergeWith(slotStack)) {
            // 合并
            i32 space = slot->getMaxStackSize(cursorItem) - slotStack.getCount();
            i32 toAdd = std::min(space, cursorItem.getCount());
            if (toAdd > 0 && slot->mayPlace(cursorItem)) {
                slotStack.grow(toAdd);
                slot->set(slotStack);
                cursorItem.shrink(toAdd);
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
            }
        } else {
            // 交换
            if (slot->mayPlace(cursorItem) && cursorItem.getCount() <= slot->getMaxStackSize(cursorItem)) {
                slot->set(cursorItem);
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
                return slotStack;
            }
        }
    }
    // 右键
    else if (button == 1) {
        if (cursorItem.isEmpty()) {
            // 拾取一半
            if (!slotStack.isEmpty()) {
                i32 toTake = (slotStack.getCount() + 1) / 2;
                ItemStack result = slotStack.split(toTake);
                slot->set(slotStack);
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
                return result;
            }
        } else if (slotStack.isEmpty()) {
            // 放置一个物品
            if (slot->mayPlace(cursorItem)) {
                ItemStack single = cursorItem.split(1);
                slot->set(single);
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
            }
        } else if (cursorItem.canMergeWith(slotStack)) {
            // 放置一个物品
            i32 space = slot->getMaxStackSize(cursorItem) - slotStack.getCount();
            if (space > 0 && slot->mayPlace(cursorItem)) {
                slotStack.grow(1);
                slot->set(slotStack);
                cursorItem.shrink(1);
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
            }
        } else {
            // 交换
            if (slot->mayPlace(cursorItem) && cursorItem.getCount() == 1) {
                slot->set(cursorItem);
                setChanged();
                if (m_onSlotChanged) {
                    m_onSlotChanged(slotIndex);
                }
                return slotStack;
            }
        }
    }

    return cursorItem;
}

ItemStack Container::handleQuickMoveClick(i32 slotIndex, ItemStack cursorItem) {
    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return cursorItem;
    }

    if (!canQuickMove(slotIndex)) {
        return cursorItem;
    }

    return doQuickMove(slotIndex, cursorItem);
}

ItemStack Container::handleThrowClick(i32 slotIndex, i32 button, ItemStack cursorItem) {
    Slot* slot = getSlot(slotIndex);
    if (!slot) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();

    // Ctrl+点击丢弃全部
    if (button == 1) {
        if (!cursorItem.isEmpty()) {
            // 丢弃鼠标物品
            return ItemStack::EMPTY;
        }
        if (!slotStack.isEmpty()) {
            // 丢弃槽位全部
            slot->set(ItemStack::EMPTY);
            setChanged();
            if (m_onSlotChanged) {
                m_onSlotChanged(slotIndex);
            }
            // TODO: 生成物品实体 - 见 inventory/README "物品丢弃（Future Work）"
        }
    }
    // 普通丢弃
    else if (button == 0) {
        if (!cursorItem.isEmpty()) {
            // 丢弃一个
            cursorItem.shrink(1);
            // TODO: 生成物品实体 - 见 inventory/README "物品丢弃（Future Work）"
            if (cursorItem.isEmpty()) {
                return ItemStack::EMPTY;
            }
            return cursorItem;
        }
        if (!slotStack.isEmpty()) {
            // 丢弃槽位一个
            ItemStack dropped = slot->remove(1);
            setChanged();
            if (m_onSlotChanged) {
                m_onSlotChanged(slotIndex);
            }
            // TODO: 生成物品实体 - 见 inventory/README "物品丢弃（Future Work）"
        }
    }

    return cursorItem;
}

ItemStack Container::handleDragClick(i32 slotIndex, i32 button, ItemStack cursorItem) {
    Slot* slot = getSlot(slotIndex);
    if (!slot) {
        return cursorItem;
    }

    // 开始拖拽
    if (button == 0) {
        m_isDragging = true;
        m_dragButton = 0;
        m_dragSlots.clear();
        m_dragSlots.push_back(slotIndex);
    }
    // 继续拖拽
    else if (button == 1 && m_isDragging && m_dragButton == 0) {
        // 添加到拖拽槽位列表
        if (std::find(m_dragSlots.begin(), m_dragSlots.end(), slotIndex) == m_dragSlots.end()) {
            m_dragSlots.push_back(slotIndex);
        }
    }
    // 结束拖拽
    else if (button == 2 && m_isDragging) {
        if (m_dragSlots.size() > 1 && !cursorItem.isEmpty()) {
            i32 countPerSlot = cursorItem.getCount() / static_cast<i32>(m_dragSlots.size());

            for (i32 idx : m_dragSlots) {
                Slot* dragSlot = getSlot(idx);
                if (!dragSlot || !dragSlot->mayPlace(cursorItem)) {
                    continue;
                }

                ItemStack slotStack = dragSlot->getItem();
                if (slotStack.isEmpty()) {
                    dragSlot->set(cursorItem.split(countPerSlot));
                    setChanged();
                    if (m_onSlotChanged) {
                        m_onSlotChanged(idx);
                    }
                } else if (slotStack.canMergeWith(cursorItem)) {
                    i32 space = dragSlot->getMaxStackSize(cursorItem) - slotStack.getCount();
                    i32 toAdd = std::min(space, countPerSlot);
                    if (toAdd > 0) {
                        slotStack.grow(toAdd);
                        dragSlot->set(slotStack);
                        cursorItem.shrink(toAdd);
                        setChanged();
                        if (m_onSlotChanged) {
                            m_onSlotChanged(idx);
                        }
                    }
                }
            }
        }

        m_isDragging = false;
        m_dragButton = -1;
        m_dragSlots.clear();
    }

    return cursorItem;
}

ItemStack Container::handleSwapClick(i32 slotIndex, i32 button, ItemStack cursorItem) {
    // 数字键交换：button 是快捷栏索引 (0-8)
    if (button < 0 || button > 8) {
        return cursorItem;
    }

    // 获取快捷栏槽位
    i32 hotbarSlot = m_playerInventoryRange.start + button;
    if (!m_playerInventoryRange.contains(hotbarSlot)) {
        return cursorItem;
    }

    Slot* fromSlot = getSlot(slotIndex);
    Slot* toSlot = getSlot(hotbarSlot);

    if (!fromSlot || !toSlot) {
        return cursorItem;
    }

    ItemStack fromStack = fromSlot->getItem();
    ItemStack toStack = toSlot->getItem();

    // 检查是否可以交换
    if (fromStack.canMergeWith(toStack) && fromSlot->mayPlace(toStack) && toSlot->mayPlace(fromStack)) {
        i32 maxFrom = fromSlot->getMaxStackSize(toStack);
        i32 maxTo = toSlot->getMaxStackSize(fromStack);

        if (toStack.getCount() <= maxFrom && fromStack.getCount() <= maxTo) {
            fromSlot->set(toStack);
            toSlot->set(fromStack);
            setChanged();
            if (m_onSlotChanged) {
                m_onSlotChanged(slotIndex);
                m_onSlotChanged(hotbarSlot);
            }
        }
    } else if (toStack.isEmpty() && fromSlot->mayPlace(toStack)) {
        fromSlot->set(toStack);
        toSlot->set(fromStack);
        setChanged();
        if (m_onSlotChanged) {
            m_onSlotChanged(slotIndex);
            m_onSlotChanged(hotbarSlot);
        }
    } else if (fromStack.isEmpty() && toSlot->mayPlace(fromStack)) {
        fromSlot->set(toStack);
        toSlot->set(fromStack);
        setChanged();
        if (m_onSlotChanged) {
            m_onSlotChanged(slotIndex);
            m_onSlotChanged(hotbarSlot);
        }
    }

    return cursorItem;
}

ItemStack Container::handleCloneClick(i32 slotIndex, ItemStack cursorItem) {
    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return cursorItem;
    }

    // 创造模式：复制物品
    ItemStack slotStack = slot->getItem();
    return slotStack.copy();
}

// ============================================================================
// PlayerContainer 实现
// ============================================================================

PlayerContainer::PlayerContainer(PlayerInventory* playerInventory)
    : Container(ContainerType::Player, 0)
    , m_playerInventory(playerInventory)
{
    if (!playerInventory) {
        return;
    }

    // 添加快捷栏槽位 (0-8)
    for (i32 i = 0; i < PlayerInventory::HOTBAR_SIZE; ++i) {
        auto slot = std::make_unique<Slot>(
            playerInventory,
            i,
            PLAYER_INV_X + i * SLOT_SPACING,
            HOTBAR_Y
        );
        addSlot(std::move(slot));
    }

    // 添加主背包槽位 (9-35)
    for (i32 i = 0; i < PlayerInventory::MAIN_SIZE; ++i) {
        i32 row = i / 9;
        i32 col = i % 9;
        auto slot = std::make_unique<Slot>(
            playerInventory,
            InventorySlots::MAIN_START + i,
            PLAYER_INV_X + col * SLOT_SPACING,
            PLAYER_INV_Y + row * SLOT_SPACING
        );
        addSlot(std::move(slot));
    }

    // 添加护甲槽位 (36-39)
    constexpr i32 ARMOR_X = 8;
    constexpr i32 ARMOR_Y = 8;
    for (i32 i = 0; i < PlayerInventory::ARMOR_SIZE; ++i) {
        auto slot = std::make_unique<ArmorSlot>(
            playerInventory,
            InventorySlots::ARMOR_START + i,
            ARMOR_X,
            ARMOR_Y + i * SLOT_SPACING,
            static_cast<ArmorSlot::ArmorType>(i)
        );
        addSlot(std::move(slot));
    }

    // 添加副手槽位 (40)
    constexpr i32 OFFHAND_X = 77;
    constexpr i32 OFFHAND_Y = 62;
    auto offhandSlot = std::make_unique<Slot>(
        playerInventory,
        InventorySlots::OFFHAND,
        OFFHAND_X,
        OFFHAND_Y
    );
    addSlot(std::move(offhandSlot));

    // 设置槽位范围
    setPlayerInventoryRange(0, 36);  // 快捷栏 + 主背包
    setContainerInventoryRange(36, 41);  // 护甲 + 副手
}

ItemStack PlayerContainer::doQuickMove(i32 slotIndex, ItemStack cursorItem) {
    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();

    // 从护甲槽移动到主背包
    if (slotIndex >= InventorySlots::ARMOR_START && slotIndex <= InventorySlots::ARMOR_END) {
        ItemStack remaining = slotStack.copy();
        if (mergeItem(remaining, InventorySlots::HOTBAR_START, InventorySlots::MAIN_END + 1, true)) {
            slot->set(remaining.isEmpty() ? ItemStack::EMPTY : remaining);
            setChanged();
        }
    }
    // 从主背包移动到护甲槽（护甲类型检查由 ArmorSlot::mayPlace 处理）
    else if (slotIndex >= InventorySlots::MAIN_START && slotIndex <= InventorySlots::MAIN_END) {
        // 护甲槽通过 ArmorSlot::mayPlace 限制物品类型
    }
    // 从快捷栏移动到主背包
    else if (slotIndex >= InventorySlots::HOTBAR_START && slotIndex <= InventorySlots::HOTBAR_END) {
        ItemStack remaining = slotStack.copy();
        if (mergeItem(remaining, InventorySlots::MAIN_START, InventorySlots::MAIN_END + 1, false)) {
            slot->set(remaining.isEmpty() ? ItemStack::EMPTY : remaining);
            setChanged();
        }
    }

    return cursorItem;
}

bool PlayerContainer::canQuickMove(i32 slotIndex) const {
    const Slot* slot = getSlot(slotIndex);
    return slot && !slot->isEmpty();
}

} // namespace mc
