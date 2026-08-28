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
 * @brief 盔甲纹饰配方（MC 1.21+）
 *
 * 对应 MC 1.21.11 的 net.minecraft.world.item.crafting.SmithingTrimRecipe。
 *
 * smithing_trim 用于给盔甲添加纹饰（ArmorTrim）。三个输入槽：template（纹饰模板）、
 * base（可纹饰盔甲）、addition（纹饰材料）。assemble 时给 base 物品添加 TRIM 数据组件
 * （由 addition 解析出的 TrimMaterial + template 决定的 TrimPattern 组成）。
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:smithing_trim",
 *   "template": "minecraft:bolt_armor_trim_smithing_template",
 *   "base": "#minecraft:trimmable_armor",
 *   "addition": "#minecraft:trim_materials",
 *   "pattern": "minecraft:bolt"
 * }
 * @endcode
 *
 * 注意：项目当前尚无 SmithingMenu 实现，且纹饰系统（TrimMaterial/TrimPattern 注册表）
 * 未实现。本类仅完成数据结构层面的解析与注册：assemble 暂返回 base 副本（TODO），
 * 待纹饰系统接入后补全 TRIM 组件写入逻辑。
 *
 * 参考: net.minecraft.world.item.crafting.SmithingTrimRecipe
 */
class SmithingTrimRecipe : public IRecipe<IInventory> {
public:
    /// 模板槽位索引（原版 1.21+ 锻造台 3 槽：template=0/base=1/addition=2）
    static constexpr i32 SLOT_TEMPLATE = 0;
    /// 基础物品槽位索引
    static constexpr i32 SLOT_BASE = 1;
    /// 添加物槽位索引
    static constexpr i32 SLOT_ADDITION = 2;

    /**
     * @brief 构造盔甲纹饰配方
     * @param id 配方ID
     * @param templateIngredient 模板原料
     * @param base 基础物品原料（可纹饰盔甲）
     * @param addition 添加物原料（纹饰材料）
     * @param pattern 纹饰图案 ResourceLocation（对应 TrimPattern，纹饰注册表未实现）
     */
    SmithingTrimRecipe(const ResourceLocation& id,
        Ingredient templateIngredient,
        Ingredient base,
        Ingredient addition,
        ResourceLocation pattern);

    ~SmithingTrimRecipe() override = default;

    // ========== IRecipe 接口实现 ==========

    [[nodiscard]] bool matches(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const IInventory& inventory) const override;
    /// trim 配方无固定结果物品（结果依赖 base + 纹饰），返回空堆
    [[nodiscard]] ItemStack getResultItem() const override { return ItemStack::EMPTY; }
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override { return m_ingredients; }
    [[nodiscard]] const std::string& getGroup() const override { return EMPTY_GROUP; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] RecipeType getType() const override { return RecipeType::SmithingTrim; }
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override { return width * height >= 3; }
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const IInventory& inventory) const override;

    /// assemble 依赖 base 物品，结果每次可能不同
    [[nodiscard]] bool isDynamic() const override { return true; }

    // ========== 盔甲纹饰配方特有方法 ==========

    [[nodiscard]] const Ingredient& getTemplate() const { return m_template; }
    [[nodiscard]] const Ingredient& getBase() const { return m_base; }
    [[nodiscard]] const Ingredient& getAddition() const { return m_addition; }
    [[nodiscard]] const ResourceLocation& getPattern() const { return m_pattern; }

private:
    static const std::string EMPTY_GROUP;

    ResourceLocation m_id;
    Ingredient m_template;                 ///< 模板原料
    Ingredient m_base;                     ///< 基础物品原料（可纹饰盔甲）
    Ingredient m_addition;                 ///< 添加物原料（纹饰材料）
    ResourceLocation m_pattern;            ///< 纹饰图案（TODO: 待 TrimPattern 注册表接入后解析为 Holder）
    std::vector<Ingredient> m_ingredients; ///< 缓存的原料列表（template + base + addition）
};

} // namespace crafting
} // namespace mc
