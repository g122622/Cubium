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
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 深海探索者附魔
 *
 * 增加水下移动速度。
 *
 * 效果:
 * - 每级减少水下移动惩罚
 * - I: 1/3 减免, II: 2/3 减免, III: 完全减免
 * - 最大 III 级
 */
class DepthStriderEnchantment : public Enchantment {
public:
    DepthStriderEnchantment()
    {
        setAnvilCost(4);
    }

    [[nodiscard]] std::string id() const override
    {
        return "minecraft:depth_strider";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.depth_strider";
    }

    [[nodiscard]] EnchantmentType type() const override
    {
        return EnchantmentType::ArmorFeet;
    }

    [[nodiscard]] i32 minLevel() const noexcept override
    {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const noexcept override
    {
        return 3;
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override
    {
        return EnchantmentRarity::Rare;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const noexcept override
    {
        return 10 + (level - 1) * 10;
    }

    [[nodiscard]] i32 getMaxCost(i32 level) const noexcept override
    {
        return getMinCost(level) + 15;
    }

    /**
     * @brief 获取水下移动速度乘数
     * @param level 附魔等级
     * @return 速度乘数 (0.0-1.0)
     */
    [[nodiscard]] static f32 getWaterSpeedMultiplier(i32 level) noexcept
    {
        // 每级减少 1/3 的水下移动惩罚
        return static_cast<f32>(level) / 3.0f;
    }

    /**
     * @brief 深海探索者的属性修饰符
     *
     * 对齐 vanilla `data/minecraft/enchantment/depth_strider.json`：
     * 属性 `minecraft:water_movement_efficiency`、id `minecraft:enchantment.depth_strider`、
     * `add_value`、amount = linear(base 0.33333334, per_level 0.33333334)，作用槽位 = FEET。
     *
     * 基础值刻意写成 f32 的 0.33333334（而不是 1.0/3.0 的 double 结果）：原版 JSON 里存的
     * 就是这个十进制字面量，读完再转 float 后三级恰好得到 0.33333334 / 0.6666667 / 1.0，
     * 用 1.0/3.0 会差 1ulp，与属性值逐位比对时会对不上。
     *
     * @param level 附魔等级
     * @return 脚部槽位的 water_movement_efficiency 修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 level) const override
    {
        item::ItemAttributeModifiers modifiers;
        if (level > 0) {
            constexpr f64 kPerLevel = 0.33333334;
            modifiers.add(entity::attribute::Attributes::WATER_MOVEMENT_EFFICIENCY,
                entity::attribute::AttributeModifier("minecraft:enchantment.depth_strider",
                    "Depth Strider",
                    kPerLevel * static_cast<f64>(level),
                    entity::attribute::Operation::Addition),
                static_cast<i32>(EquipmentSlot::Feet));
        }
        return modifiers;
    }

    /**
     * @brief 检查是否与冰霜行者互斥
     */
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
