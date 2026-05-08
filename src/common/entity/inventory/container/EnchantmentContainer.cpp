#include "EnchantmentContainer.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/core/Item.hpp"
#include "item/Items.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "network/packet/PacketSerializer.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/VanillaBlocks.hpp"
#include <algorithm>

namespace mc {

// ========== 简单背包实现 ==========

namespace {

/**
 * @brief 附魔台背包
 */
class EnchantmentInventory : public IInventory {
public:
    static constexpr i32 SIZE = 2;  // 物品槽 + 青金石槽

    i32 getContainerSize() const override { return SIZE; }

    bool isEmpty() const override {
        return m_items[0].isEmpty() && m_items[1].isEmpty();
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

/**
 * @brief 物品槽
 */
class EnchantmentItemSlot : public Slot {
public:
    EnchantmentItemSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
        : Slot(inventory, slotIndex, x, y) {}

    bool mayPlace(const ItemStack& stack) const override {
        if (stack.isEmpty() || stack.getItem() == nullptr) {
            return false;
        }
        // 只接受可附魔的物品
        i32 enchantability = stack.getItem()->getItemEnchantability();
        return enchantability > 0;
    }

    i32 getMaxStackSize(const ItemStack& stack) const override {
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
        : Slot(inventory, slotIndex, x, y) {}

    bool mayPlace(const ItemStack& stack) const override {
        // 只接受青金石
        return !stack.isEmpty() && stack.getItem() != nullptr &&
               stack.getItem() == Items::LAPIS_LAZULI;
    }
};

} // anonymous namespace

// ========== EnchantmentContainer 实现 ==========

EnchantmentContainer::EnchantmentContainer(ContainerId id,
                                           PlayerInventory* playerInventory,
                                           const BlockPos& position,
                                           IWorld* world)
    : AbstractContainerMenu(id, playerInventory)
    , m_enchantmentInventory(std::make_unique<EnchantmentInventory>())
    , m_position(position)
    , m_world(world)
    , m_random(0) {  // 初始化随机数生成器，种子稍后设置
    initSlots(playerInventory);
    m_enchantPower = calculateEnchantPower();
}

void EnchantmentContainer::initSlots(PlayerInventory* playerInventory) {
    // 添加附魔台槽位
    addSlot(std::make_unique<EnchantmentItemSlot>(m_enchantmentInventory.get(), SLOT_ITEM, ITEM_SLOT_X, ITEM_SLOT_Y));
    addSlot(std::make_unique<LapisSlot>(m_enchantmentInventory.get(), SLOT_LAPIS, LAPIS_SLOT_X, LAPIS_SLOT_Y));

    // 添加玩家背包槽位
    addPlayerInventorySlots(PLAYER_INV_Y, PLAYER_INV_Y);
    addPlayerHotbarSlots(PLAYER_INV_Y, HOTBAR_Y);
}

ItemStack EnchantmentContainer::getItemSlot() const {
    return m_enchantmentInventory->getItem(SLOT_ITEM);
}

ItemStack EnchantmentContainer::getLapisSlot() const {
    return m_enchantmentInventory->getItem(SLOT_LAPIS);
}

i32 EnchantmentContainer::getEnchantmentLevel(i32 index) const {
    if (index >= 0 && index < ENCHANTMENT_OPTIONS) {
        return m_enchantmentLevels[index];
    }
    return 0;
}

std::string EnchantmentContainer::getEnchantmentClue(i32 index) const {
    if (index >= 0 && index < ENCHANTMENT_OPTIONS) {
        return m_enchantmentClues[index];
    }
    return "";
}

std::string EnchantmentContainer::getEnchantmentClueId(i32 index) const {
    return getEnchantmentClue(index);
}

i32 EnchantmentContainer::getEnchantmentWorldClue(i32 index) const {
    if (index >= 0 && index < ENCHANTMENT_OPTIONS) {
        return m_enchantmentWorldClues[index];
    }
    return 0;
}

bool EnchantmentContainer::isEnchantmentOptionAvailable(i32 index) const {
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
    i32 lapisNeeded = index + 1;  // 槽位0需要1个，槽位1需要2个，槽位2需要3个
    if (lapis.getCount() < lapisNeeded) {
        return false;
    }

    return true;
}

bool EnchantmentContainer::enchantItem(Player& player, i32 optionIndex) {
    if (!isEnchantmentOptionAvailable(optionIndex)) {
        return false;
    }

    i32 level = m_enchantmentLevels[optionIndex];
    if (level <= 0) {
        return false;
    }

    // MC 1.16.5: 消耗的经验等级为选项索引+1
    // 消耗青金石数量也为选项索引+1
    i32 cost = optionIndex + 1;

    // 消耗青金石
    ItemStack lapis = m_enchantmentInventory->getItem(SLOT_LAPIS);
    lapis.shrink(cost);
    m_enchantmentInventory->setItem(SLOT_LAPIS, lapis);

    // MC 1.16.5: 消耗玩家经验（创造模式不消耗）
    // 参考 EnchantmentContainer line 214-215
    if (!player.isCreative()) {
        player.addExperienceLevels(-cost);
    }

    // 获取物品
    ItemStack item = m_enchantmentInventory->getItem(SLOT_ITEM);
    if (item.isEmpty()) {
        return false;
    }

    // 使用确定性的种子重新生成附魔列表
    // 参考 MC 1.16.5: rand.setSeed(xpSeed + optionIndex)
    math::Random enchantRandom(m_enchantmentSeed + optionIndex);

    // 构建附魔列表
    auto enchantments = item::enchant::EnchantmentHelper::buildEnchantmentList(
        enchantRandom, item, level, false);

    if (enchantments.empty()) {
        // 如果没有可用附魔，仍然更新种子
        updateEnchantmentSeed(player);
        updateEnchantmentOptions();
        return true;
    }

    // 检查是否是书 -> 附魔书转换
    bool isBook = item.getItem() != nullptr &&
                  item.getItem() == Items::BOOK;

    if (isBook) {
        // 转换为附魔书
        // TODO: 创建附魔书物品
    }

    // 应用所有附魔
    for (const auto& data : enchantments) {
        if (data.enchantment != nullptr && data.level > 0) {
            item.addEnchantment(data.enchantment->id(), data.level);
        }
    }

    m_enchantmentInventory->setItem(SLOT_ITEM, item);

    // 更新种子
    updateEnchantmentSeed(player);
    updateEnchantmentOptions();

    return true;
}

bool EnchantmentContainer::stillValid(const Player& player) const {
    // MC 1.16.5: 检查玩家是否在附魔台附近（64格范围内）
    // 参考: net.minecraft.inventory.container.EnchantmentContainer.canInteractWith
    return isWithinDistance(player, m_position);
}

void EnchantmentContainer::slotsChanged(IInventory* inventory) {
    if (inventory == m_enchantmentInventory.get()) {
        // 物品变化时更新附魔选项
        updateEnchantmentOptions();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack EnchantmentContainer::quickMoveStack(i32 slotIndex, Player& player) {
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

void EnchantmentContainer::updateEnchantmentOptions() {
    ItemStack item = getItemSlot();

    if (item.isEmpty()) {
        m_enchantmentLevels = {0, 0, 0};
        m_enchantmentClues = {"", "", ""};
        m_enchantmentWorldClues = {0, 0, 0};
        return;
    }

    // 计算书架力量
    m_enchantPower = calculateEnchantPower();

    // 使用种子生成附魔选项
    // 参考 MC 1.16.5: rand.setSeed(xpSeed)
    m_random.setSeed(m_enchantmentSeed);

    for (i32 i = 0; i < ENCHANTMENT_OPTIONS; ++i) {
        // 计算附魔等级
        m_enchantmentLevels[i] = item::enchant::EnchantmentHelper::calcItemStackEnchantability(
            m_random, i, m_enchantPower, item);

        // 生成附魔预览
        if (m_enchantmentLevels[i] > 0) {
            // 参考 MC 1.16.5: getEnchantmentList 使用 xpSeed + slot 作为种子
            // 这样每个槽位的预览是确定性的
            m_random.setSeed(m_enchantmentSeed + i);
            auto enchantments = item::enchant::EnchantmentHelper::buildEnchantmentList(
                m_random, item, m_enchantmentLevels[i], false);

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

void EnchantmentContainer::updateEnchantmentSeed(Player& player) {
    // 使用玩家的 XP 种子更新附魔种子
    // 参考 MC 1.16.5: container.xpSeed = player.getXPSeed()
    m_enchantmentSeed = static_cast<i64>(player.xpSeed());
}

i32 EnchantmentContainer::calculateEnchantPower() const {
    if (!m_world) {
        return 0;
    }

    i32 power = 0;
    BlockPos tablePos = m_position;

    // MC 1.16.5 附魔台书架检测逻辑
    // 检查附魔台周围2格范围内（5x5区域）的书架
    // 书架必须满足：与附魔台距离为2，中间有空隙

    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) continue;

            // 检查两层高度（0和1层）
            // 中间必须有空气
            bool hasAirGap = true;
            for (i32 dy = 0; dy <= 1; ++dy) {
                BlockPos airPos(tablePos.x + dx, tablePos.y + dy, tablePos.z + dz);
                if (!isAirBlock(airPos)) {
                    hasAirGap = false;
                    break;
                }
            }

            if (!hasAirGap) continue;

            // 检查书架（距离为2）
            // 角落位置有额外的书架检测
            i32 dx2 = dx * 2;
            i32 dz2 = dz * 2;

            // 主要书架位置
            for (i32 dy = 0; dy <= 1; ++dy) {
                BlockPos shelfPos(tablePos.x + dx2, tablePos.y + dy, tablePos.z + dz2);
                if (isValidBookshelf(shelfPos)) {
                    power++;
                }
            }

            // 角落位置额外书架
            if (dx != 0 && dz != 0) {
                // 角落有三个额外的书架位置
                for (i32 dy = 0; dy <= 1; ++dy) {
                    BlockPos shelfPos1(tablePos.x + dx2, tablePos.y + dy, tablePos.z + dz);
                    BlockPos shelfPos2(tablePos.x + dx, tablePos.y + dy, tablePos.z + dz2);

                    if (isValidBookshelf(shelfPos1)) {
                        power++;
                    }
                    if (isValidBookshelf(shelfPos2)) {
                        power++;
                    }
                }
            }
        }
    }

    return std::min(power, 15);  // 最大15
}

bool EnchantmentContainer::isValidBookshelf(const BlockPos& pos) const {
    if (!m_world) {
        return false;
    }

    const BlockState* blockState = m_world->getBlockState(pos);
    if (!blockState) {
        return false;
    }

    // 检查是否为书架
    const Block& block = blockState->getBlock();
    return &block == VanillaBlocks::BOOKSHELF;
}

bool EnchantmentContainer::isAirBlock(const BlockPos& pos) const {
    if (!m_world) {
        return false;
    }
    const BlockState* blockState = m_world->getBlockState(pos);
    if (!blockState) {
        return true;  // 未加载的区块视为空气
    }
    return blockState->getBlock().isAir(*blockState);
}

} // namespace mc
