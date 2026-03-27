#include "entity/inventory/container/HopperContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

HopperContainer::HopperContainer(ContainerId id,
                                 PlayerInventory* playerInventory,
                                 IInventory* hopperInventory)
    : Container(ContainerType::Hopper, id)
    , m_hopperInventory(hopperInventory) {

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(hopperInventory != nullptr);
    MC_ASSERT(hopperInventory->getContainerSize() == HOPPER_SIZE);

    // 初始化槽位布局
    initSlots(playerInventory);
}

// ========== 快速移动 ==========

ItemStack HopperContainer::doQuickMove(i32 slotIndex, ItemStack cursorItem) {
    // 获取槽位
    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();
    const i32 originalCount = slotStack.getCount();

    if (slotIndex < HOPPER_SIZE) {
        // 从漏斗移到玩家背包
        if (!mergeItem(slotStack, playerInventoryRange(), true)) {
            return cursorItem;
        }
    } else {
        // 从玩家背包移到漏斗
        if (!mergeItem(slotStack, containerInventoryRange(), false)) {
            return cursorItem;
        }
    }

    // 更新槽位
    if (slotStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->getInventory()->setChanged();
    }

    // 如果数量没变，表示没有移动成功
    if (slotStack.getCount() == originalCount) {
        return cursorItem;
    }

    return cursorItem;
}

// ========== 私有方法 ==========

void HopperContainer::initSlots(PlayerInventory* playerInventory) {
    // ========== 漏斗槽位（1行5列）==========

    constexpr i32 hopperStartX = 8;
    constexpr i32 hopperStartY = HOPPER_SLOT_Y;

    for (i32 col = 0; col < HOPPER_SIZE; ++col) {
        i32 x = hopperStartX + col * SLOT_SIZE;
        i32 y = hopperStartY;

        addSlot(std::make_unique<Slot>(m_hopperInventory, col, x, y));
    }

    // 记录漏斗槽位范围
    setContainerInventoryRange(0, HOPPER_SIZE);

    // ========== 玩家主背包（3行9列）==========

    constexpr i32 playerStartY = PLAYER_INV_Y;

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col;  // 玩家背包从索引9开始
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = playerStartY + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏（1行9列）==========

    constexpr i32 hotbarY = HOTBAR_Y;

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col;  // 快捷栏从索引0开始
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = hotbarY;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }

    // 记录玩家背包槽位范围
    setPlayerInventoryRange(HOPPER_SIZE, HOPPER_SIZE + 36);  // 27主背包 + 9快捷栏
}

} // namespace blockentity
} // namespace mc
