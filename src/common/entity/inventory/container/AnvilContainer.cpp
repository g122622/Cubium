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

#include "entity/inventory/container/AnvilContainer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/Items.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "item/enchantment/Enchantment.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/blocks/functional/AnvilBlock.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

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
    AnvilResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, AnvilContainer* container)
        : Slot(inventory, slotIndex, x, y)
        , m_container(container)
    {}

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override
    {
        (void)stack;
        return false; // 结果槽不能放入物品
    }

    [[nodiscard]] bool mayPickup(Player& player) const override
    {
        // 创造模式玩家可以无视经验等级要求取出物品
        if (player.isCreative()) {
            return m_container && !m_container->getOutputSlot().isEmpty();
        }
        // 生存模式检查经验等级是否足够
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
    static constexpr i32 SIZE = 3; // 输入1 + 输入2 + 输出

    i32 getContainerSize() const override { return SIZE; }

    bool isEmpty() const override { return m_items[0].isEmpty() && m_items[1].isEmpty() && m_items[2].isEmpty(); }

    ItemStack getItem(i32 slot) const override
    {
        if (slot >= 0 && slot < SIZE) {
            return m_items[slot];
        }
        return ItemStack();
    }

    void setItem(i32 slot, const ItemStack& stack) override
    {
        if (slot >= 0 && slot < SIZE) {
            m_items[slot] = stack;
            setChanged();
        }
    }

    ItemStack removeItem(i32 slot, i32 count) override
    {
        if (slot >= 0 && slot < SIZE) {
            return m_items[slot].split(count);
        }
        return ItemStack();
    }

    ItemStack removeItemNoUpdate(i32 slot) override
    {
        if (slot >= 0 && slot < SIZE) {
            ItemStack result = std::move(m_items[slot]);
            m_items[slot] = ItemStack();
            return result;
        }
        return ItemStack();
    }

    void clear() override
    {
        for (auto& item : m_items) {
            item = ItemStack();
        }
        setChanged();
    }

    void setChanged() override { m_changed = true; }

    bool isChanged() const { return m_changed; }
    void clearChanged() { m_changed = false; }

private:
    std::array<ItemStack, SIZE> m_items;
    bool m_changed = false;
};

} // anonymous namespace

// ========== 构造函数 ==========

AnvilContainer::AnvilContainer(
    ContainerId id, PlayerInventory* playerInventory, const BlockPos& position, IWorld* world)
    : AbstractContainerMenu(id, playerInventory)
    , m_anvilInventory(std::make_unique<AnvilInventory>())
    , m_position(position)
    , m_world(world)
{

    MC_ASSERT(playerInventory != nullptr);
    _initSlots(playerInventory);
}

// ========== 重命名 ==========

void AnvilContainer::setItemName(const std::string& name)
{
    m_itemName = name;
    _updateRepairOutput();
}

bool AnvilContainer::isRenameOnly() const
{
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

ItemStack AnvilContainer::getInputSlot1() const
{
    return m_anvilInventory->getItem(SLOT_INPUT_1);
}

ItemStack AnvilContainer::getInputSlot2() const
{
    return m_anvilInventory->getItem(SLOT_INPUT_2);
}

ItemStack AnvilContainer::getOutputSlot() const
{
    return m_anvilInventory->getItem(SLOT_OUTPUT);
}

// ========== 容器接口 ==========

bool AnvilContainer::stillValid(const Player& player) const
{
    // 检查玩家是否在铁砧附近（64格范围内）
    return isWithinDistance(player, m_position);
}

void AnvilContainer::slotsChanged(IInventory* inventory)
{
    if (inventory == m_anvilInventory.get()) {
        _updateRepairOutput();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

void AnvilContainer::removed(Player& player)
{
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

ItemStack AnvilContainer::quickMoveStack(i32 slotIndex, Player& player)
{

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

        // 铁砧损坏机制：非创造模式下，从输出槽取出结果时有 12% 概率使铁砧降级
        _damageAnvilIfNecessary(player);
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

void AnvilContainer::_initSlots(PlayerInventory* playerInventory)
{
    // ========== 铁砧槽位 ==========

    // 输入槽1（左侧）
    addSlot(std::make_unique<Slot>(m_anvilInventory.get(), SLOT_INPUT_1, INPUT_SLOT_X[0], INPUT_SLOT_Y));

    // 输入槽2（右侧）
    addSlot(std::make_unique<Slot>(m_anvilInventory.get(), SLOT_INPUT_2, INPUT_SLOT_X[1], INPUT_SLOT_Y));

    // 输出槽（底部中央）
    addSlot(std::make_unique<AnvilResultSlot>(m_anvilInventory.get(), SLOT_OUTPUT, OUTPUT_SLOT_X, OUTPUT_SLOT_Y, this));

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

void AnvilContainer::_updateRepairOutput()
{
    ItemStack input1 = getInputSlot1();
    m_repairCost = 1; // 基础成本
    i32 totalCost = 0;
    i32 renameCost = 0;
    m_materialCost = 0;

    if (input1.isEmpty()) {
        m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
        m_repairCost = 0;
        return;
    }

    ItemStack result = input1.copy();
    ItemStack input2 = getInputSlot2();

    // 获取输入物品的附魔
    auto enchantments1 = item::enchant::EnchantmentHelper::getEnchantments(result);

    // 计算基础修复成本（两个输入物品的修复成本之和）
    i32 baseRepairCost = input1.getRepairCost();
    if (!input2.isEmpty()) {
        baseRepairCost += input2.getRepairCost();
    }

    if (!input2.isEmpty()) {
        // 检查是否是附魔书
        bool isEnchantedBook =
            input2.getItem() != nullptr && input2.getItem() == Items::ENCHANTED_BOOK && input2.hasEnchantments();

        // 检查是否可以用材料修复
        bool canRepairWithMaterial = false;
        if (input1.isDamageable() && input1.getItem() != nullptr) {
            canRepairWithMaterial = input1.getItem()->getIsRepairable(input1, input2);
        }

        if (canRepairWithMaterial) {
            // 使用相同物品修复耐久度
            i32 repairAmount = std::min(input1.getDamage(), input1.getMaxDamage() / 4);

            if (repairAmount <= 0) {
                m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
                m_repairCost = 0;
                return;
            }

            i32 materialUsed = 0;
            while (repairAmount > 0 && materialUsed < input2.getCount()) {
                i32 newDamage = result.getDamage() - repairAmount;
                result.setDamage(std::max(0, newDamage));
                ++materialUsed;
                ++totalCost;
                repairAmount = std::min(result.getDamage(), result.getMaxDamage() / 4);
            }
            m_materialCost = materialUsed;
        } else {
            // 不是材料修复，检查是否可以合并
            if (!isEnchantedBook && (input1.getItem() != input2.getItem() || !input1.isDamageable())) {
                // 不同物品不能合并（除非是附魔书）
                m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
                m_repairCost = 0;
                return;
            }

            // 合并耐久度（如果不是附魔书）
            if (input1.isDamageable() && !isEnchantedBook) {
                i32 durability1 = input1.getMaxDamage() - input1.getDamage();
                i32 durability2 = input2.getMaxDamage() - input2.getDamage();
                i32 bonus = durability2 + input1.getMaxDamage() * 12 / 100;
                i32 totalDurability = durability1 + bonus;
                i32 newDamage = input1.getMaxDamage() - totalDurability;

                if (newDamage < 0) {
                    newDamage = 0;
                }

                if (newDamage < result.getDamage()) {
                    result.setDamage(newDamage);
                    totalCost += 2;
                }
            }

            // 合并附魔
            auto enchantments2 = item::enchant::EnchantmentHelper::getEnchantments(input2);
            bool hasValidEnchantment = false;
            bool hasIncompatibleEnchantment = false;

            for (const auto& [enchant2, level2] : enchantments2) {
                if (enchant2 == nullptr) continue;

                // 查找是否已有此附魔
                i32 existingLevel = 0;
                for (const auto& [enchant1, level1] : enchantments1) {
                    if (enchant1 == enchant2) {
                        existingLevel = level1;
                        break;
                    }
                }

                // 计算新等级：同等级+1，否则取最大值
                i32 newLevel = (existingLevel == level2) ? level2 + 1 : std::max(level2, existingLevel);
                newLevel = std::min(newLevel, enchant2->maxLevel());

                // 检查附魔是否可以应用到结果物品
                bool canApply = enchant2->canApply(result);
                // 创造模式或附魔书可以应用任何附魔
                if (isPlayerCreative() || input2.getItem() == Items::ENCHANTED_BOOK) {
                    canApply = true;
                }

                // 检查与现有附魔的兼容性
                for (const auto& [enchant1, level1] : enchantments1) {
                    if (enchant1 != enchant2 && !enchant2->isCompatibleWith(*enchant1)) {
                        canApply = false;
                        ++totalCost; // 冲突增加成本
                        break;
                    }
                }

                if (!canApply) {
                    hasIncompatibleEnchantment = true;
                } else {
                    hasValidEnchantment = true;

                    // 更新附魔等级
                    bool found = false;
                    for (auto& [enchant, level] : enchantments1) {
                        if (enchant == enchant2) {
                            level = newLevel;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        enchantments1.push_back({enchant2, newLevel});
                    }

                    // 计算附魔成本（基于稀有度）
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

                    totalCost += rarityCost * newLevel;

                    // 堆叠物品限制（可堆叠物品附魔成本设为40，即"太贵"）
                    if (input1.getCount() > 1) {
                        totalCost = 40;
                    }
                }
            }

            // 如果只有不兼容的附魔，清空结果
            if (hasIncompatibleEnchantment && !hasValidEnchantment) {
                m_anvilInventory->setItem(SLOT_OUTPUT, ItemStack());
                m_repairCost = 0;
                return;
            }
        }
    }

    // 处理重命名
    if (m_itemName.empty()) {
        // 清除自定义名称
        if (input1.hasCustomName()) {
            renameCost = 1;
            totalCost += renameCost;
            result.clearCustomName();
        }
    } else if (m_itemName != input1.getCustomName()) {
        // 设置新名称
        renameCost = 1;
        totalCost += renameCost;
        result.setCustomName(m_itemName);
    }

    // 计算最终修复成本
    m_repairCost = baseRepairCost + totalCost;

    if (totalCost <= 0) {
        result = ItemStack();
    }

    // 特殊情况：如果只有重命名且成本达到40，降低到39
    if (renameCost == totalCost && renameCost > 0 && m_repairCost >= MAX_REPAIR_COST) {
        m_repairCost = 39;
    }

    // 检查是否太贵
    // 创造模式可以绕过 40 级费用上限
    if (m_repairCost >= MAX_REPAIR_COST && !isPlayerCreative()) {
        result = ItemStack();
        m_repairCost = 0;
    }

    if (!result.isEmpty()) {
        // 计算并设置结果物品的修复成本
        i32 newRepairCost = result.getRepairCost();
        if (!input2.isEmpty() && newRepairCost < input2.getRepairCost()) {
            newRepairCost = input2.getRepairCost();
        }

        // 如果不只是重命名，增加修复成本
        if (renameCost != totalCost || renameCost == 0) {
            newRepairCost = _getNewRepairCost(newRepairCost);
        }

        result.setRepairCost(newRepairCost);

        // 应用附魔到结果物品
        item::enchant::EnchantmentHelper::setEnchantments(enchantments1, result);
    }

    m_anvilInventory->setItem(SLOT_OUTPUT, result);

    // 同步变化到客户端
    detectAndSendChanges();
}

i32 AnvilContainer::_getNewRepairCost(i32 oldRepairCost)
{
    return oldRepairCost * 2 + 1;
}

bool AnvilContainer::isPlayerCreative() const
{
    // 创造模式玩家在铁砧中有特殊权限
    if (m_playerInventory == nullptr) {
        return false;
    }
    Player* player = m_playerInventory->getPlayer();
    return player != nullptr && player->isCreative();
}

void AnvilContainer::_damageAnvilIfNecessary(Player& player)
{
    // 铁砧损坏逻辑仅在服务端执行
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 创造模式玩家不会触发铁砧损坏，但仍需播放使用音效
    if (player.isCreative()) {
        m_world->playEvent(world::WorldEvents::ANVIL_USE_SOUND, m_position, 0);
        return;
    }

    // 获取铁砧当前位置的方块状态
    const BlockState* currentState = m_world->getBlockState(m_position);
    if (currentState == nullptr) {
        return;
    }

    // 确认当前方块确实是铁砧（通过 BlockTags 判断）
    if (!BlockTags::ANVIL().contains(currentState->getBlock())) {
        return;
    }

    // 12% 概率触发损坏
    math::Random& rng = m_world->getRandom();
    if (rng.nextFloat() < 0.12f) {
        // 调用 damageAnvil 获取降级后的方块状态
        const BlockState* damagedState = blocks::AnvilBlock::damageAnvil(*currentState);
        if (damagedState != nullptr) {
            // 降级成功：替换为损坏等级更高的铁砧
            m_world->setBlockState(m_position, damagedState, 3);
        } else {
            // 完全损坏：移除铁砧方块（设为空气）
            m_world->setBlockState(m_position, nullptr, 3);
        }
        // 完全损坏播放 1029 (ANVIL_DESTROYED_SOUND)，降级播放 1030 (ANVIL_USE_SOUND)
        if (damagedState == nullptr) {
            m_world->playEvent(world::WorldEvents::ANVIL_DESTROYED_SOUND, m_position, 0);
        } else {
            m_world->playEvent(world::WorldEvents::ANVIL_USE_SOUND, m_position, 0);
        }
    } else {
        // 未触发损坏，播放使用音效
        m_world->playEvent(world::WorldEvents::ANVIL_USE_SOUND, m_position, 0);
    }
}

} // namespace mc
