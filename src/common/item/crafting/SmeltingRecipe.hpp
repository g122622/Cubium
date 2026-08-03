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

#include "IRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/core/ItemStack.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace blockentity {
class FurnaceInventory;
}

namespace crafting {

/**
 * @brief 熔炼配方基类
 *
 * 用于熔炉、高炉、烟熏炉、营火等熔炼类配方的基类。
 * 参考: net.minecraft.item.cooking.AbstractCookingRecipe
 *
 * 熔炼配方只有一个输入原料，产生一个输出结果。
 * 熔炼过程会产生经验值，并有固定的熔炼时间。
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
    SmeltingRecipe(const ResourceLocation& id,
        const std::string& group,
        const Ingredient& ingredient,
        const ItemStack& result,
        f32 experience,
        i32 cookTime);

    ~SmeltingRecipe() override = default;

    // ========== 拷贝和移动操作 ==========

    SmeltingRecipe(const SmeltingRecipe&) = default;
    SmeltingRecipe& operator=(const SmeltingRecipe&) = default;
    SmeltingRecipe(SmeltingRecipe&&) noexcept = default;
    SmeltingRecipe& operator=(SmeltingRecipe&&) noexcept = default;

    // ========== IRecipe 接口实现 ==========

    [[nodiscard]] bool matches(const blockentity::FurnaceInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const blockentity::FurnaceInventory& inventory) const override;
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override;
    [[nodiscard]] const std::string& getGroup() const override { return m_group; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] RecipeType getType() const override { return RecipeType::Smelting; }

    /**
     * @brief 熔炼类配方可以适应任何尺寸（始终返回 true）
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override
    {
        (void)width;
        (void)height;
        return true;
    }

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 熔炉容器
     * @return 每个槽位的剩余物品堆列表
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(
        const blockentity::FurnaceInventory& inventory) const override;

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

    /**
     * @brief 获取输入原料
     * @return 输入原料
     */
    [[nodiscard]] const Ingredient& getIngredient() const { return m_ingredient; }

protected:
    ResourceLocation m_id;
    std::string m_group;
    Ingredient m_ingredient;
    ItemStack m_result;
    f32 m_experience;
    i32 m_cookTime;
    mutable std::vector<Ingredient> m_ingredients; ///< 缓存的原料列表
};

/**
 * @brief 高炉配方
 *
 * 与普通熔炼配方类似，但熔炼时间减半（100 tick vs 200 tick）
 * 仅适用于矿石和金属物品。
 * 参考: net.minecraft.item.cooking.BlastingRecipe
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
 * 参考: net.minecraft.item.cooking.SmokingRecipe
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
 * 参考: net.minecraft.item.cooking.CampfireCookingRecipe
 */
class CampfireCookingRecipe : public SmeltingRecipe {
public:
    using SmeltingRecipe::SmeltingRecipe;

    [[nodiscard]] RecipeType getType() const override { return RecipeType::CampfireCooking; }
};

} // namespace crafting
} // namespace mc
