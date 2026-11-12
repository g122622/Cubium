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
#include "common/world/block/base/types/BlockPos.hpp"
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

    [[nodiscard]] DimensionId dimension() const override
    {
        return m_ultraWarm ? 1 : 0;
    }

    [[nodiscard]] bool isUltraWarm() const override
    {
        return m_ultraWarm;
    }

    void setHeight(ChunkCoord chunkX, ChunkCoord chunkZ, i32 height)
    {
        m_heights[ChunkPos(chunkX, chunkZ)] = height;
    }

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

    void setUltraWarm(bool value)
    {
        m_ultraWarm = value;
    }

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
    void SetUp() override
    {
        BiomeRegistry::instance().initialize();
    }
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
    state.raining = false;
    state.thundering = false;
    EXPECT_EQ(state.weatherType(), WeatherType::Clear);

    state.raining = true;
    state.thundering = false;
    EXPECT_EQ(state.weatherType(), WeatherType::Rain);

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
    // 雷暴判定使用"雷暴强度 × 降雨强度"。
    state.rainStrength = 1.0f;

    state.thunderStrength = 0.8f;
    EXPECT_FALSE(state.isThundering());

    state.thunderStrength = WeatherConstants::THUNDER_THRESHOLD;
    EXPECT_FALSE(state.isThundering());

    state.thunderStrength = WeatherConstants::THUNDER_THRESHOLD + 0.01f;
    EXPECT_TRUE(state.isThundering());

    state.thunderStrength = 1.0f;
    EXPECT_TRUE(state.isThundering());
}

TEST_F(WeatherStateTest, IsThunderingRequiresRain)
{
    state.rainStrength = 0.0f;
    state.thunderStrength = 1.0f;

    EXPECT_FALSE(state.isThundering());
}

TEST_F(WeatherStateTest, GetRainStrengthInterpolates)
{
    state.prevRainStrength = 0.0f;
    state.rainStrength = 1.0f;

    EXPECT_FLOAT_EQ(state.getRainStrength(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(state.getRainStrength(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(state.getRainStrength(1.0f), 1.0f);
}

TEST_F(WeatherStateTest, GetRawThunderStrengthInterpolates)
{
    state.prevThunderStrength = 0.0f;
    state.thunderStrength = 1.0f;

    EXPECT_FLOAT_EQ(state.getRawThunderStrength(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(state.getRawThunderStrength(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(state.getRawThunderStrength(1.0f), 1.0f);
}

TEST_F(WeatherStateTest, GetThunderStrengthMultipliesRain)
{
    state.prevRainStrength = 1.0f;
    state.rainStrength = 1.0f;
    state.prevThunderStrength = 0.0f;
    state.thunderStrength = 1.0f;

    EXPECT_FLOAT_EQ(state.getThunderStrength(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(state.getThunderStrength(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(state.getThunderStrength(1.0f), 1.0f);

    state.rainStrength = 0.5f;
    state.prevRainStrength = 0.5f;
    EXPECT_FLOAT_EQ(state.getThunderStrength(1.0f), 0.5f);
}

TEST_F(WeatherStateTest, ResetWeatherClearsTimersAndFlags)
{
    state.raining = true;
    state.thundering = true;
    state.clearWeatherTime = 1000;
    state.rainTime = 500;
    state.thunderTime = 300;

    state.resetWeather();

    EXPECT_FALSE(state.raining);
    EXPECT_FALSE(state.thundering);
    // 原版 resetWeatherCycle 不清除 clearWeatherTime。
    EXPECT_EQ(state.clearWeatherTime, 1000);
    EXPECT_EQ(state.rainTime, 0);
    EXPECT_EQ(state.thunderTime, 0);
}

// ============================================================================
// WeatherManager 测试
// ============================================================================

class WeatherManagerTest : public ::testing::Test {
protected:
    WeatherManager manager;

    void SetUp() override
    {
        manager.initialize(12345, true);
    }
};

TEST_F(WeatherManagerTest, InitialStateIsClearWithZeroTimers)
{
    EXPECT_FALSE(manager.isRaining());
    EXPECT_FALSE(manager.isThundering());
    EXPECT_EQ(manager.state().clearWeatherTime, 0);
    EXPECT_EQ(manager.state().rainTime, 0);
    EXPECT_EQ(manager.state().thunderTime, 0);
}

TEST_F(WeatherManagerTest, FirstTickSamplesDelayRanges)
{
    manager.tick();

    EXPECT_GE(manager.state().rainTime, WeatherConstants::RAIN_DELAY_MIN);
    EXPECT_LE(manager.state().rainTime, WeatherConstants::RAIN_DELAY_MAX);
    EXPECT_GE(manager.state().thunderTime, WeatherConstants::THUNDER_DELAY_MIN);
    EXPECT_LE(manager.state().thunderTime, WeatherConstants::THUNDER_DELAY_MAX);
}

TEST_F(WeatherManagerTest, SetClearStopsRainAndThunder)
{
    manager.setRain(100);
    for (int i = 0; i < 30; ++i) {
        manager.tick();
    }
    EXPECT_TRUE(manager.isRaining());

    manager.setClear(1000);
    manager.tick();

    // 强度会渐变，不会立即变为 0
    EXPECT_LT(manager.rainStrength(1.0f), 1.0f);
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
    EXPECT_EQ(manager.state().thunderTime, 6000);
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

TEST_F(WeatherManagerTest, SetClearResetsWeatherTimersToZero)
{
    manager.setRain(6000);
    manager.setClear(6000);

    EXPECT_EQ(manager.state().clearWeatherTime, 6000);
    EXPECT_EQ(manager.state().rainTime, 0);
    EXPECT_EQ(manager.state().thunderTime, 0);
}

TEST_F(WeatherManagerTest, SetClearWithDefaultDurationUsesRainDelay)
{
    const i32 applied = manager.setClear(0);

    EXPECT_EQ(applied, manager.state().clearWeatherTime);
    EXPECT_GE(applied, WeatherConstants::RAIN_DELAY_MIN);
    EXPECT_LE(applied, WeatherConstants::RAIN_DELAY_MAX);
}

TEST_F(WeatherManagerTest, SetRainWithDefaultDurationUsesRainDuration)
{
    const i32 applied = manager.setRain(0);

    EXPECT_EQ(applied, manager.state().rainTime);
    EXPECT_EQ(applied, manager.state().thunderTime);
    EXPECT_GE(applied, WeatherConstants::RAIN_DURATION_MIN);
    EXPECT_LE(applied, WeatherConstants::RAIN_DURATION_MAX);
}

TEST_F(WeatherManagerTest, SetThunderWithDefaultDurationUsesThunderDuration)
{
    const i32 applied = manager.setThunder(0);

    EXPECT_EQ(applied, manager.state().rainTime);
    EXPECT_EQ(applied, manager.state().thunderTime);
    EXPECT_GE(applied, WeatherConstants::THUNDER_DURATION_MIN);
    EXPECT_LE(applied, WeatherConstants::THUNDER_DURATION_MAX);
}

TEST_F(WeatherManagerTest, ResetWeatherClearsCycle)
{
    manager.setThunder(1000);
    manager.tick();

    manager.resetWeather();

    EXPECT_FALSE(manager.state().raining);
    EXPECT_FALSE(manager.state().thundering);
    EXPECT_EQ(manager.state().rainTime, 0);
    EXPECT_EQ(manager.state().thunderTime, 0);
    EXPECT_TRUE(manager.hasWeatherChanged());
}

TEST_F(WeatherManagerTest, StrengthTransitionsGradually)
{
    manager.setRain(10000);

    EXPECT_FLOAT_EQ(manager.rainStrength(1.0f), 0.0f);

    for (int i = 0; i < 50; ++i) {
        manager.tick();
    }
    EXPECT_NEAR(manager.rainStrength(1.0f), 0.5f, 0.02f);

    for (int i = 0; i < 60; ++i) {
        manager.tick();
    }
    EXPECT_NEAR(manager.rainStrength(1.0f), 1.0f, 0.02f);
}

TEST_F(WeatherManagerTest, WeatherCycleDisabledDoesNotChangeWeather)
{
    manager.setWeatherCycleEnabled(false);
    const i32 applied = manager.setClear(0);
    manager.tick();

    // 即使计时器会被外部修改，天气周期也不会自然推进。
    EXPECT_EQ(manager.state().clearWeatherTime, applied);
}

TEST_F(WeatherManagerTest, HasStrengthChangedDetectsChanges)
{
    manager.setRain(10000);

    manager.tick();
    EXPECT_TRUE(manager.hasStrengthChanged());

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
    for (int i = 0; i < 25; ++i) {
        manager.tick();
    }

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(oldType, WeatherType::Clear);
    EXPECT_EQ(newType, WeatherType::Rain);
}

TEST_F(WeatherManagerTest, RestoreStateRecoversStrengthFromSavedFlags)
{
    manager.restoreState(100, 200, true, 300, true);

    EXPECT_EQ(manager.state().clearWeatherTime, 100);
    EXPECT_EQ(manager.state().rainTime, 200);
    EXPECT_EQ(manager.state().thunderTime, 300);
    EXPECT_TRUE(manager.state().raining);
    EXPECT_TRUE(manager.state().thundering);
    EXPECT_FLOAT_EQ(manager.state().rainStrength, 1.0f);
    EXPECT_FLOAT_EQ(manager.state().thunderStrength, 1.0f);
    // 存档恢复不应把已有天气误报为本 tick 的变化。
    EXPECT_FALSE(manager.hasWeatherChanged());
}

TEST_F(WeatherManagerTest, CanHaveWeatherGatesAllQueries)
{
    WeatherManager dryManager;
    dryManager.initialize(777, false);
    dryManager.setRain(1000);
    for (int i = 0; i < 200; ++i) {
        dryManager.tick();
    }

    EXPECT_FALSE(dryManager.isRaining());
    EXPECT_FALSE(dryManager.isThundering());
    EXPECT_EQ(dryManager.weatherType(), WeatherType::Clear);
    // 无天气能力的维度不渐变强度。
    EXPECT_FLOAT_EQ(dryManager.rainStrength(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(dryManager.rawThunderStrength(1.0f), 0.0f);
}

// ============================================================================
// WeatherUtils 测试
// ============================================================================

TEST_F(WeatherUtilsTest, GetPrecipitationTypeReturnsCorrectValue)
{
    // 温度 < 0.15 是雪，>= 0.15 是雨
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.0f), 2);  // 雪
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.14f), 2); // 雪
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.15f), 1); // 边界值 - 雨
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.16f), 1); // 雨
    EXPECT_EQ(WeatherUtils::getPrecipitationType(0.5f), 1);  // 雨
    EXPECT_EQ(WeatherUtils::getPrecipitationType(2.0f), 1);  // 雨
}

TEST_F(WeatherUtilsTest, CalculateSkyDarkenFactor)
{
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(0.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(1.0f, 0.0f), 0.3125f);
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(1.0f, 1.0f), 1.0f - (11.0f / 16.0f) * (11.0f / 16.0f));
    EXPECT_FLOAT_EQ(WeatherUtils::calculateSkyDarkenFactor(0.5f, 0.5f), 1.0f - (27.0f / 32.0f) * (27.0f / 32.0f));
}

TEST_F(WeatherUtilsTest, CalculateCelestialVisibility)
{
    EXPECT_FLOAT_EQ(WeatherUtils::calculateCelestialVisibility(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(WeatherUtils::calculateCelestialVisibility(1.0f), 0.0f);
    EXPECT_FLOAT_EQ(WeatherUtils::calculateCelestialVisibility(0.5f), 0.5f);
}

TEST_F(WeatherUtilsTest, CalculateStarBrightness)
{
    EXPECT_FLOAT_EQ(WeatherUtils::calculateStarBrightness(0.0f, 0), 0.0f);
    EXPECT_FLOAT_EQ(WeatherUtils::calculateStarBrightness(0.0f, 6000), 0.0f);

    f32 midnightBrightness = WeatherUtils::calculateStarBrightness(0.0f, 18000);
    EXPECT_GT(midnightBrightness, 0.0f);

    f32 rainBrightness = WeatherUtils::calculateStarBrightness(1.0f, 18000);
    EXPECT_FLOAT_EQ(rainBrightness, 0.0f);
}

TEST_F(WeatherUtilsTest, GetRandomWeatherDurationUsesInclusiveRange)
{
    mc::math::Random rng(12345);

    // 用小范围确认闭区间语义：5 和 6 都必须能取到。
    // 直接对 12 万级的真实区间测端点命中率会引入 flaky。
    bool sawMin = false;
    bool sawMax = false;
    for (int i = 0; i < 200; ++i) {
        const i32 value = WeatherUtils::getRandomWeatherDuration(rng, 5, 6);
        EXPECT_GE(value, 5);
        EXPECT_LE(value, 6);
        sawMin = sawMin || value == 5;
        sawMax = sawMax || value == 6;
    }
    EXPECT_TRUE(sawMin);
    EXPECT_TRUE(sawMax);

    // 各封装函数的采样结果必须落在对应区间内。
    for (int i = 0; i < 200; ++i) {
        const i32 rainDelay = WeatherUtils::getRandomRainDelay(rng);
        EXPECT_GE(rainDelay, WeatherConstants::RAIN_DELAY_MIN);
        EXPECT_LE(rainDelay, WeatherConstants::RAIN_DELAY_MAX);

        const i32 rainDuration = WeatherUtils::getRandomRainDuration(rng);
        EXPECT_GE(rainDuration, WeatherConstants::RAIN_DURATION_MIN);
        EXPECT_LE(rainDuration, WeatherConstants::RAIN_DURATION_MAX);

        const i32 thunderDelay = WeatherUtils::getRandomThunderDelay(rng);
        EXPECT_GE(thunderDelay, WeatherConstants::THUNDER_DELAY_MIN);
        EXPECT_LE(thunderDelay, WeatherConstants::THUNDER_DELAY_MAX);

        const i32 thunderDuration = WeatherUtils::getRandomThunderDuration(rng);
        EXPECT_GE(thunderDuration, WeatherConstants::THUNDER_DURATION_MIN);
        EXPECT_LE(thunderDuration, WeatherConstants::THUNDER_DURATION_MAX);
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

TEST(WeatherConstantsTest, CycleRangesMatchVanilla)
{
    // 四组区间与原版 ServerLevel.RAIN_DELAY / RAIN_DURATION /
    // THUNDER_DELAY / THUNDER_DURATION 完全一致。
    EXPECT_EQ(WeatherConstants::RAIN_DELAY_MIN, 12000);
    EXPECT_EQ(WeatherConstants::RAIN_DELAY_MAX, 180000);
    EXPECT_EQ(WeatherConstants::RAIN_DURATION_MIN, 12000);
    EXPECT_EQ(WeatherConstants::RAIN_DURATION_MAX, 24000);
    EXPECT_EQ(WeatherConstants::THUNDER_DELAY_MIN, 12000);
    EXPECT_EQ(WeatherConstants::THUNDER_DELAY_MAX, 180000);
    EXPECT_EQ(WeatherConstants::THUNDER_DURATION_MIN, 3600);
    EXPECT_EQ(WeatherConstants::THUNDER_DURATION_MAX, 15600);
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
    EXPECT_FLOAT_EQ(WeatherConstants::STRENGTH_CHANGE_RATE, 0.01f);
}

TEST(WeatherConstantsTest, LightningChanceIsRare)
{
    EXPECT_EQ(WeatherConstants::LIGHTNING_CHANCE_DENOMINATOR, 100000);
}
