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

#include "common/sound/SoundCategory.hpp"
#include "client/sound/resource/SoundDefinition.hpp"
#include "common/sound/SoundEvent.hpp"
#include "common/sound/SoundTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include <gtest/gtest.h>

#include <chrono>
#include <unordered_map>

namespace mc::sound {
namespace {

// ============================================================================
// SoundCategory 测试
// ============================================================================

TEST(SoundCategoryTest, GetCategoryName)
{
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Master), "master");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Music), "music");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Records), "record");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Weather), "weather");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Blocks), "block");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Hostile), "hostile");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Neutral), "neutral");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Players), "player");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Ambient), "ambient");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::Voice), "voice");
    EXPECT_EQ(getSoundCategoryName(SoundCategory::UI), "ui");
}

TEST(SoundCategoryTest, ParseValidNames)
{
    EXPECT_EQ(parseSoundCategory("master"), SoundCategory::Master);
    EXPECT_EQ(parseSoundCategory("music"), SoundCategory::Music);
    EXPECT_EQ(parseSoundCategory("record"), SoundCategory::Records);
    EXPECT_EQ(parseSoundCategory("weather"), SoundCategory::Weather);
    EXPECT_EQ(parseSoundCategory("block"), SoundCategory::Blocks);
    EXPECT_EQ(parseSoundCategory("hostile"), SoundCategory::Hostile);
    EXPECT_EQ(parseSoundCategory("neutral"), SoundCategory::Neutral);
    EXPECT_EQ(parseSoundCategory("player"), SoundCategory::Players);
    EXPECT_EQ(parseSoundCategory("ambient"), SoundCategory::Ambient);
    EXPECT_EQ(parseSoundCategory("voice"), SoundCategory::Voice);
    EXPECT_EQ(parseSoundCategory("ui"), SoundCategory::UI);
}

TEST(SoundCategoryTest, ParseCaseInsensitive)
{
    EXPECT_EQ(parseSoundCategory("MASTER"), SoundCategory::Master);
    EXPECT_EQ(parseSoundCategory("Music"), SoundCategory::Music);
    EXPECT_EQ(parseSoundCategory("BLOCK"), SoundCategory::Blocks);
    EXPECT_EQ(parseSoundCategory("Player"), SoundCategory::Players);
    EXPECT_EQ(parseSoundCategory("UI"), SoundCategory::UI);
    EXPECT_EQ(parseSoundCategory("Ui"), SoundCategory::UI);
}

TEST(SoundCategoryTest, ParsePluralAliases)
{
    // MC Java 有时使用复数形式
    EXPECT_EQ(parseSoundCategory("blocks"), SoundCategory::Blocks);
    EXPECT_EQ(parseSoundCategory("players"), SoundCategory::Players);
    EXPECT_EQ(parseSoundCategory("records"), SoundCategory::Records);
}

TEST(SoundCategoryTest, ParseInvalidNames)
{
    EXPECT_FALSE(parseSoundCategory("").has_value());
    EXPECT_FALSE(parseSoundCategory("invalid").has_value());
    EXPECT_FALSE(parseSoundCategory("unknown").has_value());
}

TEST(SoundCategoryTest, IsValidCategory)
{
    EXPECT_TRUE(isValidSoundCategory(SoundCategory::Master));
    EXPECT_TRUE(isValidSoundCategory(SoundCategory::Voice));
    EXPECT_TRUE(isValidSoundCategory(SoundCategory::UI));
    EXPECT_FALSE(isValidSoundCategory(SoundCategory::Count));
    EXPECT_FALSE(isValidSoundCategory(static_cast<SoundCategory>(255)));
}

// ============================================================================
// SoundEvent 测试
// ============================================================================

TEST(SoundEventTest, ConstructWithResourceLocation)
{
    ResourceLocation loc("minecraft:block.stone.break");
    SoundEvent event(loc);

    EXPECT_EQ(event.getId().toString(), "minecraft:block.stone.break");
    EXPECT_EQ(event.getAttenuationDistance(), 16.0f);
    EXPECT_TRUE(event.isValid());
}

TEST(SoundEventTest, ConstructWithString)
{
    SoundEvent event("minecraft:entity.cow.ambient");

    EXPECT_EQ(event.getId().toString(), "minecraft:entity.cow.ambient");
    EXPECT_TRUE(event.isValid());
}

TEST(SoundEventTest, SetAttenuationDistance)
{
    SoundEvent event("minecraft:test.sound");
    event.setAttenuationDistance(32.0f);

    EXPECT_EQ(event.getAttenuationDistance(), 32.0f);
}

TEST(SoundEventTest, EmptyEvent)
{
    SoundEvent event = SoundEvent::empty();

    EXPECT_FALSE(event.isValid());
    EXPECT_TRUE(event.getId().path().empty());
}

TEST(SoundEventTest, Comparison)
{
    SoundEvent event1("minecraft:test.sound");
    SoundEvent event2("minecraft:test.sound");
    SoundEvent event3("minecraft:aaa.sound"); // 'aaa' < 'test' in alphabetical order

    EXPECT_EQ(event1, event2);
    EXPECT_NE(event1, event3);
    EXPECT_LT(event3, event1); // 'aaa' < 'test'
}

// ============================================================================
// SoundTypes 测试
// ============================================================================

TEST(SoundTypesTest, Constants)
{
    EXPECT_EQ(INVALID_SOUND_INSTANCE_ID, 0u);
    EXPECT_EQ(DEFAULT_ATTENUATION_DISTANCE, 16.0f);
    EXPECT_EQ(MAX_CONCURRENT_SOUNDS, 256u);
    EXPECT_EQ(STREAM_BUFFER_SIZE, 65536u);
    EXPECT_EQ(AUDIO_SAMPLE_RATE, 44100u);
    EXPECT_EQ(AUDIO_CHANNELS, 2u);
    EXPECT_EQ(AUDIO_BITS_PER_SAMPLE, 16u);
}

} // anonymous namespace
} // namespace mc::sound

namespace mc::client::sound {
namespace {

// ============================================================================
// SoundDefinition 测试
// ============================================================================

TEST(SoundDefinitionTest, ConstructFromPath)
{
    SoundDefinition def("minecraft:dig/stone1");

    EXPECT_EQ(def.type, SoundType::File);
    EXPECT_EQ(def.volume, 1.0f);
    EXPECT_EQ(def.pitch, 1.0f);
    EXPECT_EQ(def.weight, 1u);
    EXPECT_FALSE(def.stream);
    EXPECT_FALSE(def.preload);
    EXPECT_EQ(def.attenuationDistance, 16u);
}

TEST(SoundDefinitionTest, ToOggLocation)
{
    SoundDefinition def("minecraft:dig/stone1");
    ResourceLocation oggLoc = def.toOggLocation();

    EXPECT_EQ(oggLoc.toString(), "minecraft:dig/stone1");
}

TEST(SoundDefinitionTest, ParseSimpleString)
{
    nlohmann::json json = "dig/stone1";

    auto result = SoundDefinition::parse(json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_EQ(def.type, SoundType::File);
    EXPECT_EQ(def.location.toString(), "minecraft:dig/stone1");
}

TEST(SoundDefinitionTest, ParseEventReference)
{
    nlohmann::json json = "#block.stone.break";

    auto result = SoundDefinition::parse(json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_EQ(def.type, SoundType::Event);
    // ResourceLocation::parse 自动添加默认命名空间 "minecraft:"
    EXPECT_EQ(def.location.toString(), "minecraft:block.stone.break");
}

TEST(SoundDefinitionTest, ParseObjectWithAllFields)
{
    nlohmann::json json = R"({
        "name": "dig/stone1",
        "volume": 0.8,
        "pitch": 1.2,
        "weight": 3,
        "stream": true,
        "preload": true,
        "attenuation_distance": 32
    })"_json;

    auto result = SoundDefinition::parse(json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_EQ(def.type, SoundType::File);
    EXPECT_EQ(def.location.toString(), "minecraft:dig/stone1");
    EXPECT_EQ(def.volume, 0.8f);
    EXPECT_EQ(def.pitch, 1.2f);
    EXPECT_EQ(def.weight, 3u);
    EXPECT_TRUE(def.stream);
    EXPECT_TRUE(def.preload);
    EXPECT_EQ(def.attenuationDistance, 32u);
}

TEST(SoundDefinitionTest, ParseWithTypeField)
{
    nlohmann::json json = R"({
        "name": "ambient.cave",
        "type": "event"
    })"_json;

    auto result = SoundDefinition::parse(json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_EQ(def.type, SoundType::Event);
}

// ============================================================================
// SoundEventDefinition 测试
// ============================================================================

TEST(SoundEventDefinitionTest, ParseMinimal)
{
    nlohmann::json json = R"({
        "sounds": ["dig/stone1", "dig/stone2"]
    })"_json;

    auto result = SoundEventDefinition::parse("minecraft:block.stone.break", json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_EQ(def.location.toString(), "minecraft:block.stone.break");
    EXPECT_EQ(def.sounds.size(), 2u);
    EXPECT_FALSE(def.replace);
    EXPECT_FALSE(def.subtitle.has_value());
}

TEST(SoundEventDefinitionTest, ParseWithSubtitle)
{
    nlohmann::json json = R"({
        "subtitle": "subtitles.block.generic.break",
        "sounds": ["dig/stone1"]
    })"_json;

    auto result = SoundEventDefinition::parse("minecraft:block.stone.break", json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_TRUE(def.subtitle.has_value());
    EXPECT_EQ(def.subtitle.value(), "subtitles.block.generic.break");
}

TEST(SoundEventDefinitionTest, ParseWithReplace)
{
    nlohmann::json json = R"({
        "replace": true,
        "sounds": ["custom/stone_break"]
    })"_json;

    auto result = SoundEventDefinition::parse("minecraft:block.stone.break", json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_TRUE(def.replace);
}

TEST(SoundEventDefinitionTest, SelectSoundWeighted)
{
    nlohmann::json json = R"({
        "sounds": [
            {"name": "sound1", "weight": 1},
            {"name": "sound2", "weight": 9}
        ]
    })"_json;

    auto result = SoundEventDefinition::parse("test:weighted", json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    EXPECT_EQ(def.totalWeight(), 10u);

    // 多次选择，统计结果
    mc::math::Random rng(42);
    int count1 = 0, count2 = 0;
    for (int i = 0; i < 100; ++i) {
        const auto* sound = def.selectSound(rng);
        ASSERT_NE(sound, nullptr);
        if (sound->location.path() == "sound1") {
            count1++;
        } else {
            count2++;
        }
    }

    // sound2 的权重是 sound1 的 9 倍，应该更常被选中
    EXPECT_GT(count2, count1);
}

TEST(SoundEventDefinitionTest, SelectSoundSingle)
{
    nlohmann::json json = R"({
        "sounds": ["only_sound"]
    })"_json;

    auto result = SoundEventDefinition::parse("test:single", json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();
    mc::math::Random rng(42);
    const auto* sound = def.selectSound(rng);

    ASSERT_NE(sound, nullptr);
    EXPECT_EQ(sound->location.path(), "only_sound");
}

TEST(SoundEventDefinitionTest, SelectSoundMultipleDifferentSelections)
{
    // 测试多次调用 selectSound 会选择不同的声音（概率性）
    nlohmann::json json = R"({
        "sounds": ["sound_a", "sound_b", "sound_c", "sound_d"]
    })"_json;

    auto result = SoundEventDefinition::parse("test:multi", json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();

    // 使用时间种子确保随机性
    mc::math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    // 统计每个声音被选择的次数
    std::unordered_map<std::string, int> selectionCounts;
    constexpr int NUM_SELECTIONS = 100;

    for (int i = 0; i < NUM_SELECTIONS; ++i) {
        const auto* sound = def.selectSound(rng);
        ASSERT_NE(sound, nullptr);
        selectionCounts[sound->location.path()]++;
    }

    // 应该选择了所有4个声音（每个至少选择几次）
    // 在 100 次选择中，每个应该平均 25 次
    // 允许一定的偏差，但确保没有哪个声音从未被选择
    for (const auto& [path, count] : selectionCounts) {
        EXPECT_GT(count, 0) << "Sound " << path << " was never selected";
        EXPECT_LT(count, 60) << "Sound " << path << " was selected too many times (may indicate biased selection)";
    }

    // 应该选择了所有4个不同的声音
    EXPECT_EQ(selectionCounts.size(), 4u);
}

TEST(SoundEventDefinitionTest, RandomNotStuckOnFirstSound)
{
    // 专门测试修复的问题：默认种子 0 不应导致总是选择第一个声音
    nlohmann::json json = R"({
        "sounds": ["first", "second", "third"]
    })"_json;

    auto result = SoundEventDefinition::parse("test:random", json, "minecraft");
    ASSERT_TRUE(result.success());

    const auto& def = result.value();

    // 模拟多次独立的随机选择（模拟 SoundEngine 的行为）
    // 使用不同种子创建多个 Random 实例
    std::unordered_map<std::string, int> counts;

    for (int i = 0; i < 50; ++i) {
        // 使用不同的种子
        mc::math::Random rng(static_cast<u64>(i * 12345 + 67890));
        const auto* sound = def.selectSound(rng);
        ASSERT_NE(sound, nullptr);
        counts[sound->location.path()]++;
    }

    // 应该选择了至少 2 个不同的声音
    // 如果只选择了 1 个，说明随机数生成器有问题
    EXPECT_GE(counts.size(), 2u) << "Random selection is stuck on single sound";
}

TEST(SoundEventDefinitionTest, EmptySoundsError)
{
    // 原版 MC 允许 sounds 为空数组（作为占位事件，不播放任何声音）。
    // parse 应当返回 success，selectSound 在空列表时返回 nullptr。
    // 参考 src/client/sound/resource/SoundDefinition.cpp:219-225 的处理逻辑。
    nlohmann::json json = R"({
        "sounds": []
    })"_json;

    auto result = SoundEventDefinition::parse("test:empty", json, "minecraft");
    EXPECT_TRUE(result.success());
    if (result.success()) {
        const auto& def = result.value();
        EXPECT_TRUE(def.sounds.empty());
        mc::math::Random rng(0);
        EXPECT_EQ(def.selectSound(rng), nullptr);
    }
}

} // anonymous namespace
} // namespace mc::client::sound
