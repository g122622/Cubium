#include "EnchantmentContainer.hpp"
#include "entity/inventory/Slot.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/player/Player.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "network/packet/PacketSerializer.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"

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
        // 只接受可附魔的物品
        return !stack.isEmpty() && stack.getItem() != nullptr;
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
               stack.getItem()->getId() == "minecraft:lapis_lazuli";
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
    , m_world(world) {
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

String EnchantmentContainer::getEnchantmentClue(i32 index) const {
    if (index >= 0 && index < ENCHANTMENT_OPTIONS) {
        return m_enchantmentClues[index];
    }
    return "";
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

    // 消耗青金石
    i32 lapisNeeded = optionIndex + 1;
    ItemStack& lapis = m_enchantmentInventory->getItem(SLOT_LAPIS);
    lapis.shrink(lapisNeeded);

    // 消耗玩家经验
    // TODO: 实现经验消耗 player.addExperienceLevels(-level);

    // 应用附魔（此处简化，实际需要生成随机附魔）
    // TODO: 实现附魔生成逻辑

    // 重置附魔选项
    updateEnchantmentOptions();

    return true;
}

bool EnchantmentContainer::stillValid(const Player& player) const {
    // 检查玩家是否在附魔台附近
    // TODO: 实现距离检查
    (void)player;
    return true;
}

void EnchantmentContainer::slotsChanged(IInventory* inventory) {
    if (inventory == m_enchantmentInventory.get()) {
        updateEnchantmentOptions();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack EnchantmentContainer::quickMoveStack(i32 slotIndex, Player& player) {
    Slot* slot = getSlot(slotIndex);
    if (!slot || !slot->hasItem()) {
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
        if (slotStack.getItem() && slotStack.getItem()->getId() == "minecraft:lapis_lazuli") {
            if (!moveItemToRange(slotStack, SLOT_LAPIS, SLOT_LAPIS, false)) {
                // 尝试放入物品槽
                if (!moveItemToRange(slotStack, SLOT_ITEM, SLOT_ITEM, false)) {
                    return ItemStack();
                }
            }
        } else {
            // 放入物品槽
            if (!moveItemToRange(slotStack, SLOT_ITEM, SLOT_ITEM, false)) {
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

    // 使用玩家XP种子生成附魔选项
    // TODO: 使用实际玩家XP种子
    i64 seed = 12345;
    math::Random random(seed);

    for (i32 i = 0; i < ENCHANTMENT_OPTIONS; ++i) {
        m_enchantmentLevels[i] = generateEnchantmentOption(random, item, i, m_enchantPower);
    }
}

i32 EnchantmentContainer::calculateEnchantPower() const {
    if (!m_world) {
        return 0;
    }

    i32 power = 0;
    BlockPos tablePos = m_position;

    // 检查附魔台周围2格范围内的书架
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dz = -2; dz <= 2; ++dz) {
            if (dx == 0 && dz == 0) continue;

            // 检查两层高度
            for (i32 dy = 0; dy <= 1; ++dy) {
                BlockPos checkPos = tablePos.offset(dx, dy, dz);

                // 检查书架和附魔台之间是否有空气
                BlockPos airPos1 = tablePos.offset(dx > 0 ? 1 : (dx < 0 ? -1 : 0), 0, dz > 0 ? 1 : (dz < 0 ? -1 : 0));
                BlockPos airPos2 = tablePos.offset(dx > 0 ? 1 : (dx < 0 ? -1 : 0), 1, dz > 0 ? 1 : (dz < 0 ? -1 : 0));

                // 检查是否为有效书架
                if (isValidBookshelf(checkPos) &&
                    m_world->isAirBlock(airPos1) &&
                    m_world->isAirBlock(airPos2)) {
                    power++;
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

    // TODO: 检查是否为书架方块
    auto* blockState = m_world->getBlockState(pos);
    if (!blockState) {
        return false;
    }

    // 检查是否为书架
    return blockState->getBlock()->getId() == "minecraft:bookshelf";
}

i32 EnchantmentContainer::generateEnchantmentOption(math::Random& random,
                                                   const ItemStack& item,
                                                   i32 optionIndex,
                                                   i32 power) {
    if (item.isEmpty()) {
        return 0;
    }

    // MC 1.16.5 附魔等级计算公式
    i32 enchantability = 10;  // 默认物品可附魔度
    if (item.getItem()) {
        // TODO: 获取物品的可附魔度
        enchantability = 10;
    }

    if (enchantability <= 0) {
        return 0;
    }

    // 基础附魔等级
    i32 base = random.nextInt(8) + 1 + (power >> 1) + random.nextInt(power + 1);

    // 根据槽位调整
    switch (optionIndex) {
        case 0:
            return std::max(base / 3, 1);
        case 1:
            return base * 2 / 3 + 1;
        case 2:
            return std::max(base, power * 2);
        default:
            return 0;
    }
}

} // namespace mc
