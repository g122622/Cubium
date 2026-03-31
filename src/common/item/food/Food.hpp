#pragma once

#include "../../core/Types.hpp"
#include <vector>

namespace mc {

// Forward declarations for potion effects
namespace entity::effect {
    class PotionEffect;
}

namespace item::food {

/**
 * @brief 食物效果条目
 *
 * 描述食物可能给予的药水效果。
 */
struct FoodEffect {
    const entity::effect::PotionEffect* effect = nullptr;  ///< 药水效果
    f32 probability = 1.0f;                                 ///< 触发概率 (0.0 - 1.0)
};

/**
 * @brief 食物属性类
 *
 * 定义食物的属性，如饥饿值、饱和度、是否可以快速食用等。
 * 参考: net.minecraft.item.Food
 *
 * 用法示例:
 * @code
 * Food apple(4, 0.3f);  // 恢复4点饥饿值，0.3饱和度
 * Food goldenApple(4, 1.2f).setAlwaysEdible(true);
 * Food pufferfish(1, 0.1f).addEffect(potionEffect, 1.0f);
 * @endcode
 */
class Food {
public:
    /**
     * @brief 构造食物属性
     * @param hunger 恢复的饥饿值 (0-20)
     * @param saturation 恢复的饱和度 (0.0-1.0+)
     */
    Food(i32 hunger, f32 saturation);

    // ========== 构建器方法 ==========

    /**
     * @brief 设置是否为肉类
     * @param isMeat 是否为肉类
     * @note 肉类食物可以喂给狼
     */
    Food& setMeat(bool isMeat = true) {
        m_isMeat = isMeat;
        return *this;
    }

    /**
     * @brief 设置是否可以快速食用
     * @param fastEat 是否快速食用
     * @note 快速食用时间为16ticks，普通为32ticks
     */
    Food& setFastEat(bool fastEat = true) {
        m_fastEat = fastEat;
        return *this;
    }

    /**
     * @brief 设置是否可以在饱食时食用
     * @param alwaysEdible 是否总是可食用
     * @note 金苹果等特殊食物需要此属性
     */
    Food& setAlwaysEdible(bool alwaysEdible = true) {
        m_alwaysEdible = alwaysEdible;
        return *this;
    }

    /**
     * @brief 添加药水效果
     * @param effect 药水效果
     * @param probability 触发概率 (0.0 - 1.0)
     * @note 可以添加多个效果，如迷之炖菜
     */
    Food& addEffect(const entity::effect::PotionEffect* effect, f32 probability) {
        m_effects.push_back({effect, probability});
        return *this;
    }

    // ========== 获取属性 ==========

    /**
     * @brief 获取恢复的饥饿值
     */
    [[nodiscard]] i32 getHunger() const { return m_hunger; }

    /**
     * @brief 获取恢复的饱和度
     */
    [[nodiscard]] f32 getSaturation() const { return m_saturation; }

    /**
     * @brief 是否为肉类
     */
    [[nodiscard]] bool isMeat() const { return m_isMeat; }

    /**
     * @brief 是否可以快速食用
     */
    [[nodiscard]] bool isFastEat() const { return m_fastEat; }

    /**
     * @brief 是否可以在饱食时食用
     */
    [[nodiscard]] bool canAlwaysEat() const { return m_alwaysEdible; }

    /**
     * @brief 获取所有药水效果
     */
    [[nodiscard]] const std::vector<FoodEffect>& getEffects() const { return m_effects; }

    /**
     * @brief 是否有药水效果
     */
    [[nodiscard]] bool hasEffects() const { return !m_effects.empty(); }

private:
    i32 m_hunger;                              ///< 恢复的饥饿值
    f32 m_saturation;                          ///< 恢复的饱和度
    bool m_isMeat = false;                     ///< 是否为肉类
    bool m_fastEat = false;                    ///< 是否快速食用
    bool m_alwaysEdible = false;               ///< 是否总是可食用
    std::vector<FoodEffect> m_effects;         ///< 药水效果列表
};

} // namespace item::food
} // namespace mc
