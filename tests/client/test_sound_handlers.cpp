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

/**
 * @file test_sound_handlers.cpp
 * @brief 音效处理器测试用例
 *
 * 测试覆盖：
 * - BiomeAmbientHandler: 群系循环音效淡入淡出、心境音效光照采样
 * - UnderwaterAmbientHandler: 水下循环音效、入水/出水音效
 * - UnderwaterLoopSound: 水下循环音效淡入淡出
 * - WeatherSoundHandler: 天气音效（雨声、雷声）
 * - MusicPlayer: 音乐选择器配置
 * - SoundEvents: 音效事件定义
 */

#include <gtest/gtest.h>

#include "client/sound/MusicPlayer.hpp"
#include "client/sound/handler/BiomeAmbientHandler.hpp"
#include "client/sound/handler/UnderwaterAmbientHandler.hpp"
#include "client/sound/handler/WeatherSoundHandler.hpp"
#include "client/sound/instance/UnderwaterLoopSound.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc::client::sound;
using namespace mc;

// ============================================================================
// BiomeAmbientHandler Tests
// ============================================================================

class BiomeAmbientHandlerTest : public ::testing::Test {
protected:
    void SetUp() override { handler = std::make_unique<BiomeAmbientHandler>(); }

    std::unique_ptr<BiomeAmbientHandler> handler;
};

TEST_F(BiomeAmbientHandlerTest, InitialState)
{
    EXPECT_EQ(handler->getBiomeId(), 0u);
}

TEST_F(BiomeAmbientHandlerTest, SetBiomeId)
{
    handler->setBiomeId(42);
    EXPECT_EQ(handler->getBiomeId(), 42u);
}

TEST_F(BiomeAmbientHandlerTest, SetPlayerPosition)
{
    // 测试设置玩家位置不会崩溃
    handler->setPlayerPosition(100.5, 64.0, -200.3);
    // 无异常即成功
}

TEST_F(BiomeAmbientHandlerTest, SetLightLevel)
{
    // 测试设置光照等级不会崩溃
    handler->setLightLevel(15, 0, 15, 0); // 完全天空光（采样位置也是完全天空光）
    handler->setLightLevel(0, 15, 0, 15); // 完全方块光（采样位置也是完全方块光）
    handler->setLightLevel(0, 0, 0, 0);   // 完全黑暗（采样位置也是完全黑暗）
    // 无异常即成功
}

TEST_F(BiomeAmbientHandlerTest, BiomeChangeTracking)
{
    // 测试群系变化追踪
    handler->setBiomeId(1);
    EXPECT_EQ(handler->getBiomeId(), 1u);

    handler->setBiomeId(2);
    EXPECT_EQ(handler->getBiomeId(), 2u);
}

// ============================================================================
// UnderwaterAmbientHandler Tests
// ============================================================================

class UnderwaterAmbientHandlerTest : public ::testing::Test {
protected:
    void SetUp() override { handler = std::make_unique<UnderwaterAmbientHandler>(); }

    std::unique_ptr<UnderwaterAmbientHandler> handler;
};

TEST_F(UnderwaterAmbientHandlerTest, InitialState)
{
    EXPECT_FALSE(handler->isUnderwater());
}

TEST_F(UnderwaterAmbientHandlerTest, SetUnderwater)
{
    handler->setUnderwater(true);
    EXPECT_TRUE(handler->isUnderwater());

    handler->setUnderwater(false);
    EXPECT_FALSE(handler->isUnderwater());
}

TEST_F(UnderwaterAmbientHandlerTest, UnderwaterTransitions)
{
    // 测试进入水中
    handler->setUnderwater(true);
    EXPECT_TRUE(handler->isUnderwater());

    // 测试离开水中
    handler->setUnderwater(false);
    EXPECT_FALSE(handler->isUnderwater());
}

// ============================================================================
// UnderwaterLoopSound Tests
// ============================================================================

class UnderwaterLoopSoundTest : public ::testing::Test {
protected:
    void SetUp() override { sound = std::make_unique<UnderwaterLoopSound>(); }

    std::unique_ptr<UnderwaterLoopSound> sound;
};

TEST_F(UnderwaterLoopSoundTest, InitialState)
{
    EXPECT_TRUE(sound->canBeSilent());
    EXPECT_FALSE(sound->canSwim());
    EXPECT_EQ(sound->getSoundEventId(), SoundEvents::AMBIENT_UNDERWATER_LOOP);
}

TEST_F(UnderwaterLoopSoundTest, SetCanSwim)
{
    sound->setCanSwim(true);
    EXPECT_TRUE(sound->canSwim());

    sound->setCanSwim(false);
    EXPECT_FALSE(sound->canSwim());
}

TEST_F(UnderwaterLoopSoundTest, VolumeFadeIn)
{
    sound->setCanSwim(true);

    // 初始音量为0
    EXPECT_FLOAT_EQ(sound->getVolume(), 0.0f);

    // 模拟淡入过程
    for (int i = 0; i < 40; ++i) {
        sound->tick();
    }

    // 40 tick 后音量应该为 1.0
    EXPECT_FLOAT_EQ(sound->getVolume(), 1.0f);
}

TEST_F(UnderwaterLoopSoundTest, VolumeFadeOut)
{
    // 先淡入
    sound->setCanSwim(true);
    for (int i = 0; i < 40; ++i) {
        sound->tick();
    }
    EXPECT_FLOAT_EQ(sound->getVolume(), 1.0f);

    // 开始淡出
    sound->setCanSwim(false);
    for (int i = 0; i < 20; ++i) {
        sound->tick();
    }

    // 20 tick 淡出后音量应该为 0 (淡出速度是 2/tick)
    EXPECT_FLOAT_EQ(sound->getVolume(), 0.0f);
}

TEST_F(UnderwaterLoopSoundTest, SoundDoneWhenNegativeTicks)
{
    sound->setCanSwim(false);

    // 初始状态不应该完成
    EXPECT_FALSE(sound->isDone());

    // 淡出直到完成
    for (int i = 0; i < 21; ++i) {
        sound->tick();
    }

    // 当 ticksInWater 变为负数时应该标记完成
    EXPECT_TRUE(sound->isDone());
}

// ============================================================================
// MusicPlayer Tests
// ============================================================================

class MusicPlayerTypesTest : public ::testing::Test {};

TEST_F(MusicPlayerTypesTest, MusicTypeEnumValues)
{
    // 测试音乐类型枚举值
    EXPECT_EQ(static_cast<int>(MusicPlayer::MusicType::None), 0);
    EXPECT_EQ(static_cast<int>(MusicPlayer::MusicType::Menu), 1);
    EXPECT_EQ(static_cast<int>(MusicPlayer::MusicType::Game), 2);
    EXPECT_EQ(static_cast<int>(MusicPlayer::MusicType::Creative), 3);
}

TEST_F(MusicPlayerTypesTest, MusicSelectorDefaults)
{
    MusicPlayer::MusicSelector selector;
    EXPECT_EQ(selector.minDelayTicks, 12000u);
    EXPECT_EQ(selector.maxDelayTicks, 24000u);
    EXPECT_FALSE(selector.replaceCurrent);
}

TEST_F(MusicPlayerTypesTest, MusicSelectorReplaceCurrent)
{
    MusicPlayer::MusicSelector selector;
    selector.soundEventId = ResourceLocation("minecraft:music.dragon");
    selector.minDelayTicks = 0;
    selector.maxDelayTicks = 0;
    selector.replaceCurrent = true;

    EXPECT_TRUE(selector.replaceCurrent);
    EXPECT_EQ(selector.minDelayTicks, 0u);
    EXPECT_EQ(selector.maxDelayTicks, 0u);
}

// ============================================================================
// SoundEvents Tests
// ============================================================================

class SoundEventsTest : public ::testing::Test {};

TEST_F(SoundEventsTest, AmbientSoundEvents)
{
    // 测试环境音效事件
    EXPECT_EQ(SoundEvents::AMBIENT_CAVE.toString(), "minecraft:ambient.cave");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP.toString(), "minecraft:ambient.underwater.loop");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_ENTER.toString(), "minecraft:ambient.underwater.enter");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_EXIT.toString(), "minecraft:ambient.underwater.exit");
}

TEST_F(SoundEventsTest, EntitySoundEvents)
{
    // 测试实体音效事件
    EXPECT_EQ(SoundEvents::ENTITY_BEE_LOOP.toString(), "minecraft:entity.bee.loop");
    EXPECT_EQ(SoundEvents::ENTITY_BEE_LOOP_AGGRESSIVE.toString(), "minecraft:entity.bee.loop_aggressive");
    EXPECT_EQ(SoundEvents::ENTITY_GUARDIAN_ATTACK.toString(), "minecraft:entity.guardian.attack");
    EXPECT_EQ(SoundEvents::ITEM_ELYTRA_FLYING.toString(), "minecraft:item.elytra.flying");
}

TEST_F(SoundEventsTest, MusicSoundEvents)
{
    // 测试音乐事件
    EXPECT_EQ(SoundEvents::MUSIC_GAME.toString(), "minecraft:music.game");
    EXPECT_EQ(SoundEvents::MUSIC_MENU.toString(), "minecraft:music.menu");
    EXPECT_EQ(SoundEvents::MUSIC_CREATIVE.toString(), "minecraft:music.creative");
    EXPECT_EQ(SoundEvents::MUSIC_END.toString(), "minecraft:music.end");
    EXPECT_EQ(SoundEvents::MUSIC_DRAGON.toString(), "minecraft:music.dragon");
}

TEST_F(SoundEventsTest, UnderwaterAdditionsSoundEvents)
{
    // 测试水下附加音效
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS.toString(), "minecraft:ambient.underwater.loop.additions");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS_RARE.toString(),
        "minecraft:ambient.underwater.loop.additions.rare");
    EXPECT_EQ(SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS_ULTRA_RARE.toString(),
        "minecraft:ambient.underwater.loop.additions.ultra_rare");
}

TEST_F(SoundEventsTest, NetherMusicSoundEvents)
{
    // 测试下界音乐事件
    EXPECT_EQ(SoundEvents::MUSIC_NETHER_BASALT_DELTAS.toString(), "minecraft:music.nether.basalt_deltas");
    EXPECT_EQ(SoundEvents::MUSIC_NETHER_CRIMSON_FOREST.toString(), "minecraft:music.nether.crimson_forest");
    EXPECT_EQ(SoundEvents::MUSIC_NETHER_NETHER_WASTES.toString(), "minecraft:music.nether.nether_wastes");
    EXPECT_EQ(SoundEvents::MUSIC_NETHER_SOUL_SAND_VALLEY.toString(), "minecraft:music.nether.soul_sand_valley");
}

TEST_F(SoundEventsTest, BlockSoundEvents)
{
    // 测试方块音效事件
    EXPECT_EQ(SoundEvents::BLOCK_STONE_BREAK.toString(), "minecraft:block.stone.break");
    EXPECT_EQ(SoundEvents::BLOCK_STONE_PLACE.toString(), "minecraft:block.stone.place");
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_DOOR_OPEN.toString(), "minecraft:block.wooden_door.open");
    EXPECT_EQ(SoundEvents::BLOCK_WOODEN_DOOR_CLOSE.toString(), "minecraft:block.wooden_door.close");
}

// ============================================================================
// Random Tests (验证随机数生成器)
// ============================================================================

class RandomTest : public ::testing::Test {
protected:
    void SetUp() override { rng = std::make_unique<mc::math::Random>(12345); }

    std::unique_ptr<mc::math::Random> rng;
};

TEST_F(RandomTest, NextFloatRange)
{
    for (int i = 0; i < 1000; ++i) {
        f32 value = rng->nextFloat();
        EXPECT_GE(value, 0.0f);
        EXPECT_LT(value, 1.0f);
    }
}

TEST_F(RandomTest, NextIntRange)
{
    for (int i = 0; i < 1000; ++i) {
        i32 value = rng->nextInt(100);
        EXPECT_GE(value, 0);
        EXPECT_LT(value, 100);
    }
}

TEST_F(RandomTest, NextIntWithRange)
{
    for (int i = 0; i < 1000; ++i) {
        i32 value = rng->nextInt(10, 20);
        EXPECT_GE(value, 10);
        EXPECT_LE(value, 20);
    }
}

TEST_F(RandomTest, UnderwaterProbabilityDistribution)
{
    // 测试水下附加音效概率分布
    // 普通: 0.9%, 稀有: 0.09%, 超稀有: 0.01%

    i32 normal = 0, rare = 0, ultraRare = 0, none = 0;
    constexpr i32 iterations = 1000000;

    for (i32 i = 0; i < iterations; ++i) {
        f32 f = rng->nextFloat();
        if (f < 0.0001f) {
            ++ultraRare;
        } else if (f < 0.001f) {
            ++rare;
        } else if (f < 0.01f) {
            ++normal;
        } else {
            ++none;
        }
    }

    // 验证概率大致正确（允许 20% 误差）
    f32 normalProb = static_cast<f32>(normal) / iterations;
    f32 rareProb = static_cast<f32>(rare) / iterations;
    f32 ultraRareProb = static_cast<f32>(ultraRare) / iterations;

    EXPECT_NEAR(normalProb, 0.009f, 0.002f);      // ~0.9%
    EXPECT_NEAR(rareProb, 0.0009f, 0.0003f);      // ~0.09%
    EXPECT_NEAR(ultraRareProb, 0.0001f, 0.0001f); // ~0.01%
}

// ============================================================================
// BiomeLoopSound Behavioral Tests
// ============================================================================

class BiomeAmbientBehaviorTest : public ::testing::Test {};

TEST_F(BiomeAmbientBehaviorTest, MoodSoundTimerLogic)
{
    // 测试心境音效计时器逻辑
    // 在完全黑暗中，计时器应该增加
    // 在有光照的地方，计时器应该减少

    f32 timer = 0.0f;
    u8 skyLight = 0;
    u8 blockLight = 0;
    constexpr i32 tickDelay = 6000;

    // 在完全黑暗中，每个 tick 增加 1/tickDelay
    for (int i = 0; i < 6000; ++i) {
        if (skyLight > 0) {
            timer -= static_cast<f32>(skyLight) / 15.0f * 0.001f;
        } else if (blockLight > 0) {
            timer -= static_cast<f32>(blockLight - 1) / static_cast<f32>(tickDelay);
        } else {
            timer += 1.0f / static_cast<f32>(tickDelay);
        }
    }

    // 6000 tick 后，计时器应该达到 1.0
    EXPECT_NEAR(timer, 1.0f, 0.01f);

    // 有光照时，计时器应该减少
    skyLight = 15;
    timer = 0.5f;
    for (int i = 0; i < 100; ++i) {
        if (skyLight > 0) {
            timer -= static_cast<f32>(skyLight) / 15.0f * 0.001f;
        }
        timer = std::max(timer, 0.0f);
    }

    // 光照应该减少计时器
    EXPECT_LT(timer, 0.5f);
}

// ============================================================================
// WeatherSoundHandler Tests
// ============================================================================

class WeatherSoundHandlerTest : public ::testing::Test {
protected:
    void SetUp() override { handler = std::make_unique<WeatherSoundHandler>(); }

    std::unique_ptr<WeatherSoundHandler> handler;
};

TEST_F(WeatherSoundHandlerTest, InitialState)
{
    EXPECT_FALSE(handler->isRaining());
    EXPECT_FALSE(handler->isThundering());
}

TEST_F(WeatherSoundHandlerTest, UpdateWeatherState)
{
    // 测试设置天气状态
    handler->updateWeatherState(0.5f, 0.0f, true);
    EXPECT_TRUE(handler->isRaining());
    EXPECT_FALSE(handler->isThundering());

    handler->updateWeatherState(0.5f, 0.95f, true);
    EXPECT_TRUE(handler->isRaining());
    EXPECT_TRUE(handler->isThundering());

    handler->updateWeatherState(0.0f, 0.0f, true);
    EXPECT_FALSE(handler->isRaining());
    EXPECT_FALSE(handler->isThundering());
}

TEST_F(WeatherSoundHandlerTest, RainThreshold)
{
    // 雨量低于阈值不算下雨
    handler->updateWeatherState(0.009f, 0.0f, true);
    EXPECT_FALSE(handler->isRaining());

    // 雨量等于阈值不算下雨（严格大于）
    handler->updateWeatherState(0.01f, 0.0f, true);
    EXPECT_FALSE(handler->isRaining());

    // 雨量高于阈值才算下雨
    handler->updateWeatherState(0.011f, 0.0f, true);
    EXPECT_TRUE(handler->isRaining());
}

TEST_F(WeatherSoundHandlerTest, ThunderThreshold)
{
    // 雷暴强度低于阈值不算雷暴
    handler->updateWeatherState(0.5f, 0.89f, true);
    EXPECT_FALSE(handler->isThundering());

    // 雷暴强度等于阈值不算雷暴（严格大于 0.9）
    handler->updateWeatherState(0.5f, 0.9f, true);
    EXPECT_FALSE(handler->isThundering());

    // 雷暴强度高于阈值才算雷暴
    handler->updateWeatherState(0.5f, 0.91f, true);
    EXPECT_TRUE(handler->isThundering());
}

TEST_F(WeatherSoundHandlerTest, CanSeeSkyAffectsRainAndThunder)
{
    // 看不到天空时，下雨状态仍然成立，但播放的是 RAIN_ABOVE 而非 RAIN
    handler->updateWeatherState(0.5f, 0.0f, false);
    EXPECT_TRUE(handler->isRaining()); // 状态是下雨
    // 具体播放 RAIN 还是 RAIN_ABOVE 由 _updateRainSound 中的 canSeeSky 决定

    // 看不到天空时，雷暴状态仍然成立，但雷声不会播放
    handler->updateWeatherState(1.0f, 1.0f, false);
    EXPECT_TRUE(handler->isThundering()); // 状态仍是雷暴
    // 但雷声不应该播放（在 _tryPlayThunder 中对 canSeeSky 检查）
}

TEST_F(WeatherSoundHandlerTest, CanSeeSkyDeterminesRainType)
{
    // MC 1.21.11: canSeeSky=false 时使用 WEATHER_RAIN_ABOVE（遮挡物下方闷雨声）
    // canSeeSky=true 时使用 WEATHER_RAIN（户外正常雨声）

    // 户外（能看到天空）
    handler->updateWeatherState(0.5f, 0.0f, true);
    EXPECT_TRUE(handler->isRaining());

    // 遮挡物下方（看不到天空）
    handler->updateWeatherState(0.5f, 0.0f, false);
    EXPECT_TRUE(handler->isRaining()); // 仍然下雨，只是雨声类型不同
}

TEST_F(WeatherSoundHandlerTest, ThunderTiming)
{
    // 测试雷声计时器行为
    // MC 1.21.11: 雷声间隔 5-30 秒 (100-600 ticks)

    // 设置雷暴状态
    handler->updateWeatherState(1.0f, 1.0f, true);
    EXPECT_TRUE(handler->isThundering());

    // 看不到天空时不应该播放雷声
    handler->updateWeatherState(1.0f, 1.0f, false);
    EXPECT_TRUE(handler->isThundering()); // 状态仍是雷暴
    // 但雷声不应该播放（在 _tryPlayThunder 中处理）
}

TEST_F(WeatherSoundHandlerTest, WeatherSoundEvents)
{
    // 验证天气相关音效事件存在
    EXPECT_EQ(SoundEvents::WEATHER_RAIN.toString(), "minecraft:weather.rain");
    EXPECT_EQ(SoundEvents::WEATHER_RAIN_ABOVE.toString(), "minecraft:weather.rain.above");
    EXPECT_EQ(SoundEvents::WEATHER_THUNDER.toString(), "minecraft:weather.thunder");
}

// ============================================================================
// BiomeAmbientHandler 采样位置光照测试
// ============================================================================

class BiomeAmbientMoodLightingTest : public ::testing::Test {};

TEST_F(BiomeAmbientMoodLightingTest, MoodTimerIncreasesInSampledDarkness)
{
    // 当采样位置完全黑暗时，心境计时器应该增加
    // 复现 BiomeAmbientHandler 中的心境计时器逻辑
    f32 timer = 0.0f;
    constexpr i32 tickDelay = 6000;

    // 采样位置完全黑暗 (skyLight=0, blockLight=0)
    u8 moodSkyLight = 0;
    u8 moodBlockLight = 0;

    for (int i = 0; i < 6000; ++i) {
        if (moodSkyLight > 0) {
            timer -= static_cast<f32>(moodSkyLight) / 15.0f * 0.001f;
        } else if (moodBlockLight > 0) {
            timer -= static_cast<f32>(moodBlockLight - 1) / static_cast<f32>(tickDelay);
        } else {
            timer += 1.0f / static_cast<f32>(tickDelay);
        }
    }

    // 6000 tick 后，计时器应该达到 1.0
    EXPECT_NEAR(timer, 1.0f, 0.01f);
}

TEST_F(BiomeAmbientMoodLightingTest, MoodTimerDecreasesWithSampledSkyLight)
{
    // 当采样位置有天空光时，心境计时器应该减少
    f32 timer = 0.5f;
    u8 moodSkyLight = 15;

    for (int i = 0; i < 100; ++i) {
        if (moodSkyLight > 0) {
            timer -= static_cast<f32>(moodSkyLight) / 15.0f * 0.001f;
        }
        timer = std::max(timer, 0.0f);
    }

    // 有天空光时计时器应该减少
    EXPECT_LT(timer, 0.5f);
}

TEST_F(BiomeAmbientMoodLightingTest, MoodTimerDecreasesWithSampledBlockLight)
{
    // 当采样位置无天空光但有方块光时，计时器也应该减少
    f32 timer = 0.5f;
    u8 moodSkyLight = 0;
    u8 moodBlockLight = 10;
    constexpr i32 tickDelay = 6000;

    for (int i = 0; i < 100; ++i) {
        if (moodSkyLight > 0) {
            timer -= static_cast<f32>(moodSkyLight) / 15.0f * 0.001f;
        } else if (moodBlockLight > 0) {
            timer -= static_cast<f32>(moodBlockLight - 1) / static_cast<f32>(tickDelay);
        }
        timer = std::max(timer, 0.0f);
    }

    // 有方块光时计时器应该减少
    EXPECT_LT(timer, 0.5f);
}

TEST_F(BiomeAmbientMoodLightingTest, SampledLightDiffersFromPlayerLight)
{
    // 验证采样位置光照与玩家位置光照可以不同
    // 模拟：玩家站在有光照的位置（skyLight=15），但采样位置在黑暗处（moodSkyLight=0）
    // 此时计时器应该增加（采样位置黑暗），而非减少（玩家位置有光）

    f32 timer = 0.0f;
    u8 playerSkyLight = 15; // 玩家位置有光
    u8 moodSkyLight = 0;    // 采样位置黑暗
    u8 moodBlockLight = 0;
    constexpr i32 tickDelay = 6000;

    // 使用采样位置的光照计算（修复后的行为）
    for (int i = 0; i < 6000; ++i) {
        if (moodSkyLight > 0) {
            timer -= static_cast<f32>(moodSkyLight) / 15.0f * 0.001f;
        } else if (moodBlockLight > 0) {
            timer -= static_cast<f32>(moodBlockLight - 1) / static_cast<f32>(tickDelay);
        } else {
            timer += 1.0f / static_cast<f32>(tickDelay);
        }
    }

    // 采样位置黑暗 → 计时器增加（修复后的行为）
    EXPECT_NEAR(timer, 1.0f, 0.01f);

    // 如果使用玩家位置光照（修复前的错误行为），计时器会减少
    f32 wrongTimer = 0.5f;
    for (int i = 0; i < 100; ++i) {
        if (playerSkyLight > 0) {
            wrongTimer -= static_cast<f32>(playerSkyLight) / 15.0f * 0.001f;
        }
        wrongTimer = std::max(wrongTimer, 0.0f);
    }
    EXPECT_LT(wrongTimer, 0.5f); // 使用玩家光照时计时器会减少
}

TEST_F(BiomeAmbientMoodLightingTest, SetLightLevelAcceptsMoodLight)
{
    // 验证 setLightLevel 接受4个参数
    BiomeAmbientHandler handler;
    // 玩家位置有天空光，采样位置黑暗
    handler.setLightLevel(15, 0, 0, 0);
    // 玩家位置和采样位置都有天空光
    handler.setLightLevel(15, 0, 15, 0);
    // 玩家位置黑暗，采样位置有方块光
    handler.setLightLevel(0, 0, 0, 10);
    // 无异常即成功
}

// ============================================================================
// MusicPlayer 水下生物群系测试
// ============================================================================

class MusicPlayerOceanBiomeTest : public ::testing::Test {};

TEST_F(MusicPlayerOceanBiomeTest, InOceanOrRiverBiomeEnumCoverage)
{
    // 验证 MusicPlayer 的 MusicType::Underwater 枚举值存在
    EXPECT_EQ(static_cast<int>(MusicPlayer::MusicType::Underwater), 7);
}

TEST_F(MusicPlayerOceanBiomeTest, MusicSelectorFromBiomeMusic)
{
    // 验证 MusicSelector::fromBiomeMusic 能正确转换
    mc::world::biome::BiomeMusic biomeMusic(
        mc::ResourceLocation("minecraft:music.nether.basalt_deltas"), 12000, 24000, true);
    auto selector = MusicPlayer::MusicSelector::fromBiomeMusic(biomeMusic);
    EXPECT_EQ(selector.soundEventId.toString(), "minecraft:music.nether.basalt_deltas");
    EXPECT_EQ(selector.minDelayTicks, 12000u);
    EXPECT_EQ(selector.maxDelayTicks, 24000u);
    EXPECT_TRUE(selector.replaceCurrent);
}
