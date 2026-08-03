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

#include "common/core/Types.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "entity/inventory/CraftingInventory.hpp" // 包含 CraftingInventory 和 CraftResultInventory
#include "item/crafting/RecipeManager.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

/**
 * @brief 工作台方块实体
 *
 * 存储玩家在工作台中放置的物品和当前匹配的配方。
 *
 * 注意：与MC原版不同，这里工作台不持久化存储物品。
 * 原版MC中工作台是"虚拟"容器，关闭后物品会弹出。
 * 此实现保留物品是为了支持未来的连续合成功能。
 */
class CraftingTableEntity : public ContainerBlockEntity {
public:
    /**
     * @brief 工作台合成网格尺寸
     */
    static constexpr i32 GRID_WIDTH = 3;
    static constexpr i32 GRID_HEIGHT = 3;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit CraftingTableEntity(const BlockPos& pos)
        : ContainerBlockEntity(BlockEntityType::CraftingTable, pos)
        , m_craftingGrid(GRID_WIDTH, GRID_HEIGHT)
        , m_result()
        , m_currentRecipe(nullptr)
    {}

    /**
     * @brief 获取合成网格
     * @return 合成网格引用
     */
    [[nodiscard]] CraftingInventory& getCraftingGrid() { return m_craftingGrid; }
    [[nodiscard]] const CraftingInventory& getCraftingGrid() const { return m_craftingGrid; }

    /**
     * @brief 获取结果槽位
     * @return 结果背包引用
     */
    [[nodiscard]] CraftResultInventory& getResultInventory() { return m_result; }
    [[nodiscard]] const CraftResultInventory& getResultInventory() const { return m_result; }

    /**
     * @brief 获取容器（合成网格）
     * @return 容器指针
     */
    [[nodiscard]] IInventory* getInventory() override { return &m_craftingGrid; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_craftingGrid; }

    /**
     * @brief 获取容器大小
     * @return 9 (3x3网格)
     */
    [[nodiscard]] i32 getContainerSize() const override { return GRID_WIDTH * GRID_HEIGHT; }

    /**
     * @brief 更新合成结果
     *
     * 根据当前网格内容查找匹配的配方，
     * 并更新结果槽位。
     *
     * @return 如果找到匹配配方返回true
     */
    bool updateCraftingResult()
    {
        const crafting::CraftingRecipe* recipe = crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);

        if (recipe != m_currentRecipe) {
            m_currentRecipe = recipe;
            if (recipe != nullptr) {
                m_result.setResultItem(recipe->assemble(m_craftingGrid));
            } else {
                m_result.clear();
            }
            setChanged();
            return true;
        }
        return false;
    }

    /**
     * @brief 获取当前匹配的配方
     * @return 配方指针，如果没有匹配返回nullptr
     */
    [[nodiscard]] const crafting::CraftingRecipe* getCurrentRecipe() const { return m_currentRecipe; }

    /**
     * @brief 执行合成
     *
     * 从网格消耗原料并返回结果。
     * 调用后需要重新调用updateCraftingResult()。
     *
     * @return 合成结果物品堆
     */
    ItemStack craft()
    {
        if (m_currentRecipe == nullptr) {
            return ItemStack();
        }

        // 获取结果
        ItemStack result = m_currentRecipe->assemble(m_craftingGrid);

        // 消耗原料
        for (i32 slot = 0; slot < m_craftingGrid.getContainerSize(); ++slot) {
            ItemStack stack = m_craftingGrid.getItem(slot);
            if (!stack.isEmpty()) {
                stack.shrink(1);
                m_craftingGrid.setItem(slot, stack.isEmpty() ? ItemStack() : stack);
            }
        }

        // 清除结果
        m_result.clear();
        m_currentRecipe = nullptr;

        // 标记已修改
        setChanged();

        return result;
    }

    /**
     * @brief 清空工作台
     *
     * 清除网格和结果槽位的所有物品。
     */
    void clear()
    {
        m_craftingGrid.clear();
        m_result.clear();
        m_currentRecipe = nullptr;
        setChanged();
    }

    /**
     * @brief 从JSON加载数据
     * @param data JSON数据
     * @return 是否成功
     */
    bool load(const nlohmann::json& data) override
    {
        if (!ContainerBlockEntity::load(data)) {
            return false;
        }

        // 从父类读取后直接根据网格状态重建结果槽
        m_result.clear();
        m_currentRecipe = nullptr;

        updateCraftingResult();

        return true;
    }

    /**
     * @brief 保存数据到JSON
     * @param data 输出JSON数据
     */
    void save(nlohmann::json& data) const override
    {
        ContainerBlockEntity::save(data);

        // 保存网格内容（可选）
        // 注：原版MC不保存工作台内容
    }

    /**
     * @brief 克隆方块实体
     * @return 副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override
    {
        auto copy = std::make_unique<CraftingTableEntity>(m_pos);
        copy->m_customName = m_customName;
        // 注：不复制物品内容
        return copy;
    }

    /**
     * @brief 设置自定义名称
     * @param name 名称
     */
    void setCustomName(const std::string& name) override { m_customName = name; }

    /**
     * @brief 获取自定义名称
     * @return 自定义名称
     */
    [[nodiscard]] std::string getCustomName() const override { return m_customName; }

private:
    CraftingInventory m_craftingGrid;
    CraftResultInventory m_result;
    const crafting::CraftingRecipe* m_currentRecipe;
    std::string m_customName;
};

} // namespace mc
