#include "entity/inventory/container/FurnaceContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

FurnaceContainer::FurnaceContainer(ContainerId id,
                                   PlayerInventory* playerInventory,
                                   IInventory* furnaceInventory,
                                   AbstractFurnaceEntity* furnaceEntity)
    : Container(ContainerType::Furnace, id)
    , m_furnaceInventory(furnaceInventory)
    , m_furnaceEntity(furnaceEntity) {

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(furnaceInventory != nullptr);
    MC_ASSERT(furnaceInventory->getContainerSize() == FURNACE_SLOTS);

    // 初始化槽位布局
    initSlots(playerInventory);
}

// ========== 经验相关 ==========

f32 FurnaceContainer::getStoredExperience() const {
    if (m_furnaceEntity) {
        return m_furnaceEntity->getStoredExperience();
    }
    return 0.0f;
}

f32 FurnaceContainer::extractStoredExperience() {
    if (m_furnaceEntity) {
        return m_furnaceEntity->extractStoredExperience();
    }
    return 0.0f;
}

void FurnaceContainer::grantExperienceForOutput(i32 extractedCount) {
    // 只有在有玩家且有累积经验时才发放
    if (!m_player || !m_furnaceEntity) {
        return;
    }

    f32 storedXp = m_furnaceEntity->getStoredExperience();
    if (storedXp <= 0.0f) {
        return;
    }

    // 计算要发放的经验（按取出数量比例发放，每次取出发放全部累积经验）
    // 参考 MC 1.16.5: 玩家从输出槽取出物品时，发放所有累积经验
    // 这里简化处理：每当玩家取出输出物品时，发放所有累积经验
    // 更精确的做法是按配方数量记录，但 MC 实际上是一次性发放所有

    f32 xpToGrant = m_furnaceEntity->extractStoredExperience();
    if (xpToGrant > 0.0f) {
        m_player->addExperience(static_cast<i32>(std::floor(xpToGrant)));
    }
}

// ========== 快速移动 ==========

ItemStack FurnaceContainer::doQuickMove(i32 slotIndex, ItemStack cursorItem) {
    // 获取槽位
    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return cursorItem;
    }

    ItemStack slotStack = slot->getItem();
    const i32 originalCount = slotStack.getCount();

    if (slotIndex < FURNACE_SLOTS) {
        // 从熔炉移到玩家背包
        if (slotIndex == SLOT_OUTPUT) {
            // 输出槽：优先移到玩家背包，并发放经验
            if (!mergeItem(slotStack, playerInventoryRange(), true)) {
                return cursorItem;
            }
            // 取出成功，发放经验
            if (slotStack.getCount() < originalCount) {
                grantExperienceForOutput(originalCount - slotStack.getCount());
            }
        } else {
            // 输入槽/燃料槽：移到玩家背包
            if (!mergeItem(slotStack, playerInventoryRange(), false)) {
                return cursorItem;
            }
        }
    } else {
        // 从玩家背包移到熔炉
        // TODO: 检查是否可以作为燃料
        // 移到输入槽
        if (!mergeItem(slotStack, SlotRange(SLOT_INPUT, SLOT_INPUT + 1), false)) {
            // 输入槽满了，尝试燃料槽
            if (!mergeItem(slotStack, SlotRange(SLOT_FUEL, SLOT_FUEL + 1), false)) {
                return cursorItem;
            }
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
