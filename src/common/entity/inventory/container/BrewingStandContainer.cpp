#include "entity/inventory/container/BrewingStandContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/core/Item.hpp"
#include "item/Items.hpp"
#include "item/potion/PotionBrewing.hpp"
#include "world/blockentity/processing/BrewingStandEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {

namespace {

/// 槽位宽度
constexpr i32 SLOT_SIZE = 18;

/**
 * @brief 药水槽位
 *
 * 只接受药水瓶，最大堆叠数为1。
 */
class PotionSlot : public Slot {
public:
    PotionSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
        : Slot(inventory, slotIndex, x, y) {}

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override {
        return !stack.isEmpty() && potion::PotionBrewing::isPotionItem(stack);
    }

    [[nodiscard]] i32 getMaxStackSize() const override {
        return 1;
    }

    [[nodiscard]] i32 getMaxStackSize(const ItemStack& stack) const override {
        (void)stack;
        return 1;
    }
};

/**
 * @brief 材料槽位
 *
 * 只接受酿造材料。
 */
class IngredientSlot : public Slot {
public:
    IngredientSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
        : Slot(inventory, slotIndex, x, y) {}

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override {
        return !stack.isEmpty() && potion::PotionBrewing::isReagent(stack);
    }

    [[nodiscard]] i32 getMaxStackSize() const override {
        return 64;
    }
};

/**
 * @brief 燃料槽位
 *
 * 只接受烈焰粉。
 */
class FuelSlot : public Slot {
public:
    FuelSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
        : Slot(inventory, slotIndex, x, y) {}

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override {
        return !stack.isEmpty() && stack.getItem() == Items::BLAZE_POWDER;
    }

    [[nodiscard]] i32 getMaxStackSize() const override {
        return 64;
    }

    /**
     * @brief 检查是否为有效的酿造燃料
     */
    [[nodiscard]] static bool isValidBrewingFuel(const ItemStack& stack) {
        return !stack.isEmpty() && stack.getItem() == Items::BLAZE_POWDER;
    }
};

} // anonymous namespace

// ========== 构造函数 ==========

BrewingStandContainer::BrewingStandContainer(ContainerId id,
                                             PlayerInventory* playerInventory,
                                             IInventory* brewingStandInventory,
                                             BrewingStandEntity* brewingStandEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_brewingStandInventory(brewingStandInventory)
    , m_brewingStandEntity(brewingStandEntity) {

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(brewingStandInventory != nullptr);
    MC_ASSERT(brewingStandInventory->getContainerSize() == BREWING_SLOTS);

    initSlots(playerInventory);
}

// ========== 容器接口 ==========

bool BrewingStandContainer::stillValid(const Player& player) const {
    (void)player;
    // TODO: 检查玩家是否在酿造台附近
    return true;
}

void BrewingStandContainer::slotsChanged(IInventory* inventory) {
    (void)inventory;
    // 酿造台会自动检测材料变化并开始酿造
    // 这里不需要额外处理
}

ItemStack BrewingStandContainer::quickMoveStack(i32 slotIndex, Player& player) {
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack result = slotStack.copy();

    // 酿造台槽位范围：0-4（药水槽0-2，材料槽3，燃料槽4）
    if (slotIndex < BREWING_SLOTS) {
        // 从酿造台移动到玩家背包
        if (!moveItemToRange(slotStack, BREWING_SLOTS, getSlotCount() - 1, true)) {
            return ItemStack();
        }
    } else {
        // 从玩家背包移动到酿造台
        // 优先尝试燃料槽
        if (FuelSlot::isValidBrewingFuel(slotStack)) {
            if (!moveItemToRange(slotStack, SLOT_FUEL, SLOT_FUEL + 1, false)) {
                // 燃料槽已满，尝试材料槽
                if (!moveItemToRange(slotStack, SLOT_INGREDIENT, SLOT_INGREDIENT + 1, false)) {
                    return ItemStack();
                }
            }
        } else if (potion::PotionBrewing::isReagent(slotStack)) {
            // 材料放入材料槽
            if (!moveItemToRange(slotStack, SLOT_INGREDIENT, SLOT_INGREDIENT + 1, false)) {
                return ItemStack();
            }
        } else if (potion::PotionBrewing::isPotionItem(slotStack) && slotStack.getCount() == 1) {
            // 药水瓶放入药水槽（需要数量为1）
            if (!moveItemToRange(slotStack, SLOT_POTION_START, SLOT_POTION_START + POTION_SLOTS, false)) {
                return ItemStack();
            }
        } else {
            // 无法放入酿造台
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

void BrewingStandContainer::initSlots(PlayerInventory* playerInventory) {
    // ========== 酿造台槽位 ==========

    // 药水槽（3个，从上到下排列）
    // 位置参考 MC 1.16.5: (56, 51), (79, 58), (102, 51)
    addSlot(std::make_unique<PotionSlot>(m_brewingStandInventory, 0, POTION_SLOT_X, POTION_SLOT_Y[0]));
    addSlot(std::make_unique<PotionSlot>(m_brewingStandInventory, 1, 79, POTION_SLOT_Y[1]));
    addSlot(std::make_unique<PotionSlot>(m_brewingStandInventory, 2, 102, POTION_SLOT_Y[2]));

    // 材料槽（顶部中央）
    addSlot(std::make_unique<IngredientSlot>(m_brewingStandInventory, SLOT_INGREDIENT,
                                              INGREDIENT_SLOT_X, INGREDIENT_SLOT_Y));

    // 燃料槽（左侧）
    addSlot(std::make_unique<FuelSlot>(m_brewingStandInventory, SLOT_FUEL,
                                        FUEL_SLOT_X, FUEL_SLOT_Y));

    // ========== 玩家主背包（3行9列）==========

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col;
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = PLAYER_INV_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏（1行9列）==========

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col;
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = HOTBAR_Y;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }
}

} // namespace mc
