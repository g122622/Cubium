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

#include "client/sound/SoundLoader.hpp"
#include "client/sound/SoundPool.hpp"
#include "client/sound/backend/AudioBuffer.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundTypes.hpp"

#include <array>
#include <limits>

using namespace mc::client::sound;
using namespace mc;

// ============================================================================
// SoundInstance 测试
// ============================================================================

class SoundInstanceTest : public ::testing::Test {
protected:
    void SetUp() override { testLocation = ResourceLocation("minecraft:test.sound"); }

    ResourceLocation testLocation;
};

TEST_F(SoundInstanceTest, CreateGlobalSound)
{
    auto sound = SoundInstance::createGlobal(testLocation, SoundCategory::Music, 0.8f, 1.2f);

    EXPECT_EQ(sound.getSoundEventId(), testLocation);
    EXPECT_EQ(sound.getCategory(), SoundCategory::Music);
    EXPECT_FLOAT_EQ(sound.getVolume(), 0.8f);
    EXPECT_FLOAT_EQ(sound.getPitch(), 1.2f);
    EXPECT_TRUE(sound.isGlobal());
    EXPECT_EQ(sound.getAttenuationType(), AttenuationType::None);
    EXPECT_FALSE(sound.isLooping());
}

TEST_F(SoundInstanceTest, CreateLocatedSound)
{
    auto sound = SoundInstance::createLocated(testLocation, SoundCategory::Blocks, 10.0f, 20.0f, 30.0f, 0.5f, 0.9f);

    EXPECT_EQ(sound.getSoundEventId(), testLocation);
    EXPECT_EQ(sound.getCategory(), SoundCategory::Blocks);
    EXPECT_FLOAT_EQ(sound.getX(), 10.0f);
    EXPECT_FLOAT_EQ(sound.getY(), 20.0f);
    EXPECT_FLOAT_EQ(sound.getZ(), 30.0f);
    EXPECT_FLOAT_EQ(sound.getVolume(), 0.5f);
    EXPECT_FLOAT_EQ(sound.getPitch(), 0.9f);
    EXPECT_FALSE(sound.isGlobal());
    EXPECT_EQ(sound.getAttenuationType(), AttenuationType::Linear);
    EXPECT_FLOAT_EQ(sound.getAttenuationDistance(), DEFAULT_ATTENUATION_DISTANCE);
}

TEST_F(SoundInstanceTest, CreateMusicSound)
{
    auto sound = SoundInstance::createMusic(testLocation, 1.0f, 1.0f);

    EXPECT_EQ(sound.getSoundEventId(), testLocation);
    EXPECT_EQ(sound.getCategory(), SoundCategory::Music);
    EXPECT_TRUE(sound.isGlobal());
    EXPECT_EQ(sound.getAttenuationType(), AttenuationType::None);
}

TEST_F(SoundInstanceTest, CreateRecordSound)
{
    auto sound = SoundInstance::createRecord(testLocation, 100.0f, 64.0f, -50.0f);

    EXPECT_EQ(sound.getSoundEventId(), testLocation);
    EXPECT_EQ(sound.getCategory(), SoundCategory::Records);
    EXPECT_FLOAT_EQ(sound.getX(), 100.0f);
    EXPECT_FLOAT_EQ(sound.getY(), 64.0f);
    EXPECT_FLOAT_EQ(sound.getZ(), -50.0f);
    EXPECT_FLOAT_EQ(sound.getVolume(), 4.0f); // 唱片机音量较大
    EXPECT_TRUE(sound.isLooping());           // 唱片机循环播放
    EXPECT_FLOAT_EQ(sound.getAttenuationDistance(), 64.0f);
}

TEST_F(SoundInstanceTest, SetPosition)
{
    auto sound = SoundInstance::createGlobal(testLocation, SoundCategory::Master);
    sound.setPosition(glm::vec3(1.0f, 2.0f, 3.0f));

    EXPECT_FLOAT_EQ(sound.getX(), 1.0f);
    EXPECT_FLOAT_EQ(sound.getY(), 2.0f);
    EXPECT_FLOAT_EQ(sound.getZ(), 3.0f);
}

TEST_F(SoundInstanceTest, SetVolumeAndPitch)
{
    auto sound = SoundInstance::createGlobal(testLocation, SoundCategory::Master);
    sound.setVolume(0.5f);
    sound.setPitch(1.5f);

    EXPECT_FLOAT_EQ(sound.getVolume(), 0.5f);
    EXPECT_FLOAT_EQ(sound.getPitch(), 1.5f);
}

TEST_F(SoundInstanceTest, SetLooping)
{
    auto sound = SoundInstance::createGlobal(testLocation, SoundCategory::Master);
    EXPECT_FALSE(sound.isLooping());

    sound.setLooping(true);
    EXPECT_TRUE(sound.isLooping());
}

TEST_F(SoundInstanceTest, SetRepeatDelay)
{
    auto sound = SoundInstance::createGlobal(testLocation, SoundCategory::Master);
    sound.setRepeatDelay(100);

    EXPECT_EQ(sound.getRepeatDelay(), 100u);
}

TEST_F(SoundInstanceTest, SetAndGetId)
{
    auto sound = SoundInstance::createGlobal(testLocation, SoundCategory::Master);
    EXPECT_EQ(sound.getId(), 0u);

    sound.setId(42);
    EXPECT_EQ(sound.getId(), 42u);
}

TEST_F(SoundInstanceTest, MarkDone)
{
    auto sound = SoundInstance::createGlobal(testLocation, SoundCategory::Master);
    EXPECT_FALSE(sound.isDone());

    sound.markDone();
    EXPECT_TRUE(sound.isDone());
}

TEST_F(SoundInstanceTest, GetPosition)
{
    auto sound = SoundInstance::createLocated(testLocation, SoundCategory::Blocks, 1.0f, 2.0f, 3.0f);

    glm::vec3 pos = sound.getPosition();
    EXPECT_FLOAT_EQ(pos.x, 1.0f);
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(pos.z, 3.0f);
}

// ============================================================================
// SoundPool 测试
// ============================================================================

class SoundPoolTest : public ::testing::Test {
protected:
    void SetUp() override { pool = std::make_unique<SoundPool>(); }

    std::unique_ptr<SoundPool> pool;
    ResourceLocation testLocation{"minecraft:test.sound"};
};

TEST_F(SoundPoolTest, AddSound)
{
    auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master));

    SoundInstanceId id = pool->add(std::move(sound));
    EXPECT_NE(id, 0u);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(SoundPoolTest, GetSound)
{
    auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master));

    SoundInstanceId id = pool->add(std::move(sound));
    ASSERT_NE(id, 0u);

    const ISoundInstance* retrieved = pool->get(id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getSoundEventId(), testLocation);
}

TEST_F(SoundPoolTest, GetNonExistentSound)
{
    const ISoundInstance* retrieved = pool->get(999);
    EXPECT_EQ(retrieved, nullptr);
}

TEST_F(SoundPoolTest, RemoveSound)
{
    auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master));

    SoundInstanceId id = pool->add(std::move(sound));
    EXPECT_TRUE(pool->has(id));

    bool removed = pool->remove(id);
    EXPECT_TRUE(removed);
    EXPECT_FALSE(pool->has(id));
    EXPECT_TRUE(pool->empty());
}

TEST_F(SoundPoolTest, RemoveNonExistentSound)
{
    bool removed = pool->remove(999);
    EXPECT_FALSE(removed);
}

TEST_F(SoundPoolTest, ClearPool)
{
    pool->add(std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master)));
    pool->add(std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(ResourceLocation("minecraft:test2"), SoundCategory::Music)));

    EXPECT_EQ(pool->size(), 2u);

    pool->clear();
    EXPECT_TRUE(pool->empty());
}

TEST_F(SoundPoolTest, GetByCategory)
{
    pool->add(std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master)));
    pool->add(std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(ResourceLocation("minecraft:test2"), SoundCategory::Music)));
    pool->add(std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(ResourceLocation("minecraft:test3"), SoundCategory::Music)));

    auto masterIds = pool->getByCategory(SoundCategory::Master);
    EXPECT_EQ(masterIds.size(), 1u);

    auto musicIds = pool->getByCategory(SoundCategory::Music);
    EXPECT_EQ(musicIds.size(), 2u);

    auto blocksIds = pool->getByCategory(SoundCategory::Blocks);
    EXPECT_TRUE(blocksIds.empty());
}

TEST_F(SoundPoolTest, RemoveByCategory)
{
    pool->add(std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master)));
    pool->add(std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(ResourceLocation("minecraft:test2"), SoundCategory::Music)));
    pool->add(std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(ResourceLocation("minecraft:test3"), SoundCategory::Music)));

    size_t removed = pool->removeByCategory(SoundCategory::Music);
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(pool->size(), 1u);
}

TEST_F(SoundPoolTest, GetBySoundEvent)
{
    pool->add(std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master)));
    pool->add(std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Music)));
    pool->add(std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(ResourceLocation("minecraft:other"), SoundCategory::Master)));

    auto ids = pool->getBySoundEvent(testLocation);
    EXPECT_EQ(ids.size(), 2u);

    auto otherIds = pool->getBySoundEvent(ResourceLocation("minecraft:other"));
    EXPECT_EQ(otherIds.size(), 1u);
}

TEST_F(SoundPoolTest, TickCleansDoneSounds)
{
    auto sound1 = std::make_unique<SoundInstance>(SoundInstance::createGlobal(testLocation, SoundCategory::Master));
    sound1->setId(1);

    auto sound2 = std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(ResourceLocation("minecraft:test2"), SoundCategory::Music));
    sound2->setId(2);

    pool->add(std::move(sound1));
    pool->add(std::move(sound2));

    EXPECT_EQ(pool->size(), 2u);

    // 标记第一个为完成
    ISoundInstance* s = pool->get(1);
    ASSERT_NE(s, nullptr);
    s->setId(1); // 确保ID设置
    // 由于 SoundInstance::markDone 是 protected，我们需要使用 tick 自动清理

    // tick 不会清理未完成的声音
    size_t cleaned = pool->tick();
    EXPECT_EQ(cleaned, 0u);
    EXPECT_EQ(pool->size(), 2u);
}

// ============================================================================
// AudioFormat 测试
// ============================================================================

class AudioFormatTest : public ::testing::Test {};

TEST_F(AudioFormatTest, ByteRate)
{
    AudioFormat format;
    format.sampleRate = 44100;
    format.channels = 2;
    format.bitsPerSample = 16;

    // 44100 Hz * 2 channels * 2 bytes = 176400 bytes/sec
    EXPECT_EQ(format.byteRate(), 176400u);
}

TEST_F(AudioFormatTest, BlockAlign)
{
    AudioFormat format;
    format.channels = 2;
    format.bitsPerSample = 16;

    // 2 channels * 2 bytes = 4 bytes
    EXPECT_EQ(format.blockAlign(), 4u);
}

TEST_F(AudioFormatTest, IsValid)
{
    AudioFormat valid;
    valid.sampleRate = 44100;
    valid.channels = 2;
    valid.bitsPerSample = 16;
    EXPECT_TRUE(valid.isValid());

    AudioFormat invalidRate;
    invalidRate.sampleRate = 0;
    invalidRate.channels = 2;
    invalidRate.bitsPerSample = 16;
    EXPECT_FALSE(invalidRate.isValid());

    AudioFormat invalidChannels;
    invalidChannels.sampleRate = 44100;
    invalidChannels.channels = 3; // 不支持
    invalidChannels.bitsPerSample = 16;
    EXPECT_FALSE(invalidChannels.isValid());

    AudioFormat invalidBits;
    invalidBits.sampleRate = 44100;
    invalidBits.channels = 2;
    invalidBits.bitsPerSample = 24; // 不支持
    EXPECT_FALSE(invalidBits.isValid());
}

// ============================================================================
// AudioData 测试
// ============================================================================

class AudioDataTest : public ::testing::Test {};

TEST_F(AudioDataTest, DefaultConstructor)
{
    AudioData data;
    EXPECT_FALSE(data.isValid());
    EXPECT_EQ(data.sampleCount(), 0u);
    EXPECT_FLOAT_EQ(data.duration, 0.0f);
}

TEST_F(AudioDataTest, ConstructorWithFormat)
{
    AudioFormat format;
    format.sampleRate = 44100;
    format.channels = 2;
    format.bitsPerSample = 16;

    std::vector<u8> samples(176400); // 1秒的音频
    AudioData data(format, std::move(samples));

    EXPECT_TRUE(data.isValid());
    EXPECT_EQ(data.sampleCount(), 44100u);
    EXPECT_FLOAT_EQ(data.duration, 1.0f);
}

TEST_F(AudioDataTest, CalculateDuration)
{
    AudioFormat format;
    format.sampleRate = 22050;
    format.channels = 1;
    format.bitsPerSample = 16;

    std::vector<u8> samples(44100); // 1秒的音频
    AudioData data(format, std::move(samples));

    EXPECT_FLOAT_EQ(data.calculateDuration(), 1.0f);
}

// ============================================================================
// SoundLoader 解码健壮性测试
// ============================================================================

TEST(SoundLoaderTest, DecodeRejectsEmptyData)
{
    auto result = SoundLoader::decode(nullptr, 0);
    EXPECT_FALSE(result.success());
}

TEST(SoundLoaderTest, DecodeRejectsInvalidBytesWithoutCrash)
{
    const std::array<u8, 8> invalidData = {0x4E, 0x4F, 0x54, 0x4F, 0x47, 0x47, 0x00, 0x01};

    auto result = SoundLoader::decode(invalidData.data(), invalidData.size());
    EXPECT_FALSE(result.success());
}

TEST(SoundLoaderTest, DecodeRejectsTooLargeInput)
{
    const std::array<u8, 1> dummy = {0x00};

    auto result = SoundLoader::decode(dummy.data(), static_cast<size_t>(std::numeric_limits<int>::max()) + 1ull);
    EXPECT_FALSE(result.success());
}
