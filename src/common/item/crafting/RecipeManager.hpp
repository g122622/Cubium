#pragma once

#include "item/crafting/IRecipe.hpp"
#include "item/crafting/SmeltingRecipe.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "resource/ResourceLocation.hpp"
#include "core/Result.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

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
     * @param recipe ���方实例（移动语义）
     * @return 注册成功返回true，ID冲突返回false
     */
    bool registerSmeltingRecipe(std::unique_ptr<SmeltingRecipe> recipe);

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
    [[nodiscard]] const SmeltingRecipe* getSmeltingRecipe(
        const ItemStack& input,
        RecipeType type) const;

    /**
     * @brief 查找匹配给定合成容器的配方
     * @param inventory 合成容器
     * @return 匹配配方，无匹配返回nullptr
     */
    [[nodiscard]] const CraftingRecipe* findMatchingRecipe(
        const CraftingInventory& inventory) const;

    /**
     * @brief 查找所有匹配给定合成容器的配方
     * @param inventory 合成容器
     * @return 匹配配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> findAllMatchingRecipes(
        const CraftingInventory& inventory) const;

    /**
     * @brief 查找可产出指定物品的合成配方
     * @param result 结果物品
     * @return 配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesForResult(
        const Item& result) const;

    /**
     * @brief 查找可产出指定物品堆的合成配方
     * @param result 结果物品堆
     * @return 配方列表
     */
    [[nodiscard]] std::vector<const CraftingRecipe*> getRecipesForResult(
        const ItemStack& result) const;

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

    mutable std::mutex m_mutex;

    std::unordered_map<ResourceLocation, std::unique_ptr<CraftingRecipe>, std::hash<ResourceLocation>> m_recipesById;
    std::unordered_map<ResourceLocation, std::unique_ptr<SmeltingRecipe>, std::hash<ResourceLocation>> m_smeltingRecipesById;

    std::unordered_map<RecipeType, std::vector<const CraftingRecipe*>> m_recipesByType;
    std::unordered_map<RecipeType, std::vector<const SmeltingRecipe*>> m_smeltingRecipesByType;

    std::unordered_map<ItemId, std::vector<const CraftingRecipe*>> m_recipesByResult;
};

} // namespace crafting
} // namespace mc