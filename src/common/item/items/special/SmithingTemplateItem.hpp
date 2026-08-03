/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "item/core/Item.hpp"
#include <string>
#include <vector>

namespace mc {
namespace item {

/**
 * @brief 锻造模板类型枚举
 *
 * 区分锻造模板的两大类别：盔甲纹饰模板和下界合金升级模板。
 * 两种类型在锻造台界面中显示不同的空槽位图标和提示文本。
 */
enum class SmithingTemplateType : u8 {
    ArmorTrim,       // 盔甲纹饰模板
    NetheriteUpgrade // 下界合金升级模板
};

/**
 * @brief 锻造模板物品
 *
 * 用于锻造台配方的模板物品。锻造台有三个槽位：
 * 模板槽、基础物品槽、附加材料槽。
 *
 * 盔甲纹饰模板（18种）：
 * - Sentry, Dune, Coast, Wild, Ward, Eye, Vex, Tide, Snout, Rib,
 *   Spire, Wayfinder, Shaper, Silence, Raiser, Host, Flow, Bolt
 *
 * 下界合金升级模板（1种）：
 * - NetheriteUpgrade
 *
 * 每种模板提供以下信息：
 * - 适用于什么（appliesTo）：提示文本说明模板可以应用在哪些物品上
 * - 需要什么材料（ingredients）：提示文本说明附加材料槽需要什么
 * - 基础槽空位图标路径
 * - 附加材料槽空位图标路径
 *
 * tooltip 显示格式（对应 MC Java SmithingTemplateItem.appendHoverText）：
 * 1. "Smithing Template"（灰色标题，翻译键 item.minecraft.smithing_template）
 * 2. 空行
 * 3. "Applies to:"（灰色标题，翻译键 item.minecraft.smithing_template.applies_to）
 * 4. " Armor"（蓝色描述，翻译键 item.minecraft.smithing_template.armor_trim.applies_to 等）
 * 5. "Ingredients:"（灰色标题，翻译键 item.minecraft.smithing_template.ingredients）
 * 6. " Ingots & Crystals"（蓝色描述，翻译键 item.minecraft.smithing_template.armor_trim.ingredients 等）
 */
class SmithingTemplateItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param type 锻造模板类型（盔甲纹饰 / 下界合金升级）
     * @param appliesTo 适用于什么的提示翻译键
     * @param ingredients 需要什么材料的提示翻译键
     * @param baseSlotDescription 基础槽描述翻译键
     * @param additionsSlotDescription 附加材料槽描述翻译键
     * @param properties 物品属性
     */
    SmithingTemplateItem(SmithingTemplateType type,
        const std::string& appliesTo,
        const std::string& ingredients,
        const std::string& baseSlotDescription,
        const std::string& additionsSlotDescription,
        ItemProperties properties);

    ~SmithingTemplateItem() override = default;

    /**
     * @brief 获取锻造模板类型
     */
    [[nodiscard]] SmithingTemplateType getTemplateType() const noexcept { return m_type; }

    /**
     * @brief 获取适用于什么的提示翻译键
     */
    [[nodiscard]] const std::string& getAppliesTo() const noexcept { return m_appliesTo; }

    /**
     * @brief 获取需要什么材料的提示翻译键
     */
    [[nodiscard]] const std::string& getIngredients() const noexcept { return m_ingredients; }

    /**
     * @brief 获取基础槽描述翻译键
     */
    [[nodiscard]] const std::string& getBaseSlotDescription() const noexcept { return m_baseSlotDescription; }

    /**
     * @brief 获取附加材料槽描述翻译键
     */
    [[nodiscard]] const std::string& getAdditionsSlotDescription() const noexcept { return m_additionsSlotDescription; }

    /**
     * @brief 添加物品提示信息
     *
     * 按顺序显示以下行：
     * 1. "Smithing Template" 灰色标题
     * 2. 空行
     * 3. "Applies to:" 灰色标题
     * 4. 具体适用描述（如 "Armor"）
     * 5. "Ingredients:" 灰色标题
     * 6. 具体材料描述（如 "Ingots & Crystals"）
     *
     * 所有文本通过 LanguageManager 从翻译键翻译后显示。
     *
     * TODO: 待锻造台配方系统（SmithingTransformRecipe / SmithingTrimRecipe）集成后，
     * 此类应与 TrimPattern 注册表关联，并提供空槽位图标路径用于客户端锻造台界面渲染。
     */
    void addInformation(
        const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const override;

    // ========== 工厂方法 ==========

    /**
     * @brief 创建盔甲纹饰锻造模板的物品属性
     *
     * 盔甲纹饰模板堆叠数为 64，与 MC 原版一致。
     *
     * @return 预配置的 ItemProperties
     */
    static ItemProperties armorTrimProperties();

    /**
     * @brief 创建下界合金升级锻造模板的物品属性
     *
     * 下界合金升级模板堆叠数为 64，与 MC 原版一致。
     *
     * @return 预配置的 ItemProperties
     */
    static ItemProperties netheriteUpgradeProperties();

private:
    SmithingTemplateType m_type;
    std::string m_appliesTo;                // 适用于什么的提示翻译键
    std::string m_ingredients;              // 需要什么材料的提示翻译键
    std::string m_baseSlotDescription;      // 基础槽描述翻译键
    std::string m_additionsSlotDescription; // 附加材料槽描述翻译键
};

} // namespace item
} // namespace mc
