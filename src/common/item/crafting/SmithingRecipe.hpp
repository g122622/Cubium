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
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 锻造台配方
 *
 * 锻造台配方将基础装备和添加物组合成升级装备。
 * 主要用于下界合金升级。
 * 参考: net.minecraft.item.crafting.SmithingRecipe
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:smithing",
 *   "base": { "item": "minecraft:diamond_sword" },
 *   "addition": { "item": "minecraft:netherite_ingot" },
 *   "result": { "item": "minecraft:netherite_sword" }
 * }
 * @endcode
 */
class SmithingRecipe : public IRecipe<IInventory> {
public:
    /// 基础物品槽位索引
    static constexpr i32 SLOT_BASE = 0;
    /// 添加物槽位索引
    static constexpr i32 SLOT_ADDITION = 1;
    /// 结果槽位索引
    static constexpr i32 SLOT_RESULT = 2;

    /**
     * @brief 构造函数
     * @param id 配方ID
     * @param base 基础物品原料
     * @param addition 添加物原料
     * @param result 结果物品
     */
    SmithingRecipe(
        const ResourceLocation& id, const Ingredient& base, const Ingredient& addition, const ItemStack& result);

    ~SmithingRecipe() override = default;

    // ========== IRecipe 接口实现 ==========

    [[nodiscard]] bool matches(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override;
    [[nodiscard]] const std::string& getGroup() const override { return EMPTY_GROUP; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] RecipeType getType() const override { return RecipeType::Smithing; }

    /**
     * @brief 锻造台配方需要2个输入槽位
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override { return width * height >= 2; }

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 容器
     * @return 每个槽位的剩余物品堆列表
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const IInventory& inventory) const override;

    // ========== 锻造台特有方法 ==========

    /**
     * @brief 获取基础物品原料
     * @return 基础物品原料
     */
    [[nodiscard]] const Ingredient& getBase() const { return m_base; }

    /**
     * @brief 获取添加物原料
     * @return 添加物原料
     */
    [[nodiscard]] const Ingredient& getAddition() const { return m_addition; }

private:
    static const std::string EMPTY_GROUP;

    ResourceLocation m_id;
    Ingredient m_base;
    Ingredient m_addition;
    ItemStack m_result;
    mutable std::vector<Ingredient> m_ingredients; ///< 缓存的原料列表
};

} // namespace crafting
} // namespace mc
