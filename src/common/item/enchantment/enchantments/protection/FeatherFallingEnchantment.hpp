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

#include "ProtectionEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 摔落保护附魔（羽毛落地）
 *
 * 减少摔落伤害。
 *
 * 效果:
 * - 对摔落伤害有三倍保护效果
 * - 可以与其他保护类附魔共存
 * - 最大 IV 级
 */
class FeatherFallingEnchantment : public ProtectionEnchantment {
public:
    FeatherFallingEnchantment()
        : ProtectionEnchantment(Type::Fall)
    {
        setAnvilCost(2);
    }

    [[nodiscard]] std::string id() const override
    {
        return "minecraft:feather_falling";
    }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.feather_falling";
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override
    {
        return EnchantmentRarity::Uncommon;
    }

    /**
     * @brief 附魔适用类型：只有靴子
     *
     * 原版 `supported_items` 是 `#minecraft:enchantable/foot_armor`。
     * 此前没有覆写，继承了父类 `ProtectionEnchantment` 的 `Armor`，
     * 于是头盔、胸甲、护腿也都能附（由差分能力 enchantment_applicability 查出：
     * 多判 22 个物品）。
     */
    [[nodiscard]] EnchantmentType type() const noexcept override
    {
        return EnchantmentType::ArmorFeet;
    }
};

} // namespace enchant
} // namespace item
} // namespace mc
