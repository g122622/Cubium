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

#include "common/sound/SoundEvents.hpp"
#include "common/sound/jukebox/JukeboxSong.hpp"
#include "common/sound/jukebox/JukeboxSongPlayer.hpp"
#include "common/sound/jukebox/JukeboxSongs.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"

using namespace mc;

namespace {

// ========== JukeboxSong 测试 ==========

TEST(JukeboxSongTest, Constructor_StoresFields)
{
    JukeboxSong song(ResourceLocation("minecraft:music_disc.13"), "jukebox_song.minecraft.13", 178.0f, 1);

    EXPECT_EQ(song.getSoundEventId(), ResourceLocation("minecraft:music_disc.13"));
    EXPECT_EQ(song.getDescriptionKey(), "jukebox_song.minecraft.13");
    EXPECT_FLOAT_EQ(song.getLengthInSeconds(), 178.0f);
    EXPECT_EQ(song.getComparatorOutput(), 1);
}

TEST(JukeboxSongTest, LengthInTicks_ConvertsCorrectly)
{
    // 178秒 = ceil(178 * 20) = 3560 tick
    JukeboxSong song1(ResourceLocation("minecraft:test"), "test", 178.0f, 1);
    EXPECT_EQ(song1.lengthInTicks(), 3560);

    // 96秒 = ceil(96 * 20) = 1920 tick
    JukeboxSong song2(ResourceLocation("minecraft:test2"), "test2", 96.0f, 7);
    EXPECT_EQ(song2.lengthInTicks(), 1920);

    // 0.5秒 = ceil(0.5 * 20) = 10 tick
    JukeboxSong song3(ResourceLocation("minecraft:test3"), "test3", 0.5f, 1);
    EXPECT_EQ(song3.lengthInTicks(), 10);

    // 0.05秒 = ceil(0.05 * 20) = 1 tick
    JukeboxSong song4(ResourceLocation("minecraft:test4"), "test4", 0.05f, 1);
    EXPECT_EQ(song4.lengthInTicks(), 1);
}

TEST(JukeboxSongTest, HasFinished_BoundaryValues)
{
    // 歌曲长度 = 100 tick，加上 SONG_END_PADDING_TICKS = 20，总共 120 tick 后完成
    JukeboxSong song(ResourceLocation("minecraft:test"), "test", 5.0f, 1);
    // 5.0秒 = 100 tick, hasFinished 需要 >= 120

    // 尚未完成
    EXPECT_FALSE(song.hasFinished(0));
    EXPECT_FALSE(song.hasFinished(99));
    EXPECT_FALSE(song.hasFinished(100));
    EXPECT_FALSE(song.hasFinished(119));

    // 边界值：恰好等于 lengthInTicks + SONG_END_PADDING_TICKS
    EXPECT_TRUE(song.hasFinished(120));

    // 超过边界值
    EXPECT_TRUE(song.hasFinished(121));
    EXPECT_TRUE(song.hasFinished(1000));

    // 负值（不应该出现，但不应崩溃）
    EXPECT_FALSE(song.hasFinished(-1));
}

TEST(JukeboxSongTest, HasFinished_ShortSong)
{
    // 1秒 = 20 tick，hasFinished 需要 >= 40
    JukeboxSong song(ResourceLocation("minecraft:short"), "short", 1.0f, 1);
    EXPECT_FALSE(song.hasFinished(39));
    EXPECT_TRUE(song.hasFinished(40));
}

TEST(JukeboxSongTest, SongEndPaddingIs20)
{
    EXPECT_EQ(JukeboxSong::SONG_END_PADDING_TICKS, 20);
}

// ========== JukeboxSongs 注册表测试 ==========

class JukeboxSongsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { JukeboxSongs::initialize(); }
};

TEST_F(JukeboxSongsTest, Initialize_CanBeCalledMultipleTimes)
{
    // 第二次调用应该是空操作
    JukeboxSongs::initialize();
    EXPECT_TRUE(JukeboxSongs::isInitialized());
}

TEST_F(JukeboxSongsTest, IsInitialized_ReturnsTrue)
{
    EXPECT_TRUE(JukeboxSongs::isInitialized());
}

TEST_F(JukeboxSongsTest, GetSong_ById)
{
    const JukeboxSong* song = JukeboxSongs::getSong(ResourceLocation("minecraft:13"));
    ASSERT_NE(song, nullptr);
    EXPECT_EQ(song->getComparatorOutput(), 1);
    EXPECT_FLOAT_EQ(song->getLengthInSeconds(), 178.0f);
}

TEST_F(JukeboxSongsTest, GetSong_AllDiscsExist)
{
    // 验证所有21首歌曲都注册了
    const char* songIds[] = {"minecraft:13",
        "minecraft:cat",
        "minecraft:blocks",
        "minecraft:chirp",
        "minecraft:far",
        "minecraft:mall",
        "minecraft:mellohi",
        "minecraft:stal",
        "minecraft:strad",
        "minecraft:ward",
        "minecraft:11",
        "minecraft:wait",
        "minecraft:pigstep",
        "minecraft:otherside",
        "minecraft:5",
        "minecraft:relic",
        "minecraft:precipice",
        "minecraft:creator",
        "minecraft:creator_music_box",
        "minecraft:tears",
        "minecraft:lava_chicken"};

    for (const char* id : songIds) {
        const JukeboxSong* song = JukeboxSongs::getSong(ResourceLocation(id));
        ASSERT_NE(song, nullptr) << "Song not found: " << id;
    }
}

TEST_F(JukeboxSongsTest, GetSong_Nonexistent_ReturnsNullptr)
{
    const JukeboxSong* song = JukeboxSongs::getSong(ResourceLocation("minecraft:nonexistent_song"));
    EXPECT_EQ(song, nullptr);
}

TEST_F(JukeboxSongsTest, GetSongByComparatorOutput_FindsCorrectSong)
{
    // 信号强度 1 对应 "13"
    const JukeboxSong* song1 = JukeboxSongs::getSongByComparatorOutput(1);
    ASSERT_NE(song1, nullptr);
    EXPECT_EQ(song1->getComparatorOutput(), 1);

    // 信号强度 15 对应 "5"
    const JukeboxSong* song15 = JukeboxSongs::getSongByComparatorOutput(15);
    ASSERT_NE(song15, nullptr);
    EXPECT_EQ(song15->getComparatorOutput(), 15);
}

TEST_F(JukeboxSongsTest, GetSongByComparatorOutput_MultipleSongsSameSignal)
{
    // 信号强度 14 同时有 "otherside" 和 "relic"
    // getSongByComparatorOutput 返回第一个匹配的
    const JukeboxSong* song14 = JukeboxSongs::getSongByComparatorOutput(14);
    ASSERT_NE(song14, nullptr);
    EXPECT_EQ(song14->getComparatorOutput(), 14);
}

TEST_F(JukeboxSongsTest, GetSongByComparatorOutput_Zero_ReturnsNullptr)
{
    const JukeboxSong* song = JukeboxSongs::getSongByComparatorOutput(0);
    EXPECT_EQ(song, nullptr);
}

TEST_F(JukeboxSongsTest, GetSongBySoundEvent_FindsCorrectSong)
{
    const JukeboxSong* song = JukeboxSongs::getSongBySoundEvent(SoundEvents::MUSIC_DISC_13);
    ASSERT_NE(song, nullptr);
    EXPECT_EQ(song->getComparatorOutput(), 1);
    EXPECT_FLOAT_EQ(song->getLengthInSeconds(), 178.0f);
}

TEST_F(JukeboxSongsTest, GetSongBySoundEvent_AllDiscsMatch)
{
    // 验证每个 SoundEvents 常量都能找到对应的 JukeboxSong
    struct DiscInfo {
        const ResourceLocation& soundEvent;
        i32 expectedOutput;
        f32 expectedLength;
    };

    const DiscInfo discs[] = {
        {SoundEvents::MUSIC_DISC_13, 1, 178.0f},
        {SoundEvents::MUSIC_DISC_CAT, 2, 185.0f},
        {SoundEvents::MUSIC_DISC_BLOCKS, 3, 345.0f},
        {SoundEvents::MUSIC_DISC_CHIRP, 4, 185.0f},
        {SoundEvents::MUSIC_DISC_FAR, 5, 174.0f},
        {SoundEvents::MUSIC_DISC_MALL, 6, 197.0f},
        {SoundEvents::MUSIC_DISC_MELLOHI, 7, 96.0f},
        {SoundEvents::MUSIC_DISC_STAL, 8, 150.0f},
        {SoundEvents::MUSIC_DISC_STRAD, 9, 188.0f},
        {SoundEvents::MUSIC_DISC_WARD, 10, 251.0f},
        {SoundEvents::MUSIC_DISC_11, 11, 71.0f},
        {SoundEvents::MUSIC_DISC_WAIT, 12, 238.0f},
        {SoundEvents::MUSIC_DISC_PIGSTEP, 13, 149.0f},
        {SoundEvents::MUSIC_DISC_OTHERSIDE, 14, 195.0f},
        {SoundEvents::MUSIC_DISC_5, 15, 178.0f},
        {SoundEvents::MUSIC_DISC_RELIC, 14, 218.0f},
        {SoundEvents::MUSIC_DISC_PRECIPICE, 13, 299.0f},
        {SoundEvents::MUSIC_DISC_CREATOR, 12, 176.0f},
        {SoundEvents::MUSIC_DISC_CREATOR_MUSIC_BOX, 11, 73.0f},
        {SoundEvents::MUSIC_DISC_TEARS, 10, 175.0f},
        {SoundEvents::MUSIC_DISC_LAVA_CHICKEN, 9, 134.0f},
    };

    for (const auto& disc : discs) {
        const JukeboxSong* song = JukeboxSongs::getSongBySoundEvent(disc.soundEvent);
        ASSERT_NE(song, nullptr) << "No song found for sound event";
        EXPECT_EQ(song->getComparatorOutput(), disc.expectedOutput);
        EXPECT_FLOAT_EQ(song->getLengthInSeconds(), disc.expectedLength);
    }
}

TEST_F(JukeboxSongsTest, GetSongBySoundEvent_Nonexistent_ReturnsNullptr)
{
    const JukeboxSong* song = JukeboxSongs::getSongBySoundEvent(ResourceLocation("minecraft:nonexistent"));
    EXPECT_EQ(song, nullptr);
}

// ========== JukeboxSongPlayer 测试 ==========

class MockSongChangedCallback {
public:
    void operator()() { callCount++; }
    int callCount = 0;
};

class JukeboxSongPlayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        pos_ = BlockPos(5, 64, 10);
        callback_ = std::make_shared<MockSongChangedCallback>();
        player_ = std::make_unique<JukeboxSongPlayer>([cb = callback_]() { (*cb)(); }, pos_);
    }

    BlockPos pos_;
    std::shared_ptr<MockSongChangedCallback> callback_;
    std::unique_ptr<JukeboxSongPlayer> player_;
};

// 注意：JukeboxSongPlayer 的 play/stop/tick 方法需要 IWorld&，
// 在单元测试中无法直接调用（需要创建完整的世界存根）。
// 因此这里只测试不需要 IWorld 的方法。

TEST_F(JukeboxSongPlayerTest, InitialState_NotPlaying)
{
    EXPECT_FALSE(player_->isPlaying());
    EXPECT_EQ(player_->getSong(), nullptr);
    EXPECT_EQ(player_->getTicksSinceSongStarted(), 0);
}

TEST_F(JukeboxSongPlayerTest, SetSongWithoutPlaying_SetsSong)
{
    JukeboxSong song(ResourceLocation("minecraft:test"), "test", 100.0f, 1);

    player_->setSongWithoutPlaying(song, 50);

    EXPECT_TRUE(player_->isPlaying());
    EXPECT_NE(player_->getSong(), nullptr);
    EXPECT_EQ(player_->getTicksSinceSongStarted(), 50);
}

TEST_F(JukeboxSongPlayerTest, SetSongWithoutPlaying_DoesNotSetIfFinished)
{
    // 歌曲长度 = 100*20 = 2000 tick，加上 20 tick 填充 = 2020 tick 后完成
    JukeboxSong song(ResourceLocation("minecraft:test"), "test", 100.0f, 1);

    // 设置已完成的 tick 数
    player_->setSongWithoutPlaying(song, 2020);

    // 歌曲已播放完毕，不应设置
    EXPECT_FALSE(player_->isPlaying());
    EXPECT_EQ(player_->getSong(), nullptr);
}

TEST_F(JukeboxSongPlayerTest, SetSongWithoutPlaying_SetsIfJustBeforeEnd)
{
    // 歌曲长度 = 100*20 = 2000 tick，加上 20 tick 填充 = 2020 tick 后完成
    JukeboxSong song(ResourceLocation("minecraft:test"), "test", 100.0f, 1);

    // 2019 tick 时歌曲尚未完成（边界测试）
    player_->setSongWithoutPlaying(song, 2019);

    EXPECT_TRUE(player_->isPlaying());
    EXPECT_EQ(player_->getTicksSinceSongStarted(), 2019);
}

TEST_F(JukeboxSongPlayerTest, ShouldEmitJukeboxPlayingEvent_Every20Ticks)
{
    // shouldEmitJukeboxPlayingEvent 是私有方法，但我们可以间接测试
    // 通过检查 PLAY_EVENT_INTERVAL_TICKS 常量
    EXPECT_EQ(JukeboxSongPlayer::PLAY_EVENT_INTERVAL_TICKS, 20);
}

} // namespace
