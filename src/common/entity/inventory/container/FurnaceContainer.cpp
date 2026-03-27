#include "entity/inventory/container/FurnaceContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

FurnaceContainer::FurnaceContainer(ContainerId id,
                                   PlayerInventory* playerInventory,
                                   IInventory* furnaceInventory)
    : Container(ContainerType::FURNACE, id)
    , m_furnaceInventory(furnaceInventory) {

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(furnaceInventory != nullptr);
    MC_ASSERT(furnaceInventory->getContainerSize() == FURNACE_SLOTS);

    // 初始化槽位布局
    initSlots(playerInventory);
}

// ========== 快速移动 ==========

ItemStack FurnaceContainer::doQuickMove(i32 slotIndex, ItemStack cursorItem) {
    // 获取槽位
    const Slot* slot = getSlot(slotIndex);
    if (!slot || !slot->hasItem()) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();
    const i32 originalCount = slotStack.getCount();

    // 确定目标范围
    ItemStack result;

    if (slotIndex < FURNACE_SLOTS) {
        // 从熔炉移到玩家背包
        if (slotIndex == SLOT_OUTPUT) {
            // 输出槽：优先移到玩家背包
            if (!mergeItem(slotStack, playerInventoryRange(), true)) {
                return cursorItem;
            }
        } else {
            // 输入槽/燃料槽：移到玩家背包
            if (!mergeItem(slotStack, playerInventoryRange(), false)) {
                return cursorItem;
            }
        }
    } else {
        // 从玩家背包移到熔炉
        // 检查是否可以作为燃料
        if (FurnaceInventory::isFuel(slotStack)) {
            // 优先移到燃料槽
            if (!mergeItem(slotStack, SlotRange(SLOT_FUEL, SLOT_FUEL + 1), false)) {
                // 燃料槽满了，移到输入槽
                if (!mergeItem(slotStack, SlotRange(SLOT_INPUT, SLOT_INPUT + 1), false)) {
                    return cursorItem;
                }
            }
        } else {
            // 移到输入槽
            if (!mergeItem(slotStack, SlotRange(SLOT_INPUT, SLOT_INPUT + 1), false)) {
                return cursorItem;
            }
        }
    }

    // 更新槽位
    if (slotStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->setChanged();
    }

    // 如果数量没变，表示没有移动成功
    if (slotStack.getCount() == originalCount) {
        return cursorItem;
    }

    return cursorItem;
}

// ========== 私有方法 ==========

void FurnaceContainer::initSlots(PlayerInventory* playerInventory) {
    // ========== 熔炉槽位 ==========

    // 输入槽（顶部中央）
    addSlot(std::make_unique<Slot>(m_furnaceInventory, SLOT_INPUT, 56, 17));

    // 燃料槽（中部中央）
    addSlot(std::make_unique<Slot>(m_furnaceInventory, SLOT_FUEL, 56, 53));

    // 输出槽（底部中央）
    addSlot(std::make_unique<Slot>(m_furnaceInventory, SLOT_OUTPUT, 116, 35));

    // 记录熔炉槽位范围
    setContainerInventoryRange(0, FURNACE_SLOTS);

    // ========== 玩家主背包（3行9列）==========

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col;  // 玩家背包从索引9开始
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = PLAYER_INV_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏（1行9列）==========

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col;  // 快捷栏从索引0开始
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = HOTBAR_Y;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }

    // 记录玩家背包槽位范围
    setPlayerInventoryRange(FURNACE_SLOTS, FURNACE_SLOTS + 36);  // 27主背包 + 9快捷栏
}

} // namespace blockentity
} // namespace mc
