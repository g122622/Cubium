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
#include <string>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 锻造升级配方（MC 1.21+）
 *
 * 对应 MC 1.21.11 的 net.minecraft.world.item.crafting.SmithingTransformRecipe。
 *
 * 1.21+ 用 smithing_transform 取代旧的 smithing 配方，用于下界合金升级等
 * "保留基础物品属性、只更换物品类型"的锻造。三个输入槽：template（锻造模板）、
 * base（基础装备）、addition（添加物）。assemble 时基于 base 物品做 transmuteCopy，
 * 保留 base 的附魔/自定义名/修复成本等数据组件，仅替换物品类型。
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:smithing_transform",
 *   "template": "minecraft:netherite_upgrade_smithing_template",
 *   "base": "minecraft:diamond_axe",
 *   "addition": "#minecraft:netherite_tool_materials",
 *   "result": { "id": "minecraft:netherite_axe" }
 * }
 * @endcode
 *
 * 注意：项目当前尚无 SmithingMenu/SmithingContainer 实现（锻造台交互未完整），
 * 本类仅完成数据结构层面的解析与注册，matches 的槽位语义按原版 3 槽实现并留 TODO，
 * 待锻造台容器接入后对齐。
 *
 * 参考: net.minecraft.world.item.crafting.SmithingTransformRecipe
 */
class SmithingTransformRecipe : public IRecipe<IInventory> {
public:
    /// 模板槽位索引（原版 1.21+ 锻造台 3 槽：template=0/base=1/addition=2）
    static constexpr i32 SLOT_TEMPLATE = 0;
    /// 基础物品槽位索引
    static constexpr i32 SLOT_BASE = 1;
    /// 添加物槽位索引
    static constexpr i32 SLOT_ADDITION = 2;

    /**
     * @brief 构造锻造升级配方
     * @param id 配方ID
     * @param templateIngredient 模板原料（可选，缺失时为空 Ingredient）
     * @param base 基础物品原料（必选）
     * @param addition 添加物原料（可选，缺失时为空 Ingredient）
     * @param result 结果物品堆（物品类型与数量用于 assemble）
     */
    SmithingTransformRecipe(const ResourceLocation& id,
        Ingredient templateIngredient,
        Ingredient base,
        Ingredient addition,
        ItemStack result);

    ~SmithingTransformRecipe() override = default;

    // ========== IRecipe 接口实现 ==========

    [[nodiscard]] bool matches(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override { return m_ingredients; }
    [[nodiscard]] const std::string& getGroup() const override { return EMPTY_GROUP; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] RecipeType getType() const override { return RecipeType::SmithingTransform; }
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override { return width * height >= 3; }
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const IInventory& inventory) const override;

    /// assemble 依赖 base 物品的 NBT，结果每次可能不同
    [[nodiscard]] bool isDynamic() const override { return true; }

    // ========== 锻造升级配方特有方法 ==========

    [[nodiscard]] const Ingredient& getTemplate() const { return m_template; }
    [[nodiscard]] const Ingredient& getBase() const { return m_base; }
    [[nodiscard]] const Ingredient& getAddition() const { return m_addition; }

private:
    static const std::string EMPTY_GROUP;

    ResourceLocation m_id;
    Ingredient m_template;                 ///< 模板原料（可选）
    Ingredient m_base;                     ///< 基础物品原料（必选）
    Ingredient m_addition;                 ///< 添加物原料（可选）
    ItemStack m_result;                    ///< 结果物品堆
    std::vector<Ingredient> m_ingredients; ///< 缓存的原料列表（template + base + addition）
};

} // namespace crafting
} // namespace mc
