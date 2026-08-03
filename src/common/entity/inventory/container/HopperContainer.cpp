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

#include "entity/inventory/container/HopperContainer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/world/block/BlockPos.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/transport/HopperEntity.hpp"
#include <memory>

namespace mc {

// ========== 构造函数 ==========

HopperContainer::HopperContainer(ContainerId id,
    PlayerInventory* playerInventory,
    IInventory* hopperInventory,
    blockentity::HopperEntity* hopperEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_hopperInventory(hopperInventory)
    , m_hopperEntity(hopperEntity)
{

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(hopperInventory != nullptr);
    MC_ASSERT(hopperInventory->getContainerSize() == HOPPER_SIZE);

    // 打开背包（通知方块实体）
    hopperInventory->openInventory(*playerInventory->getPlayer());

    // 初始化槽位布局
    _initSlots(playerInventory);
}

// ========== 容器接口 ==========

bool HopperContainer::stillValid(const Player& player) const
{
    // 如果没有关联的方块实体，背包可访问
    if (m_hopperEntity == nullptr) {
        return m_hopperInventory->isUsableByPlayer(player);
    }

    // 检查玩家是否在漏斗附近（8格范围内）
    const BlockPos pos = m_hopperEntity->getPos();
    return player.distanceSqTo(static_cast<f32>(pos.x) + 0.5f,
               static_cast<f32>(pos.y) + 0.5f,
               static_cast<f32>(pos.z) + 0.5f) <= 64.0f; // 8^2 = 64
}

void HopperContainer::slotsChanged(IInventory* inventory)
{
    AbstractContainerMenu::slotsChanged(inventory);
}

void HopperContainer::removed(Player& player)
{
    // 配对构造函数中的 openInventory 调用，通知方块实体玩家已关闭容器
    m_hopperInventory->closeInventory(player);
    AbstractContainerMenu::removed(player);
}

ItemStack HopperContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack result = slotStack.copy();

    // 漏斗槽位范围 0-4，玩家背包槽位范围 5-40
    if (slotIndex < HOPPER_SIZE) {
        // 从漏斗移到玩家背包
        if (!moveItemToRange(slotStack, HOPPER_SIZE, getSlotCount(), true)) {
            return ItemStack();
        }
    } else {
        // 从玩家背包移到漏斗
        if (!moveItemToRange(slotStack, 0, HOPPER_SIZE, false)) {
            return ItemStack();
        }
    }

    if (slotStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->setChanged();
    }

    return result;
}

// ========== 私有方法 ==========

void HopperContainer::_initSlots(PlayerInventory* playerInventory)
{
    // ========== 漏斗槽位（1行5列）==========

    for (i32 col = 0; col < HOPPER_SIZE; ++col) {
        i32 x = HOPPER_SLOT_START_X + col * SLOT_SIZE;
        i32 y = HOPPER_SLOT_Y;

        addSlot(std::make_unique<Slot>(m_hopperInventory, col, x, y));
    }

    // ========== 玩家主背包（3行9列）==========

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col; // 玩家背包从索引9开始
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = PLAYER_INV_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏（1行9列）==========

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col; // 快捷栏从索引0开始
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = HOTBAR_Y;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }
}

} // namespace mc
