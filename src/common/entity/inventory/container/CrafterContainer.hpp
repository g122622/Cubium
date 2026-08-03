/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "entity/inventory/IInventory.hpp"
#include "item/crafting/RecipeManager.hpp"
#include <memory>

namespace mc {

class PlayerInventory;
class CrafterBlockEntity;

/**
 * @brief 自动合成器容器
 *
 * 管理自动合成器GUI的槽位布局。
 *
 * 槽位布局：
 * - 合成网格：9格（3x3），对应CrafterBlockEntity的9个槽位
 * - 预览结果：1格（不可交互，仅展示合成结果）
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 参考: net.minecraft.world.inventory.CrafterMenu
 */
class CrafterContainer : public AbstractContainerMenu {
public:
    /// 合成网格槽位数
    static constexpr i32 CRAFT_SLOTS = 9;
    /// 预览结果槽位索引
    static constexpr i32 RESULT_SLOT = CRAFT_SLOTS;
    /// 合成器总槽位数（9合成 + 1预览结果）
    static constexpr i32 CRAFTER_SLOTS = CRAFT_SLOTS + 1;

    /// 合成网格起始X位置（MC原版 CrafterScreen: 26 + j * 18）
    static constexpr i32 CRAFT_SLOT_START_X = 26;
    /// 合成网格起始Y位置（MC原版 CrafterScreen: 17 + i * 18）
    static constexpr i32 CRAFT_SLOT_START_Y = 17;
    /// 槽位宽度
    static constexpr i32 SLOT_SIZE = 18;

    /// 预览结果槽位X位置（MC原版: 134）
    static constexpr i32 RESULT_SLOT_X = 134;
    /// 预览结果槽位Y位置（MC原版: 35）
    static constexpr i32 RESULT_SLOT_Y = 35;

    /// 玩家背包起始Y位置
    static constexpr i32 PLAYER_INV_Y = 84;
    /// 快捷栏Y位置
    static constexpr i32 HOTBAR_Y = 142;

    // ========== 构造函数 ==========

    /**
     * @brief 构造自动合成器容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param crafterInventory 合成器背包（来自CrafterBlockEntity）
     * @param crafterEntity 合成器方块实体（可选，用于距离检查）
     */
    CrafterContainer(ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* crafterInventory,
        CrafterBlockEntity* crafterEntity = nullptr);

    /**
     * @brief 析构函数
     */
    ~CrafterContainer() override = default;

    // ========== 容器接口 ==========

    /**
     * @brief 检查玩家是否仍可访问合成器
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 容器内容变化时调用
     *
     * 当合成器背包内容或禁用槽位状态发生变化时，重新查找匹配配方
     * 并更新预览结果槽位。
     */
    void slotsChanged(IInventory* inventory) override;

    /**
     * @brief 关闭容器时调用，配对构造函数中的openInventory
     */
    void removed(Player& player) override;

    // ========== 合成器特有方法 ==========

    /**
     * @brief 检查指定合成槽位是否被禁用
     * @param slot 合成网格槽位索引 (0-8)
     * @return 如果槽位被禁用返回true
     */
    [[nodiscard]] bool isSlotDisabled(i32 slot) const;

    /**
     * @brief 设置指定合成槽位的启用/禁用状态
     * @param slot 合成网格槽位索引 (0-8)
     * @param enabled true为启用，false为禁用
     */
    void setSlotState(i32 slot, bool enabled);

    /**
     * @brief 获取合成器方块实体
     */
    [[nodiscard]] CrafterBlockEntity* getCrafterEntity() const { return m_crafterEntity; }

    /**
     * @brief 获取合成器背包
     */
    [[nodiscard]] IInventory* getCrafterInventory() const { return m_crafterInventory; }

    /**
     * @brief 获取预览结果背包
     */
    [[nodiscard]] CraftResultInventory& getResultInventory() noexcept { return *m_resultInventory; }
    [[nodiscard]] const CraftResultInventory& getResultInventory() const noexcept { return *m_resultInventory; }

    /**
     * @brief 更新合成预览结果
     *
     * 通过 RecipeManager 查找匹配配方，使用 CrafterBlockEntity::asCraftInput()
     * 构建合成输入（禁用槽位视为空），并将组装结果写入预览槽位。
     */
    void updateResult();

    /**
     * @brief 获取当前匹配的配方ID
     * @return 当前配方的资源位置ID，如果没有匹配配方则返回空
     */
    [[nodiscard]] ResourceLocation getCurrentRecipeId() const noexcept override
    {
        return m_currentRecipe != nullptr ? m_currentRecipe->getId() : ResourceLocation();
    }

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     */
    void _initSlots(PlayerInventory* playerInventory);

    IInventory* m_crafterInventory;                            ///< 合成器背包
    CrafterBlockEntity* m_crafterEntity;                       ///< 合成器方块实体
    std::unique_ptr<CraftResultInventory> m_resultInventory;   ///< 预览结果背包（仅用于显示）
    const crafting::CraftingRecipe* m_currentRecipe = nullptr; ///< 当前匹配的配方
};

} // namespace mc
