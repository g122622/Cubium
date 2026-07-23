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

#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "screen/ScreenType.hpp"
#include <memory>

namespace mc {

class CraftingTableEntity;
class World;

/**
 * @brief 工作台容器菜单
 *
 * 管理3x3合成网格和结果槽位。
 * 槽位布局：
 * - 槽位 0-8: 合成网格 (3x3)
 * - 槽位 9: 结果槽位
 * - 槽位 10-36: 玩家主背包 (3x9)
 * - 槽位 37-45: 玩家快捷栏 (1x9)
 */
class CraftingMenu : public AbstractContainerMenu {
public:
    /**
     * @brief 合成网格宽度
     */
    static constexpr i32 GRID_WIDTH = 3;

    /**
     * @brief 合成网格高度
     */
    static constexpr i32 GRID_HEIGHT = 3;

    /**
     * @brief 构造函数
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param blockEntity 工作台方块实体（可为nullptr表示玩家背包合成）
     */
    CraftingMenu(ContainerId id, PlayerInventory* playerInventory, CraftingTableEntity* blockEntity);

    /**
     * @brief 构造函数（无方块实体，用于玩家背包2x2合成）
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param width 网格宽度
     * @param height 网格高度
     */
    CraftingMenu(ContainerId id, PlayerInventory* playerInventory, i32 width, i32 height);

    /**
     * @brief 获取合成网格
     * @return 合成网格引用
     */
    [[nodiscard]] CraftingInventory& getCraftingGrid() noexcept { return m_craftingGrid; }
    [[nodiscard]] const CraftingInventory& getCraftingGrid() const noexcept { return m_craftingGrid; }

    /**
     * @brief 获取结果槽位
     * @return 结果背包引用
     */
    [[nodiscard]] CraftResultInventory& getResultInventory() noexcept { return m_result; }
    [[nodiscard]] const CraftResultInventory& getResultInventory() const noexcept { return m_result; }

    /**
     * @brief 容器内容变化时调用
     * @param inventory 变化的背包
     */
    void slotsChanged(IInventory* inventory) override;

    ItemStack clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player) override;

    /**
     * @brief 检查玩家是否可以访问容器
     * @param player 玩家
     * @return 如果可以访问返回true
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 快速移动物品
     * @param slotIndex 槽位索引
     * @param player 玩家
     * @return 移动后的物品堆
     */
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

    /**
     * @brief 关闭容器
     * @param player 玩家
     */
    void removed(Player& player) override;

    /**
     * @brief 更新合成结果
     *
     * 查找匹配的配方并更新结果槽位。
     */
    void updateResult();

    /**
     * @brief 获取屏幕类型
     * @return 屏幕类型
     */
    [[nodiscard]] ScreenType getScreenType() const noexcept { return m_screenType; }

    [[nodiscard]] i32 getResultSlotIndex() const noexcept override { return RESULT_SLOT; }

    /**
     * @brief 获取当前匹配的配方ID
     * @return 当前配方的资源位置ID，如果没有匹配配方则返回空
     */
    [[nodiscard]] ResourceLocation getCurrentRecipeId() const noexcept override
    {
        return m_currentRecipe != nullptr ? m_currentRecipe->getId() : ResourceLocation();
    }

    /**
     * @brief 槽位索引常量
     */
    static constexpr i32 GRID_SLOT_START = 0;
    static constexpr i32 GRID_SLOT_COUNT = 9;
    static constexpr i32 RESULT_SLOT = 9;
    static constexpr i32 PLAYER_INV_START = 10;
    static constexpr i32 PLAYER_INV_COUNT = 27;
    static constexpr i32 HOTBAR_START = 37;
    static constexpr i32 HOTBAR_COUNT = 9;
    static constexpr i32 TOTAL_SLOT_COUNT = 46; ///< 9 网格 + 1 结果 + 27 主背包 + 9 快捷栏

    /**
     * @brief 检查是否是合成网格槽位
     */
    [[nodiscard]] bool isGridSlot(i32 slotIndex) const noexcept
    {
        return slotIndex >= GRID_SLOT_START && slotIndex < GRID_SLOT_START + GRID_SLOT_COUNT;
    }

protected:
    /**
     * @brief 添加合成网格槽位
     * @param startX 起始X坐标
     * @param startY 起始Y坐标
     */
    void addCraftingGridSlots(i32 startX, i32 startY);

    /**
     * @brief 添加结果槽位
     * @param x X坐标
     * @param y Y坐标
     */
    void addResultSlot(i32 x, i32 y);

private:
    /**
     * @brief 处理结果槽位点击
     * @return 如果成功处理返回配方指针，否则返回nullptr
     */
    const crafting::CraftingRecipe* _handleResultSlotClick();

    /**
     * @brief 消耗合成原料
     * @param recipe 已查找的配方指针（避免重复查找）
     */
    void _consumeIngredients(const crafting::CraftingRecipe* recipe);

    CraftingInventory m_craftingGrid;
    CraftResultInventory m_result;
    CraftingTableEntity* m_blockEntity;
    ScreenType m_screenType;
    const crafting::CraftingRecipe* m_currentRecipe = nullptr; ///< 当前匹配的配方
};

/**
 * @brief 玩家背包合成菜单 (2x2)
 *
 * 管理玩家背包中的2x2合成网格、护甲槽、副手槽。
 *
 * 槽位布局：
 * - 槽位 0: 合成结果 (154, 28)
 * - 槽位 1-4: 合成网格 (2x2) (98, 18) 到 (116, 36)
 * - 槽位 5: 头盔 (8, 8)
 * - 槽位 6: 胸甲 (8, 26)
 * - 槽位 7: 护腿 (8, 44)
 * - 槽位 8: 靴子 (8, 62)
 * - 槽位 9-35: 玩家主背包 (3x9) (8, 84) 到 (152, 120)
 * - 槽位 36-44: 玩家快捷栏 (1x9) (8, 142) 到 (152, 142)
 * - 槽位 45: 副手 (77, 62)
 *
 * GUI尺寸: 176 x 166 像素
 */
class InventoryCraftingMenu : public AbstractContainerMenu {
public:
    /**
     * @brief 构造函数
     * @param id 容器ID
     * @param playerInventory 玩家背包
     */
    InventoryCraftingMenu(ContainerId id, PlayerInventory* playerInventory);

    /**
     * @brief 获取合成网格
     * @return 合成网格引用
     */
    [[nodiscard]] CraftingInventory& getCraftingGrid() noexcept { return m_craftingGrid; }
    [[nodiscard]] const CraftingInventory& getCraftingGrid() const noexcept { return m_craftingGrid; }

    /**
     * @brief 获取结果槽位
     * @return 结果背包引用
     */
    [[nodiscard]] CraftResultInventory& getResultInventory() noexcept { return m_result; }
    [[nodiscard]] const CraftResultInventory& getResultInventory() const noexcept { return m_result; }

    /**
     * @brief 容器内容变化时调用
     * @param inventory 变化的背包
     */
    void slotsChanged(IInventory* inventory) override;

    ItemStack clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player) override;

    /**
     * @brief 检查玩家是否可以访问容器
     * @param player 玩家
     * @return 总是返回true（玩家背包）
     */
    [[nodiscard]] bool stillValid(const Player& player) const override { return true; }

    /**
     * @brief 快速移动物品
     * @param slotIndex 槽位索引
     * @param player 玩家
     * @return 移动后的物品堆
     */
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

    /**
     * @brief 更新合成结果
     */
    void updateResult();

    [[nodiscard]] i32 getResultSlotIndex() const noexcept override { return RESULT_SLOT; }

    /**
     * @brief 获取当前匹配的配方ID
     * @return 当前配方的资源位置ID，如果没有匹配配方则返回空
     */
    [[nodiscard]] ResourceLocation getCurrentRecipeId() const noexcept override
    {
        return m_currentRecipe != nullptr ? m_currentRecipe->getId() : ResourceLocation();
    }

    /**
     * @brief 槽位索引常量
     *
     * 布局顺序：
     * - 0: 合成结果
     * - 1-4: 合成网格 (2x2)
     * - 5-8: 护甲 (头盔、胸甲、护腿、靴子)
     * - 9-35: 主背包 (3x9)
     * - 36-44: 快捷栏 (1x9)
     * - 45: 副手
     */
    static constexpr i32 RESULT_SLOT = 0;       ///< 合成结果槽位
    static constexpr i32 GRID_SLOT_START = 1;   ///< 合成网格起始槽位
    static constexpr i32 GRID_SLOT_COUNT = 4;   ///< 合成网格槽位数量
    static constexpr i32 GRID_SLOT_END = 4;     ///< 合成网格结束槽位
    static constexpr i32 ARMOR_SLOT_START = 5;  ///< 护甲槽起始槽位
    static constexpr i32 ARMOR_SLOT_COUNT = 4;  ///< 护甲槽数量
    static constexpr i32 ARMOR_HEAD = 5;        ///< 头盔槽位
    static constexpr i32 ARMOR_CHEST = 6;       ///< 胸甲槽位
    static constexpr i32 ARMOR_LEGS = 7;        ///< 护腿槽位
    static constexpr i32 ARMOR_FEET = 8;        ///< 靴子槽位
    static constexpr i32 PLAYER_INV_START = 9;  ///< 玩家主背包起始槽位
    static constexpr i32 PLAYER_INV_COUNT = 27; ///< 玩家主背包槽位数量
    static constexpr i32 PLAYER_INV_END = 35;   ///< 玩家主背包结束槽位
    static constexpr i32 HOTBAR_START = 36;     ///< 快捷栏起始槽位
    static constexpr i32 HOTBAR_COUNT = 9;      ///< 快捷栏槽位数量
    static constexpr i32 HOTBAR_END = 44;       ///< 快捷栏结束槽位
    static constexpr i32 OFFHAND_SLOT = 45;     ///< 副手槽位
    static constexpr i32 TOTAL_SLOT_COUNT = 46; ///< 总槽位数量

private:
    /**
     * @brief 处理结果槽位点击
     * @return 如果成功处理返回配方指针，否则返回nullptr
     */
    const crafting::CraftingRecipe* _handleResultSlotClick();

    /**
     * @brief 消耗合成原料
     * @param recipe 已查找的配方指针（避免重复查找）
     */
    void _consumeIngredients(const crafting::CraftingRecipe* recipe);

    CraftingInventory m_craftingGrid;
    CraftResultInventory m_result;
    const crafting::CraftingRecipe* m_currentRecipe = nullptr; ///< 当前匹配的配方
};

} // namespace mc
