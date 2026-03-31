#pragma once

#include "../../core/Types.hpp"
#include <string>

namespace mc {
namespace entity {
namespace effect {

/**
 * @brief 效果类型枚举
 *
 * 定义所有状态效果类型。
 * 参考 MC 1.16.5 Effect
 */
enum class EffectType : u8 {
    // 有益效果
    Speed = 1,              // 速度
    Slowness = 2,           // 缓慢
    Haste = 3,              // 急迫
    MiningFatigue = 4,      // 挖掘疲劳
    Strength = 5,           // 力量
    InstantHealth = 6,      // 瞬间治疗
    InstantDamage = 7,      // 瞬间伤害
    JumpBoost = 8,          // 跳跃提升
    Nausea = 9,             // 反胃
    Regeneration = 10,      // 生命恢复
    Resistance = 11,        // 抗性提升
    FireResistance = 12,    // 防火
    WaterBreathing = 13,    // 水下呼吸
    Invisibility = 14,      // 隐身
    Blindness = 15,         // 失明
    NightVision = 16,       // 夜视
    Hunger = 17,            // 饥饿
    Weakness = 18,          // 虚弱
    Poison = 19,            // 中毒
    Wither = 20,            // 凋零
    HealthBoost = 21,       // 生命提升
    Absorption = 22,        // 伤害吸收
    Saturation = 23,        // 饱和

    // 不祥之兆相关
    BadOmen = 31,           // 不祥之兆
    HeroOfTheVillage = 32,  // 村庄英雄

    // 其他
    Levitation = 25,        // 漂浮
    Luck = 26,              // 幸运
    BadLuck = 27,           // 霉运
    SlowFalling = 28,       // 缓降
    ConduitPower = 29,      // 潮涌能量
    DolphinsGrace = 30,     // 海豚的恩惠

    // 数量
    Count
};

/**
 * @brief 获取效果名称
 */
[[nodiscard]] const char* getEffectName(EffectType type);

/**
 * @brief 检查效果是否有益
 */
[[nodiscard]] bool isBeneficialEffect(EffectType type);

/**
 * @brief 获取效果默认颜色
 */
[[nodiscard]] u32 getEffectColor(EffectType type);

} // namespace effect
} // namespace entity
} // namespace mc
