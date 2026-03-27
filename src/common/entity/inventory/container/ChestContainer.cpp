#include "entity/inventory/container/ChestContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
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
    : Container(ContainerType::Chest, id)
    , m_chestInventory(chestInventory)
    , m_rows(rows) {
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(chestInventory != nullptr);
    MC_ASSERT(rows == SINGLE_CHEST_ROWS || rows == DOUBLE_CHEST_ROWS);
    MC_ASSERT(chestInventory->getContainerSize() == rows * SLOTS_PER_ROW);

    // 初始化槽位布局
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

ItemStack ChestContainer::doQuickMove(i32 slotIndex, ItemStack cursorItem) {
    // 获取槽位
    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();
    const i32 originalCount = slotStack.getCount();

    // 确定目标范围
    const i32 chestSlotCount = getChestSlotCount();
    ItemStack result;

    if (slotIndex < chestSlotCount) {
        // 从箱子移到玩家背包
        if (!mergeItem(slotStack, playerInventoryRange(), true)) {
            return cursorItem;
        }
    } else {
        // 从玩家背包移到箱子
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

void ChestContainer::initSlots(PlayerInventory* playerInventory) {
    // 计算偏移量（用于双箱时调整玩家背包位置）
    const i32 yOffset = (m_rows == DOUBLE_CHEST_ROWS) ? PLAYER_INV_Y_OFFSET_DOUBLE : PLAYER_INV_Y_OFFSET;
    const i32 hotbarY = (m_rows == DOUBLE_CHEST_ROWS) ? HOTBAR_Y_DOUBLE : HOTBAR_Y;

    // ========== 箱子槽位 ==========

    for (i32 row = 0; row < m_rows; ++row) {
        for (i32 col = 0; col < SLOTS_PER_ROW; ++col) {
            i32 slotIndex = row * SLOTS_PER_ROW + col;
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = CHEST_SLOT_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(m_chestInventory, slotIndex, x, y));
        }
    }

    // 记录箱子槽位范围
    const i32 chestSlotCount = getChestSlotCount();
    setContainerInventoryRange(0, chestSlotCount);

    // ========== 玩家主背包 ==========

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col;  // 玩家背包从索引9开始
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = yOffset + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏 ==========

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col;  // 快捷栏从索引0开始
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = hotbarY;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }

    // 记录玩家背包槽位范围
    setPlayerInventoryRange(chestSlotCount, chestSlotCount + 36);  // 27主背包 + 9快捷栏
}

} // namespace blockentity
} // namespace mc
