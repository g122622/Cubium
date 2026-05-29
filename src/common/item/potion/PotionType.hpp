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

#include "../../core/Types.hpp"
#include <cstddef>
#include <functional>

namespace mc {
namespace potion {

/**
 * @brief 药水类型枚举
 *
 * 定义所有原版药水类型。
 * 参考 MC 1.16.5 Potion
 */
enum class PotionId : u16 {
    // 基础药水
    Empty = 0,
    Water = 1,
    Mundane = 2,
    Thick = 3,
    Awkward = 4,

    // 夜视药水
    NightVision = 5,
    LongNightVision = 6,

    // 隐身药水
    Invisibility = 7,
    LongInvisibility = 8,

    // 跳跃提升药水
    Leaping = 9,
    LongLeaping = 10,
    StrongLeaping = 11,

    // 防火药水
    FireResistance = 12,
    LongFireResistance = 13,

    // 速度药水
    Swiftness = 14,
    LongSwiftness = 15,
    StrongSwiftness = 16,

    // 缓慢药水
    Slowness = 17,
    LongSlowness = 18,
    StrongSlowness = 19,

    // 海龟大师药水
    TurtleMaster = 20,
    LongTurtleMaster = 21,
    StrongTurtleMaster = 22,

    // 水下呼吸药水
    WaterBreathing = 23,
    LongWaterBreathing = 24,

    // 瞬间治疗药水
    Healing = 25,
    StrongHealing = 26,

    // 瞬间伤害药水
    Harming = 27,
    StrongHarming = 28,

    // 中毒药水
    Poison = 29,
    LongPoison = 30,
    StrongPoison = 31,

    // 生命恢复药水
    Regeneration = 32,
    LongRegeneration = 33,
    StrongRegeneration = 34,

    // 力量药水
    Strength = 35,
    LongStrength = 36,
    StrongStrength = 37,

    // 虚弱药水
    Weakness = 38,
    LongWeakness = 39,

    // 幸运药水
    Luck = 40,

    // 缓降药水
    SlowFalling = 41,
    LongSlowFalling = 42,

    // 总数
    Count = 43
};

/**
 * @brief 药水名称前缀类型
 *
 * 用于生成药水物品的翻译键
 */
enum class PotionPrefix : u8 {
    Empty,   // 空
    Water,   // 水瓶
    Mundane, // 平凡的药水
    Thick,   // 浓稠的药水
    Awkward, // 尴尬的药水
    Regular, // 普通药水
    Long,    // 延长版
    Strong   // 加强版
};

/**
 * @brief 获取药水ID的字符串名称
 */
[[nodiscard]] const char* getPotionIdName(PotionId id);

/**
 * @brief 获取药水的基础名称
 *
 * 例如：NightVision 返回 "night_vision"
 */
[[nodiscard]] const char* getPotionBaseName(PotionId id);

/**
 * @brief 获取药水的前缀类型
 */
[[nodiscard]] PotionPrefix getPotionPrefix(PotionId id);

/**
 * @brief 检查药水是否有效果
 */
[[nodiscard]] bool hasPotionEffects(PotionId id);

/**
 * @brief 检查药水是否为瞬间效果
 */
[[nodiscard]] bool isInstantPotion(PotionId id);

} // namespace potion
} // namespace mc

// std::hash 特化
namespace std {
template <>
struct hash<mc::potion::PotionId> {
    size_t operator()(mc::potion::PotionId id) const noexcept { return static_cast<size_t>(id); }
};
} // namespace std
