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

#include <gtest/gtest.h>

#include "client/world/ClientWeather.hpp"

using namespace mc::client;

/**
 * @brief ClientWeather 闪电闪烁效果单元测试
 *
 * 测试闪电击中时天空闪烁效果的实现。
 * 参考 MC 1.16.5 World.setTimeLightningFlash()
 */
class ClientWeatherLightningFlashTest : public ::testing::Test {
protected:
    ClientWeather weather;

    void SetUp() override
    {
        // 每个测试开始时重置天气状态
        weather = ClientWeather();
    }
};

// ========== 基本功能测试 ==========

TEST_F(ClientWeatherLightningFlashTest, DefaultNoLightningFlash)
{
    // 默认状态下应该没有闪电闪烁
    EXPECT_FALSE(weather.hasLightningFlash());
    EXPECT_EQ(weather.lightningFlashTime(), 0);
    EXPECT_EQ(weather.lightningFlashBrightness(), 0.0f);
}

TEST_F(ClientWeatherLightningFlashTest, SetLightningFlashTime)
{
    // 设置闪电闪烁时间为 2 ticks (MC 1.16.5 标准)
    weather.setTimeLightningFlash(2);

    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_EQ(weather.lightningFlashTime(), 2);
    EXPECT_EQ(weather.lightningFlashBrightness(), 1.0f);
}

TEST_F(ClientWeatherLightningFlashTest, SetLightningFlashTimeZero)
{
    // 设置为 0 应该没有闪烁效果
    weather.setTimeLightningFlash(0);

    EXPECT_FALSE(weather.hasLightningFlash());
    EXPECT_EQ(weather.lightningFlashTime(), 0);
    EXPECT_EQ(weather.lightningFlashBrightness(), 0.0f);
}

TEST_F(ClientWeatherLightningFlashTest, SetLightningFlashTimeNegative)
{
    // 负值应该被视为无闪烁
    weather.setTimeLightningFlash(-1);

    EXPECT_FALSE(weather.hasLightningFlash());
    EXPECT_EQ(weather.lightningFlashTime(), -1);
    EXPECT_EQ(weather.lightningFlashBrightness(), 0.0f);
}

// ========== tickLightningFlash 测试 ==========

TEST_F(ClientWeatherLightningFlashTest, TickDecreasesFlashTime)
{
    // 设置闪烁时间为 2
    weather.setTimeLightningFlash(2);

    // 第一个 tick 后应该变为 1
    weather.tickLightningFlash();
    EXPECT_EQ(weather.lightningFlashTime(), 1);
    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_EQ(weather.lightningFlashBrightness(), 1.0f);

    // 第二个 tick 后应该变为 0
    weather.tickLightningFlash();
    EXPECT_EQ(weather.lightningFlashTime(), 0);
    EXPECT_FALSE(weather.hasLightningFlash());
    EXPECT_EQ(weather.lightningFlashBrightness(), 0.0f);
}

TEST_F(ClientWeatherLightningFlashTest, TickDoesNotGoNegative)
{
    // 设置闪烁时间为 0
    weather.setTimeLightningFlash(0);

    // tick 后不应该变成负数
    weather.tickLightningFlash();
    EXPECT_EQ(weather.lightningFlashTime(), 0);
    EXPECT_FALSE(weather.hasLightningFlash());
}

TEST_F(ClientWeatherLightningFlashTest, TickStopsAtZero)
{
    // 设置闪烁时间为 1
    weather.setTimeLightningFlash(1);

    // tick 后变为 0
    weather.tickLightningFlash();
    EXPECT_EQ(weather.lightningFlashTime(), 0);

    // 再次 tick 应该保持 0
    weather.tickLightningFlash();
    EXPECT_EQ(weather.lightningFlashTime(), 0);
    EXPECT_FALSE(weather.hasLightningFlash());
}

// ========== 闪烁亮度测试 ==========

TEST_F(ClientWeatherLightningFlashTest, BrightnessIsOneWhenFlashing)
{
    // 任何正数闪烁时间都应该返回亮度 1.0
    weather.setTimeLightningFlash(1);
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);

    weather.setTimeLightningFlash(2);
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);

    weather.setTimeLightningFlash(10);
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);
}

TEST_F(ClientWeatherLightningFlashTest, BrightnessIsZeroWhenNotFlashing)
{
    // 闪烁时间为 0 或负数时亮度应该是 0
    weather.setTimeLightningFlash(0);
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 0.0f);

    weather.setTimeLightningFlash(-5);
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 0.0f);
}

// ========== 与天气状态结合测试 ==========

TEST_F(ClientWeatherLightningFlashTest, LightningFlashIndependentOfRain)
{
    // 闪电闪烁效果应该独立于雨强度
    weather.setRainStrength(0.5f);
    weather.setTimeLightningFlash(2);

    // 闪烁效果应该正常工作
    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);
    EXPECT_FLOAT_EQ(weather.rainStrength(0.0f), 0.5f);
}

TEST_F(ClientWeatherLightningFlashTest, LightningFlashIndependentOfThunder)
{
    // 闪电闪烁效果应该独立于雷暴强度
    weather.setThunderStrength(0.8f);
    weather.setTimeLightningFlash(2);

    // 闪烁效果应该正常工作
    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);
    EXPECT_FLOAT_EQ(weather.thunderStrength(0.0f), 0.8f * 0.0f); // thunderStrength *= rainStrength
}

TEST_F(ClientWeatherLightningFlashTest, LightningFlashDuringThunderstorm)
{
    // 模拟雷暴期间的闪电
    weather.setRainStrength(1.0f);
    weather.setThunderStrength(1.0f);
    weather.setTimeLightningFlash(2);

    // 所有天气状态应该正确
    EXPECT_TRUE(weather.isRaining());
    EXPECT_TRUE(weather.isThundering());
    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);
}

// ========== 完整生命周期测试 ==========

TEST_F(ClientWeatherLightningFlashTest, CompleteLightningFlashCycle)
{
    // 模拟完整的闪电闪烁周期 (MC 1.16.5: setTimeLightningFlash(2) -> 2 ticks -> 0)

    // 闪电击中
    weather.setTimeLightningFlash(2);
    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);

    // 第一个 tick
    weather.tickLightningFlash();
    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);

    // 第二个 tick (闪烁结束)
    weather.tickLightningFlash();
    EXPECT_FALSE(weather.hasLightningFlash());
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 0.0f);

    // 后续 tick 应该保持无闪烁
    weather.tickLightningFlash();
    EXPECT_FALSE(weather.hasLightningFlash());
}

TEST_F(ClientWeatherLightningFlashTest, MultipleLightningFlashes)
{
    // 模拟多次闪电击中

    // 第一次闪电
    weather.setTimeLightningFlash(2);
    EXPECT_TRUE(weather.hasLightningFlash());

    weather.tickLightningFlash();
    EXPECT_TRUE(weather.hasLightningFlash());

    // 在闪烁期间又发生一次闪电（重置时间）
    weather.setTimeLightningFlash(2);
    EXPECT_EQ(weather.lightningFlashTime(), 2);

    weather.tickLightningFlash();
    EXPECT_TRUE(weather.hasLightningFlash());

    weather.tickLightningFlash();
    EXPECT_FALSE(weather.hasLightningFlash());
}

// ========== 边界条件测试 ==========

TEST_F(ClientWeatherLightningFlashTest, LargeFlashTime)
{
    // 测试大的闪烁时间值
    weather.setTimeLightningFlash(100);
    EXPECT_EQ(weather.lightningFlashTime(), 100);
    EXPECT_TRUE(weather.hasLightningFlash());
    EXPECT_FLOAT_EQ(weather.lightningFlashBrightness(), 1.0f);
}

TEST_F(ClientWeatherLightningFlashTest, FlashTimeDoesNotAutoDecay)
{
    // 闪烁时间不应该自动递减，必须显式调用 tickLightningFlash
    weather.setTimeLightningFlash(5);

    // 不调用 tickLightningFlash，时间应该保持不变
    EXPECT_EQ(weather.lightningFlashTime(), 5);

    // 调用其他方法也不应该影响闪烁时间
    weather.setRainStrength(0.5f);
    EXPECT_EQ(weather.lightningFlashTime(), 5);

    weather.isRaining();
    EXPECT_EQ(weather.lightningFlashTime(), 5);

    weather.rainStrength(0.0f);
    EXPECT_EQ(weather.lightningFlashTime(), 5);
}
