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
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "core/Result.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "item/crafting/IRecipe.hpp"
#include "item/crafting/SmeltingRecipe.hpp"
#include "item/crafting/SmithingRecipe.hpp"
#include "item/crafting/SmithingTransformRecipe.hpp"
#include "item/crafting/SmithingTrimRecipe.hpp"
#include "item/crafting/StonecuttingRecipe.hpp"
#include "resource/ResourceLocation.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 合成配方类型别名
 */
using CraftingRecipe = IRecipe<CraftingInventory>;

/**
 * @brief 配方管理器，存储并管理已注册配方
 */
class RecipeManager {
public:
    /**
     * @brief 获取单例实例
     * @return RecipeManager实例引用
     */
    static RecipeManager& instance();

    /**
     * @brief 注册合成配方
     * @param recipe 配方实例（移动语义）
     * @return 注册成功返回true，ID冲突返回false
     */
    bool registerRecipe(std::unique_ptr<CraftingRecipe> recipe);

    /**
     * @brief 注册熔炼类配方（熔炉/高炉/烟熏炉/营火）
     * @param recipe 配方实例（移动语义）
     * @return 注册成功返回true，ID冲突返回false
     */
    bool registerSmeltingRecipe(std::unique_ptr<SmeltingRecipe> recipe);

    /**
     * @brief 注册切石机配方
     * @param recipe 配方实例（移动语义）
     * @return 注册成功返回true，ID冲突返回false
     */
    bool registerStonecuttingRecipe(std::unique_ptr<StonecuttingRecipe> recipe);

    /**
     * @brief 注册锻造台配方
     * @param recipe 配方实例（移动语义）
     * @return 注册成功返回true，ID冲突返回false
     */
    bool registerSmithingRecipe(std::unique_ptr<SmithingRecipe> recipe);

    /**
     * @brief 注册锻造升级配方（MC 1.21+ smithing_transform）
     * @param recipe 配方实例（移动语义）
     * @return 注册成功返回true，ID冲突返回false
     */
    bool registerSmithingTransformRecipe(std::unique_ptr<SmithingTransformRecipe> recipe);

    /**
     * @brief 注册盔甲纹饰配方（MC 1.21+ smithing_trim）
     * @param recipe 配方实例（移动语义）
     * @return 注册成功返回true，ID冲突返回false
     */
    bool registerSmithingTrimRecipe(std::unique_ptr<SmithingTrimRecipe> recipe);

    /**
     * @brief 按ID获取合成配方
     * @param id 配方ID
     * @return 配方指针，不存在返回nullptr
     */
    [[nodiscard]] const CraftingRecipe* getRecipe(const ResourceLocation& id) const;

    /**
     * @brief 检查配方是否存在（包含合成与熔炼类）
     * @param id 配方ID
     * @return 存在返回true
     */
    [[nodiscard]] bool hasRecipe(const ResourceLocation& id) const;

    /**
     * @brief 获取全部合成配方
     * @return 合成配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getAllRecipes() const;

    /**
     * @brief 按类型获取合成配方
     * @param type 配方类型
     * @return 对应类型的合成配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesByType(RecipeType type) const;

    /**
     * @brief 按类型获取熔炼类配方
     * @param type 配方类型（Smelting/Blasting/Smoking/CampfireCooking）
     * @return 对应类型的熔炼类配方列表
     */
    [[nodiscard]] std::vector<const SmeltingRecipe*> getSmeltingRecipesByType(RecipeType type) const;

    /**
     * @brief 根据输入物品与配方类型查找熔炼配方
     * @param input 输入物品
     * @param type 目标配方类型
     * @return 匹配配方指针，无匹配返回nullptr
     */
    [[nodiscard]] const SmeltingRecipe* getSmeltingRecipe(const ItemStack& input, RecipeType type) const;

    /**
     * @brief 按ID获取切石机配方
     * @param id 配方ID
     * @return 配方指针，不存在返回nullptr
     */
    [[nodiscard]] const StonecuttingRecipe* getStonecuttingRecipe(const ResourceLocation& id) const;

    /**
     * @brief 按ID获取锻造台配方
     * @param id 配方ID
     * @return 配方指针，不存在返回nullptr
     */
    [[nodiscard]] const SmithingRecipe* getSmithingRecipe(const ResourceLocation& id) const;

    /**
     * @brief 获取所有切石机配方
     * @return 切石机配方列表
     */
    [[nodiscard]] std::vector<const StonecuttingRecipe*> getAllStonecuttingRecipes() const;

    /**
     * @brief 获取所有锻造台配方
     * @return 锻造台配方列表
     */
    [[nodiscard]] std::vector<const SmithingRecipe*> getAllSmithingRecipes() const;

    /**
     * @brief 根据输入物品查找切石机配方
     * @param input 输入物品
     * @return 匹配的切石机配方列表
     */
    [[nodiscard]] std::vector<const StonecuttingRecipe*> findStonecuttingRecipes(const ItemStack& input) const;

    /**
     * @brief 根据输入物品查找锻造台配方
     * @param input 输入物品
     * @return 匹配的锻造台配方列表
     */
    [[nodiscard]] std::vector<const SmithingRecipe*> findSmithingRecipes(const ItemStack& input) const;

    /**
     * @brief 查找匹配给定合成容器的配方
     * @param inventory 合成容器
     * @return 匹配配方，无匹配返回nullptr
     */
    [[nodiscard]] const CraftingRecipe* findMatchingRecipe(const CraftingInventory& inventory) const;

    /**
     * @brief 查找所有匹配给定合成容器的配方
     * @param inventory 合成容器
     * @return 匹配配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> findAllMatchingRecipes(const CraftingInventory& inventory) const;

    /**
     * @brief 查找可产出指定物品的合成配方
     * @param result 结果物品
     * @return 配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesForResult(const Item& result) const;

    /**
     * @brief 查找可产出指定物品堆的合成配方
     * @param result 结果物品堆
     * @return 配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesForResult(const ItemStack& result) const;

    /**
     * @brief 获取配方总数（合成+熔炼）
     * @return 配方总数
     */
    [[nodiscard]] size_t getRecipeCount() const;

    /**
     * @brief 清空所有配方
     */
    void clear();

    /**
     * @brief 遍历所有合成配方
     * @param callback 回调函数
     */
    void forEachRecipe(std::function<void(const CraftingRecipe&)> callback) const;

private:
    RecipeManager() = default;
    ~RecipeManager() = default;
    RecipeManager(const RecipeManager&) = delete;
    RecipeManager& operator=(const RecipeManager&) = delete;
    RecipeManager(RecipeManager&&) = delete;
    RecipeManager& operator=(RecipeManager&&) = delete;

    mutable std::mutex m_mutex;

    std::unordered_map<ResourceLocation, std::unique_ptr<CraftingRecipe>, std::hash<ResourceLocation>> m_recipesById;
    std::unordered_map<ResourceLocation, std::unique_ptr<SmeltingRecipe>, std::hash<ResourceLocation>>
        m_smeltingRecipesById;
    std::unordered_map<ResourceLocation, std::unique_ptr<StonecuttingRecipe>, std::hash<ResourceLocation>>
        m_stonecuttingRecipesById;
    std::unordered_map<ResourceLocation, std::unique_ptr<SmithingRecipe>, std::hash<ResourceLocation>>
        m_smithingRecipesById;
    std::unordered_map<ResourceLocation, std::unique_ptr<SmithingTransformRecipe>, std::hash<ResourceLocation>>
        m_smithingTransformRecipesById;
    std::unordered_map<ResourceLocation, std::unique_ptr<SmithingTrimRecipe>, std::hash<ResourceLocation>>
        m_smithingTrimRecipesById;

    std::unordered_map<RecipeType, std::vector<const CraftingRecipe*>> m_recipesByType;
    std::unordered_map<RecipeType, std::vector<const SmeltingRecipe*>> m_smeltingRecipesByType;
    std::unordered_map<RecipeType, std::vector<const StonecuttingRecipe*>> m_stonecuttingRecipesByType;
    std::unordered_map<RecipeType, std::vector<const SmithingRecipe*>> m_smithingRecipesByType;
    std::unordered_map<RecipeType, std::vector<const SmithingTransformRecipe*>> m_smithingTransformRecipesByType;
    std::unordered_map<RecipeType, std::vector<const SmithingTrimRecipe*>> m_smithingTrimRecipesByType;

    std::unordered_map<ItemId, std::vector<const CraftingRecipe*>> m_recipesByResult;
};

} // namespace crafting
} // namespace mc
