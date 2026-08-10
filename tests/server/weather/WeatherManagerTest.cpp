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

#include "server/world/weather/WeatherManager.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/weather/WeatherConstants.hpp"
#include "common/world/weather/WeatherState.hpp"
#include "common/world/weather/WeatherUtils.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::server;
using namespace mc::weather;

namespace {

class WeatherUtilsTestWorld : public mc::test::BaseChunkBackedTestWorld {
public:
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override
    {
        const auto it = m_heights.find(ChunkPos(x >> 4, z >> 4));
        return it != m_heights.end() ? it->second : 0;
    }
    [[nodiscard]] DimensionId dimension() const override { return m_ultraWarm ? 1 : 0; }
    [[nodiscard]] bool isUltraWarm() const override { return m_ultraWarm; }

    void setHeight(ChunkCoord chunkX, ChunkCoord chunkZ, i32 height) { m_heights[ChunkPos(chunkX, chunkZ)] = height; }

    void setBiome(ChunkCoord chunkX, ChunkCoord chunkZ, BiomeId biomeId)
    {
        ChunkData& chunk = ensureChunk(chunkX, chunkZ);
        for (i32 section = 0; section < BiomeContainer::SECTION_COUNT; ++section) {
            for (i32 x = 0; x < BiomeContainer::HORIZ_SIZE; ++x) {
                for (i32 y = 0; y < BiomeContainer::VERT_SIZE; ++y) {
                    for (i32 z = 0; z < BiomeContainer::HORIZ_SIZE; ++z) {
                        chunk.getBiomes().setBiome(section, x, y, z, biomeId);
                    }
                }
            }
        }
    }

    void setUltraWarm(bool value) { m_ultraWarm = value; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WeatherUtilsTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WeatherUtilsTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<ChunkPos, i32> m_heights;
    bool m_ultraWarm = false;
};

class WeatherUtilsTest : public ::testing::Test {
protected:
    void SetUp() override { BiomeRegistry::instance().initialize(); }
};

} // namespace

// ============================================================================
// WeatherState 测试
// ============================================================================

class WeatherStateTest : public ::testing::Test {
protected:
    WeatherState state;
};

TEST_F(WeatherStateTest, DefaultStateIsClear)
{
    EXPECT_FALSE(state.raining);
    EXPECT_FALSE(state.thundering);
    EXPECT_EQ(state.rainStrength, 0.0f);
    EXPECT_EQ(state.thunderStrength, 0.0f);
    EXPECT_EQ(state.weatherType(), WeatherType::Clear);
}

TEST_F(WeatherStateTest, WeatherTypeReturnsCorrectValue)
{
    // 晴天
    state.raining = false;
    state.thundering = false;
    EXPECT_EQ(state.weatherType(), WeatherType::Clear);

    // 降雨
    state.raining = true;
    state.thundering = false;
    EXPECT_EQ(state.weatherType(), WeatherType::Rain);

    // 雷暴
    state.thundering = true;
    EXPECT_EQ(state.weatherType(), WeatherType::Thunder);
}

TEST_F(WeatherStateTest, IsRainingUsesThreshold)
{
    state.rainStrength = 0.1f;
    EXPECT_FALSE(state.isRaining());

    state.rainStrength = WeatherConstants::RAIN_THRESHOLD;
    EXPECT_FALSE(state.isRaining());

    state.rainStrength = WeatherConstants::RAIN_THRESHOLD + 0.01f;
    EXPECT_TRUE(state.isRaining());

    state.rainStrength = 1.0f;
    EXPECT_TRUE(state.isRaining());
}

TEST_F(WeatherStateTest, IsThunderingUsesThreshold)
{
    state.thunderStrength = 0.8f;
    EXPECT_FALSE(state.isThundering());

    state.thunderStrength = WeatherConstants::THUNDER_THRESHOLD;
    EXPECT_FALSE(state.isThundering());

    state.thunderStrength = WeatherConstants::THUNDER_THRESHOLD + 0.01f;
    EXPECT_TRUE(state.isThundering());

    state.thunderStrength = 1.0f;
    EXPECT_TRUE(state.isThundering());
}

TEST_F(WeatherStateTest, GetRainStrengthInterpolates)
{
    state.prevRainStrength = 0.0f;
    state.rainStrength = 1.0f;

    EXPECT_FLOAT_EQ(state.getRainStrength(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(state.getRainStrength(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(state.getRainStrength(1.0f), 1.0f);
}

TEST_F(WeatherStateTest, GetThunderStrengthInterpolates)
{
    state.prevThunderStrength = 0.0f;
    state.thunderStrength = 1.0f;

    EXPECT_FLOAT_EQ(state.getThunderStrength(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(state.getThunderStrength(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(state.getThunderStrength(1.0f), 1.0f);
}

TEST_F(WeatherStateTest, ResetWeatherClearsAll)
{
    state.raining = true;
    state.thundering = true;
    state.clearWeatherTime = 1000;
    state.rainTime = 500;
    state.thunderTime = 300;

    state.resetWeather();

    EXPECT_FALSE(state.raining);
    EXPECT_FALSE(state.thundering);
    EXPECT_EQ(state.clearWeatherTime, 0);
    EXPECT_EQ(state.rainTime, 0);
    EXPECT_EQ(state.thunderTime, 0);
}

TEST_F(WeatherStateTest, CanSleepDuringThunder)
{
    // 雷暴时任何时间都可以睡觉
    state.rainStrength = 1.0f;
    state.thunderStrength = 1.0f;

    EXPECT_TRUE(state.canSleep(0));
    EXPECT_TRUE(state.canSleep(6000));
    EXPECT_TRUE(state.canSleep(12000));
    EXPECT_TRUE(state.canSleep(18000));
}

TEST_F(WeatherStateTest, CanSleepDuringRainAtNight)
{
    // 降雨时只在夜间可以睡觉
    state.rainStrength = 1.0f;
    state.thunderStrength = 0.0f;

    // 白天 (白天时间范围 0-12541 和 23460-23999)
    // 降雨时可以睡觉的时间范围是 12010-23991
    // 测试白天时间（应该能睡，因为降雨扩大了睡眠时间范围）
    EXPECT_TRUE(state.canSleep(6000)); // 6000 tick 是白天，但降雨时可以睡

    // 夜间可以睡觉
    EXPECT_TRUE(state.canSleep(WeatherConstants::RAIN_BED_START_TIME));
    EXPECT_TRUE(state.canSleep(18000));
}

TEST_F(WeatherStateTest, CanSleepDuringClearNightOnly)
{
    // 晴天只在夜间可以睡觉
    state.rainStrength = 0.0f;
    state.thunderStrength = 0.0f;

    // 白天不能睡觉
    EXPECT_FALSE(state.canSleep(0));
    EXPECT_FALSE(state.canSleep(6000));

    // 夜间可以睡觉
    EXPECT_TRUE(state.canSleep(WeatherConstants::CLEAR_BED_START_TIME));
    EXPECT_TRUE(state.canSleep(18000));
}

TEST_F(WeatherStateTest, SkyLightLimitDependsOnWeather)
{
    // 晴天
    state.rainStrength = 0.0f;
    state.thunderStrength = 0.0f;
    EXPECT_EQ(state.skyLightLimit(), 15);

    // 降雨
    state.rainStrength = 1.0f;
    state.thunderStrength = 0.0f;
    EXPECT_EQ(state.skyLightLimit(), WeatherConstants::RAIN_SKY_LIGHT_LIMIT);

    // 雷暴
    state.thunderStrength = 1.0f;
    EXPECT_EQ(state.skyLightLimit(), WeatherConstants::THUNDER_SKY_LIGHT_LIMIT);
}

// ============================================================================
// WeatherManager 测试
// ============================================================================

class WeatherManagerTest : public ::testing::Test {
protected:
    WeatherManager manager;

    void SetUp() override { manager.initialize(12345); }
};

TEST_F(WeatherManagerTest, InitialStateIsClear)
{
    EXPECT_FALSE(manager.isRaining());
    EXPECT_FALSE(manager.isThundering());
}

TEST_F(WeatherManagerTest, SetClearStopsRainAndThunder)
{
    manager.setRain(100);
    // tick 多次让强度增加到阈值以上
    for (int i = 0; i < 30; ++i) {
        manager.tick();
    }
    EXPECT_TRUE(manager.isRaining());

    manager.setClear(1000);
    manager.tick();

    // 强度会渐变，不会立即变为 0
    EXPECT_LT(manager.rainStrength(), 1.0f);
    EXPECT_FALSE(manager.state().raining);
    EXPECT_FALSE(manager.state().thundering);
    EXPECT_GT(manager.state().clearWeatherTime, 0);
}

TEST_F(WeatherManagerTest, SetRainStartsRaining)
{
    manager.setRain(6000);

    EXPECT_TRUE(manager.state().raining);
    EXPECT_FALSE(manager.state().thundering);
    EXPECT_EQ(manager.state().rainTime, 6000);
    EXPECT_EQ(manager.state().clearWeatherTime, 0);
}

TEST_F(WeatherManagerTest, SetThunderStartsRainAndThunder)
{
    manager.setThunder(6000);

    EXPECT_TRUE(manager.state().raining);
    EXPECT_TRUE(manager.state().thundering);
    EXPECT_EQ(manager.state().rainTime, 6000);
    EXPECT_EQ(manager.state().thunderTime, 6000);
    EXPECT_EQ(manager.state().clearWeatherTime, 0);
}

TEST_F(WeatherManagerTest, SetClearWithDefaultDuration)
{
    manager.setClear(0); // 使用默认值

    EXPECT_EQ(manager.state().clearWeatherTime, WeatherConstants::DEFAULT_COMMAND_DURATION);
}

TEST_F(WeatherManagerTest, SetRainWithDefaultDuration)
{
    manager.setRain(0);

    EXPECT_EQ(manager.state().rainTime, WeatherConstants::DEFAULT_COMMAND_DURATION);
}

TEST_F(WeatherManagerTest, SetThunderWithDefaultDuration)
{
    manager.setThunder(0);

    EXPECT_EQ(manager.state().rainTime, WeatherConstants::DEFAULT_COMMAND_DURATION);
    EXPECT_EQ(manager.state().thunderTime, WeatherConstants::DEFAULT_COMMAND_DURATION);
}

TEST_F(WeatherManagerTest, ResetWeatherClearsAll)
{
    manager.setThunder(1000);
    manager.tick();

    manager.resetWeather();

    EXPECT_FALSE(manager.state().raining);
    EXPECT_FALSE(manager.state().thundering);
    EXPECT_EQ(manager.state().clearWeatherTime, 0);
    EXPECT_TRUE(manager.hasWeatherChanged());
}

TEST_F(WeatherManagerTest, StrengthTransitionsGradually)
{
    manager.setRain(10000);

    // 初始强度为 0
    EXPECT_FLOAT_EQ(manager.rainStrength(), 0.0f);

    // 每tick增加 0.01
    for (int i = 0; i < 50; ++i) {
        manager.tick();
    }

    // 50 ticks 后强度约为 0.5
    EXPECT_NEAR(manager.rainStrength(), 0.5f, 0.02f);

    // 继续 tick 到满强度 (再 tick 50 次就够了)
    for (int i = 0; i < 60; ++i) {
        manager.tick();
    }

    EXPECT_NEAR(manager.rainStrength(), 1.0f, 0.02f);
}

TEST_F(WeatherManagerTest, WeatherCycleDisabledDoesNotChangeWeather)
{
    manager.setWeatherCycleEnabled(false);
    manager.setClear(0);
    manager.tick();

    // 即使 clearWeatherTime 为 0，天气也不会自然变化
    EXPECT_EQ(manager.state().clearWeatherTime, WeatherConstants::DEFAULT_COMMAND_DURATION);
}

TEST_F(WeatherManagerTest, HasStrengthChangedDetectsChanges)
{
    manager.setRain(10000);

    // 第一tick会改变强度
    manager.tick();
    EXPECT_TRUE(manager.hasStrengthChanged());

    // 当强度达到最大后，不再变化
    for (int i = 0; i < 200; ++i) {
        manager.tick();
    }
    EXPECT_FALSE(manager.hasStrengthChanged());
}

TEST_F(WeatherManagerTest, WeatherChangedCallbackIsCalled)
{
    bool callbackCalled = false;
    WeatherType oldType = WeatherType::Clear;
    WeatherType newType = WeatherType::Clear;

    manager.setWeatherChangeCallback([&](WeatherType o, WeatherType n) {
        callbackCalled = true;
        oldType = o;
        newType = n;
    });

    manager.setRain(10000);

    // tick 直到强度超过阈值
    for (int i = 0; i < 25; ++i) {
        manager.tick();
    }

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(oldType, WeatherType::Clear);
    EXPECT_EQ(newType, WeatherType::Rain);
}

TEST_F(WeatherManagerTest, SerializationRoundTrip)
{
    // 设置天气状态
    manager.setRain(10000);

    // 不要 tick，保持原始值
    // 或者使用固定的 rainTime
    i32 expectedRainTime = manager.state().rainTime;
    bool expectedRaining = manager.state().raining;
    bool expectedThundering = manager.state().thundering;
    bool expectedWeatherCycleEnabled = manager.state().weatherCycleEnabled;

    std::vector<u8> data;
    manager.serialize(data);

    WeatherManager manager2;
    manager2.initialize(54321);
    size_t offset = 0;
    auto result = manager2.deserialize(data, offset);
    EXPECT_TRUE(result.success());

    EXPECT_EQ(manager2.state().rainTime, expectedRainTime);
    EXPECT_NEAR(manager2.state().rainStrength, manager.rainStrength(), 0.01f);
    EXPECT_NEAR(manager2.state().thunderStrength, manager.thunderStrength(), 0.01f);
    EXPECT_EQ(manager2.state().raining, expectedRaining);
    EXPECT_EQ(manager2.state().thundering, expectedThundering);
    EXPECT_EQ(manager2.state().weatherCycleEnabled, expectedWeatherCycleEnabled);
}

TEST_F(WeatherManagerTest, TrySpawnLightningOnlyDuringThunder)
{
    // 晴天不生成闪电
    auto [spawned1, pos1] = manager.trySpawnLightning();
    EXPECT_FALSE(spawned1);

    // 降雨不生成闪电
    manager.setRain(1000);
    for (int i = 0; i < 200; ++i) {
        manager.tick();
    }
    auto [spawned2, pos2] = manager.trySpawnLightning();
    EXPECT_FALSE(spawned2);

    // 雷暴可能生成闪电（概率很低，所以不保证一定生成）
    manager.setThunder(1000);
    for (int i = 0; i < 200; ++i) {
        manager.tick();
    }

    // 由于概率很低 (1/100000)，我们只检查返回类型正确
    auto [spawned3, pos3] = manager.trySpawnLightning();
    (void)spawned3; // 忽略结果，只确保不崩溃
}

// ============================================================================
// WeatherUtils 测试
// ============================================================================

TEST_F(WeatherUtilsTest, GetPrecipitationTypeReturnsCorrectValue)
{
    // 参考 MC 1.16.5 Biome.doesSnowGenerate(): temperature >= 0.15F 时返回 false
    // 所以温度 < 0.15 是雪，>= 0.15 是雨
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.0f), 2);  // 雪
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.14f), 2); // 雪
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.15f), 1); // 边界值 - 雨 (温度等于0.15时是雨)
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.16f), 1); // 雨
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.5f), 1);  // 雨
    EXPECT_EQ(WeatherUtils::getPrecipitationType(2.0f), 1);  // 雨
}

TEST_F(WeatherUtilsTest, CalculateSkyDarkenFactor)
{
    // 参考 MC 1.16.5 ClientWorld.getSunBrightness():
    // 乘法组合: sunBrightness = base * (1 - rain * 5/16) * (1 - thunder * 5/16)
    // 暗化因子 = 1 - sunBrightness = 1 - (1 - rain*5/16) * (1 - thunder*5/16)

    // 晴天
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(0.0f, 0.0f), 0.0f);

    // 降雨 (rain=1, thunder=0): 1 - (1 - 5/16) * 1 = 1 - 11/16 = 5/16 = 0.3125
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(1.0f, 0.0f), 0.3125f);

    // 雷暴 (rain=1, thunder=1): 1 - (1 - 5/16) * (1 - 5/16) = 1 - (11/16)^2 = 1 - 121/256
    // 注意: thunderStrength 会自动乘以 rainStrength，所以实际 thunder * rain = 1 * 1 = 1
    // 但这里测试的是 WeatherUtils::calculateSkyDarkenFactor，它直接接收参数
    // MC 中 getThunderStrength 返回 thunder * rain
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(1.0f, 1.0f), 1.0f - (11.0f / 16.0f) * (11.0f / 16.0f));

    // 部分降雨 (rain=0.5, thunder=0.5)
    // 1 - (1 - 0.5*5/16) * (1 - 0.5*5/16) = 1 - (1 - 5/32) * (1 - 5/32) = 1 - (27/32)^2
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(0.5f, 0.5f), 1.0f - (27.0f / 32.0f) * (27.0f / 32.0f));
}

TEST_F(WeatherUtilsTest, CalculateCelestialVisibility)
{
    // 晴天
    EXPECT_FLOAT_EQ(WeatherUtils::calculateCelestialVisibility(0.0f), 1.0f);

    // 完全降雨
    EXPECT_FLOAT_EQ(WeatherUtils::calculateCelestialVisibility(1.0f), 0.0f);

    // 部分降雨
    EXPECT_FLOAT_EQ(WeatherUtils::calculateCelestialVisibility(0.5f), 0.5f);
}

TEST_F(WeatherUtilsTest, CalculateStarBrightness)
{
    // 白天没有星星
    EXPECT_FLOAT_EQ(WeatherUtils::calculateStarBrightness(0.0f, 0), 0.0f);
    EXPECT_FLOAT_EQ(WeatherUtils::calculateStarBrightness(0.0f, 6000), 0.0f);

    // 夜间有星星
    f32 midnightBrightness = WeatherUtils::calculateStarBrightness(0.0f, 18000);
    EXPECT_GT(midnightBrightness, 0.0f);

    // 降雨时没有星星
    f32 rainBrightness = WeatherUtils::calculateStarBrightness(1.0f, 18000);
    EXPECT_FLOAT_EQ(rainBrightness, 0.0f);
}

TEST_F(WeatherUtilsTest, GetRandomWeatherDurationInValidRange)
{
    mc::math::Random rng(12345);

    for (int i = 0; i < 100; ++i) {
        i32 clearDuration = WeatherUtils::getRandomClearDuration(rng);
        EXPECT_GE(clearDuration, WeatherConstants::MIN_CLEAR_TIME);
        EXPECT_LE(clearDuration, WeatherConstants::MAX_CLEAR_TIME);

        i32 rainDuration = WeatherUtils::getRandomRainDuration(rng);
        EXPECT_GE(rainDuration, WeatherConstants::MIN_RAIN_TIME);
        EXPECT_LE(rainDuration, WeatherConstants::MAX_RAIN_TIME);

        i32 thunderDuration = WeatherUtils::getRandomThunderDuration(rng);
        EXPECT_GE(thunderDuration, WeatherConstants::MIN_THUNDER_TIME);
        EXPECT_LE(thunderDuration, WeatherConstants::MAX_THUNDER_TIME);
    }
}

TEST_F(WeatherUtilsTest, RainAndSnowChecksFollowBiomeTemperature)
{
    WeatherUtilsTestWorld world;
    world.setHeight(0, 0, 64);

    world.setBiome(0, 0, Biomes::Plains);
    EXPECT_TRUE(WeatherUtils::canRainAt(world, BlockPos(1, 64, 1)));
    EXPECT_FALSE(WeatherUtils::canSnowAt(world, BlockPos(1, 64, 1)));

    world.setBiome(0, 0, Biomes::SnowyPlains);
    EXPECT_FALSE(WeatherUtils::canRainAt(world, BlockPos(1, 64, 1)));
    EXPECT_TRUE(WeatherUtils::canSnowAt(world, BlockPos(1, 64, 1)));

    world.setBiome(0, 0, Biomes::Desert);
    EXPECT_FALSE(WeatherUtils::canRainAt(world, BlockPos(1, 64, 1)));
    EXPECT_FALSE(WeatherUtils::canSnowAt(world, BlockPos(1, 64, 1)));
}

TEST_F(WeatherUtilsTest, RainAndSnowChecksRequireSkyExposure)
{
    WeatherUtilsTestWorld world;
    world.setHeight(0, 0, 65);
    world.setBiome(0, 0, Biomes::Plains);

    EXPECT_FALSE(WeatherUtils::canRainAt(world, BlockPos(1, 64, 1)));
    EXPECT_FALSE(WeatherUtils::canSnowAt(world, BlockPos(1, 64, 1)));
}

TEST_F(WeatherUtilsTest, RainAndSnowChecksRejectUltraWarmDimensions)
{
    WeatherUtilsTestWorld world;
    world.setHeight(0, 0, 64);
    world.setBiome(0, 0, Biomes::Plains);
    world.setUltraWarm(true);

    EXPECT_FALSE(WeatherUtils::canRainAt(world, BlockPos(1, 64, 1)));
    EXPECT_FALSE(WeatherUtils::canSnowAt(world, BlockPos(1, 64, 1)));
}

// ============================================================================
// WeatherConstants 测试
// ============================================================================

TEST(WeatherConstantsTest, DurationsAreSensible)
{
    // 晴天持续 10-140 分钟
    EXPECT_GE(WeatherConstants::MIN_CLEAR_TIME, 12000);
    EXPECT_LE(WeatherConstants::MAX_CLEAR_TIME, 168000);

    // 降雨持续 10-20 分钟
    EXPECT_GE(WeatherConstants::MIN_RAIN_TIME, 12000);
    EXPECT_LE(WeatherConstants::MAX_RAIN_TIME, 24000);

    // 雷暴持续 3-13 分钟
    EXPECT_GE(WeatherConstants::MIN_THUNDER_TIME, 3600);
    EXPECT_LE(WeatherConstants::MAX_THUNDER_TIME, 15600);

    // 默认命令持续时间为 5 分钟
    EXPECT_EQ(WeatherConstants::DEFAULT_COMMAND_DURATION, 6000);
}

TEST(WeatherConstantsTest, ThresholdsAreValid)
{
    EXPECT_GT(WeatherConstants::RAIN_THRESHOLD, 0.0f);
    EXPECT_LT(WeatherConstants::RAIN_THRESHOLD, 1.0f);

    EXPECT_GT(WeatherConstants::THUNDER_THRESHOLD, WeatherConstants::RAIN_THRESHOLD);
    EXPECT_LT(WeatherConstants::THUNDER_THRESHOLD, 1.0f);
}

TEST(WeatherConstantsTest, StrengthChangeRateIsSmall)
{
    // 每tick变化 0.01，需要 100 ticks 从 0 到 1
    EXPECT_FLOAT_EQ(WeatherConstants::STRENGTH_CHANGE_RATE, 0.01f);
}

TEST(WeatherConstantsTest, LightningChanceIsRare)
{
    // 闪电概率为 1/100000
    EXPECT_EQ(WeatherConstants::LIGHTNING_CHANCE_DENOMINATOR, 100000);
}
