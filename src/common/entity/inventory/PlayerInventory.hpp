#pragma once

#include "IInventory.hpp"
#include "Slot.hpp"
#include "../../item/ItemStack.hpp"
#include <array>
#include <functional>

namespace mc {

// Forward declarations
class Player;

/**
 * @brief 玩家背包
 *
 * 实现玩家的完整背包系统，包含：
 * - 快捷栏 (9个槽位)
 * - 主背包 (27个槽位)
 * - 护甲槽 (4个槽位)
 * - 副手槽 (1个槽位)
 *
 * 槽位索引：
 * - 0-8: 快捷栏
 * - 9-35: 主背包
 * - 36-39: 护甲 (头盔、胸甲、护腿、靴子)
 * - 40: 副手
 *
 * 参考: net.minecraft.entity.player.PlayerInventory
 */
class PlayerInventory : public IInventory {
public:
    // ========== 常量 ==========

    /// 快捷栏大小
    static constexpr i32 HOTBAR_SIZE = 9;

    /// 主背包大小
    static constexpr i32 MAIN_SIZE = 27;

    /// 护甲槽大小
    static constexpr i32 ARMOR_SIZE = 4;

    /// 副手槽大小
    static constexpr i32 OFFHAND_SIZE = 1;

    /// 总槽位数
    static constexpr i32 TOTAL_SIZE = HOTBAR_SIZE + MAIN_SIZE + ARMOR_SIZE + OFFHAND_SIZE;  // 41

    // ========== 构造函数 ==========

    /**
     * @brief 构造玩家背包
     * @param player 拥有此背包的玩家（可为nullptr用于离线背包）
     */
    explicit PlayerInventory(Player* player = nullptr);

    ~PlayerInventory() override = default;

    // 禁止拷贝
    PlayerInventory(const PlayerInventory&) = delete;
    PlayerInventory& operator=(const PlayerInventory&) = delete;

    // 允许移动
    PlayerInventory(PlayerInventory&&) = default;
    PlayerInventory& operator=(PlayerInventory&&) = default;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] i32 getContainerSize() const override { return TOTAL_SIZE; }

    [[nodiscard]] bool isEmpty() const override;

    [[nodiscard]] ItemStack getItem(i32 slot) const override;

    void setItem(i32 slot, const ItemStack& stack) override;

    ItemStack removeItem(i32 slot, i32 count) override;

    ItemStack removeItemNoUpdate(i32 slot) override;

    void clear() override;

    void setChanged() override;

    void serialize(network::PacketSerializer& ser) const override;

    // ========== 快捷栏操作 ==========

    /**
     * @brief 获取当前选中的快捷栏槽位
     * @return 槽位索引 (0-8)
     */
    [[nodiscard]] i32 getSelectedSlot() const { return m_selectedSlot; }

    /**
     * @brief 设置选中的快捷栏槽位
     * @param slot 槽位索引 (0-8)
     */
    void setSelectedSlot(i32 slot);

    /**
     * @brief 获取当前选中的物品
     */
    [[nodiscard]] ItemStack getSelectedStack() const;

    /**
     * @brief 获取当前选中物品的引用（可修改）
     * @return 物品堆引用
     */
    [[nodiscard]] ItemStack& getSelectedStackRef() {
        return m_items[m_selectedSlot];
    }

    /**
     * @brief 获取当前选中物品的const引用
     * @return 物品堆引用
     */
    [[nodiscard]] const ItemStack& getSelectedStackRef() const {
        return m_items[m_selectedSlot];
    }

    /**
     * @brief 检查索引是否在快捷栏范围内
     */
    [[nodiscard]] static bool isHotbar(i32 slot) {
        return slot >= InventorySlots::HOTBAR_START && slot <= InventorySlots::HOTBAR_END;
    }

    /**
     * @brief 获取最佳快捷栏槽位
     *
     * 优先选择空槽位，其次选择非附魔物品槽位。
     *
     * @return 槽位索引
     */
    [[nodiscard]] i32 getBestHotbarSlot() const;

    // ========== 物品添加 ==========

    /**
     * @brief 尝试添加物品到背包
     *
     * 优先合并到现有堆叠，然后放入空槽位。
     *
     * @param stack 要添加的物品堆（会被修改）
     * @return 剩余未添加的数量
     */
    i32 add(ItemStack& stack);

    /**
     * @brief 尝试添加物品到指定槽位范围
     * @param stack 要添加的物品堆
     * @param start 起始槽位
     * @param end 结束槽位
     * @return 剩余未添加的数量
     */
    i32 addInRange(ItemStack& stack, i32 start, i32 end);

    /**
     * @brief 添加物品到背包，返回新堆
     *
     * 不会修改原始堆。
     *
     * @param stack 要添加的物品堆
     * @return 添加后的剩余物品，如果全部添加成功返回空堆
     */
    [[nodiscard]] ItemStack addItemCopy(const ItemStack& stack) const;

    // ========== 物品查找 ==========

    /**
     * @brief 查找第一个空槽位
     * @return 槽位索引，没有返回-1
     */
    [[nodiscard]] i32 getFirstEmptySlot() const override;

    /**
     * @brief 查找指定物品的槽位
     * @param item 要查找的物品
     * @return 槽位索引，未找到返回-1
     */
    [[nodiscard]] i32 findSlot(const Item& item) const override;

    /**
     * @brief 查找可以与指定物品合并的槽位
     * @param stack 要合并的物品
     * @return 槽位索引，未找到返回-1
     */
    [[nodiscard]] i32 findSlotMatching(const ItemStack& stack) const;

    /**
     * @brief 查找可以与指定物品合并的槽位（在指定范围内）
     * @param stack 要合并的物品
     * @param start 起始槽位
     * @param end 结束槽位
     * @return 槽位索引，未找到返回-1
     */
    [[nodiscard]] i32 findSlotMatchingInRange(const ItemStack& stack, i32 start, i32 end) const;

    // ========== 护甲操作 ==========

    /**
     * @brief 获取头盔槽物品
     */
    [[nodiscard]] ItemStack getHelmet() const { return m_items[InventorySlots::ARMOR_HEAD]; }

    /**
     * @brief 获取胸甲槽物品
     */
    [[nodiscard]] ItemStack getChestplate() const { return m_items[InventorySlots::ARMOR_CHEST]; }

    /**
     * @brief 获取护腿槽物品
     */
    [[nodiscard]] ItemStack getLeggings() const { return m_items[InventorySlots::ARMOR_LEGS]; }

    /**
     * @brief 获取靴子槽物品
     */
    [[nodiscard]] ItemStack getBoots() const { return m_items[InventorySlots::ARMOR_FEET]; }

    /**
     * @brief 获取副手物品
     */
    [[nodiscard]] ItemStack getOffhandItem() const { return m_items[InventorySlots::OFFHAND]; }

    /**
     * @brief 获取副手物品的引用（可修改）
     * @return 物品堆引用
     */
    [[nodiscard]] ItemStack& getOffhandItemRef() { return m_items[InventorySlots::OFFHAND]; }

    /**
     * @brief 获取副手物品的const引用
     * @return 物品堆引用
     */
    [[nodiscard]] const ItemStack& getOffhandItemRef() const { return m_items[InventorySlots::OFFHAND]; }

    /**
     * @brief 设置头盔
     */
    void setHelmet(const ItemStack& stack) { m_items[InventorySlots::ARMOR_HEAD] = stack; }

    /**
     * @brief 设置胸甲
     */
    void setChestplate(const ItemStack& stack) { m_items[InventorySlots::ARMOR_CHEST] = stack; }

    /**
     * @brief 设置护腿
     */
    void setLeggings(const ItemStack& stack) { m_items[InventorySlots::ARMOR_LEGS] = stack; }

    /**
     * @brief 设置靴子
     */
    void setBoots(const ItemStack& stack) { m_items[InventorySlots::ARMOR_FEET] = stack; }

    /**
     * @brief 设置副手物品
     */
    void setOffhandItem(const ItemStack& stack) { m_items[InventorySlots::OFFHAND] = stack; }

    // ========== 槽位操作 ==========

    /**
     * @brief 交换两个槽位的物品
     * @param slot1 第一个槽位
     * @param slot2 第二个槽位
     */
    void swapSlots(i32 slot1, i32 slot2);

    /**
     * @brief 将物品放置到指定槽位
     *
     * 如果槽位为空，直接放入。
     * 如果槽位有相同物品，尝试合并。
     * 如果槽位有不同物品，交换。
     *
     * @param slot 目标槽位
     * @param stack 要放置的物品（鼠标上的物品）
     * @return 操作后的鼠标物品
     */
    ItemStack placeItem(i32 slot, ItemStack stack);

    // ========== 统计 ==========

    /**
     * @brief 统计指定物品的总数量
     */
    [[nodiscard]] i32 countItem(const Item& item) const override;

    /**
     * @brief 检查是否包含指定物品
     */
    [[nodiscard]] bool hasItem(const Item& item) const override;

    // ========== 玩家引用 ==========

    /**
     * @brief 获取拥有此背包的玩家
     */
    [[nodiscard]] Player* getPlayer() const { return m_player; }

    // ========== 序列化 ==========

    /**
     * @brief 从反序列化器创建背包
     */
    [[nodiscard]] static Result<PlayerInventory> deserialize(network::PacketDeserializer& deser);

private:
    /**
     * @brief 检查两个物品堆是否可以合并
     */
    [[nodiscard]] bool canMergeStacks(const ItemStack& stack1, const ItemStack& stack2) const;

    /**
     * @brief 检查物品是否完全相同（包括NBT）
     */
    [[nodiscard]] bool stacksEqualExact(const ItemStack& stack1, const ItemStack& stack2) const;

    std::array<ItemStack, TOTAL_SIZE> m_items;
    Player* m_player;
    i32 m_selectedSlot = 0;  // 当前选中的快捷栏槽位 (0-8)
    i32 m_timesChanged = 0;  // 变更计数器（用于同步）
};

} // namespace mc
