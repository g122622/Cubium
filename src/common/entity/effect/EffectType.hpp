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
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>

namespace mc {
namespace entity {
namespace effect {

/**
 * @brief 效果类型枚举
 *
 * 定义所有状态效果类型。
 */
enum class EffectType : u8 {
    // 有益效果
    Speed = 1,             // 速度
    Slowness = 2,          // 缓慢
    Haste = 3,             // 急迫
    MiningFatigue = 4,     // 挖掘疲劳
    Strength = 5,          // 力量
    InstantHealth = 6,     // 瞬间治疗
    InstantDamage = 7,     // 瞬间伤害
    JumpBoost = 8,         // 跳跃提升
    Nausea = 9,            // 反胃
    Regeneration = 10,     // 生命恢复
    Resistance = 11,       // 抗性提升
    FireResistance = 12,   // 防火
    WaterBreathing = 13,   // 水下呼吸
    Invisibility = 14,     // 隐身
    Blindness = 15,        // 失明
    NightVision = 16,      // 夜视
    Hunger = 17,           // 饥饿
    Weakness = 18,         // 虚弱
    Poison = 19,           // 中毒
    Wither = 20,           // 凋零
    HealthBoost = 21,      // 生命提升
    Absorption = 22,       // 伤害吸收
    Saturation = 23,       // 饱和
    Glowing = 24,          // 发光（MC 1.16.5 ID=24）
    Levitation = 25,       // 漂浮
    Luck = 26,             // 幸运
    BadLuck = 27,          // 霉运
    SlowFalling = 28,      // 缓降
    ConduitPower = 29,     // 潮涌能量
    DolphinsGrace = 30,    // 海豚的恩惠
    BadOmen = 31,          // 不祥之兆
    HeroOfTheVillage = 32, // 村庄英雄

    // MC 1.21 试炼密室效果
    TrialOmen = 33,   // 试炼之兆
    WindCharged = 34, // 风充能
    RaidOmen = 35,    // 袭击之兆

    // MC 1.19 深暗之域效果
    Darkness = 36, // 黑暗

    // MC 1.21 试炼密室效果（续，vanilla MobEffects.bootstrap 末尾 4 项）
    Weaving = 37,             // 盘绕
    Oozing = 38,              // 渗浆
    Infested = 39,            // 寄生
    BreathOfTheNautilus = 40, // 鹦鹉螺之力

    // 数量
    Count
};

/**
 * @brief 获取效果名称
 */
[[nodiscard]] const char* getEffectName(EffectType type) noexcept;

/**
 * @brief 检查效果是否有益
 */
[[nodiscard]] bool isBeneficialEffect(EffectType type) noexcept;

/**
 * @brief 获取效果默认颜色
 */
[[nodiscard]] u32 getEffectColor(EffectType type) noexcept;

/**
 * @brief 从数值ID获取效果类型
 *
 * @param id 效果ID（1-36）
 * @return 效果类型，如果ID无效返回 std::nullopt
 */
[[nodiscard]] std::optional<EffectType> getEffectById(i32 id) noexcept;

/**
 * @brief 从资源位置获取效果类型
 *
 * 支持 "minecraft:speed" 格式或简写 "speed" 格式。
 *
 * @param id 资源位置
 * @return 效果类型，如果未找到返回 std::nullopt
 */
[[nodiscard]] std::optional<EffectType> getEffectByResourceLocation(const ResourceLocation& id) noexcept;

/**
 * @brief 获取效果的资源位置
 *
 * @param type 效果类型
 * @return 资源位置（如 "minecraft:speed"）
 */
[[nodiscard]] ResourceLocation getEffectResourceLocation(EffectType type) noexcept;

/**
 * @brief 获取效果的资源名称（不含命名空间）
 *
 * @param type 效果类型
 * @return 资源名称（如 "speed"）
 */
[[nodiscard]] const char* getEffectResourceName(EffectType type) noexcept;

/**
 * @brief 检查效果是否为瞬间效果
 *
 * 瞬间效果包括：瞬间治疗、瞬间伤害、饱和
 *
 * @param type 效果类型
 * @return 如果是瞬间效果返回 true
 */
[[nodiscard]] bool isInstantEffect(EffectType type) noexcept;

} // namespace effect
} // namespace entity
} // namespace mc
