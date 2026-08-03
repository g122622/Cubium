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

#include "DamageEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/util/math/random/Random.hpp"
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 节肢杀手附魔
 *
 * 增加对节肢生物的伤害。
 *
 * 效果:
 * - 对节肢生物每级增加 2.5 点伤害
 * - 节肢生物包括：蜘蛛、洞穴蜘蛛、蠹虫、末影螨、蜜蜂等
 * - 被击中的节肢生物会获得缓慢 IV 效果 1-1.5 秒
 * - 最大 V 级
 * - 与锋利、亡灵杀手互斥
 */
class BaneOfArthropodsEnchantment : public DamageEnchantment {
public:
    BaneOfArthropodsEnchantment()
        : DamageEnchantment(Type::Arthropods)
    {}

    [[nodiscard]] std::string id() const override { return "minecraft:bane_of_arthropods"; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.bane_of_arthropods";
    }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Uncommon; }

    /**
     * @brief 获取缓慢效果的持续时间（tick）
     *
     * @param level 附魔等级
     * @param random 随机数生成器
     * @return 持续时间（tick）
     */
    [[nodiscard]] static i32 getSlownessDuration(i32 level, math::Random& random)
    {
        return 20 + random.nextInt(10 * level);
    }

    /**
     * @brief 获取缓慢效果等级（固定为 IV）
     * @return 缓慢效果等级 (3 = Slowness IV)
     */
    [[nodiscard]] static constexpr i32 getSlownessAmplifier()
    {
        return 3; // Slowness IV
    }

    /**
     * @brief 当攻击目标实体时调用
     *
     * 对节肢生物施加缓慢 IV 效果。
     *
     * @param user 攻击者
     * @param target 目标实体
     * @param level 附魔等级
     */
    void onEntityDamaged(LivingEntity& user, Entity& target, i32 level) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
