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

#include <cmath>
#include <gtest/gtest.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include "client/renderer/trident/sky/CelestialCalculations.hpp"
#include "common/core/Types.hpp"
#include "common/world/time/GameTime.hpp"

using namespace mc;
using namespace mc::time;
using namespace mc::client;

// ============================================================================
// GameTime 测试
// ============================================================================

TEST(GameTimeTest, InitialState)
{
    GameTime time;
    EXPECT_EQ(time.dayTime(), 0);
    EXPECT_EQ(time.dayTimeOfDay(), 0);
    EXPECT_EQ(time.gameTime(), 0);
    EXPECT_TRUE(time.daylightCycleEnabled());
}

TEST(GameTimeTest, TickIncrement)
{
    GameTime time;

    time.tick();
    EXPECT_EQ(time.dayTime(), 1);
    EXPECT_EQ(time.dayTimeOfDay(), 1);
    EXPECT_EQ(time.gameTime(), 1);

    time.tick();
    EXPECT_EQ(time.dayTime(), 2);
    EXPECT_EQ(time.dayTimeOfDay(), 2);
    EXPECT_EQ(time.gameTime(), 2);
}

TEST(GameTimeTest, DayTimeUnbounded)
{
    // MC 1.16.5 行为：dayTime 是无边界计数器
    GameTime time;
    time.setDayTime(23999);
    time.tick();

    // dayTime 超过 23999，不取模
    EXPECT_EQ(time.dayTime(), 24000);
    // dayTimeOfDay 返回归一化值 (0-23999)
    EXPECT_EQ(time.dayTimeOfDay(), 0);
    EXPECT_EQ(time.gameTime(), 1);
}

TEST(GameTimeTest, SetDayTimeUnbounded)
{
    // MC 1.16.5 行为：setDayTime 直接存储，不取模
    GameTime time;

    time.setDayTime(25000);
    EXPECT_EQ(time.dayTime(), 25000);
    EXPECT_EQ(time.dayTimeOfDay(), 1000);

    time.setDayTime(100000);
    EXPECT_EQ(time.dayTime(), 100000);
    EXPECT_EQ(time.dayTimeOfDay(), 100000 % 24000);

    // 负数也被存储
    time.setDayTime(-100);
    EXPECT_EQ(time.dayTime(), -100);
    // dayTimeOfDay 使用数学公式处理负数
    EXPECT_EQ(time.dayTimeOfDay(), ((-100 % 24000) + 24000) % 24000); // = 23900
}

TEST(GameTimeTest, AddDayTime)
{
    GameTime time;

    time.addDayTime(1000);
    EXPECT_EQ(time.dayTime(), 1000);
    EXPECT_EQ(time.dayTimeOfDay(), 1000);

    time.setDayTime(1000);
    time.addDayTime(23000);
    // (1000 + 23000) = 24000 - 无边界存储
    EXPECT_EQ(time.dayTime(), 24000);
    EXPECT_EQ(time.dayTimeOfDay(), 0);
}

TEST(GameTimeTest, DaylightCycleDisabled)
{
    GameTime time;
    time.setDaylightCycleEnabled(false);

    EXPECT_FALSE(time.daylightCycleEnabled());

    time.tick();
    EXPECT_EQ(time.dayTime(), 0);  // dayTime 不递增
    EXPECT_EQ(time.gameTime(), 1); // gameTime 仍然递增
}

TEST(GameTimeTest, IsDayAndIsNight)
{
    GameTime time;

    time.setDayTime(0); // 日出
    EXPECT_TRUE(time.isDay());

    time.setDayTime(6000); // 正午
    EXPECT_TRUE(time.isDay());

    time.setDayTime(12000); // 日落
    EXPECT_TRUE(time.isNight());

    time.setDayTime(18000); // 午夜
    EXPECT_TRUE(time.isNight());

    // 测试超过 24000 的情况
    time.setDayTime(25000); // 25000 % 24000 = 1000，应该是白天
    EXPECT_TRUE(time.isDay());
    EXPECT_FALSE(time.isNight());

    time.setDayTime(36000); // 36000 % 24000 = 12000，应该是夜晚
    EXPECT_TRUE(time.isNight());
    EXPECT_FALSE(time.isDay());
}

TEST(GameTimeTest, DayCount)
{
    GameTime time;

    time.setGameTime(0);
    EXPECT_EQ(time.dayCount(), 0);

    time.setGameTime(24000);
    EXPECT_EQ(time.dayCount(), 1);

    time.setGameTime(48000);
    EXPECT_EQ(time.dayCount(), 2);
}

TEST(GameTimeTest, DayTimeForNetwork)
{
    GameTime time;

    time.setDayTime(1000);
    EXPECT_EQ(time.dayTimeForNetwork(), 1000);

    time.setDaylightCycleEnabled(false);
    EXPECT_EQ(time.dayTimeForNetwork(), -1000); // 负数表示日光周期禁用

    // 测试超过 24000 的情况
    time.setDaylightCycleEnabled(true);
    time.setDayTime(25000);
    // dayTimeForNetwork 返回 dayTimeOfDay (0-23999)
    EXPECT_EQ(time.dayTimeForNetwork(), 1000);
}

// ============================================================================
// CelestialCalculations 测试
// ============================================================================

TEST(CelestialCalculationsTest, NoonCelestialAngleNearZero)
{
    // 正午 = 6000 ticks
    f32 angle = CelestialCalculations::calculateCelestialAngle(6000);
    EXPECT_NEAR(angle, 0.0f, 0.1f);
}

TEST(CelestialCalculationsTest, MidnightCelestialAngleNearHalf)
{
    // 午夜 = 18000 ticks
    f32 angle = CelestialCalculations::calculateCelestialAngle(18000);
    EXPECT_NEAR(angle, 0.5f, 0.1f);
}

TEST(CelestialCalculationsTest, CelestialAngleInRange)
{
    for (i64 t = 0; t <= 24000; t += 1000) {
        f32 angle = CelestialCalculations::calculateCelestialAngle(t);
        EXPECT_GE(angle, 0.0f);
        EXPECT_LE(angle, 1.0f);
    }
}

TEST(CelestialCalculationsTest, MoonPhaseDay0FullMoon)
{
    EXPECT_EQ(CelestialCalculations::calculateMoonPhase(0), 0);
}

TEST(CelestialCalculationsTest, MoonPhaseDay1WaxingGibbous)
{
    EXPECT_EQ(CelestialCalculations::calculateMoonPhase(24000), 1);
}

TEST(CelestialCalculationsTest, MoonPhaseDay7WaningGibbous)
{
    EXPECT_EQ(CelestialCalculations::calculateMoonPhase(24000 * 7), 7);
}

TEST(CelestialCalculationsTest, MoonPhaseDay8CycleBackToFullMoon)
{
    EXPECT_EQ(CelestialCalculations::calculateMoonPhase(24000 * 8), 0);
}

TEST(CelestialCalculationsTest, MoonPhaseFactorFullMoon)
{
    EXPECT_FLOAT_EQ(CelestialCalculations::getMoonPhaseFactor(0), 1.0f); // 满月
}

TEST(CelestialCalculationsTest, MoonPhaseFactorNewMoon)
{
    EXPECT_FLOAT_EQ(CelestialCalculations::getMoonPhaseFactor(4), 0.0f); // 新月
}

TEST(CelestialCalculationsTest, MoonPhaseFactorFirstQuarter)
{
    EXPECT_FLOAT_EQ(CelestialCalculations::getMoonPhaseFactor(2), 0.5f); // 上弦月
}

TEST(CelestialCalculationsTest, SunDirectionNoonUpward)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(6000);
    glm::vec3 dir = CelestialCalculations::calculateSunDirection(angle);

    // 正午时 celestialAngle ≈ 0, 太阳应该在最高点
    // 太阳高度 cos(0) = 1, 所以 dir.y 应该接近 1
    EXPECT_GT(dir.y, 0.9f);
}

TEST(CelestialCalculationsTest, SunDirectionMidnightDownward)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(18000);
    glm::vec3 dir = CelestialCalculations::calculateSunDirection(angle);

    // 午夜时 celestialAngle ≈ 0.5, 太阳应该在地下
    // 太阳高度 cos(π) = -1, 所以 dir.y 应该接近 -1
    EXPECT_LT(dir.y, -0.9f);
}

TEST(CelestialCalculationsTest, SunDirectionNormalized)
{
    for (i64 t = 0; t <= 24000; t += 2000) {
        f32 angle = CelestialCalculations::calculateCelestialAngle(t);
        glm::vec3 dir = CelestialCalculations::calculateSunDirection(angle);
        f32 length = glm::length(dir);
        EXPECT_NEAR(length, 1.0f, 0.001f);
    }
}

TEST(CelestialCalculationsTest, SunIntensityNoonHighest)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(6000);
    f32 intensity = CelestialCalculations::calculateSunIntensity(angle);
    EXPECT_GT(intensity, 0.9f);
}

TEST(CelestialCalculationsTest, SunIntensityMidnightZero)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(18000);
    f32 intensity = CelestialCalculations::calculateSunIntensity(angle);
    EXPECT_LT(intensity, 0.1f);
}

TEST(CelestialCalculationsTest, SunIntensityInRange)
{
    for (i64 t = 0; t <= 24000; t += 1000) {
        f32 angle = CelestialCalculations::calculateCelestialAngle(t);
        f32 intensity = CelestialCalculations::calculateSunIntensity(angle);
        EXPECT_GE(intensity, 0.0f);
        EXPECT_LE(intensity, 1.0f);
    }
}

TEST(CelestialCalculationsTest, SkyColorNoonBlue)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(6000);
    glm::vec4 color = CelestialCalculations::calculateSkyColor(angle, 0.0f, 0.0f);

    // 正午天太阳角度≈0, 颜色索引0应该是蓝色
    // 蓝色: B > R 且 B > G
    EXPECT_GT(color.b, color.r);
    EXPECT_GT(color.b, color.g);
    EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST(CelestialCalculationsTest, SkyColorNoonMatchesOverworldDefault78A7FF)
{
    const f32 angle = CelestialCalculations::calculateCelestialAngle(6000);
    const glm::vec4 color = CelestialCalculations::calculateSkyColor(angle, 0.0f, 0.0f);
    const glm::vec3 base = CelestialCalculations::getOverworldBaseSkyColor();

    EXPECT_NEAR(color.r, base.r, 0.01f);
    EXPECT_NEAR(color.g, base.g, 0.01f);
    EXPECT_NEAR(color.b, base.b, 0.01f);
}

TEST(CelestialCalculationsTest, SkyColorMidnightDark)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(18000);
    glm::vec4 color = CelestialCalculations::calculateSkyColor(angle, 0.0f, 0.0f);

    // 午夜天太阳角度≈0.5, 颜色索引2应该是深蓝色
    // 深色: RGB 分量都很低 (< 0.15)
    EXPECT_LT(color.r, 0.15f);
    EXPECT_LT(color.g, 0.15f);
    EXPECT_LT(color.b, 0.2f);
}

TEST(CelestialCalculationsTest, SunriseSunsetColorAppearsNearHorizonAndPeaksThenFades)
{
    f32 maxAlpha = 0.0f;
    i64 maxAlphaTime = -1;

    // 扫描整天，确认效果只在晨昏附近出现。
    for (i64 t = 0; t < 24000; t += 100) {
        const f32 angle = CelestialCalculations::calculateCelestialAngle(t);
        const glm::vec4 sunrise = CelestialCalculations::calculateSunriseSunsetColor(angle, 0.0f, 0.0f);
        if (sunrise.a > maxAlpha) {
            maxAlpha = sunrise.a;
            maxAlphaTime = t;
        }
    }

    EXPECT_GT(maxAlpha, 0.3f);
    EXPECT_GE(maxAlphaTime, 0);

    // 正午应无晨昏色。
    const f32 noonAngle = CelestialCalculations::calculateCelestialAngle(6000);
    const glm::vec4 noonSunrise = CelestialCalculations::calculateSunriseSunsetColor(noonAngle, 0.0f, 0.0f);
    EXPECT_LT(noonSunrise.a, 0.001f);
}

TEST(CelestialCalculationsTest, SunriseSunsetColorIsAttenuatedByWeather)
{
    // 取一段接近晨昏的时间（靠近日落）。
    const f32 angle = CelestialCalculations::calculateCelestialAngle(12000);
    const glm::vec4 clear = CelestialCalculations::calculateSunriseSunsetColor(angle, 0.0f, 0.0f);
    const glm::vec4 rain = CelestialCalculations::calculateSunriseSunsetColor(angle, 1.0f, 0.0f);
    const glm::vec4 thunder = CelestialCalculations::calculateSunriseSunsetColor(angle, 0.0f, 1.0f);

    EXPECT_GE(clear.a, rain.a);
    EXPECT_GE(clear.a, thunder.a);
}

TEST(CelestialCalculationsTest, SunriseFacingFactorUsesHorizontalDirectionOnly)
{
    const glm::vec3 sunriseDir(1.0f, 0.0f, 0.0f);

    const f32 facing =
        CelestialCalculations::calculateSunriseFacingFactor(glm::normalize(glm::vec3(1.0f, 0.8f, 0.0f)), sunriseDir);
    const f32 opposite =
        CelestialCalculations::calculateSunriseFacingFactor(glm::normalize(glm::vec3(-1.0f, -0.2f, 0.0f)), sunriseDir);

    EXPECT_GT(facing, 0.95f);
    EXPECT_LT(opposite, 0.05f);
}

TEST(CelestialCalculationsTest, StarBrightnessNoonZero)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(6000);
    f32 brightness = CelestialCalculations::calculateStarBrightness(angle, 0.0);
    EXPECT_LT(brightness, 0.1f);
}

TEST(CelestialCalculationsTest, StarBrightnessMidnightVisible)
{
    f32 angle = CelestialCalculations::calculateCelestialAngle(18000);
    f32 brightness = CelestialCalculations::calculateStarBrightness(angle, 0.0);
    // 午夜时星星亮度应 >= 0.5
    EXPECT_GE(brightness, 0.5f);
}

TEST(CelestialCalculationsTest, StarBrightnessInRange)
{
    for (i64 t = 0; t <= 24000; t += 1000) {
        f32 angle = CelestialCalculations::calculateCelestialAngle(t);
        f32 brightness = CelestialCalculations::calculateStarBrightness(angle, 0.0);
        EXPECT_GE(brightness, 0.0f);
        EXPECT_LE(brightness, 1.0f);
    }
}

TEST(CelestialCalculationsTest, InterpolatedCelestialAngleInRange)
{
    f32 angle0 = CelestialCalculations::calculateCelestialAngleInterpolated(0, 0.0f);
    f32 angle0_5 = CelestialCalculations::calculateCelestialAngleInterpolated(0, 0.5f);
    f32 angle1 = CelestialCalculations::calculateCelestialAngleInterpolated(0, 1.0f);

    EXPECT_GE(angle0, 0.0f);
    EXPECT_LE(angle0, 1.0f);
    EXPECT_GE(angle0_5, 0.0f);
    EXPECT_LE(angle0_5, 1.0f);
    EXPECT_GE(angle1, 0.0f);
    EXPECT_LE(angle1, 1.0f);
}

TEST(CelestialCalculationsTest, StarSeed)
{
    EXPECT_EQ(CelestialCalculations::getStarSeed(), 10842L);
}

TEST(CelestialCalculationsTest, StarCount)
{
    EXPECT_EQ(CelestialCalculations::getStarCount(), 1500);
}
