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

#include "entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "core/Types.hpp"
#include "entity/entities/player/GameModeUtils.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/IInventory.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/core/Item.hpp"
#include "world/block/BlockPos.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

AbstractContainerMenu::AbstractContainerMenu(ContainerId id, PlayerInventory* playerInventory)
    : m_id(id)
    , m_playerInventory(playerInventory)
    , m_carried()
{
    // 初始化槽位状态缓存
    m_lastSlotStates.reserve(64); // 预分配容量
}

// ========== 静态工具方法 ==========

bool AbstractContainerMenu::isWithinDistance(const Player& player, const BlockPos& blockPos, f32 maxDistanceSq)
{
    // 检查玩家是否在指定方块附近
    return player.position().distanceSquared(blockPos.center()) <= maxDistanceSq;
}

Slot* AbstractContainerMenu::getSlot(i32 index)
{
    if (index >= 0 && index < static_cast<i32>(m_slots.size())) {
        return m_slots[index].get();
    }
    return nullptr;
}

const Slot* AbstractContainerMenu::getSlot(i32 index) const
{
    if (index >= 0 && index < static_cast<i32>(m_slots.size())) {
        return m_slots[index].get();
    }
    return nullptr;
}

i32 AbstractContainerMenu::addSlot(std::unique_ptr<Slot> slot)
{
    i32 index = static_cast<i32>(m_slots.size());
    m_slots.push_back(std::move(slot));
    // 同步扩展缓存
    m_lastSlotStates.push_back(ItemStack());
    return index;
}

void AbstractContainerMenu::addPlayerInventorySlots(i32 startX, i32 startY)
{
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

void AbstractContainerMenu::addPlayerHotbarSlots(i32 startX, i32 startY)
{
    m_hotbarStart = static_cast<i32>(m_slots.size());

    // 快捷栏：1行9列（槽位 0-8 在 PlayerInventory 中）
    for (i32 col = 0; col < 9; ++col) {
        i32 posX = startX + col * 18;
        i32 posY = startY;
        addSlot(std::make_unique<Slot>(m_playerInventory, col, posX, posY));
    }

    m_hotbarEnd = static_cast<i32>(m_slots.size()) - 1;
}

void AbstractContainerMenu::addPlayerArmorSlots(i32 startX, i32 startY)
{
    // 护甲槽位顺序（从上到下）：头盔、胸甲、护腿、靴子
    // 对应 PlayerInventory 的槽位 36-39
    constexpr i32 ARMOR_INDICES[] = {
        InventorySlots::ARMOR_HEAD,  // 36 头盔
        InventorySlots::ARMOR_CHEST, // 37 胸甲
        InventorySlots::ARMOR_LEGS,  // 38 护腿
        InventorySlots::ARMOR_FEET   // 39 靴子
    };

    for (i32 i = 0; i < 4; ++i) {
        i32 posX = startX;
        i32 posY = startY + i * 18;
        addSlot(std::make_unique<ArmorSlot>(
            m_playerInventory, ARMOR_INDICES[i], posX, posY, static_cast<ArmorSlot::ArmorType>(i)));
    }
}

void AbstractContainerMenu::addPlayerOffhandSlot(i32 x, i32 y)
{
    // 副手槽对应 PlayerInventory 的槽位 40
    addSlot(std::make_unique<Slot>(m_playerInventory, InventorySlots::OFFHAND, x, y));
}

void AbstractContainerMenu::setCarriedItem(const ItemStack& stack)
{
    m_carried = stack;
}

void AbstractContainerMenu::broadcastChanges()
{
    // 调用 detectAndSendChanges 实现变化检测
    detectAndSendChanges();
}

void AbstractContainerMenu::detectAndSendChanges()
{
    // 检查槽位变化
    for (i32 i = 0; i < static_cast<i32>(m_slots.size()); ++i) {
        const Slot* slot = m_slots[i].get();
        if (slot == nullptr) continue;

        ItemStack currentStack = slot->getItem();

        // 检查是否有变化
        bool changed = false;
        if (i >= static_cast<i32>(m_lastSlotStates.size())) {
            // 缓存不够大，扩展并填充空 ItemStack
            m_lastSlotStates.resize(i + 1);
            changed = !currentStack.isEmpty();
        } else {
            const ItemStack& lastStack = m_lastSlotStates[i];
            // 物品变化检测：检查物品类型、数量、NBT
            if (currentStack.isEmpty() != lastStack.isEmpty()) {
                changed = true;
            } else if (!currentStack.isEmpty() && !lastStack.isEmpty()) {
                if (currentStack.getItem() != lastStack.getItem() || currentStack.getCount() != lastStack.getCount() ||
                    !currentStack.isSameItem(lastStack)) {
                    changed = true;
                }
            }
        }

        if (changed) {
            // 更新缓存
            m_lastSlotStates[i] = currentStack.copy();
            // 通知监听器
            notifySlotChanged(i, currentStack);
        }
    }

    // 检查整型数据变化
    for (i32 i = 0; i < static_cast<i32>(m_trackedInts.size()); ++i) {
        IntReferenceHolder* holder = m_trackedInts[i].get();
        if (holder != nullptr && holder->isDirty()) {
            notifyIntChanged(i, holder->get());
        }
    }
}

i32 AbstractContainerMenu::addListener(std::function<void(i32, ItemStack)> listener)
{
    i32 id = m_nextListenerId++;
    m_listeners[id] = std::move(listener);
    return id;
}

void AbstractContainerMenu::removeListener(i32 listenerId)
{
    m_listeners.erase(listenerId);
}

i32 AbstractContainerMenu::addIntListener(std::function<void(i32, i32)> listener)
{
    i32 id = m_nextIntListenerId++;
    m_intListeners[id] = std::move(listener);
    return id;
}

void AbstractContainerMenu::removeIntListener(i32 listenerId)
{
    m_intListeners.erase(listenerId);
}

i32 AbstractContainerMenu::trackInt(std::unique_ptr<IntReferenceHolder> holder)
{
    i32 index = static_cast<i32>(m_trackedInts.size());
    m_trackedInts.push_back(std::move(holder));
    return index;
}

i32 AbstractContainerMenu::trackInt(std::function<i32()> getter, std::function<void(i32)> setter)
{
    return trackInt(std::make_unique<FunctionalIntReferenceHolder>(std::move(getter), std::move(setter)));
}

IntReferenceHolder* AbstractContainerMenu::getTrackedInt(i32 index)
{
    if (index >= 0 && index < static_cast<i32>(m_trackedInts.size())) {
        return m_trackedInts[index].get();
    }
    return nullptr;
}

const IntReferenceHolder* AbstractContainerMenu::getTrackedInt(i32 index) const
{
    if (index >= 0 && index < static_cast<i32>(m_trackedInts.size())) {
        return m_trackedInts[index].get();
    }
    return nullptr;
}

void AbstractContainerMenu::setTrackedInt(i32 index, i32 value)
{
    IntReferenceHolder* holder = getTrackedInt(index);
    if (holder != nullptr) {
        holder->set(value);
    }
}

void AbstractContainerMenu::notifyIntChanged(i32 index, i32 value)
{
    for (auto& pair : m_intListeners) {
        pair.second(index, value);
    }
}

bool AbstractContainerMenu::getCanCraft(const Player& player) const
{
    return m_cannotCraftPlayers.find(player.uuid()) == m_cannotCraftPlayers.end();
}

void AbstractContainerMenu::setCanCraft(const Player& player, bool canCraft)
{
    if (canCraft) {
        m_cannotCraftPlayers.erase(player.uuid());
    } else {
        m_cannotCraftPlayers.insert(player.uuid());
    }
}

void AbstractContainerMenu::putStackInSlot(i32 slotIndex, const ItemStack& stack)
{
    Slot* slot = getSlot(slotIndex);
    if (slot != nullptr) {
        slot->set(stack);
    }
}

void AbstractContainerMenu::setAll(const std::vector<ItemStack>& stacks)
{
    for (i32 i = 0; i < static_cast<i32>(m_slots.size()) && i < static_cast<i32>(stacks.size()); ++i) {
        Slot* slot = m_slots[i].get();
        if (slot != nullptr) {
            slot->set(stacks[i]);
        }
    }
}

void AbstractContainerMenu::clearContainer(Player& player, IInventory* inventory)
{
    if (inventory == nullptr) {
        return;
    }

    // 如果玩家死亡或断线，物品掉落到世界；否则尝试放回玩家背包
    // 检查玩家是否存活（死亡时物品直接掉落，不尝试放回背包）
    bool playerIsAlive = player.isAlive();

    for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
        ItemStack stack = inventory->removeItemNoUpdate(i);
        if (!stack.isEmpty()) {
            if (playerIsAlive && m_playerInventory != nullptr) {
                // 玩家存活且有背包，尝试放回玩家背包
                // add() 返回剩余未添加的数量，会修改 stack 的数量
                i32 remaining = m_playerInventory->add(stack);
                if (remaining > 0) {
                    // 放不下的物品掉落到世界
                    stack.setCount(remaining);
                    dropItem(stack, player, false);
                }
            } else {
                // 玩家死亡或没有背包，物品掉落到世界
                dropItem(stack, player, false);
            }
        }
    }
}

void AbstractContainerMenu::notifySlotChanged(i32 slotIndex, const ItemStack& stack)
{
    for (auto& pair : m_listeners) {
        pair.second(slotIndex, stack);
    }
}

ItemStack AbstractContainerMenu::clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player)
{
    // 特殊槽位索引：-999 表示点击了屏幕外部
    if (slotIndex == -999) {
        // 点击屏幕外部 - 根据点击类型处理光标物品
        if (m_carried.isEmpty()) {
            return m_carried;
        }

        switch (clickType) {
            case ClickType::Pick:
                // 左键点击外部：丢弃光标上全部物品
                dropItem(m_carried, player, false);
                m_carried = ItemStack();
                break;

            case ClickType::PickSome:
                // 右键点击外部：丢弃光标上一个物品
                {
                    ItemStack toDrop = m_carried.split(1);
                    dropItem(toDrop, player, false);
                }
                break;

            case ClickType::Throw:
                // Q键丢弃：button=0丢一个，button=1丢全部
                {
                    ItemStack toDrop = m_carried.split(button == 1 ? m_carried.getCount() : 1);
                    dropItem(toDrop, player, false);
                }
                break;

            case ClickType::ThrowAll:
                // Ctrl+Q丢弃全部
                dropItem(m_carried, player, false);
                m_carried = ItemStack();
                break;

            case ClickType::QuickCraft:
                // 拖拽分发的 START 和 END 事件使用 -999 槽位
                // START：初始化拖拽状态（不需要槽位引用）
                // END：分发物品到所有目标槽位（不需要当前槽位引用）
                // 单槽降级时需要 player 进行递归调用
                _handleQuickCraftStartEnd(button, player);
                break;

            default:
                // 其他点击类型在外部不做处理
                break;
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
            return _handleClickPick(*slot, slotIndex, slotStack, button);

        case ClickType::QuickMove:
            // Shift+点击快速移动
            return _handleQuickMove(*slot, slotIndex, slotStack);

        case ClickType::Swap:
            // 数字键交换 (button = 0-8 对应快捷栏1-9, button = 40 对应副手)
            return _handleSwap(*slot, slotIndex, slotStack, button);

        case ClickType::Clone:
            // 创造模式中键复制
            return _handleClone(*slot, slotIndex, slotStack, player);

        case ClickType::Throw:
            // Q键丢弃
            return _handleThrow(*slot, slotIndex, slotStack, button);

        case ClickType::QuickCraft:
            // 拖拽分发
            return _handleQuickCraft(*slot, slotIndex, button, player);

        case ClickType::PickAll:
            // 双击拾取全部
            return _handlePickupAll(*slot, slotIndex, slotStack);

        default:
            return m_carried;
    }
}

ItemStack AbstractContainerMenu::_handleClickPick(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button)
{
    Player* player = m_playerInventory->getPlayer();

    // 槽位覆写协议（对应 MC 1.21.11 AbstractContainerMenu#tryItemClickBehaviourOverride）
    // 在常规拾取/放置逻辑之前调用，给物品机会自定义交互行为（如收纳袋）
    // - 左键（button == 0）→ SlotClickAction::Primary
    // - 右键（button == 1）→ SlotClickAction::Secondary
    SlotClickAction clickAction = (button == 0) ? SlotClickAction::Primary : SlotClickAction::Secondary;
    if (_tryItemClickBehaviourOverride(slot, clickAction, *player)) {
        // 物品已处理此次点击，跳过默认逻辑
        notifySlotChanged(slotIndex, slot.getItem());
        return m_carried;
    }

    // 左键
    if (button == 0) {
        if (m_carried.isEmpty()) {
            // 拾取整个槽位
            if (!slotStack.isEmpty() && slot.mayPickup(*player)) {
                m_carried = slot.remove(slotStack.getCount());
                // 取走物品时触发 onTake 回调
                m_carried = slot.onTake(*player, m_carried);
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
            // 交换时触发 onTake（取走原槽位物品）
            m_carried = slot.onTake(*player, m_carried);
            slot.setChanged();
            notifySlotChanged(slotIndex, slot.getItem());
        }
    }
    // 右键
    else if (button == 1) {
        if (m_carried.isEmpty()) {
            // 拾取一半
            if (!slotStack.isEmpty() && slot.mayPickup(*player)) {
                i32 toTake = (slotStack.getCount() + 1) / 2;
                m_carried = slot.remove(toTake);
                // 取走物品时触发 onTake 回调
                m_carried = slot.onTake(*player, m_carried);
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
            // 交换时触发 onTake（取走原槽位物品）
            m_carried = slot.onTake(*player, m_carried);
            slot.setChanged();
            notifySlotChanged(slotIndex, slot.getItem());
        }
    }

    return m_carried;
}

ItemStack AbstractContainerMenu::_handleQuickMove(Slot& slot, i32 slotIndex, const ItemStack& slotStack)
{
    if (slotStack.isEmpty()) {
        return m_carried;
    }

    ItemStack result = quickMoveStack(slotIndex, *m_playerInventory->getPlayer());
    if (!result.isEmpty()) {
        // 快速移动后触发 onTake 回调
        // 计算实际取出的数量
        ItemStack taken = result;
        slot.onTake(*m_playerInventory->getPlayer(), taken);
        slot.setChanged();
        notifySlotChanged(slotIndex, slot.getItem());
    }
    return m_carried;
}

ItemStack AbstractContainerMenu::_handleSwap(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button)
{
    // button = 0-8 对应快捷栏槽位，button = 40 对应副手
    PlayerInventory* inv = m_playerInventory;
    if (!inv) {
        return m_carried;
    }

    i32 swapSlot = -1;
    if (button >= 0 && button <= 8) {
        swapSlot = button; // 快捷栏
    } else if (button == 40) {
        swapSlot = InventorySlots::OFFHAND; // 副手
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

ItemStack AbstractContainerMenu::_handleClone(Slot& slot, i32 slotIndex, const ItemStack& slotStack, Player& player)
{
    // 创造模式中键复制 - 只在创造模式下可用
    if (!entity::GameModeUtils::isCreative(player.gameMode())) {
        return m_carried;
    }

    if (!slotStack.isEmpty() && m_carried.isEmpty()) {
        // 复制整组物品
        m_carried = slotStack.copy();
        m_carried.setCount(slotStack.getMaxStackSize());
    }

    (void)slotIndex;
    return m_carried;
}

ItemStack AbstractContainerMenu::_handleThrow(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button)
{
    // Ctrl+点击丢弃全部 (button=1)，普通点击丢弃一个 (button=0)
    if (!slotStack.isEmpty()) {
        i32 toDrop = (button == 1) ? slotStack.getCount() : 1;
        ItemStack dropped = slot.remove(toDrop);
        slot.setChanged();
        notifySlotChanged(slotIndex, slot.getItem());
        // 丢弃物品到世界
        dropItem(dropped, *m_playerInventory->getPlayer(), false);
    }

    return m_carried;
}

ItemStack AbstractContainerMenu::_handleQuickCraft(Slot& slot, i32 slotIndex, i32 button, Player& player)
{
    // 拖拽分发状态机
    // m_dragMode: 0=均匀分发(左键), 1=逐个分发(右键), 2=全部分发(中键)

    i32 prevDragEvent = m_dragEvent;
    m_dragEvent = _getDragEvent(button);

    // 检查状态是否有效
    if ((prevDragEvent != DragConstants::EVENT_ADD_SLOT || m_dragEvent != DragConstants::EVENT_END) &&
        prevDragEvent != m_dragEvent) {
        _resetDrag();
    } else if (m_carried.isEmpty()) {
        _resetDrag();
    } else if (m_dragEvent == DragConstants::EVENT_START) {
        // 开始拖拽 - 确定拖拽模式
        m_dragMode = _extractDragMode(button);
        if (_isValidDragMode(m_dragMode)) {
            m_dragEvent = DragConstants::EVENT_ADD_SLOT;
            m_dragSlots.clear();
        } else {
            _resetDrag();
        }
    } else if (m_dragEvent == DragConstants::EVENT_ADD_SLOT) {
        // 添加槽位到拖拽列表
        ItemStack carried = m_carried;
        if (_canDragIntoSlot(slot, carried) && slot.mayPlace(carried)) {
            if (m_dragMode == DragConstants::MODE_FILL || carried.getCount() > static_cast<i32>(m_dragSlots.size())) {
                m_dragSlots.push_back(slotIndex);
            }
        }
    } else if (m_dragEvent == DragConstants::EVENT_END) {
        // 结束拖拽 - 单槽降级或多槽分发
        // 对应 MC 1.21.11 AbstractContainerMenu#doClick 中 quickcraftSlots.size()==1 的降级路径：
        // 当仅有一个拖拽槽位时，重置拖拽状态后递归调用 clicked(slotIndex, dragMode, Pick, player)，
        // 让单槽拖拽降级为普通 PICKUP 点击，从而触发 _tryItemClickBehaviourOverride
        // （收纳袋的 overrideStackedOnOther/overrideOtherStackedOnMe）。
        if (m_dragSlots.size() == 1) {
            const i32 singleSlotIndex = m_dragSlots[0];
            const i32 dragMode = m_dragMode;
            _resetDrag();
            // MODE_EVEN(0)/MODE_SINGLE(1) 分别对应 PICKUP 的左/右键（button==0/1），
            // 与 MC Java 行为一致；MODE_FILL(2) 不进入 PICKUP 分支（创造模式专属，
            // 单槽时无意义，直接返回）。
            if (dragMode == DragConstants::MODE_EVEN || dragMode == DragConstants::MODE_SINGLE) {
                return clicked(singleSlotIndex, dragMode, ClickType::Pick, player);
            }
            return m_carried;
        }

        // 多槽分发
        if (!m_dragSlots.empty()) {
            ItemStack toDistribute = m_carried.copy();

            // 计算每个槽位可以放入多少
            std::vector<std::pair<i32, i32>> slotAmounts; // slotIndex, space
            for (i32 dragSlotIndex : m_dragSlots) {
                Slot* dragSlot = getSlot(dragSlotIndex);
                if (dragSlot == nullptr) continue;

                ItemStack existing = dragSlot->getItem();
                i32 maxStackSize = dragSlot->getMaxStackSize(toDistribute);
                i32 space = existing.isEmpty() ? maxStackSize : maxStackSize - existing.getCount();

                if (space > 0 && (existing.isEmpty() || existing.canMergeWith(toDistribute))) {
                    slotAmounts.push_back({dragSlotIndex, space});
                }
            }

            // 根据拖拽模式分发
            if (m_dragMode == DragConstants::MODE_EVEN) {
                // 均匀分发
                i32 slotsRemaining = static_cast<i32>(slotAmounts.size());
                for (auto& [idx, space] : slotAmounts) {
                    if (toDistribute.isEmpty()) break;

                    i32 perSlot = toDistribute.getCount() / slotsRemaining;
                    if (perSlot == 0) perSlot = 1;
                    perSlot = std::min(perSlot, space);
                    _distributeToDragSlot(toDistribute, idx, perSlot);
                    slotsRemaining--;
                }
            } else if (m_dragMode == DragConstants::MODE_SINGLE) {
                // 逐个分发 (右键拖拽)
                for (auto& [idx, space] : slotAmounts) {
                    if (toDistribute.isEmpty()) break;
                    _distributeToDragSlot(toDistribute, idx, 1);
                }
            } else if (m_dragMode == DragConstants::MODE_FILL) {
                // 全部分发 (中键拖拽) - 尝试填满每个槽位
                for (auto& [idx, space] : slotAmounts) {
                    if (toDistribute.isEmpty()) break;
                    _distributeToDragSlot(toDistribute, idx, space);
                }
            }

            // 更新鼠标物品
            m_carried = toDistribute.isEmpty() ? ItemStack() : toDistribute;
        }
        _resetDrag();
    } else {
        _resetDrag();
    }

    (void)slot;
    return m_carried;
}

void AbstractContainerMenu::_handleQuickCraftStartEnd(i32 button, Player& player)
{
    // 拖拽分发的 START 和 END 事件使用 -999 槽位
    // 这部分逻辑不需要访问具体槽位

    i32 prevDragEvent = m_dragEvent;
    m_dragEvent = _getDragEvent(button);

    // 检查状态是否有效
    if ((prevDragEvent != DragConstants::EVENT_ADD_SLOT || m_dragEvent != DragConstants::EVENT_END) &&
        prevDragEvent != m_dragEvent) {
        _resetDrag();
        return;
    }

    if (m_carried.isEmpty()) {
        _resetDrag();
        return;
    }

    if (m_dragEvent == DragConstants::EVENT_START) {
        // 开始拖拽 - 确定拖拽模式
        m_dragMode = _extractDragMode(button);
        if (_isValidDragMode(m_dragMode)) {
            m_dragEvent = DragConstants::EVENT_ADD_SLOT;
            m_dragSlots.clear();
        } else {
            _resetDrag();
        }
    } else if (m_dragEvent == DragConstants::EVENT_END) {
        // 结束拖拽 - 单槽降级或多槽分发
        // 对应 MC 1.21.11 AbstractContainerMenu#doClick 中 quickcraftSlots.size()==1 的降级路径：
        // 当仅有一个拖拽槽位时，重置拖拽状态后递归调用 clicked(slotIndex, dragMode, Pick, player)，
        // 让单槽拖拽降级为普通 PICKUP 点击，从而触发 _tryItemClickBehaviourOverride
        // （收纳袋的 overrideStackedOnOther/overrideOtherStackedOnMe）。
        if (m_dragSlots.size() == 1) {
            const i32 singleSlotIndex = m_dragSlots[0];
            const i32 dragMode = m_dragMode;
            _resetDrag();
            // MODE_EVEN(0)/MODE_SINGLE(1) 分别对应 PICKUP 的左/右键（button==0/1），
            // 与 MC Java 行为一致；MODE_FILL(2) 不进入 PICKUP 分支（创造模式专属，
            // 单槽时无意义，直接返回）。
            if (dragMode == DragConstants::MODE_EVEN || dragMode == DragConstants::MODE_SINGLE) {
                // 递归调用 clicked 会更新 m_carried，调用方在 clicked 中已 return m_carried
                m_carried = clicked(singleSlotIndex, dragMode, ClickType::Pick, player);
            }
            return;
        }

        // 多槽分发
        if (!m_dragSlots.empty()) {
            ItemStack toDistribute = m_carried.copy();

            // 计算每个槽位可以放入多少
            std::vector<std::pair<i32, i32>> slotAmounts;
            for (i32 dragSlotIndex : m_dragSlots) {
                Slot* dragSlot = getSlot(dragSlotIndex);
                if (dragSlot == nullptr) {
                    continue;
                }

                ItemStack existing = dragSlot->getItem();
                i32 maxStackSize = dragSlot->getMaxStackSize(toDistribute);
                i32 space = existing.isEmpty() ? maxStackSize : maxStackSize - existing.getCount();

                if (space > 0 && (existing.isEmpty() || existing.canMergeWith(toDistribute))) {
                    slotAmounts.push_back({dragSlotIndex, space});
                }
            }

            // 根据拖拽模式分发
            if (m_dragMode == DragConstants::MODE_EVEN) {
                i32 slotsRemaining = static_cast<i32>(slotAmounts.size());
                for (auto& [idx, space] : slotAmounts) {
                    if (toDistribute.isEmpty()) {
                        break;
                    }

                    i32 perSlot = toDistribute.getCount() / slotsRemaining;
                    if (perSlot == 0) {
                        perSlot = 1;
                    }
                    perSlot = std::min(perSlot, space);
                    _distributeToDragSlot(toDistribute, idx, perSlot);
                    slotsRemaining--;
                }
            } else if (m_dragMode == DragConstants::MODE_SINGLE) {
                for (auto& [idx, space] : slotAmounts) {
                    if (toDistribute.isEmpty()) {
                        break;
                    }
                    _distributeToDragSlot(toDistribute, idx, 1);
                }
            } else if (m_dragMode == DragConstants::MODE_FILL) {
                for (auto& [idx, space] : slotAmounts) {
                    if (toDistribute.isEmpty()) {
                        break;
                    }
                    _distributeToDragSlot(toDistribute, idx, space);
                }
            }

            // 更新鼠标物品
            m_carried = toDistribute.isEmpty() ? ItemStack() : toDistribute;
        }
        _resetDrag();
    } else {
        _resetDrag();
    }
}

void AbstractContainerMenu::_resetDrag()
{
    m_dragEvent = 0;
    m_dragMode = DragConstants::DRAG_MODE_NONE;
    m_dragSlots.clear();
}

i32 AbstractContainerMenu::_getDragEvent(i32 button)
{
    return button & DragConstants::EVENT_MASK;
}

i32 AbstractContainerMenu::_extractDragMode(i32 button)
{
    return (button >> DragConstants::MODE_SHIFT) & DragConstants::MODE_MASK;
}

bool AbstractContainerMenu::_isValidDragMode(i32 dragMode) const
{
    if (m_playerInventory == nullptr) return false;

    const Player* player = m_playerInventory->getPlayer();
    if (player == nullptr) return false;

    // 模式 2 (全部分发) 只在创造模式下有效
    if (dragMode == 2 && !entity::GameModeUtils::isCreative(player->gameMode())) {
        return false;
    }

    return true;
}

bool AbstractContainerMenu::_canDragIntoSlot(Slot& slot, const ItemStack& stack) const
{
    // 检查是否可以拖拽物品到该槽位
    if (slot.isEmpty()) {
        return slot.mayPlace(stack);
    }
    return slot.mayPlace(stack) && slot.getItem().canMergeWith(stack);
}

i32 AbstractContainerMenu::_distributeToDragSlot(ItemStack& toDistribute, i32 slotIdx, i32 amount)
{
    Slot* dragSlot = getSlot(slotIdx);
    if (dragSlot == nullptr) {
        return 0;
    }

    amount = std::min(amount, toDistribute.getCount());
    if (amount <= 0) {
        return 0;
    }

    ItemStack existing = dragSlot->getItem();
    if (existing.isEmpty()) {
        dragSlot->set(toDistribute.split(amount));
    } else {
        existing.grow(amount);
        dragSlot->set(existing);
        toDistribute.shrink(amount);
    }
    dragSlot->setChanged();
    notifySlotChanged(slotIdx, dragSlot->getItem());

    return amount;
}

ItemStack AbstractContainerMenu::_handlePickupAll(Slot& slot, i32 slotIndex, const ItemStack& slotStack)
{
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

ItemStack AbstractContainerMenu::quickMoveStack(i32 slotIndex, Player& player)
{
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

bool AbstractContainerMenu::moveItemToRange(ItemStack& stack, i32 startIndex, i32 endIndex, bool reverse)
{
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

void AbstractContainerMenu::removed(Player& player)
{
    // 关闭容器时，如果玩家光标上有物品，丢弃到世界中
    if (!m_carried.isEmpty()) {
        // 尝试放回玩家背包
        if (m_playerInventory != nullptr) {
            // add() 返回剩余未添加的数量，会修改 m_carried 的数量
            i32 remaining = m_playerInventory->add(m_carried);
            if (remaining > 0) {
                // 放不下的物品掉落
                m_carried.setCount(remaining);
                dropItem(m_carried, player, false);
            }
            m_carried = ItemStack();
        } else {
            // 没有玩家背包，直接掉落
            dropItem(m_carried, player, false);
            m_carried = ItemStack();
        }
    }

    // 清理不能合成玩家列表
    m_cannotCraftPlayers.clear();
}

void AbstractContainerMenu::dropItem(const ItemStack& stack, Player& player, bool retainOwnership)
{
    if (stack.isEmpty()) {
        return;
    }
    if (m_itemDropCallback) {
        m_itemDropCallback(stack, player, retainOwnership);
    }
}

bool AbstractContainerMenu::_tryItemClickBehaviourOverride(Slot& slot, SlotClickAction clickAction, Player& player)
{
    // 对应 MC 1.21.11 AbstractContainerMenu#tryItemClickBehaviourOverride
    // 给光标物品和槽位物品各一次机会自定义交互行为：
    // 1. 若光标物品（m_carried）非空且其 Item.overrideStackedOnOther 返回 true → 处理完毕
    // 2. 否则若槽位物品（slotStack）非空且其 Item.overrideOtherStackedOnMe 返回 true → 处理完毕
    // 3. 否则返回 false，走默认拾取/放置逻辑
    //
    // 收纳袋（BundleItem）重写了这两个方法以实现：
    // - 手持收纳袋点击其他槽位：插入/取出（overrideStackedOnOther）
    // - 手持其他物品点击收纳袋槽位：插入/取出（overrideOtherStackedOnMe）

    if (!m_carried.isEmpty()) {
        // 光标物品非空：先尝试 overrideStackedOnOther
        // 使用 Item::getItem(itemId) 获取非 const Item* 以调用非 const 虚方法
        Item* carriedItem = const_cast<Item*>(m_carried.getItem());
        if (carriedItem != nullptr && carriedItem->overrideStackedOnOther(m_carried, slot, clickAction, player)) {
            return true;
        }
    }

    ItemStack slotStack = slot.getItem();
    if (!slotStack.isEmpty()) {
        // 槽位物品非空：尝试 overrideOtherStackedOnMe
        Item* slotItem = const_cast<Item*>(slotStack.getItem());
        if (slotItem != nullptr &&
            slotItem->overrideOtherStackedOnMe(slotStack, m_carried, slot, clickAction, player)) {
            // overrideOtherStackedOnMe 修改的是 slotStack（slot.getItem() 的拷贝），
            // 需要写回槽位才能让修改生效（如收纳袋内容物 NBT 变化）。
            slot.set(slotStack);
            return true;
        }
    }

    return false;
}

} // namespace mc
