#include "entity/inventory/container/FurnaceContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"
#include <cmath>

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

FurnaceContainer::FurnaceContainer(ContainerId id,
                                   PlayerInventory* playerInventory,
                                   IInventory* furnaceInventory,
                                   AbstractFurnaceEntity* furnaceEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_furnaceInventory(furnaceInventory)
    , m_furnaceInventoryOwner()
    , m_furnaceEntity(furnaceEntity) {

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(furnaceInventory != nullptr);
    MC_ASSERT(furnaceInventory->getContainerSize() == FURNACE_SLOTS);

    // 初始化槽位布局
    initSlots(playerInventory);
}

FurnaceContainer::FurnaceContainer(ContainerId id,
                                   PlayerInventory* playerInventory,
                                   std::shared_ptr<IInventory> furnaceInventoryOwner,
                                   AbstractFurnaceEntity* furnaceEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_furnaceInventory(furnaceInventoryOwner.get())
    , m_furnaceInventoryOwner(std::move(furnaceInventoryOwner))
    , m_furnaceEntity(furnaceEntity) {

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(m_furnaceInventory != nullptr);
    MC_ASSERT(m_furnaceInventory->getContainerSize() == FURNACE_SLOTS);

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

bool FurnaceContainer::stillValid(const Player& player) const {
    // MC 1.16.5: 如果没有关联的方块实体，背包可访问
    if (m_furnaceEntity == nullptr) {
        return true;
    }

    // MC 1.16.5: 检查玩家是否在熔炉附近（8格范围内）
    // 参考 net.minecraft.inventory.container.AbstractFurnaceContainer.canInteractWith
    // -> furnaceInventory.isUsableByPlayer(playerIn)
    const BlockPos pos = m_furnaceEntity->getPos();
    return player.distanceSqTo(
               static_cast<f32>(pos.x) + 0.5f,
               static_cast<f32>(pos.y) + 0.5f,
               static_cast<f32>(pos.z) + 0.5f) <= 64.0f;  // 8^2 = 64
}

ItemStack FurnaceContainer::quickMoveStack(i32 slotIndex, Player& player) {
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack originalStack = slotStack.copy();

    if (slotIndex < FURNACE_SLOTS) {
        if (slotIndex == SLOT_OUTPUT) {
            if (!moveItemToRange(slotStack, FURNACE_SLOTS, getSlotCount() - 1, true)) {
                return ItemStack();
            }
            grantExperienceForOutput(originalStack.getCount() - slotStack.getCount());
        } else {
            if (!moveItemToRange(slotStack, FURNACE_SLOTS, getSlotCount() - 1, false)) {
                return ItemStack();
            }
        }
    } else {
        if (!moveItemToRange(slotStack, SLOT_INPUT, SLOT_INPUT, false)) {
            if (!moveItemToRange(slotStack, SLOT_FUEL, SLOT_FUEL, false)) {
                return ItemStack();
            }
        }
    }

    slot->set(slotStack.isEmpty() ? ItemStack() : slotStack);

    return originalStack;
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

}

} // namespace blockentity
} // namespace mc
