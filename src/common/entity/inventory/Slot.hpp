#pragma once

#include "../../core/Types.hpp"
#include "../../item/core/ItemStack.hpp"
#include <functional>
#include <memory>

namespace mc {

// Forward declarations
class IInventory;
class Player;
class Container;

/**
 * @brief 槽位索引常量
 *
 * 玩家背包槽位布局：
 * - 0-8: 快捷栏 (Hotbar)
 * - 9-35: 主背包 (Main Inventory)
 * - 36-39: 护甲 (Armor) - 头盔、胸甲、护腿、靴子
 * - 40: 副手 (Offhand)
 */
namespace InventorySlots {
    // 快捷栏
    constexpr i32 HOTBAR_START = 0;
    constexpr i32 HOTBAR_END = 8;
    constexpr i32 HOTBAR_SIZE = 9;

    // 主背包
    constexpr i32 MAIN_START = 9;
    constexpr i32 MAIN_END = 35;
    constexpr i32 MAIN_SIZE = 27;

    // 护甲
    constexpr i32 ARMOR_START = 36;
    constexpr i32 ARMOR_END = 39;
    constexpr i32 ARMOR_SIZE = 4;
    constexpr i32 ARMOR_HEAD = 36;     // 头盔
    constexpr i32 ARMOR_CHEST = 37;    // 胸甲
    constexpr i32 ARMOR_LEGS = 38;     // 护腿
    constexpr i32 ARMOR_FEET = 39;     // 靴子

    // 副手
    constexpr i32 OFFHAND = 40;

    // 总大小
    constexpr i32 TOTAL_SIZE = 41;
}

/**
 * @brief 槽位类
 *
 * 表示背包中的一个槽位，用于容器UI显示和交互。
 * 包含槽位索引、所属背包引用和可放置性检查。
 *
 * 参考: net.minecraft.inventory.container.Slot
 */
class Slot {
public:
    /**
     * @brief 构造槽位
     * @param inventory 所属背包
     * @param slotIndex 槽位索引
     * @param x 显示位置X
     * @param y 显示位置Y
     */
    Slot(IInventory* inventory, i32 slotIndex, i32 x, i32 y);

    virtual ~Slot() = default;

    // ========== 基本信息 ==========

    /**
     * @brief 获取槽位索引
     */
    [[nodiscard]] i32 getIndex() const { return m_slotIndex; }

    /**
     * @brief 获取所属背包
     */
    [[nodiscard]] IInventory* getInventory() const { return m_inventory; }

    /**
     * @brief 获取显示位置X
     */
    [[nodiscard]] i32 getX() const { return m_x; }

    /**
     * @brief 获取显示位置Y
     */
    [[nodiscard]] i32 getY() const { return m_y; }

    // ========== 物品操作 ==========

    /**
     * @brief 获取槽位中的物品
     */
    [[nodiscard]] ItemStack getItem() const;

    /**
     * @brief 设置槽位中的物品
     */
    void set(const ItemStack& stack);

    /**
     * @brief 槽位是否为空
     */
    [[nodiscard]] bool isEmpty() const;

    /**
     * @brief 从槽位移除物品
     * @param amount 要移除的数量
     * @return 被移除的物品堆
     */
    ItemStack remove(i32 amount);

    // ========== 可放置性检查 ==========

    /**
     * @brief 检查物品是否可以放入此槽位
     * @param stack 要放入的物品
     */
    [[nodiscard]] virtual bool mayPlace(const ItemStack& stack) const;

    /**
     * @brief 检查槽位是否有效（如护甲槽只接受护甲）
     */
    [[nodiscard]] virtual bool isValid() const { return true; }

    /**
     * @brief 获取最大堆叠数量
     */
    [[nodiscard]] virtual i32 getMaxStackSize() const;

    /**
     * @brief 获取最大堆叠数量（考虑物品本身）
     * @param stack 要放入的物品
     */
    [[nodiscard]] virtual i32 getMaxStackSize(const ItemStack& stack) const;

    // ========== 激活状态 ==========

    /**
     * @brief 检查槽位是否激活（鼠标悬停）
     */
    [[nodiscard]] bool isActive() const { return m_active; }

    /**
     * @brief 设置激活状态
     */
    void setActive(bool active) { m_active = active; }

private:
    IInventory* m_inventory;
    i32 m_slotIndex;
    i32 m_x;
    i32 m_y;
    bool m_active = false;
};

/**
 * @brief 护甲槽位
 *
 * 特殊槽位，只接受对应类型的护甲。
 */
class ArmorSlot : public Slot {
public:
    /**
     * @brief 护甲类型
     */
    enum class ArmorType : u8 {
        Head = 0,   // 头盔
        Chest = 1,  // 胸甲
        Legs = 2,   // 护腿
        Feet = 3    // 靴子
    };

    ArmorSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, ArmorType armorType);

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;

private:
    ArmorType m_armorType;
};

// Forward declaration
class CraftingInventory;

/**
 * @brief 合成结果槽位
 *
 * 特殊槽位，用于显示合成结果。
 * 不能直接放入物品，只能取出。
 *
 * 参考: net.minecraft.inventory.container.CraftingResultSlot
 */
class ResultSlot : public Slot {
public:
    /**
     * @brief 构造结果槽位
     * @param inventory 所属背包（通常是 CraftResultInventory）
     * @param slotIndex 槽位索引（通常为0）
     * @param x 显示位置X
     * @param y 显示位置Y
     * @param craftingGrid 关联的合成网格
     */
    ResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y,
               CraftingInventory* craftingGrid);

    /**
     * @brief 结果槽位不能放置物品
     */
    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override {
        (void)stack;
        return false;
    }

    /**
     * @brief 检查槽位是否有效
     */
    [[nodiscard]] bool isValid() const override { return true; }

    /**
     * @brief 获取关联的合成网格
     */
    [[nodiscard]] CraftingInventory* getCraftingGrid() const { return m_craftingGrid; }

private:
    CraftingInventory* m_craftingGrid;
};

} // namespace mc
