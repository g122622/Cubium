/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
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
#include <string>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 风爆附魔（重锤专属宝藏附魔）
 *
 * 重锤专属宝藏附魔，下落攻击命中后产生风爆将攻击者弹起，
 * 允许连续砸地连击。
 *
 * 效果：
 * - 下落攻击命中后，在攻击者位置产生风爆爆炸
 * - 爆炸不破坏方块，仅触发击退效果
 * - 爆炸半径随等级增加
 * - 最大 III 级
 * - 宝藏附魔，不能从附魔台获得
 * - 不与任何伤害附魔互斥（可与致密或破甲共存）
 *
 * 爆炸半径（击退乘数）：
 * - I: 1.2
 * - II: 1.75
 * - III: 2.2
 *
 * 触发条件：攻击者正在下落（fallDistance >= 1.5）且不在滑翔
 *
 * 命名空间ID: minecraft:wind_burst
 */
class WindBurstEnchantment : public Enchantment {
public:
    /// 附魔ID
    static constexpr const char* ENCHANTMENT_ID = "minecraft:wind_burst";

    WindBurstEnchantment() = default;

    [[nodiscard]] std::string id() const override { return ENCHANTMENT_ID; }

    [[nodiscard]] std::string getNameKey(i32 level) const override
    {
        (void)level;
        return "enchantment.minecraft.wind_burst";
    }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }
    [[nodiscard]] i32 maxLevel() const noexcept override { return 3; }

    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::Weapon; }

    [[nodiscard]] EnchantmentRarity rarity() const noexcept override { return EnchantmentRarity::Rare; }

    [[nodiscard]] i32 getMinCost(i32 level) const override { return 15 + (level - 1) * 9; }

    [[nodiscard]] i32 getMaxCost(i32 level) const override { return 65 + (level - 1) * 9; }

    /// 风爆是宝藏附魔，不能从附魔台获得
    [[nodiscard]] bool isTreasure() const noexcept override { return true; }

    /// 风爆不能出现在村民交易中
    [[nodiscard]] bool canVillagerTrade() const noexcept override { return false; }

    /// 风爆不能出现在战利品箱中（只能从试炼密室获取）
    [[nodiscard]] bool canGenerateInLoot() const noexcept override { return true; }

    /**
     * @brief 获取风爆爆炸的击退乘数
     *
     * 击退乘数按等级查找表：
     * - I: 1.2
     * - II: 1.75
     * - III: 2.2
     *
     * @param level 附魔等级
     * @return 击退乘数
     */
    [[nodiscard]] static f32 getExplosionKnockbackMultiplier(i32 level) noexcept
    {
        switch (level) {
            case 1:
                return 1.2f;
            case 2:
                return 1.75f;
            case 3:
                return 2.2f;
            default:
                return 1.5f + 0.35f * static_cast<f32>(level); // fallback
        }
    }

    /**
     * @brief 获取风爆爆炸的交互范围
     * @return 爆炸交互范围
     */
    [[nodiscard]] static constexpr f32 getExplosionInteractionRange() noexcept { return 3.5f; }

    /// 风爆不与伤害附魔互斥（可与致密或破甲共存）
    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;
};

} // namespace enchant
} // namespace item
} // namespace mc
