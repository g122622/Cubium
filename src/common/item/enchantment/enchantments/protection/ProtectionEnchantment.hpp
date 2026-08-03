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
#include "common/item/enchantment/Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 保护类附魔基类
 *
 * 所有保护类附魔的抽象基类。
 *
 * 保护类型:
 * - 全保护: 减少所有伤害
 * - 火焰保护: 减少火焰伤害
 * - 摔落保护: 减少摔落伤害
 * - 爆炸保护: 减少爆炸伤害
 * - 弹射物保护: 减少弹射物伤害
 */
class ProtectionEnchantment : public Enchantment {
public:
    /**
     * @brief 保护类型枚举
     */
    enum class Type : u8 {
        All,       ///< 全保护（减少所有伤害）
        Fire,      ///< 火焰保护
        Fall,      ///< 摔落保护（羽毛落地）
        Explosion, ///< 爆炸保护
        Projectile ///< 弹射物保护
    };

    explicit ProtectionEnchantment(Type protectionType);

    // ========== Enchantment 接口实现 ==========

    [[nodiscard]] EnchantmentType type() const noexcept override { return EnchantmentType::Armor; }

    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }

    [[nodiscard]] i32 maxLevel() const noexcept override { return 4; }

    [[nodiscard]] i32 getMinCost(i32 level) const override;

    [[nodiscard]] i32 getMaxCost(i32 level) const override;

    [[nodiscard]] i32 getDamageProtection(i32 level, u32 damageType) const noexcept override;

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;

    // ========== 保护类特有方法 ==========

    /**
     * @brief 获取保护类型
     */
    [[nodiscard]] Type getProtectionType() const noexcept { return m_protectionType; }

    /**
     * @brief 检查是否减少火焰伤害
     */
    [[nodiscard]] bool reducesFireDamage() const noexcept
    {
        return m_protectionType == Type::Fire || m_protectionType == Type::All;
    }

    /**
     * @brief 检查是否减少摔落伤害
     */
    [[nodiscard]] bool reducesFallDamage() const noexcept
    {
        return m_protectionType == Type::Fall || m_protectionType == Type::All;
    }

    /**
     * @brief 检查是否减少爆炸伤害
     */
    [[nodiscard]] bool reducesExplosionDamage() const noexcept
    {
        return m_protectionType == Type::Explosion || m_protectionType == Type::All;
    }

    /**
     * @brief 检查是否减少弹射物伤害
     */
    [[nodiscard]] bool reducesProjectileDamage() const noexcept
    {
        return m_protectionType == Type::Projectile || m_protectionType == Type::All;
    }

protected:
    Type m_protectionType;
};

} // namespace enchant
} // namespace item
} // namespace mc
