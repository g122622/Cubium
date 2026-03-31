#pragma once

#include "IRecipe.hpp"
#include "../core/ItemStack.hpp"
#include "../../world/blockentity/processing/FurnaceInventory.hpp"
#include <memory>

namespace mc {
namespace crafting {

// Forward declaration
class FurnaceInventory;

/**
 * @brief 熔炼配方
 *
 * 用于熔炉、高炉、烟熏炉等熔炼类配方的基类。
 * 参考: net.minecraft.item.cooking.AbstractCookingRecipe
 */
class SmeltingRecipe : public IRecipe<class mc::blockentity::FurnaceInventory> {
public:
    /**
     * @brief 构造函数
     * @param id 配方ID
     * @param group 分组名
     * @param ingredient 输入原料
     * @param result 结果物品
     * @param experience 经验值
     * @param cookTime 熔炼时间（tick）
     */
    SmeltingRecipe(
        const ResourceLocation& id,
        const String& group,
        const Ingredient& ingredient,
        const ItemStack& result,
        f32 experience,
        i32 cookTime
    );

    ~SmeltingRecipe() override = default;

    // ========== IRecipe 接口实现 ==========

    [[nodiscard]] bool matches(const blockentity::FurnaceInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const blockentity::FurnaceInventory& inventory) const override;
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override;
    [[nodiscard]] const String& getGroup() const override { return m_group; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] RecipeType getType() const override { return RecipeType::Smelting; }

    // ========== 熔炼特有方法 ==========

    /**
     * @brief 获取经验值
     * @return 熔炼获得的经验值
     */
    [[nodiscard]] f32 getExperience() const { return m_experience; }

    /**
     * @brief 获取熔炼时间
     * @return 熔炼时间（tick）
     */
    [[nodiscard]] i32 getCookTime() const { return m_cookTime; }

protected:
    ResourceLocation m_id;
    String m_group;
    Ingredient m_ingredient;
    ItemStack m_result;
    f32 m_experience;
    i32 m_cookTime;
    mutable std::vector<Ingredient> m_ingredients;  ///< 缓存的原料列表
};

/**
 * @brief 高炉配方
 *
 * 与普通熔炼配方类似，但熔炼时间减半（100 tick vs 200 tick）
 * 仅适用于矿石和金属物品。
 */
class BlastingRecipe : public SmeltingRecipe {
public:
    using SmeltingRecipe::SmeltingRecipe;

    [[nodiscard]] RecipeType getType() const override { return RecipeType::Blasting; }
};

/**
 * @brief 烟熏炉配方
 *
 * 与普通熔炼配方类似，但熔炼时间减半（100 tick vs 200 tick）
 * 仅适用于食物。
 */
class SmokingRecipe : public SmeltingRecipe {
public:
    using SmeltingRecipe::SmeltingRecipe;

    [[nodiscard]] RecipeType getType() const override { return RecipeType::Smoking; }
};

/**
 * @brief 营火烹饪配方
 *
 * 在营火上烹饪食物的配方。
 */
class CampfireCookingRecipe : public SmeltingRecipe {
public:
    using SmeltingRecipe::SmeltingRecipe;

    [[nodiscard]] RecipeType getType() const override { return RecipeType::CampfireCooking; }
};

} // namespace crafting
} // namespace mc
