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

#include "entity/inventory/container/ChestContainer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/world/block/BlockPos.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace blockentity {

// ========== 常量 ==========

namespace {
/// 箱子槽位起始Y位置
constexpr i32 CHEST_SLOT_Y = 18;
/// 玩家背包起始Y位置（箱子后）
constexpr i32 PLAYER_INV_Y_OFFSET = 103;
/// 玩家背包起始Y位置（双箱后）
constexpr i32 PLAYER_INV_Y_OFFSET_DOUBLE = 139;
/// 快捷栏Y位置
constexpr i32 HOTBAR_Y = 161;
/// 快捷栏Y位置（双箱）
constexpr i32 HOTBAR_Y_DOUBLE = 197;
/// 槽位宽度
constexpr i32 SLOT_SIZE = 18;
} // namespace

// ========== 构造函数 ==========

ChestContainer::ChestContainer(ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory, i32 rows)
    : AbstractContainerMenu(id, playerInventory)
    , m_chestInventory(chestInventory)
    , m_chestInventoryOwner()
    , m_rows(rows)
{
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(chestInventory != nullptr);
    MC_ASSERT(rows == SINGLE_CHEST_ROWS || rows == DOUBLE_CHEST_ROWS);
    MC_ASSERT(chestInventory->getContainerSize() == rows * SLOTS_PER_ROW);

    m_chestSlotCount = rows * SLOTS_PER_ROW;
    _initSlots(playerInventory);
}

ChestContainer::ChestContainer(
    ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory, ChestEntity* chestEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_chestInventory(chestInventory)
    , m_chestInventoryOwner()
    , m_rows(SINGLE_CHEST_ROWS)
    , m_chestEntityA(chestEntity)
{
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(chestInventory != nullptr);
    MC_ASSERT(chestInventory->getContainerSize() == SINGLE_CHEST_ROWS * SLOTS_PER_ROW);

    m_chestSlotCount = SINGLE_CHEST_ROWS * SLOTS_PER_ROW;
    _initSlots(playerInventory);
}

ChestContainer::ChestContainer(ContainerId id,
    PlayerInventory* playerInventory,
    IInventory* chestInventory,
    ChestEntity* chestEntityA,
    ChestEntity* chestEntityB)
    : AbstractContainerMenu(id, playerInventory)
    , m_chestInventory(chestInventory)
    , m_chestInventoryOwner()
    , m_rows(DOUBLE_CHEST_ROWS)
    , m_chestEntityA(chestEntityA)
    , m_chestEntityB(chestEntityB)
{
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(chestInventory != nullptr);
    MC_ASSERT(chestInventory->getContainerSize() == DOUBLE_CHEST_ROWS * SLOTS_PER_ROW);

    m_chestSlotCount = DOUBLE_CHEST_ROWS * SLOTS_PER_ROW;
    _initSlots(playerInventory);
}

ChestContainer::ChestContainer(
    ContainerId id, PlayerInventory* playerInventory, std::shared_ptr<IInventory> chestInventoryOwner, i32 rows)
    : AbstractContainerMenu(id, playerInventory)
    , m_chestInventory(chestInventoryOwner.get())
    , m_chestInventoryOwner(std::move(chestInventoryOwner))
    , m_rows(rows)
{
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(m_chestInventory != nullptr);
    MC_ASSERT(rows == SINGLE_CHEST_ROWS || rows == DOUBLE_CHEST_ROWS);
    MC_ASSERT(m_chestInventory->getContainerSize() == rows * SLOTS_PER_ROW);

    m_chestSlotCount = rows * SLOTS_PER_ROW;
    _initSlots(playerInventory);
}

// ========== 静态工厂方法 ==========

std::unique_ptr<ChestContainer> ChestContainer::createSingle(
    ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory)
{
    return std::make_unique<ChestContainer>(id, playerInventory, chestInventory, SINGLE_CHEST_ROWS);
}

std::unique_ptr<ChestContainer> ChestContainer::createSingle(
    ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory, ChestEntity* chestEntity)
{
    return std::make_unique<ChestContainer>(id, playerInventory, chestInventory, chestEntity);
}

std::unique_ptr<ChestContainer> ChestContainer::createDouble(
    ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory)
{
    return std::make_unique<ChestContainer>(id, playerInventory, chestInventory, DOUBLE_CHEST_ROWS);
}

std::unique_ptr<ChestContainer> ChestContainer::createDouble(ContainerId id,
    PlayerInventory* playerInventory,
    IInventory* chestInventory,
    ChestEntity* chestEntityA,
    ChestEntity* chestEntityB)
{
    return std::make_unique<ChestContainer>(id, playerInventory, chestInventory, chestEntityA, chestEntityB);
}

// ========== 容器关闭 ==========

void ChestContainer::removed(Player& player)
{
    AbstractContainerMenu::removed(player);
    // 对末影箱物品栏：closeInventory → stopOpen，处理关盖动画、音效和清理 activeChest 引用
    // 对普通箱子：closeInventory 是 IInventory 基类的空操作（no-op），普通箱子的关闭逻辑
    //   （如 ChestEntity 的 openCount 递减和音效）由 StandaloneServer 的容器关闭回调处理
    m_chestInventory->closeInventory(player);
}

// ========== 快速移动 ==========

ItemStack ChestContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack originalStack = slotStack.copy();

    if (slotIndex < m_chestSlotCount) {
        if (!moveItemToRange(slotStack, m_chestSlotCount, getSlotCount() - 1, true)) {
            return ItemStack();
        }
    } else if (!moveItemToRange(slotStack, 0, m_chestSlotCount - 1, false)) {
        return ItemStack();
    }

    slot->set(slotStack.isEmpty() ? ItemStack() : slotStack);

    return originalStack;
}

// ========== 私有方法 ==========

void ChestContainer::_initSlots(PlayerInventory* playerInventory)
{
    for (i32 row = 0; row < m_rows; ++row) {
        for (i32 col = 0; col < SLOTS_PER_ROW; ++col) {
            i32 slotIndex = row * SLOTS_PER_ROW + col;
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = CHEST_SLOT_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(m_chestInventory, slotIndex, x, y));
        }
    }

    addPlayerInventorySlots(8, (m_rows == DOUBLE_CHEST_ROWS) ? PLAYER_INV_Y_OFFSET_DOUBLE : PLAYER_INV_Y_OFFSET);
    addPlayerHotbarSlots(8, (m_rows == DOUBLE_CHEST_ROWS) ? HOTBAR_Y_DOUBLE : HOTBAR_Y);
}

bool ChestContainer::stillValid(const Player& player) const
{
    // 如果没有关联的箱子实体，使用背包的 isUsableByPlayer 方法
    if (m_chestEntityA == nullptr && m_chestEntityB == nullptr) {
        return m_chestInventory->isUsableByPlayer(player);
    }

    // 检查第一个箱子实体的距离
    if (m_chestEntityA != nullptr) {
        if (m_chestEntityA->isRemoved()) {
            return false;
        }
        const BlockPos posA = m_chestEntityA->getPos();
        f32 distSqA = player.distanceSqTo(
            static_cast<f32>(posA.x) + 0.5f, static_cast<f32>(posA.y) + 0.5f, static_cast<f32>(posA.z) + 0.5f);
        if (distSqA <= 64.0f) {
            return true;
        }
    }

    // 检查第二个箱子实体的距离（双箱情况）
    if (m_chestEntityB != nullptr) {
        if (m_chestEntityB->isRemoved()) {
            return false;
        }
        const BlockPos posB = m_chestEntityB->getPos();
        f32 distSqB = player.distanceSqTo(
            static_cast<f32>(posB.x) + 0.5f, static_cast<f32>(posB.y) + 0.5f, static_cast<f32>(posB.z) + 0.5f);
        if (distSqB <= 64.0f) {
            return true;
        }
    }

    return false;
}

} // namespace blockentity
} // namespace mc
