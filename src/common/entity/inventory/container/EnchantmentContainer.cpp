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

#include "EnchantmentContainer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/Items.hpp"
#include "item/core/Item.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "item/items/special/EnchantedBookItem.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/blockentity/interactive/EnchantingTableEntity.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// ========== 简单背包实现 ==========

namespace {

/**
 * @brief 附魔台背包
 */
class EnchantmentInventory : public IInventory {
public:
    static constexpr i32 SIZE = 2; // 物品槽 + 青金石槽

    i32 getContainerSize() const override { return SIZE; }

    bool isEmpty() const override { return m_items[0].isEmpty() && m_items[1].isEmpty(); }

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

/**
 * @brief 物品槽
 */
class EnchantmentItemSlot : public Slot {
public:
    EnchantmentItemSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
        : Slot(inventory, slotIndex, x, y)
    {}

    bool mayPlace(const ItemStack& stack) const override
    {
        if (stack.isEmpty() || stack.getItem() == nullptr) {
            return false;
        }
        // 只接受可附魔的物品
        i32 enchantability = stack.getItem()->getItemEnchantability();
        return enchantability > 0;
    }

    i32 getMaxStackSize(const ItemStack& stack) const override
    {
        // 附魔物品只能放一个
        return 1;
    }
};

/**
 * @brief 青金石槽
 */
class LapisSlot : public Slot {
public:
    LapisSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
        : Slot(inventory, slotIndex, x, y)
    {}

    bool mayPlace(const ItemStack& stack) const override
    {
        // 只接受青金石
        return !stack.isEmpty() && stack.getItem() != nullptr && stack.getItem() == Items::LAPIS_LAZULI;
    }
};

} // anonymous namespace

// ========== EnchantmentContainer 实现 ==========

EnchantmentContainer::EnchantmentContainer(
    ContainerId id, PlayerInventory* playerInventory, const BlockPos& position, IWorld* world)
    : AbstractContainerMenu(id, playerInventory)
    , m_enchantmentInventory(std::make_unique<EnchantmentInventory>())
    , m_position(position)
    , m_world(world)
    , m_random(0)
{
    _initSlots(playerInventory);
    m_enchantPower = _calculateEnchantPower();
}

void EnchantmentContainer::_initSlots(PlayerInventory* playerInventory)
{
    // 添加附魔台槽位
    addSlot(std::make_unique<EnchantmentItemSlot>(m_enchantmentInventory.get(), SLOT_ITEM, ITEM_SLOT_X, ITEM_SLOT_Y));
    addSlot(std::make_unique<LapisSlot>(m_enchantmentInventory.get(), SLOT_LAPIS, LAPIS_SLOT_X, LAPIS_SLOT_Y));

    // 添加玩家背包槽位
    addPlayerInventorySlots(PLAYER_INV_Y, PLAYER_INV_Y);
    addPlayerHotbarSlots(PLAYER_INV_Y, HOTBAR_Y);
}

ItemStack EnchantmentContainer::getItemSlot() const
{
    return m_enchantmentInventory->getItem(SLOT_ITEM);
}

ItemStack EnchantmentContainer::getLapisSlot() const
{
    return m_enchantmentInventory->getItem(SLOT_LAPIS);
}

i32 EnchantmentContainer::getEnchantmentLevel(i32 index) const
{
    if (index >= 0 && index < ENCHANTMENT_OPTIONS) {
        return m_enchantmentLevels[index];
    }
    return 0;
}

std::string EnchantmentContainer::getEnchantmentClue(i32 index) const
{
    if (index >= 0 && index < ENCHANTMENT_OPTIONS) {
        return m_enchantmentClues[index];
    }
    return "";
}

std::string EnchantmentContainer::getEnchantmentClueId(i32 index) const
{
    return getEnchantmentClue(index);
}

i32 EnchantmentContainer::getEnchantmentWorldClue(i32 index) const
{
    if (index >= 0 && index < ENCHANTMENT_OPTIONS) {
        return m_enchantmentWorldClues[index];
    }
    return 0;
}

bool EnchantmentContainer::isEnchantmentOptionAvailable(i32 index) const
{
    if (index < 0 || index >= ENCHANTMENT_OPTIONS) {
        return false;
    }

    ItemStack item = getItemSlot();
    if (item.isEmpty()) {
        return false;
    }

    i32 level = m_enchantmentLevels[index];
    if (level <= 0) {
        return false;
    }

    // 检查青金石是否足够
    ItemStack lapis = getLapisSlot();
    i32 lapisNeeded = index + 1; // 槽位0需要1个，槽位1需要2个，槽位2需要3个
    if (lapis.getCount() < lapisNeeded) {
        return false;
    }

    return true;
}

bool EnchantmentContainer::isPlayerCreative() const
{
    if (m_playerInventory == nullptr) {
        return false;
    }
    Player* player = m_playerInventory->getPlayer();
    return player != nullptr && player->isCreative();
}

bool EnchantmentContainer::enchantItem(Player& player, i32 optionIndex)
{
    if (!isEnchantmentOptionAvailable(optionIndex)) {
        return false;
    }

    i32 level = m_enchantmentLevels[optionIndex];
    if (level <= 0) {
        return false;
    }

    // 消耗的经验等级为选项索引+1，消耗青金石数量也为选项索引+1
    i32 cost = optionIndex + 1;

    // 消耗青金石
    ItemStack lapis = m_enchantmentInventory->getItem(SLOT_LAPIS);
    lapis.shrink(cost);
    m_enchantmentInventory->setItem(SLOT_LAPIS, lapis);

    // 创造模式不消耗经验
    if (!player.isCreative()) {
        player.addExperienceLevels(-cost);
    }

    // 获取物品
    ItemStack item = m_enchantmentInventory->getItem(SLOT_ITEM);
    if (item.isEmpty()) {
        return false;
    }

    // 使用确定性的种子重新生成附魔列表
    math::Random enchantRandom(m_enchantmentSeed + optionIndex);

    // 构建附魔列表
    auto enchantments = item::enchant::EnchantmentHelper::buildEnchantmentList(enchantRandom, item, level, false);

    if (enchantments.empty()) {
        // 如果没有可用附魔，仍然更新种子
        _updateEnchantmentSeed(player);
        _updateEnchantmentOptions();
        return true;
    }

    // 检查是否是书 -> 附魔书转换
    bool isBook = item.getItem() != nullptr && item.getItem() == Items::BOOK;

    if (isBook) {
        // 创建附魔书物品
        ItemStack enchantedBook(Items::ENCHANTED_BOOK, 1);

        // 复制原有NBT标签（如果有）
        if (item.hasTag()) {
            const nlohmann::json* customData = item.getTag();
            if (customData != nullptr) {
                enchantedBook.getOrCreateTag() = *customData;
            }
        }

        // 复制自定义名称
        if (item.hasCustomName()) {
            const text::ITextComponent* customName = item.getCustomNameComponent();
            if (customName != nullptr) {
                enchantedBook.setCustomNameComponent(customName->deepCopy());
            }
        }

        // 替换物品槽中的书为附魔书
        item = enchantedBook;
        m_enchantmentInventory->setItem(SLOT_ITEM, item);
    }

    // 应用附魔
    for (const auto& data : enchantments) {
        if (data.enchantment != nullptr && data.level > 0) {
            if (isBook) {
                // 附魔书使用 StoredEnchantments 标签存储附魔
                item::items::EnchantedBookItem::addEnchantment(item, *data.enchantment, data.level);
            } else {
                // 普通物品使用 Enchantments 标签存储附魔
                item.addEnchantment(data.enchantment->id(), data.level);
            }
        }
    }

    m_enchantmentInventory->setItem(SLOT_ITEM, item);

    // 触发附魔事件（进度系统）
    if (m_world != nullptr) {
        m_world->onEnchantItem(player.id(), item, level);
    }

    // 更新种子
    _updateEnchantmentSeed(player);
    _updateEnchantmentOptions();

    return true;
}

bool EnchantmentContainer::stillValid(const Player& player) const
{
    // 检查玩家是否在附魔台附近（64格范围内）
    return isWithinDistance(player, m_position);
}

void EnchantmentContainer::slotsChanged(IInventory* inventory)
{
    if (inventory == m_enchantmentInventory.get()) {
        // 物品变化时更新附魔选项
        _updateEnchantmentOptions();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack EnchantmentContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack result = slotStack.copy();

    if (slotIndex < ENCHANTMENT_SLOTS) {
        // 从附魔台槽位移动到玩家背包
        if (!moveItemToRange(slotStack, ENCHANTMENT_SLOTS, getSlotCount() - 1, true)) {
            return ItemStack();
        }
    } else {
        // 从玩家背包移动到附魔台
        // 尝试放入青金石槽
        if (slotStack.getItem() && slotStack.getItem() == Items::LAPIS_LAZULI) {
            if (!moveItemToRange(slotStack, SLOT_LAPIS, SLOT_LAPIS + 1, false)) {
                // 尝试放入物品槽
                if (!moveItemToRange(slotStack, SLOT_ITEM, SLOT_ITEM + 1, false)) {
                    return ItemStack();
                }
            }
        } else {
            // 放入物品槽
            if (!moveItemToRange(slotStack, SLOT_ITEM, SLOT_ITEM + 1, false)) {
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

void EnchantmentContainer::_updateEnchantmentOptions()
{
    ItemStack item = getItemSlot();

    if (item.isEmpty()) {
        m_enchantmentLevels = {0, 0, 0};
        m_enchantmentClues = {"", "", ""};
        m_enchantmentWorldClues = {0, 0, 0};
        return;
    }

    // 计算书架力量
    m_enchantPower = _calculateEnchantPower();

    // 使用种子生成附魔选项
    m_random.setSeed(m_enchantmentSeed);

    for (i32 i = 0; i < ENCHANTMENT_OPTIONS; ++i) {
        // 计算附魔等级
        m_enchantmentLevels[i] =
            item::enchant::EnchantmentHelper::calcItemStackEnchantability(m_random, i, m_enchantPower, item);

        // 生成附魔预览
        if (m_enchantmentLevels[i] > 0) {
            // 每个槽位的预览是确定性的
            m_random.setSeed(m_enchantmentSeed + i);
            auto enchantments =
                item::enchant::EnchantmentHelper::buildEnchantmentList(m_random, item, m_enchantmentLevels[i], false);

            if (!enchantments.empty()) {
                // 只显示第一个附魔作为预览
                const auto& first = enchantments[0];
                if (first.enchantment != nullptr) {
                    m_enchantmentClues[i] = first.enchantment->id();
                    m_enchantmentWorldClues[i] = first.level;
                } else {
                    m_enchantmentClues[i] = "";
                    m_enchantmentWorldClues[i] = 0;
                }
            } else {
                m_enchantmentClues[i] = "";
                m_enchantmentWorldClues[i] = 0;
            }
        } else {
            m_enchantmentClues[i] = "";
            m_enchantmentWorldClues[i] = 0;
        }
    }

    // 同步变化到客户端
    detectAndSendChanges();
}

void EnchantmentContainer::_updateEnchantmentSeed(Player& player)
{
    // 使用玩家的 XP 种子更新附魔种子
    m_enchantmentSeed = static_cast<i64>(player.xpSeed());
}

i32 EnchantmentContainer::_calculateEnchantPower() const
{
    if (!m_world) {
        return 0;
    }

    i32 power = 0;
    BlockPos tablePos = m_position;

    // 使用与EnchantingTableEntity相同的书架偏移量列表和验证逻辑
    // 对应MC的EnchantingTableBlock.BOOKSHELF_OFFSETS
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                // 仅检查外圈位置（|x|==2 或 |z|==2）
                if (std::abs(x) != 2 && std::abs(z) != 2) {
                    continue;
                }

                BlockPos offset(x, y, z);
                if (blockentity::EnchantingTableEntity::isValidBookshelf(*m_world, tablePos, offset)) {
                    power++;
                }
            }
        }
    }

    return std::min(power, 15);
}

} // namespace mc
