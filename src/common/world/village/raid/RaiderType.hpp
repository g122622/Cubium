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

namespace mc::world::village::raid {

/**
 * @brief 袭击者类型。
 *
 * 用于描述袭击事件中可能生成的敌对生物类别。
 */
enum class RaiderType : u8 { Pillager = 0, Vindicator, Evoker, Ravager, Witch };

/**
 * @brief 袭击者类型辅助工具。
 *
 * 该工具只包含纯函数，不持有状态，适合作为后续数据驱动配置接入前的过渡层。
 */
class RaiderTypeHelper {
public:
    /**
     * @brief 获取袭击者类型名称。
     *
     * @param type 袭击者类型。
     * @return 对应的稳定英文名称。
     *
     * @note 返回值为静态字符串字面量，调用方不得释放。
     */
    [[nodiscard]] static const char* getName(RaiderType type) noexcept;

    /**
     * @brief 获取袭击者基础生命值。
     *
     * @param type 袭击者类型。
     * @return 基础生命值。
     *
     * @note 该值目前用于简化袭击配置，后续可迁移到实体属性模板。
     */
    [[nodiscard]] static f32 getBaseHealth(RaiderType type) noexcept;

    /**
     * @brief 获取指定波次下的生成权重。
     *
     * @param type 袭击者类型。
     * @param wave 波次编号，从 1 开始。
     * @return 生成权重，0 表示当前波次不会生成该类型。
     *
     * @warning 调用方需保证 `wave >= 1`，本函数不做防御性兜底。
     */
    [[nodiscard]] static i32 getSpawnWeight(RaiderType type, i32 wave) noexcept;

    /**
     * @brief 判断该袭击者是否可骑乘劫掠兽。
     *
     * @param type 袭击者类型。
     * @return 是否允许骑乘劫掠兽。
     */
    [[nodiscard]] static bool canRideRavager(RaiderType type) noexcept;
};

/**
 * @brief 袭击状态。
 */
enum class RaidStatus : u8 { Ongoing = 0, Victory, Loss, Stopped };

/**
 * @brief 袭击唯一标识。
 */
using RaidId = u64;

/**
 * @brief 袭击系统静态配置。
 *
 * 当前为编译期常量配置，后续若接入数据包或游戏规则，可在外层再封装运行时配置层。
 */
struct RaidConfig {
    static constexpr i32 MAX_WAVES_EASY = 3;
    static constexpr i32 MAX_WAVES_NORMAL = 5;
    static constexpr i32 MAX_WAVES_HARD = 7;

    static constexpr i64 WAVE_INTERVAL = 1200;
    static constexpr i64 RAID_TIMEOUT = 48000;

    /// Java 版 1.21.11：波次之间的冷却 tick 数，用于 Boss 栏进度展示。
    static constexpr i32 RAID_COOLDOWN_TICKS = 300;

    static constexpr f32 SPAWN_DISTANCE_MIN = 45.0f;
    static constexpr f32 SPAWN_DISTANCE_MAX = 52.0f;

    static constexpr i32 BAD_OMEN_WAVE_BONUS = 1;
    static constexpr i32 MIN_RAIDERS_PER_WAVE = 3;
    static constexpr i32 MAX_RAIDERS_PER_WAVE = 20;
};

} // namespace mc::world::village::raid
