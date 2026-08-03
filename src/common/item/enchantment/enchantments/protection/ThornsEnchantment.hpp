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

#include "../../Enchantment.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 荆棘附魔
 *
 * 攻击者会受到反伤。
 *
 * 效果:
 * - 每级增加触发概率和伤害
 * - I: 15%概率, 0.5-1.5伤害
 * - II: 30%概率, 0.5-2.5伤害
 * - III: 45%概率, 0.5-3.5伤害
 * - 最大 III 级
 */
class ThornsEnchantment : public Enchantment {
public:
    ThornsEnchantment() = default;

    [[nodiscard]] std::string id() const override { return "minecraft:thorns"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.thorns";
    }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::ArmorChest; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::VeryRare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 10 + (level - 1) * 20; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return getMinCost(level) + 50; }

    /**
     * @brief 检查是否触发荆棘效果
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return true 如果触发
     */
    [[nodiscard]] static bool shouldTrigger(i32 level, math::Random& random);

    /**
     * @brief 获取荆棘反伤
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return 反伤点数
     */
    [[nodiscard]] static i32 getThornsDamage(i32 level, math::Random& random);

    /**
     * @brief 获取触发概率
     * @param level 附魔等级
     * @return 触发概率 (0.0-1.0)
     */
    [[nodiscard]] static f32 getTriggerChance(i32 level)
    {
        // 每级 15%
        return static_cast<f32>(level) * 0.15f;
    }

    /**
     * @brief 当持有者受到伤害时调用
     *
     * 对攻击者造成反伤。
     *
     * @param user 受伤者（持有荆棘附魔装备的实体）
     * @param attacker 攻击者
     * @param level 附魔等级
     */
    void onUserHurt(LivingEntity& user, Entity& attacker, i32 level) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
