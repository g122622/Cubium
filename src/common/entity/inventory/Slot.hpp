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

#pragma once

#include "../../core/Types.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace mc {

// Forward declarations
class IInventory;
class Player;

namespace blockentity {
class AbstractFurnaceEntity;
} // namespace blockentity

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
constexpr i32 ARMOR_HEAD = 36;  // 头盔
constexpr i32 ARMOR_CHEST = 37; // 胸甲
constexpr i32 ARMOR_LEGS = 38;  // 护腿
constexpr i32 ARMOR_FEET = 39;  // 靴子

// 副手
constexpr i32 OFFHAND = 40;

// 总大小
constexpr i32 TOTAL_SIZE = 41;

// ========== MC Java NBT 槽位索引映射 ==========
//
// MC Java 存档格式中，Inventory 列表的 Slot 字段使用不同于内部索引的编号：
// - 快捷栏和主背包 (0-35): NBT Slot 值与内部索引相同
// - 护甲和副手: NBT Slot 值使用特殊编码
//
// 旧版格式 (1.21.11 之前):
//   内部 36 (HEAD)   → NBT 103 (armor.head)
//   内部 37 (CHEST)  → NBT 102 (armor.chest)
//   内部 38 (LEGS)   → NBT 101 (armor.legs)
//   内部 39 (FEET)   → NBT 100 (armor.feet)
//   内部 40 (OFFHAND)→ NBT -106 (weapon.offhand)
//
// 1.21.11 新格式:
//   装备通过 "equipment" 字段以枚举名保存，不再出现在 Inventory 列表中
//   Inventory 列表仅包含快捷栏和主背包 (Slot 0-35)
//   旧版存档由 PlayerEquipmentFix 数据迁移器自动转换

/// NBT 护甲槽位起始编号 (armor.feet)
static constexpr i32 NBT_ARMOR_SLOT_START = 100;

/// NBT 副手槽位编号
static constexpr i32 NBT_OFFHAND_SLOT = -106;

/**
 * @brief 将内部背包索引转换为 MC Java NBT Slot 值
 *
 * 快捷栏和主背包 (0-35) 直接使用内部索引。
 * 护甲 (36-39) 映射为 100-103，副手 (40) 映射为 -106。
 *
 * @param internalSlot 内部背包索引 (0-40)
 * @return NBT Slot 值
 */
[[nodiscard]] constexpr i32 toNbtSlot(i32 internalSlot) noexcept
{
    switch (internalSlot) {
        case ARMOR_HEAD:
            return 103; // armor.head
        case ARMOR_CHEST:
            return 102; // armor.chest
        case ARMOR_LEGS:
            return 101; // armor.legs
        case ARMOR_FEET:
            return 100; // armor.feet
        case OFFHAND:
            return NBT_OFFHAND_SLOT; // weapon.offhand
        default:
            return internalSlot; // 0-35 快捷栏和主背包
    }
}

/**
 * @brief 将 MC Java NBT Slot 值转换为内部背包索引
 *
 * 护甲 NBT 100-103 映射回 36-39，副手 -106 映射回 40。
 * 快捷栏和主背包 (0-35) 直接使用 NBT Slot 值。
 * 无法识别的值返回 -1。
 *
 * @param nbtSlot NBT Slot 值
 * @return 内部背包索引 (0-40)，无效返回 -1
 */
[[nodiscard]] constexpr i32 fromNbtSlot(i32 nbtSlot) noexcept
{
    switch (nbtSlot) {
        case 100:
            return ARMOR_FEET; // armor.feet → 39
        case 101:
            return ARMOR_LEGS; // armor.legs → 38
        case 102:
            return ARMOR_CHEST; // armor.chest → 37
        case 103:
            return ARMOR_HEAD; // armor.head → 36
        case -106:
            return OFFHAND; // weapon.offhand → 40
        default:
            if (nbtSlot >= 0 && nbtSlot <= MAIN_END) {
                return nbtSlot; // 0-35 快捷栏和主背包
            }
            return -1; // 无效槽位
    }
}
} // namespace InventorySlots

/**
 * @brief 槽位背景图标
 *
 * 用于在空槽位显示轮廓图标（如护甲槽的护甲轮廓）。
 */
struct SlotBackground {
    ResourceLocation atlas;  ///< 图集位置
    ResourceLocation sprite; ///< 精灵位置

    [[nodiscard]] bool isValid() const noexcept { return !atlas.path().empty() && !sprite.path().empty(); }
};

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
     * @param slotIndex 槽位索引（在背包中的索引）
     * @param x 显示位置X
     * @param y 显示位置Y
     */
    Slot(IInventory* inventory, i32 slotIndex, i32 x, i32 y);

    virtual ~Slot() = default;

    // ========== 基本信息 ==========

    /**
     * @brief 获取槽位索引（在背包中的索引）
     */
    [[nodiscard]] i32 getIndex() const noexcept { return m_slotIndex; }

    /**
     * @brief 获取槽位编号（在容器中的索引，由容器设置）
     */
    [[nodiscard]] i32 getSlotNumber() const noexcept { return m_slotNumber; }

    /**
     * @brief 设置槽位编号
     * @param number 槽位编号
     */
    void setSlotNumber(i32 number) noexcept { m_slotNumber = number; }

    /**
     * @brief 获取所属背包
     */
    [[nodiscard]] IInventory* getInventory() const noexcept { return m_inventory; }

    /**
     * @brief 获取显示位置X
     */
    [[nodiscard]] i32 getX() const noexcept { return m_x; }

    /**
     * @brief 获取显示位置Y
     */
    [[nodiscard]] i32 getY() const noexcept { return m_y; }

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
     * @brief 槽位是否有物品
     */
    [[nodiscard]] bool hasItem() const;

    /**
     * @brief 槽位是否为空
     */
    [[nodiscard]] bool isEmpty() const;

    /**
     * @brief 从槽位移除物品
     * @param amount 要移除的数量
     * @return 被移除的物品堆
     */
    virtual ItemStack remove(i32 amount);

    // ========== 可放置性检查 ==========

    /**
     * @brief 检查物品是否可以放入此槽位
     * @param stack 要放入的物品
     */
    [[nodiscard]] virtual bool mayPlace(const ItemStack& stack) const;

    /**
     * @brief 检查玩家是否可以从此槽位拾取物品
     * @param player 玩家
     */
    [[nodiscard]] virtual bool mayPickup(Player& player) const;

    /**
     * @brief 检查槽位是否有效（如护甲槽只接受护甲）
     */
    [[nodiscard]] virtual bool isValid() const noexcept { return true; }

    /**
     * @brief 检查槽位是否启用（客户端渲染用）
     *
     * 某些槽位可能被禁用（如驴的装备槽在未装备鞍时）。
     */
    [[nodiscard]] virtual bool isEnabled() const noexcept { return true; }

    /**
     * @brief 获取最大堆叠数量
     */
    [[nodiscard]] virtual i32 getMaxStackSize() const;

    /**
     * @brief 获取最大堆叠数量（考虑物品本身）
     * @param stack 要放入的物品
     */
    [[nodiscard]] virtual i32 getMaxStackSize(const ItemStack& stack) const;

    // ========== 变化通知 ==========

    /**
     * @brief 通知槽位内容变化
     */
    void setChanged();

    /**
     * @brief 槽位内容变化回调
     * @param oldStack 旧物品
     * @param newStack 新物品
     *
     * 当物品数量增加时，调用 onCrafting(newStack, countIncrease)。
     */
    virtual void onSlotChange(const ItemStack& oldStack, const ItemStack& newStack);

    /**
     * @brief 合成完成回调（带数量）
     * @param stack 合成结果
     * @param amount 增加的数量
     *
     * 当物品数量增加时调用，用于追踪合成数量。
     */
    virtual void onCrafting(const ItemStack& stack, i32 amount);

    /**
     * @brief 数字键交换回调
     * @param numItemsCrafted 交换的物品数量
     */
    virtual void onSwapCraft(i32 numItemsCrafted);

    /**
     * @brief 合成完成回调
     * @param stack 合成结果
     *
     * 当物品从合成结果槽取出时调用。
     */
    virtual void onCrafting(const ItemStack& stack);

    /**
     * @brief 物品取出回调
     * @param player 取出物品的玩家
     * @param stack 取出的物品
     * @return 取出的物品堆
     *
     * 当物品从槽位取出时调用，用于触发成就、经验等。
     */
    virtual ItemStack onTake(Player& player, ItemStack stack);

    // ========== 激活状态 ==========

    /**
     * @brief 检查槽位是否激活（鼠标悬停）
     */
    [[nodiscard]] bool isActive() const noexcept { return m_active; }

    /**
     * @brief 设置激活状态
     */
    void setActive(bool active) noexcept { m_active = active; }

    // ========== 背景图标 ==========

    /**
     * @brief 获取背景图标
     * @return 背景图标，无效返回空
     */
    [[nodiscard]] const SlotBackground& getBackground() const { return m_background; }

    /**
     * @brief 设置背景图标
     * @param atlas 图集位置
     * @param sprite 精灵位置
     * @return this，支持链式调用
     */
    Slot& setBackground(const ResourceLocation& atlas, const ResourceLocation& sprite);

    // ========== 工具方法 ==========

    /**
     * @brief 检查是否与另一个槽位属于同一背包
     * @param other 另一个槽位
     * @return 是否属于同一背包
     */
    [[nodiscard]] bool isSameInventory(const Slot& other) const noexcept;

protected:
    i32 m_slotNumber = -1;       ///< 槽位编号（在容器中的索引）
    bool m_active = false;       ///< 激活状态
    SlotBackground m_background; ///< 背景图标

private:
    IInventory* m_inventory;
    i32 m_slotIndex;
    i32 m_x;
    i32 m_y;
};

/**
 * @brief 护甲槽位
 *
 * 特殊槽位，只接受对应类型的护甲。
 * 有绑定诅咒的护甲无法取下（除非创造模式）。
 */
class ArmorSlot : public Slot {
public:
    /**
     * @brief 护甲类型
     */
    enum class ArmorType : u8 {
        Head = 0,  // 头盔
        Chest = 1, // 胸甲
        Legs = 2,  // 护腿
        Feet = 3   // 靴子
    };

    ArmorSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, ArmorType armorType);

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;

    /**
     * @brief 检查玩家是否可以取下护甲
     *
     * 如果护甲有绑定诅咒且玩家不是创造模式，则无法取下。
     */
    [[nodiscard]] bool mayPickup(Player& player) const override;

    /**
     * @brief 护甲槽最大堆叠数为1
     */
    [[nodiscard]] i32 getMaxStackSize() const noexcept override { return 1; }

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
     * @param player 玩家（用于触发成就）
     */
    ResultSlot(
        IInventory* inventory, i32 slotIndex, i32 x, i32 y, CraftingInventory* craftingGrid, Player* player = nullptr);

    /**
     * @brief 结果槽位不能放置物品
     */
    [[nodiscard]] bool mayPlace(const ItemStack& stack) const noexcept override
    {
        (void)stack;
        return false;
    }

    /**
     * @brief 检查槽位是否有效
     */
    [[nodiscard]] bool isValid() const noexcept override { return true; }

    /**
     * @brief 合成完成回调
     *
     * 触发成就和配方解锁。
     */
    void onCrafting(const ItemStack& stack, i32 amount) override;

    /**
     * @brief 合成完成回调
     */
    void onCrafting(const ItemStack& stack) override;

    /**
     * @brief 数字键交换回调
     */
    void onSwapCraft(i32 numItemsCrafted) override;

    /**
     * @brief 物品取出回调
     *
     * 触发材料消耗、成就等。
     */
    ItemStack onTake(Player& player, ItemStack stack) override;

    /**
     * @brief 获取关联的合成网格
     */
    [[nodiscard]] CraftingInventory* getCraftingGrid() const noexcept { return m_craftingGrid; }

private:
    CraftingInventory* m_craftingGrid;
    Player* m_player;
    i32 m_amountCrafted = 0; ///< 已合成数量（用于成就追踪）
};

/**
 * @brief 熔炉燃料槽位
 *
 * 只接受燃料物品，桶类物品限制堆叠数为1。
 *
 * 参考: net.minecraft.inventory.container.FurnaceFuelSlot
 */
class FurnaceFuelSlot : public Slot {
public:
    /**
     * @brief 构造熔炉燃料槽位
     * @param inventory 所属背包
     * @param slotIndex 槽位索引
     * @param x 显示位置X
     * @param y 显示位置Y
     */
    FurnaceFuelSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y);

    /**
     * @brief 检查物品是否可以作为燃料
     * @param stack 要检查的物品
     * @return 是否可以放入燃料槽
     */
    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;

    /**
     * @brief 获取最大堆叠数量
     *
     * 如果是桶，返回1；否则返回默认值。
     */
    [[nodiscard]] i32 getMaxStackSize(const ItemStack& stack) const override;

    /**
     * @brief 静态方法：检查物品是否可以作为燃料
     * @param stack 要检查的物品
     * @return 是否是燃料
     */
    [[nodiscard]] static bool isFuel(const ItemStack& stack);

    /**
     * @brief 静态方法：检查物品是否是桶
     * @param stack 要检查的物品
     * @return 是否是桶
     */
    [[nodiscard]] static bool isBucket(const ItemStack& stack);
};

/**
 * @brief 熔炉输出槽位
 *
 * 只能取出，不能放入。取出时触发经验发放。
 *
 * 参考: net.minecraft.inventory.container.FurnaceResultSlot
 */
class FurnaceResultSlot : public Slot {
public:
    /**
     * @brief 构造熔炉输出槽位
     * @param player 玩家（用于发放经验）
     * @param inventory 所属背包
     * @param slotIndex 槽位索引
     * @param x 显示位置X
     * @param y 显示位置Y
     * @param furnaceEntity 熔炉实体（用于提取累积经验）
     */
    FurnaceResultSlot(Player* player,
        IInventory* inventory,
        i32 slotIndex,
        i32 x,
        i32 y,
        blockentity::AbstractFurnaceEntity* furnaceEntity = nullptr);

    /**
     * @brief 输出槽不能放入物品
     */
    [[nodiscard]] bool mayPlace(const ItemStack& stack) const noexcept override
    {
        (void)stack;
        return false;
    }

    /**
     * @brief 从输出槽移除物品时追踪数量
     */
    ItemStack remove(i32 amount) override;

    /**
     * @brief 物品取出时触发经验发放
     */
    ItemStack onTake(Player& player, ItemStack stack) override;

    /**
     * @brief 设置熔炉实体
     * @param furnaceEntity 熔炉实体指针
     */
    void setFurnaceEntity(blockentity::AbstractFurnaceEntity* furnaceEntity) noexcept
    {
        m_furnaceEntity = furnaceEntity;
    }

    /**
     * @brief 获取熔炉实体
     */
    [[nodiscard]] blockentity::AbstractFurnaceEntity* getFurnaceEntity() const noexcept { return m_furnaceEntity; }

protected:
    /**
     * @brief 合成完成回调（带数量）
     */
    void onCrafting(const ItemStack& stack, i32 amount) override;

    /**
     * @brief 合成完成回调
     */
    void onCrafting(const ItemStack& stack) override;

private:
    Player* m_player;
    i32 m_removeCount = 0;                                         ///< 已取出数量（用于经验计算）
    blockentity::AbstractFurnaceEntity* m_furnaceEntity = nullptr; ///< 熔炉实体（用于提取经验）
};

} // namespace mc
