#include "entity/inventory/container/AnvilContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/core/Item.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/Enchantment.hpp"
#include "network/packet/PacketSerializer.hpp"
#include "util/assert/AssertAll.hpp"
#include <algorithm>

namespace mc {

namespace {

/// 槽位宽度
constexpr i32 SLOT_SIZE = 18;

/**
 * @brief 结果槽位
 *
 * 不能直接放入物品，只能取出。
 */
class AnvilResultSlot : public Slot {
public:
    AnvilResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y,
                    AnvilContainer* container)
        : Slot(inventory, slotIndex, x, y)
        , m_container(container) {}

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override {
        (void)stack;
        return false;  // 结果槽不能放入物品
    }

    [[nodiscard]] bool mayPickup(Player& player) const override {
        (void)player;
        // 检查是否可以取出（经验等级是否足够）
        return m_container && !m_container->isTooExpensive();
    }

private:
    AnvilContainer* m_container;
};

/**
 * @brief 简单背包实现
 */
class AnvilInventory : public IInventory {
public:
    static constexpr i32 SIZE = 3;  // 输入1 + 输入2 + 输出

    i32 getContainerSize() const override { return SIZE; }

    bool isEmpty() const override {
        return m_items[0].isEmpty() && m_items[1].isEmpty() && m_items[2].isEmpty();
    }

    ItemStack getItem(i32 slot) const override {
        if (slot >= 0 && slot < SIZE) {
            return m_items[slot];
        }
        return ItemStack();
    }

    void setItem(i32 slot, const ItemStack& stack) override {
        if (slot >= 0 && slot < SIZE) {
            m_items[slot] = stack;
            setChanged();
        }
    }

    ItemStack removeItem(i32 slot, i32 count) override {
        if (slot >= 0 && slot < SIZE) {
            return m_items[slot].split(count);
        }
        return ItemStack();
    }

    ItemStack removeItemNoUpdate(i32 slot) override {
        if (slot >= 0 && slot < SIZE) {
            ItemStack result = std::move(m_items[slot]);
            m_items[slot] = ItemStack();
            return result;
        }
        return ItemStack();
    }

    void clear() override {
        for (auto& item : m_items) {
            item = ItemStack();
        }
        setChanged();
    }

    void setChanged() override {
        m_changed = true;
    }

    void serialize(network::PacketSerializer& ser) const override {
        // 简单序列化：写入槽位数量和每个物品
        ser.writeVarInt(SIZE);
        for (const auto& item : m_items) {
            item.serialize(ser);
        }
    }

    bool isChanged() const { return m_changed; }
    void clearChanged() { m_changed = false; }

private:
    std::array<ItemStack, SIZE> m_items;
    bool m_changed = false;
};

} // anonymous namespace

// ========== 构造函数 ==========

AnvilContainer::AnvilContainer(ContainerId id,
                               PlayerInventory* playerInventory,
                               const BlockPos& position,
                               IWorld* world)
    : AbstractContainerMenu(id, playerInventory)
    , m_anvilInventory(std::make_unique<AnvilInventory>())
    , m_position(position)
    , m_world(world) {

    MC_ASSERT(playerInventory != nullptr);
    initSlots(playerInventory);
}

// ========== 重命名 ==========

void AnvilContainer::setItemName(const String& name) {
    m_itemName = name;
    updateRepairOutput();
}

bool AnvilContainer::isRenameOnly() const {
    if (m_repairCost <= 0) {
        return false;
    }

    ItemStack input1 = getInputSlot1();
    ItemStack input2 = getInputSlot2();

    // 如果没有输入物品，返回false
    if (input1.isEmpty()) {
        return false;
    }

    // 如果有第二个输入物品，则不是仅重命名
    if (!input2.isEmpty()) {
        return false;
    }

    // 只有重命名操作（有自定义名称或名称改变）
    return m_repairCost == 1 && !m_itemName.empty();
}

// ========== 槽位访问 ==========

ItemStack AnvilContainer::getInputSlot1() const {
    return m_anvilInventory->getItem(SLOT_INPUT_1);
}

ItemStack AnvilContainer::getInputSlot2() const {
    return m_anvilInventory->getItem(SLOT_INPUT_2);
}

ItemStack AnvilContainer::getOutputSlot() const {
    return m_anvilInventory->getItem(SLOT_OUTPUT);
}

// ========== 容器接口 ==========

bool AnvilContainer::stillValid(const Player& player) const {
    (void)player;
    // TODO: 检查玩家是否在铁砧附近
    return true;
}

void AnvilContainer::slotsChanged(IInventory* inventory) {
    if (inventory == m_anvilInventory.get()) {
        updateRepairOutput();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

void AnvilContainer::removed(Player& player) {
    // 返回输入槽的物品给玩家
    ItemStack input1 = getInputSlot1();
    if (!input1.isEmpty()) {
        player.inventory().add(input1);
    }

    ItemStack input2 = getInputSlot2();
    if (!input2.isEmpty()) {
        player.inventory().add(input2);
    }

    // 清空所有槽位
    m_anvilInventory->clear();

    AbstractContainerMenu::removed(player);
}

ItemStack AnvilContainer::quickMoveStack(i32 slotIndex, Player& player) {
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack result = slotStack.copy();

    if (slotIndex == SLOT_OUTPUT) {
        // 从输出槽移动到玩家背包
        if (!moveItemToRange(slotStack, ANVIL_SLOTS, getSlotCount() - 1, true)) {
            return ItemStack();
        }
    } else if (slotIndex == SLOT_INPUT_1 || slotIndex == SLOT_INPUT_2) {
        // 从输入槽移动到玩家背包
        if (!moveItemToRange(slotStack, ANVIL_SLOTS, getSlotCount() - 1, false)) {
            return ItemStack();
        }
    } else {
        // 从玩家背包移动到输入槽
        // 优先放入输入槽1
        if (!moveItemToRange(slotStack, SLOT_INPUT_1, SLOT_INPUT_1 + 1, false)) {
            // 输入槽1已满，尝试放入输入槽2
            if (!moveItemToRange(slotStack, SLOT_INPUT_2, SLOT_INPUT_2 + 1, false)) {
                return ItemStack();
            }
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

void AnvilContainer::initSlots(PlayerInventory* playerInventory) {
    // ========== 铁砧槽位 ==========

    // 输入槽1（左侧）
    addSlot(std::make_unique<Slot>(m_anvilInventory.get(), SLOT_INPUT_1,
                                    INPUT_SLOT_X[0], INPUT_SLOT_Y));

    // 输入槽2（右侧）
    addSlot(std::make_unique<Slot>(m_anvilInventory.get(), SLOT_INPUT_2,
                                    INPUT_SLOT_X[1], INPUT_SLOT_Y));

    // 输出槽（底部中央）
    addSlot(std::make_unique<AnvilResultSlot>(m_anvilInventory.get(), SLOT_OUTPUT,
                                               OUTPUT_SLOT_X, OUTPUT_SLOT_Y, this));

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

void AnvilContainer::updateRepairOutput() {
    ItemStack input1 = getInputSlot1();
    m_repairCost = 0;
    m_materialCost = 0;

    if (input1.isEmpty()) {
        m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
        return;
    }

    ItemStack result = input1.copy();
    ItemStack input2 = getInputSlot2();

    // 计算基础修复成本
    i32 baseCost = input1.getRepairCost();
    if (!input2.isEmpty()) {
        baseCost += input2.getRepairCost();
    }

    bool hasChanges = false;

    if (!input2.isEmpty()) {
        // 检查是否是附魔书合并
        bool isEnchantedBook = input2.getItem() != nullptr &&
                               input2.getItem()->itemLocation().toString() == "minecraft:enchanted_book" &&
                               input2.hasEnchantments();

        // 检查是否可以修复
        if (input1.isDamageable() && input1.getItem()->isRepairable() &&
            input1.getItem() == input2.getItem()) {
            // 使用相同物品修复耐久度
            i32 damage1 = input1.getDamage();
            i32 maxDamage = input1.getMaxDamage();
            i32 repairAmount = std::min(damage1, maxDamage / 4);

            i32 materialUsed = 0;
            while (repairAmount > 0 && materialUsed < input2.getCount()) {
                result.setDamage(std::max(0, result.getDamage() - repairAmount));
                ++materialUsed;
                repairAmount = std::min(result.getDamage(), maxDamage / 4);
            }
            m_materialCost = materialUsed;
            m_repairCost += 2;
            hasChanges = true;
        } else if (!isEnchantedBook && input1.getItem() != input2.getItem()) {
            // 不同物品不能合并（除非是附魔书）
            m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
            m_repairCost = 0;
            return;
        }

        // 合并附魔
        auto enchantments1 = item::enchant::EnchantmentHelper::getEnchantments(input1);
        auto enchantments2 = item::enchant::EnchantmentHelper::getEnchantments(input2);

        i32 enchantmentCost = 0;
        bool hasConflict = false;
        bool hasValidEnchantment = false;

        for (const auto& [enchant2, level2] : enchantments2) {
            if (enchant2 == nullptr) continue;

            i32 existingLevel = 0;
            for (const auto& [enchant1, level1] : enchantments1) {
                if (enchant1 == enchant2) {
                    existingLevel = level1;
                    break;
                }
            }

            i32 newLevel = (existingLevel == level2) ? level2 + 1 : std::max(level2, existingLevel);
            newLevel = std::min(newLevel, enchant2->maxLevel());

            // 检查附魔兼容性
            bool isCompatible = true;
            for (const auto& [enchant1, level1] : enchantments1) {
                if (enchant1 != enchant2 && !enchant2->isCompatibleWith(*enchant1)) {
                    isCompatible = false;
                    ++enchantmentCost;  // 冲突增加成本
                    break;
                }
            }

            if (!isCompatible) {
                hasConflict = true;
            } else {
                hasValidEnchantment = true;
                // 添加附魔到结果
                result.addEnchantment(enchant2->id(), newLevel);

                // 计算附魔成本
                i32 rarityCost = 0;
                using namespace item::enchant;
                switch (enchant2->rarity()) {
                    case EnchantmentRarity::Common:
                        rarityCost = 1;
                        break;
                    case EnchantmentRarity::Uncommon:
                        rarityCost = 2;
                        break;
                    case EnchantmentRarity::Rare:
                        rarityCost = 4;
                        break;
                    case EnchantmentRarity::VeryRare:
                        rarityCost = 8;
                        break;
                }

                if (isEnchantedBook) {
                    rarityCost = std::max(1, rarityCost / 2);
                }

                enchantmentCost += rarityCost * newLevel;

                // 堆叠物品限制
                if (input1.getCount() > 1) {
                    enchantmentCost = 40;  // 太贵
                }
            }
        }

        if (hasConflict && !hasValidEnchantment) {
            m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
            m_repairCost = 0;
            return;
        }

        m_repairCost += enchantmentCost;
        hasChanges = hasChanges || hasValidEnchantment;
    }

    // 处理重命名
    if (!m_itemName.empty() && m_itemName != input1.getCustomName()) {
        result.setCustomName(m_itemName);
        m_repairCost += 1;
        hasChanges = true;
    } else if (m_itemName.empty() && input1.hasCustomName()) {
        // 清除自定义名称
        result.clearCustomName();
        m_repairCost += 1;
        hasChanges = true;
    }

    // 检查是否太贵
    if (m_repairCost >= MAX_REPAIR_COST) {
        m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
        m_repairCost = 0;
        return;
    }

    // 计算最终修复成本
    m_repairCost = baseCost + m_repairCost;

    if (!hasChanges || m_repairCost <= 0) {
        m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
        m_repairCost = 0;
        return;
    }

    // 设置结果
    m_anvilInventory->setItem(SLOT_OUTPUT, result);
}

i32 AnvilContainer::calculateRepairCost() {
    return m_repairCost;
}

bool AnvilContainer::tryRepair() {
    // 修复逻辑已在 updateRepairOutput 中处理
    return true;
}

bool AnvilContainer::tryCombine() {
    // 合并逻辑已在 updateRepairOutput 中处理
    return true;
}

bool AnvilContainer::tryCombineEnchantedBooks() {
    // 附魔书合并逻辑已在 updateRepairOutput 中处理
    return true;
}

bool AnvilContainer::tryRename() {
    // 重命名逻辑已在 updateRepairOutput 中处理
    return true;
}

bool AnvilContainer::areEnchantmentsCompatible(const String& ench1, const String& ench2) const {
    auto* enchantment1 = item::enchant::EnchantmentRegistry::get(ench1);
    auto* enchantment2 = item::enchant::EnchantmentRegistry::get(ench2);

    if (enchantment1 == nullptr || enchantment2 == nullptr) {
        return true;  // 未知附魔，默认兼容
    }

    return enchantment1->isCompatibleWith(*enchantment2);
}

} // namespace mc
