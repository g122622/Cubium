#include "entity/inventory/container/HopperContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {

// ========== 构造函数 ==========

HopperContainer::HopperContainer(ContainerId id,
                                 PlayerInventory* playerInventory,
                                 IInventory* hopperInventory)
    : AbstractContainerMenu(id, playerInventory)
    , m_hopperInventory(hopperInventory) {

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(hopperInventory != nullptr);
    MC_ASSERT(hopperInventory->getContainerSize() == HOPPER_SIZE);

    // 打开背包（通知方块实体）
    hopperInventory->openInventory(*playerInventory->getPlayer());

    // 初始化槽位布局
    initSlots(playerInventory);
}

// ========== 容器接口 ==========

bool HopperContainer::stillValid(const Player& player) const {
    // MC 1.16.5: 检查背包是否可用
    return m_hopperInventory->isUsableByPlayer(player);
}

void HopperContainer::slotsChanged(IInventory* inventory) {
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack HopperContainer::quickMoveStack(i32 slotIndex, Player& player) {
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack result = slotStack.copy();

    // MC 1.16.5: 漏斗槽位范围 0-4，玩家背包槽位范围 5-40
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

void HopperContainer::initSlots(PlayerInventory* playerInventory) {
    // ========== 漏斗槽位（1行5列）==========
    // MC 1.16.5: x从44开始，y=20

    for (i32 col = 0; col < HOPPER_SIZE; ++col) {
        i32 x = HOPPER_SLOT_START_X + col * SLOT_SIZE;
        i32 y = HOPPER_SLOT_Y;

        addSlot(std::make_unique<Slot>(m_hopperInventory, col, x, y));
    }

    // ========== 玩家主背包（3行9列）==========
    // MC 1.16.5: y从51开始

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col;  // 玩家背包从索引9开始
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = PLAYER_INV_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏（1行9列）==========
    // MC 1.16.5: y=109

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col;  // 快捷栏从索引0开始
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = HOTBAR_Y;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }
}

} // namespace mc
