#pragma once

#include "Potion.hpp"
#include "PotionRegistry.hpp"

namespace mc {
namespace potion {

/**
 * @brief 原版药水静态引用
 *
 * 提供所有原版药水的静态指针，便于快速访问。
 * 在游戏初始化时调用 Potions::initialize() 进行注册。
 *
 * 参考: net.minecraft.potion.Potions
 */
class Potions {
public:
    /**
     * @brief 初始化所有原版药水
     *
     * 必须在使用任何药水前调用。
     */
    static void initialize();

    // ========== 基础药水 ==========

    /// 空药水
    static const Potion* EMPTY;
    /// 水瓶
    static const Potion* WATER;
    /// 平凡的药水
    static const Potion* MUNDANE;
    /// 浓稠的药水
    static const Potion* THICK;
    /// 尴尬的药水
    static const Potion* AWKWARD;

    // ========== 夜视 ==========

    /// 夜视药水 (3:00)
    static const Potion* NIGHT_VISION;
    /// 夜视药水 (8:00)
    static const Potion* LONG_NIGHT_VISION;

    // ========== 隐身 ==========

    /// 隐身药水 (3:00)
    static const Potion* INVISIBILITY;
    /// 隐身药水 (8:00)
    static const Potion* LONG_INVISIBILITY;

    // ========== 跳跃提升 ==========

    /// 跳跃提升药水 (3:00)
    static const Potion* LEAPING;
    /// 跳跃提升药水 (8:00)
    static const Potion* LONG_LEAPING;
    /// 跳跃提升药水 II (1:30)
    static const Potion* STRONG_LEAPING;

    // ========== 防火 ==========

    /// 防火药水 (3:00)
    static const Potion* FIRE_RESISTANCE;
    /// 防火药水 (8:00)
    static const Potion* LONG_FIRE_RESISTANCE;

    // ========== 速度 ==========

    /// 速度药水 (3:00)
    static const Potion* SWIFTNESS;
    /// 速度药水 (8:00)
    static const Potion* LONG_SWIFTNESS;
    /// 速度药水 II (1:30)
    static const Potion* STRONG_SWIFTNESS;

    // ========== 缓慢 ==========

    /// 缓慢药水 (1:30)
    static const Potion* SLOWNESS;
    /// 缓慢药水 (4:00)
    static const Potion* LONG_SLOWNESS;
    /// 缓慢药水 IV (0:20)
    static const Potion* STRONG_SLOWNESS;

    // ========== 海龟大师 ==========

    /// 海龟大师药水 (0:20)
    static const Potion* TURTLE_MASTER;
    /// 海龟大师药水 (0:40)
    static const Potion* LONG_TURTLE_MASTER;
    /// 海龟大师药水 II (0:20)
    static const Potion* STRONG_TURTLE_MASTER;

    // ========== 水下呼吸 ==========

    /// 水下呼吸药水 (3:00)
    static const Potion* WATER_BREATHING;
    /// 水下呼吸药水 (8:00)
    static const Potion* LONG_WATER_BREATHING;

    // ========== 瞬间治疗 ==========

    /// 瞬间治疗药水
    static const Potion* HEALING;
    /// 瞬间治疗药水 II
    static const Potion* STRONG_HEALING;

    // ========== 瞬间伤害 ==========

    /// 瞬间伤害药水
    static const Potion* HARMING;
    /// 瞬间伤害药水 II
    static const Potion* STRONG_HARMING;

    // ========== 中毒 ==========

    /// 中毒药水 (0:45)
    static const Potion* POISON;
    /// 中毒药水 (1:30)
    static const Potion* LONG_POISON;
    /// 中毒药水 II (0:21)
    static const Potion* STRONG_POISON;

    // ========== 生命恢复 ==========

    /// 生命恢复药水 (0:45)
    static const Potion* REGENERATION;
    /// 生命恢复药水 (1:30)
    static const Potion* LONG_REGENERATION;
    /// 生命恢复药水 II (0:22)
    static const Potion* STRONG_REGENERATION;

    // ========== 力量 ==========

    /// 力量药水 (3:00)
    static const Potion* STRENGTH;
    /// 力量药水 (8:00)
    static const Potion* LONG_STRENGTH;
    /// 力量药水 II (1:30)
    static const Potion* STRONG_STRENGTH;

    // ========== 虚弱 ==========

    /// 虚弱药水 (1:30)
    static const Potion* WEAKNESS;
    /// 虚弱药水 (4:00)
    static const Potion* LONG_WEAKNESS;

    // ========== 幸运 ==========

    /// 幸运药水 (5:00)
    static const Potion* LUCK;

    // ========== 缓降 ==========

    /// 缓降药水 (1:30)
    static const Potion* SLOW_FALLING;
    /// 缓降药水 (4:00)
    static const Potion* LONG_SLOW_FALLING;

private:
    static bool s_initialized;

    /**
     * @brief 注册药水的辅助方法
     * @param name 药水名称（不含命名空间）
     * @param potion 药水实例
     * @return 注册后的药水指针
     */
    static const Potion* registerPotion(const char* name, Potion potion);
};

} // namespace potion
} // namespace mc
