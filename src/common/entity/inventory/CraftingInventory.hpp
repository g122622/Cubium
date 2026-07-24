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

#include "entity/inventory/IInventory.hpp"
#include "entity/inventory/IRecipeHelperPopulator.hpp"
#include "entity/inventory/IRecipeHolder.hpp"
#include "entity/inventory/Slot.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

namespace mc {

// Forward declarations
class CraftingInventory;

namespace crafting {
template <typename C>
class IRecipe;
// CraftingRecipe is a type alias defined in RecipeManager.hpp
// using CraftingRecipe = IRecipe<CraftingInventory>;
} // namespace crafting

/**
 * @brief 合成网格背包
 *
 * 用于工作台(3x3)和玩家背包(2x2)的合成网格。
 * 继承自IInventory和IRecipeHelperPopulator，提供网格坐标访问方法。
 *
 * 使用示例：
 * @code
 * // 创建3x3工作台网格
 * CraftingInventory grid(3, 3);
 *
 * // 设置物品
 * grid.setItemAt(0, 0, ItemStack(Items::OAK_PLANKS, 1));
 * grid.setItemAt(1, 0, ItemStack(Items::OAK_PLANKS, 1));
 *
 * // 获取物品
 * ItemStack stack = grid.getItemAt(0, 0);
 *
 * // 使用槽位索引访问
 * grid.setItem(0, ItemStack(Items::OAK_PLANKS, 1));
 * @endcode
 *
 * 注意事项：
 * - 槽位索引从左到右、从上到下排列（行优先）
 * - 坐标(0,0)在左上角
 * - 内容变更时会触发回调，用于更新合成结果
 */
class CraftingInventory : public IInventory, public IRecipeHelperPopulator {
public:
    /**
     * @brief 构造合成网格
     * @param width 网格宽度（1-3）
     * @param height 网格高度（1-3）
     */
    explicit CraftingInventory(i32 width, i32 height);

    ~CraftingInventory() override = default;

    // 禁止拷贝，允许移动
    CraftingInventory(const CraftingInventory&) = delete;
    CraftingInventory& operator=(const CraftingInventory&) = delete;
    CraftingInventory(CraftingInventory&&) noexcept = default;
    CraftingInventory& operator=(CraftingInventory&&) noexcept = default;

    // ========== IInventory接口 ==========

    i32 getContainerSize() const override { return m_width * m_height; }

    bool isEmpty() const override;

    ItemStack getItem(i32 slot) const override;
    void setItem(i32 slot, const ItemStack& stack) override;
    ItemStack removeItem(i32 slot, i32 count) override;
    ItemStack removeItemNoUpdate(i32 slot) override;
    void clear() override;
    void setChanged() override;

    // ========== 网格特定方法 ==========

    /**
     * @brief 获取网格宽度
     */
    i32 getWidth() const { return m_width; }

    /**
     * @brief 获取网格高度
     */
    i32 getHeight() const { return m_height; }

    /**
     * @brief 按网格坐标获取物品
     * @param x X坐标（0到width-1）
     * @param y Y坐标（0到height-1）
     * @return 该位置的物品堆
     *
     * 注意：坐标越界时返回空堆
     */
    ItemStack getItemAt(i32 x, i32 y) const;

    /**
     * @brief 按网格坐标设置物品
     * @param x X坐标（0到width-1）
     * @param y Y坐标（0到height-1）
     * @param stack 要设置的物品堆
     *
     * 注意：坐标越界时忽略操作
     */
    void setItemAt(i32 x, i32 y, const ItemStack& stack);

    /**
     * @brief 按网格坐标移除物品
     * @param x X坐标
     * @param y Y坐标
     * @param count 要移除的数量
     * @return 移除的物品堆
     */
    ItemStack removeItemAt(i32 x, i32 y, i32 count);

    /**
     * @brief 坐标转槽位索引
     * @param x X坐标
     * @param y Y坐标
     * @return 槽位索引，越界返回-1
     */
    i32 posToSlot(i32 x, i32 y) const;

    /**
     * @brief 槽位索引转坐标
     * @param slot 槽位索引
     * @param outX 输出X坐标
     * @param outY 输出Y坐标
     * @return 如果索引有效返回true
     */
    bool slotToPos(i32 slot, i32& outX, i32& outY) const;

    /**
     * @brief 设置内容变更回调
     * @param callback 回调函数
     *
     * 当背包内容改变时调用此回调，用于触发合成结果更新
     */
    void setContentChangedCallback(std::function<void()> callback) { m_onContentChanged = std::move(callback); }

    /**
     * @brief 获取所有物品
     * @return 物品列表的常量引用
     */
    const std::vector<ItemStack>& getItems() const { return m_items; }

    /**
     * @brief 设置所有物品
     * @param items 物品列表（大小必须等于容器大小）
     */
    void setItems(const std::vector<ItemStack>& items);

    /**
     * @brief 检查网格是否全空
     * @return 如果所有槽位都为空返回true
     */
    bool isAllEmpty() const;

    /**
     * @brief 计算网格中物品的边界框
     * @param outMinX 输出最小X坐标
     * @param outMinY 输出最小Y坐标
     * @param outMaxX 输出最大X坐标
     * @param outMaxY 输出最大Y坐标
     * @return 如果有物品返回true
     *
     * 边界框是包含所有非空槽位的最小矩形
     */
    bool getContentBounds(i32& outMinX, i32& outMinY, i32& outMaxX, i32& outMaxY) const;

    // ========== IRecipeHelperPopulator接口 ==========

    /**
     * @brief 将背包内容填充到物品计数器
     * @param itemCounts 物品ID到数量的映射（输出参数）
     *
     * 遍历所有物品，只计数未损坏、未附魔、无自定义名称的物品。
     */
    void fillStackedContents(std::unordered_map<i32, i32>& itemCounts) const override;

private:
    std::vector<ItemStack> m_items;
    i32 m_width;
    i32 m_height;
    std::function<void()> m_onContentChanged;
};

/**
 * @brief 合成结果背包
 *
 * 单槽位背包，用于存储合成结果。
 * 在工作台UI中显示在右侧结果槽位。
 * 实现IRecipeHolder接口以追踪当前使用的配方。
 *
 * 特殊行为：
 * - 只有一个槽位（槽位0）
 * - 存储当前匹配配方的结果
 * - 追踪使用的配方（用于解锁配方成就）
 *
 * 使用示例：
 * @code
 * CraftResultInventory result;
 * result.setResultItem(ItemStack(Items::CRAFTING_TABLE, 1));
 *
 * // 玩家取走结果
 * ItemStack taken = result.removeItem(0, 1);
 * @endcode
 */
class CraftResultInventory : public IInventory, public IRecipeHolder {
public:
    CraftResultInventory() = default;
    ~CraftResultInventory() override = default;

    // ========== IInventory接口 ==========

    i32 getContainerSize() const override { return 1; }

    bool isEmpty() const override { return m_result.isEmpty(); }

    ItemStack getItem(i32 slot) const override;
    void setItem(i32 slot, const ItemStack& stack) override;
    ItemStack removeItem(i32 slot, i32 count) override;
    ItemStack removeItemNoUpdate(i32 slot) override;
    void clear() override;
    void setChanged() override;

    // ========== 结果特定方法 ==========

    /**
     * @brief 设置结果物品
     * @param stack 结果物品堆
     */
    void setResultItem(const ItemStack& stack)
    {
        m_result = stack;
        setChanged();
    }

    /**
     * @brief 获取结果物品
     * @return 结果物品堆
     */
    const ItemStack& getResultItem() const { return m_result; }

    /**
     * @brief 检查是否有结果
     * @return 如果结果槽位有物品返回true
     */
    bool hasResult() const { return !m_result.isEmpty(); }

    // ========== IRecipeHolder接口 ==========

    /**
     * @brief 设置当前使用的配方（IInventory版本，不推荐使用）
     * @param recipe 配方指针，nullptr表示清除
     * @deprecated 使用 setCraftingRecipeUsed 代替
     */
    void setRecipeUsed(const crafting::IRecipe<IInventory>* recipe) override
    {
        // 注意：由于模板类型不兼容，此方法不推荐使用
        // 使用 setCraftingRecipeUsed 代替
        m_craftingRecipeUsed = nullptr;
        (void)recipe;
    }

    /**
     * @brief 获取当前使用的配方（IInventory版本，不推荐使用）
     * @return 配方指针，如果没有返回nullptr
     * @deprecated 使用 getCraftingRecipeUsed 代替
     */
    [[nodiscard]] const crafting::IRecipe<IInventory>* getRecipeUsed() const override
    {
        // 注意：由于模板类型不兼容，此方法不推荐使用
        // 使用 getCraftingRecipeUsed 代替
        return nullptr;
    }

    /**
     * @brief 合成完成时调用（重写IRecipeHolder）
     * @param player 玩家
     *
     * 如果配方不是动态的，解锁配方并清除。
     */
    void onCrafting(Player& player) override;

    /**
     * @brief 设置当前使用的合成配方
     * @param recipe 合成配方指针，nullptr表示清除
     * @note 使用 crafting::CraftingRecipe 类型（IRecipe<CraftingInventory> 的别名）
     */
    void setCraftingRecipeUsed(const crafting::IRecipe<mc::CraftingInventory>* recipe)
    {
        m_craftingRecipeUsed = recipe;
    }

    /**
     * @brief 获取当前使用的合成配方
     * @return 合成配方指针，如果没有返回nullptr
     * @note 返回 crafting::CraftingRecipe 类型（IRecipe<CraftingInventory> 的别名）
     */
    [[nodiscard]] const crafting::IRecipe<mc::CraftingInventory>* getCraftingRecipeUsed() const
    {
        return m_craftingRecipeUsed;
    }

    /**
     * @brief 获取当前使用的配方ID
     * @return 配方ID，如果没有返回空
     */
    [[nodiscard]] ResourceLocation getRecipeUsedId() const;

private:
    ItemStack m_result;
    const crafting::IRecipe<mc::CraftingInventory>* m_craftingRecipeUsed = nullptr; ///< 当前使用的合成配方
};

} // namespace mc
