#pragma once

#include "../../Enchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 伤害加成附魔基类
 *
 * 锋利、亡灵杀手、节肢杀手的共同基类。
 * 参考 MC 1.16.5 DamageEnchantment
 *
 * 这三类附魔互斥，不能同时存在于同一物品上。
 */
class DamageEnchantment : public Enchantment {
public:
    /**
     * @brief 伤害类型枚举
     */
    enum class Type : u8 {
        All,        ///< 锋利 - 对所有生物有效
        Undead,     ///< 亡灵杀手 - 对亡灵生物有效
        Arthropods  ///< 节肢杀手 - 对节肢生物有效
    };

    explicit DamageEnchantment(Type damageType);

    // ========== Enchantment 接口实现 ==========

    [[nodiscard]] EnchantmentType type() const override {
        return EnchantmentType::Weapon;
    }

    [[nodiscard]] i32 minLevel() const override {
        return 1;
    }

    [[nodiscard]] i32 maxLevel() const override {
        return 5;
    }

    [[nodiscard]] i32 getMinCost(i32 level) const override;

    [[nodiscard]] i32 getMaxCost(i32 level) const override;

    [[nodiscard]] f32 getDamageBonus(i32 level, u32 entityType) const override;

    [[nodiscard]] bool isCompatibleWith(const Enchantment& other) const override;

    // ========== 伤害类特有方法 ==========

    /**
     * @brief 获取伤害类型
     */
    [[nodiscard]] Type getDamageType() const { return m_damageType; }

protected:
    Type m_damageType;
};

} // namespace enchant
} // namespace item
} // namespace mc
