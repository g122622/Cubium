#include "entity/inventory/container/ChestContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"

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
}

// ========== 构造函数 ==========

ChestContainer::ChestContainer(ContainerId id,
                               PlayerInventory* playerInventory,
                               IInventory* chestInventory,
                               i32 rows)
    : AbstractContainerMenu(id, playerInventory)
    , m_chestInventory(chestInventory)
    , m_chestInventoryOwner()
    , m_rows(rows) {
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(chestInventory != nullptr);
    MC_ASSERT(rows == SINGLE_CHEST_ROWS || rows == DOUBLE_CHEST_ROWS);
    MC_ASSERT(chestInventory->getContainerSize() == rows * SLOTS_PER_ROW);

    m_chestSlotCount = rows * SLOTS_PER_ROW;
    initSlots(playerInventory);
}

ChestContainer::ChestContainer(ContainerId id,
                               PlayerInventory* playerInventory,
                               std::shared_ptr<IInventory> chestInventoryOwner,
                               i32 rows)
    : AbstractContainerMenu(id, playerInventory)
    , m_chestInventory(chestInventoryOwner.get())
    , m_chestInventoryOwner(std::move(chestInventoryOwner))
    , m_rows(rows) {
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(m_chestInventory != nullptr);
    MC_ASSERT(rows == SINGLE_CHEST_ROWS || rows == DOUBLE_CHEST_ROWS);
    MC_ASSERT(m_chestInventory->getContainerSize() == rows * SLOTS_PER_ROW);

    m_chestSlotCount = rows * SLOTS_PER_ROW;
    initSlots(playerInventory);
}

// ========== 静态工厂方法 ==========

std::unique_ptr<ChestContainer> ChestContainer::createSingle(
    ContainerId id,
    PlayerInventory* playerInventory,
    IInventory* chestInventory) {
    return std::make_unique<ChestContainer>(id, playerInventory, chestInventory, SINGLE_CHEST_ROWS);
}

std::unique_ptr<ChestContainer> ChestContainer::createDouble(
    ContainerId id,
    PlayerInventory* playerInventory,
    IInventory* chestInventory) {
    return std::make_unique<ChestContainer>(id, playerInventory, chestInventory, DOUBLE_CHEST_ROWS);
}

// ========== 快速移动 ==========

ItemStack ChestContainer::quickMoveStack(i32 slotIndex, Player& player) {
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

void ChestContainer::initSlots(PlayerInventory* playerInventory) {
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

bool ChestContainer::stillValid(const Player& player) const {
    (void)player;
    return true;
}

} // namespace blockentity
} // namespace mc
