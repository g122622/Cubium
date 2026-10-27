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

#include <array>

namespace mc {
namespace world {

/**
 * @brief 月相
 *
 * MC 1.21.11 enum MoonPhase。
 * 8 个相位循环，每个相位持续 PHASE_LENGTH 个 dayTime tick（一个游戏日）。
 * 索引顺序即 vanilla 顺序：0=满月 …… 4=新月 …… 7=盈凸月。
 */
enum class MoonPhase : u8 {
    FullMoon = 0,
    WaningGibbous = 1,
    ThirdQuarter = 2,
    WaningCrescent = 3,
    NewMoon = 4,
    WaxingCrescent = 5,
    FirstQuarter = 6,
    WaxingGibbous = 7,
};

/// 月相总数
inline constexpr i32 MOON_PHASE_COUNT = 8;
/// 单个月相持续的 dayTime tick 数（= 一个游戏日）
inline constexpr i32 MOON_PHASE_LENGTH = 24000;

/**
 * @brief 全部月相，按索引顺序排列
 */
inline constexpr std::array<MoonPhase, MOON_PHASE_COUNT> MOON_PHASES = {MoonPhase::FullMoon,
    MoonPhase::WaningGibbous,
    MoonPhase::ThirdQuarter,
    MoonPhase::WaningCrescent,
    MoonPhase::NewMoon,
    MoonPhase::WaxingCrescent,
    MoonPhase::FirstQuarter,
    MoonPhase::WaxingGibbous};

/**
 * @brief 各月相下天空光照亮度（MC 1.21.11 DimensionType.MOON_BRIGHTNESS_PER_PHASE）
 */
inline constexpr std::array<f32, MOON_PHASE_COUNT> MOON_BRIGHTNESS_PER_PHASE = {
    1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f};

/**
 * @brief 月相索引（0-7）
 */
[[nodiscard]] constexpr i32 moonPhaseIndex(MoonPhase phase) noexcept
{
    return static_cast<i32>(phase);
}

/**
 * @brief 月相生效区间的起始 dayTime tick
 *
 * 月相时间线的周期为 MOON_PHASE_LENGTH * MOON_PHASE_COUNT，
 * 第 i 个相位从 i * MOON_PHASE_LENGTH 开始。
 */
[[nodiscard]] constexpr i32 moonPhaseStartTick(MoonPhase phase) noexcept
{
    return moonPhaseIndex(phase) * MOON_PHASE_LENGTH;
}

} // namespace world
} // namespace mc
