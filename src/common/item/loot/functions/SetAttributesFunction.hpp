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

#include "LootFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/RandomRanges.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief Set attributes function
 *
 * Adds attribute modifiers to an item.
 * See: net.minecraft.loot.functions.SetAttributes
 *
 * Used for generating equipment with specific attributes.
 * Attribute modifiers are stored in the item's AttributeModifiers NBT tag.
 */
class SetAttributesFunction : public LootFunction {
public:
    /**
     * @brief 属性修饰符定义
     */
    struct Modifier {
        std::string name;               ///< 修饰符名称
        std::string attributeId;        ///< 属性ID（如 "minecraft:generic.attack_damage"）
        math::RandomValueRange amount;  ///< 值范围（支持随机）
        u8 operation;                   ///< 操作类型（0=加法, 1=乘法基础, 2=乘法总计）
        std::vector<std::string> slots; ///< 装备槽位列表（运行时随机选择一个）
        std::string uuid;               ///< 可选UUID；若为空则运行时随机生成

        Modifier() = default;
        Modifier(const std::string& n,
            const std::string& attr,
            const math::RandomValueRange& amt,
            u8 op,
            const std::vector<std::string>& s,
            const std::string& u = "")
            : name(n)
            , attributeId(attr)
            , amount(amt)
            , operation(op)
            , slots(s)
            , uuid(u)
        {}
    };

    /**
     * @brief Construct set attributes function
     */
    SetAttributesFunction() = default;

    /**
     * @brief Add an attribute modifier
     */
    void addModifier(const Modifier& modifier);

    [[nodiscard]] ItemStack apply(ItemStack stack, LootContext& context) const override;
    [[nodiscard]] std::unique_ptr<LootFunction> clone() const noexcept override;
    [[nodiscard]] std::string getType() const override { return "set_attributes"; }

    [[nodiscard]] const std::vector<Modifier>& getModifiers() const { return m_modifiers; }

private:
    std::vector<Modifier> m_modifiers;

    /**
     * @brief 解析槽位名称为 EquipmentSlot 枚举值
     * @param slotName 槽位名称（如 "mainhand", "offhand", "feet", "legs", "chest", "head"）
     * @return EquipmentSlot 枚举值；无效名称返回 MainHand
     */
    static i32 _parseSlotName(const std::string& slotName);

    /**
     * @brief 生成随机 UUID
     * @param random 随机数生成器
     * @return UUID 字符串（格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）
     */
    static std::string _generateUUID(math::Random& random);
};

} // namespace loot
} // namespace mc
