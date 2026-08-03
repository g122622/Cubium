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
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/crafting/RecipeManager.hpp"
#include <string>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 物品转化配方（MC 1.21+）
 *
 * 对应 MC 1.21.11 的 net.minecraft.world.item.crafting.TransmuteRecipe。
 *
 * 转化配方接受两个输入：一个被转化的物品（input）和一个材料（material），
 * 输出指定物品，并保留被转化物品的 NBT 数据（如收纳袋内容物）。
 *
 * 典型用例：
 * - 收纳袋染色：bundle + dye → colored_bundle（保留 BundleContents）
 *
 * 匹配规则：
 * - 网格中恰好 2 个非空物品
 * - 其中一个匹配 input，另一个匹配 material
 * - 转化结果不能与原物品相同（避免无意义转化，如红色收纳袋 + 红色染料）
 *
 * assemble 行为：
 * - 找到匹配 input 的物品堆
 * - 使用 transmuteCopy 将其物品类型替换为 result 物品，保留 NBT
 *
 * JSON 格式示例（MC 1.21+）：
 * @code
 * {
 *   "type": "minecraft:crafting_transmute",
 *   "category": "equipment",
 *   "group": "bundle_dye",
 *   "input": "#minecraft:bundles",
 *   "material": "minecraft:white_dye",
 *   "result": {
 *     "id": "minecraft:white_bundle"
 *   }
 * }
 * @endcode
 *
 * 参考: net.minecraft.world.item.crafting.TransmuteRecipe
 */
class TransmuteRecipe : public CraftingRecipe {
public:
    /**
     * @brief 构造转化配方
     * @param id 配方ID
     * @param input 被转化的物品原料
     * @param material 材料原料
     * @param result 结果物品堆（数量用于 assemble，NBT 不被使用）
     * @param group 配方分组（可选）
     */
    TransmuteRecipe(const ResourceLocation& id,
        Ingredient input,
        Ingredient material,
        ItemStack result,
        const std::string& group = "");

    /**
     * @brief 检查配方是否匹配给定容器
     * @param inventory 合成网格
     * @return 如果匹配返回true
     *
     * 匹配条件：
     * - 恰好 2 个非空物品
     * - 一个匹配 input，一个匹配 material
     * - 转化结果不能与原物品相同（避免无意义转化）
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成结果物品堆
     * @param inventory 合成网格（用于获取 input 物品的 NBT）
     * @return 转化后的物品堆（保留 input 的 NBT）
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取结果物品
     * @return 结果物品堆
     */
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }

    /**
     * @brief 获取原料列表（input + material）
     * @return 原料列表
     */
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override { return m_ingredients; }

    /**
     * @brief 获取 input 原料
     */
    [[nodiscard]] const Ingredient& getInput() const { return m_input; }

    /**
     * @brief 获取 material 原料
     */
    [[nodiscard]] const Ingredient& getMaterial() const { return m_material; }

    /**
     * @brief 获取配方分组
     */
    [[nodiscard]] const std::string& getGroup() const override { return m_group; }

    /**
     * @brief 获取配方ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }

    /**
     * @brief 获取配方类型
     * @return RecipeType::Transmute
     */
    [[nodiscard]] RecipeType getType() const override { return RecipeType::Transmute; }

    /**
     * @brief 转化配方为特殊配方（不出现在配方书中）
     */
    [[nodiscard]] bool isSpecial() const override { return true; }

    /**
     * @brief 转化配方为动态配方
     *
     * 因为 assemble() 的结果依赖于 input 物品的 NBT，结果可能每次不同。
     */
    [[nodiscard]] bool isDynamic() const override { return true; }

    /**
     * @brief 检查配方是否适合给定尺寸的网格
     * @param width 网格宽度
     * @param height 网格高度
     * @return 如果原料数量不超过网格大小返回true
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override { return 2 <= width * height; }

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 合成网格
     * @return 每个槽位的剩余物品堆列表
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    ResourceLocation m_id;
    Ingredient m_input;
    Ingredient m_material;
    ItemStack m_result;
    std::string m_group;
    std::vector<Ingredient> m_ingredients; ///< getIngredients() 返回的列表（input + material）

    /**
     * @brief 检查转化结果是否与原物品相同（避免无意义转化）
     * @param stack input 物品堆
     * @return 如果转化后物品类型相同且 NBT 相同返回true
     *
     * 对应 MC 1.21.11 TransmuteResult#isResultUnchanged。
     * 简化判断：物品类型相同则视为无意义转化（NBT 会被保留，所以相同物品
     * 类型意味着结果与输入完全相同）。
     */
    [[nodiscard]] bool _isResultUnchanged(const ItemStack& stack) const;
};

} // namespace crafting
} // namespace mc
