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

#include "InternalLightUtils.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace InternalLightUtils {

// ============================================================================
// 天空减暗计算
// ============================================================================

i32 calculateSkyDarkening(i64 dayTime, bool isRaining, bool isThundering) noexcept
{
    // 基础天空减暗（基于时间）
    i32 baseDarkening = calculateDefaultSkyDarkening(dayTime);

    // 天气影响
    if (isThundering) {
        // 雷暴时天空更暗
        baseDarkening = std::min(baseDarkening + 10, 11);
    } else if (isRaining) {
        // 下雨时天空变暗
        baseDarkening = std::min(baseDarkening + 3, 11);
    }

    return baseDarkening;
}

i32 calculateDefaultSkyDarkening(i64 dayTime) noexcept
{
    f32 celestialAngle = getCelestialAngle(dayTime);
    return calculateSkyDarkeningFromAngle(celestialAngle);
}

i32 calculateSkyDarkeningFromAngle(f32 celestialAngle) noexcept
{
    // celestialAngle: 0.0 = 日出, 0.25 = 正午, 0.5 = 日落, 0.75 = 午夜
    // 天空减暗范围: 0 (正午) 到 11 (午夜)

    // 计算太阳高度
    // 使用 sin 而不是 cos：
    // - 正午 (0.25): sin(π/2) = 1 (太阳最高)
    // - 午夜 (0.75): sin(3π/2) = -1 (太阳最低)
    f32 sunAngle = celestialAngle * mc::math::TWO_PI;
    f32 sunHeight = std::sin(sunAngle); // [-1, 1]

    // 将太阳高度映射到亮度因子 [0, 1]
    // sunHeight = 1 → brightness = 1 (最亮)
    // sunHeight = -1 → brightness = 0 (最暗)
    f32 brightness = (sunHeight + 1.0f) / 2.0f;

    // 天空减暗 = (1 - 亮度) * 11
    // 正午 → darkening = 0
    // 午夜 → darkening = 11
    i32 darkening = static_cast<i32>((1.0f - brightness) * 11.0f);
    return std::clamp(darkening, 0, 11);
}

// ============================================================================
// 内部光照计算
// ============================================================================

i32 calculateInternalLight(u8 blockLight, u8 skyLight) noexcept
{
    return std::max(static_cast<i32>(blockLight), static_cast<i32>(skyLight));
}

i32 calculateRawBrightness(u8 blockLight, u8 skyLight, i32 skyDarkening) noexcept
{
    // 天空光照减去减暗因子，但不低于0
    i32 effectiveSkyLight = static_cast<i32>(skyLight) - skyDarkening;
    effectiveSkyLight = std::max(0, effectiveSkyLight);

    // 取最大值
    return std::max(static_cast<i32>(blockLight), effectiveSkyLight);
}

// ============================================================================
// 生物生成条件
// ============================================================================

bool isDarkEnoughForSpawning(i32 rawBrightness) noexcept
{
    // 亮度 <= 7 时可以生成敌对生物
    return rawBrightness <= 7;
}

bool isBrightEnoughToPreventSpawning(i32 rawBrightness) noexcept
{
    // 亮度 > 7 时不能生成敌对生物
    return rawBrightness > 7;
}

bool canSleep(i32 rawBrightness, bool isThundering) noexcept
{
    // 夜晚或雷暴时可以睡觉
    // 亮度 <= 7 表示夜晚
    return rawBrightness <= 7 || isThundering;
}

// ============================================================================
// 天体计算
// ============================================================================

f32 getCelestialAngle(i64 dayTime) noexcept
{
    // 一天有 DAY_LENGTH_TICKS ticks
    // celestialAngle = 0.0 时日出
    // celestialAngle = 0.25 时正午
    // celestialAngle = 0.5 时日落
    // celestialAngle = 0.75 时午夜

    // MC中 dayTime:
    // 0 = 日出
    // 6000 = 正午
    // 12000 = 日落
    // 18000 = 午夜

    // 直接计算，不需要偏移
    i64 timeOfDay = dayTime % game::DAY_LENGTH_TICKS;
    return static_cast<f32>(timeOfDay) / static_cast<f32>(game::DAY_LENGTH_TICKS);
}

f32 getCelestialAngleMC(i64 dayTime) noexcept
{
    // 原版天体角度计算
    //
    // 公式:
    //   d0 = frac(dayTime / DAY_LENGTH_TICKS - 0.25)
    //   d1 = 0.5 - cos(d0 * π) / 2.0
    //   result = (d0 * 2.0 + d1) / 3.0
    //
    // 这个公式使得：
    // - 正午 (dayTime = 6000)  时 celestialAngle = 0.0
    // - 日落 (dayTime = 12000) 时 celestialAngle ≈ 0.25
    // - 午夜 (dayTime = 18000) 时 celestialAngle = 0.5
    // - 日出 (dayTime = 0)     时 celestialAngle ≈ 0.75
    //
    // 黎明时分 (dayTime ≈ 22000-22600) 时 celestialAngle ≈ 0.65-0.69
    // 这是海龟蛋孵化的最佳时间

    constexpr f64 TICKS_PER_DAY = static_cast<f64>(game::DAY_LENGTH_TICKS);

    f64 normalizedTime = static_cast<f64>(dayTime % game::DAY_LENGTH_TICKS);
    f64 d0 = normalizedTime / TICKS_PER_DAY - 0.25;

    // 确保 d0 在 [0, 1) 范围内 (frac 函数)
    if (d0 < 0.0) {
        d0 += 1.0;
    }

    // d1 = 0.5 - cos(d0 * π) / 2.0
    // 这是一个平滑曲线，在 d0=0 时 d1=0，在 d0=0.5 时 d1=1
    f64 d1 = 0.5 - std::cos(d0 * math::PI_DOUBLE) / 2.0;

    // result = (d0 * 2.0 + d1) / 3.0
    f64 celestialAngle = (d0 * 2.0 + d1) / 3.0;

    return static_cast<f32>(celestialAngle);
}

f32 getSunAngle(f32 celestialAngle) noexcept
{
    // 太阳高度角
    // 0 = 地平线，正值 = 白天，负值 = 夜晚

    f32 angle = celestialAngle * mc::math::TWO_PI;
    return std::sin(angle);
}

bool isDaytime(i64 dayTime) noexcept
{
    // 白天: 0 - 12000 (日出到日落)
    // 注意：MC中时间从日出开始
    i64 timeOfDay = dayTime % game::DAY_LENGTH_TICKS;
    return timeOfDay < 12000;
}

bool isNighttime(i64 dayTime) noexcept
{
    // 夜晚: 12000 - 24000 (日落到日出)
    i64 timeOfDay = dayTime % game::DAY_LENGTH_TICKS;
    return timeOfDay >= 12000;
}

i32 getMoonPhase(i64 dayTime) noexcept
{
    // 月相周期为8天
    // 每天有 DAY_LENGTH_TICKS ticks
    i64 dayNumber = dayTime / game::DAY_LENGTH_TICKS;
    return static_cast<i32>(dayNumber % 8);
}

f32 getMoonBrightness(i32 moonPhase) noexcept
{
    // 月相亮度：
    // 0: 满月 (1.0)
    // 1: 亏凸月 (0.75)
    // 2: 下弦月 (0.5)
    // 3: 亏月 (0.25)
    // 4: 新月 (0.0)
    // 5: 盈月 (0.25)
    // 6: 上弦月 (0.5)
    // 7: 盈凸月 (0.75)

    static const f32 PHASE_BRIGHTNESS[8] = {1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f};

    return PHASE_BRIGHTNESS[moonPhase % 8];
}

} // namespace InternalLightUtils
} // namespace mc
