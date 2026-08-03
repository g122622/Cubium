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

#include "JukeboxSongs.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/sound/jukebox/JukeboxSong.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <string>
#include <unordered_map>
#include <utility>

namespace mc {

bool JukeboxSongs::s_initialized = false;
std::unordered_map<ResourceLocation, JukeboxSong> JukeboxSongs::s_songs;
std::unordered_map<ResourceLocation, ResourceLocation> JukeboxSongs::s_soundEventToSongId;

void JukeboxSongs::initialize()
{
    if (s_initialized) {
        return;
    }

    // 注册所有原版唱片机歌曲
    // 参考: net.minecraft.world.item.JukeboxSongs.bootstrap()
    // 参数: 歌曲ID, 声音事件ID, 描述翻译键, 歌曲长度(秒), 比较器输出(1-15)

    registerSong(ResourceLocation("minecraft:13"), SoundEvents::MUSIC_DISC_13, "jukebox_song.minecraft.13", 178.0f, 1);

    registerSong(
        ResourceLocation("minecraft:cat"), SoundEvents::MUSIC_DISC_CAT, "jukebox_song.minecraft.cat", 185.0f, 2);

    registerSong(ResourceLocation("minecraft:blocks"),
        SoundEvents::MUSIC_DISC_BLOCKS,
        "jukebox_song.minecraft.blocks",
        345.0f,
        3);

    registerSong(
        ResourceLocation("minecraft:chirp"), SoundEvents::MUSIC_DISC_CHIRP, "jukebox_song.minecraft.chirp", 185.0f, 4);

    registerSong(
        ResourceLocation("minecraft:far"), SoundEvents::MUSIC_DISC_FAR, "jukebox_song.minecraft.far", 174.0f, 5);

    registerSong(
        ResourceLocation("minecraft:mall"), SoundEvents::MUSIC_DISC_MALL, "jukebox_song.minecraft.mall", 197.0f, 6);

    registerSong(ResourceLocation("minecraft:mellohi"),
        SoundEvents::MUSIC_DISC_MELLOHI,
        "jukebox_song.minecraft.mellohi",
        96.0f,
        7);

    registerSong(
        ResourceLocation("minecraft:stal"), SoundEvents::MUSIC_DISC_STAL, "jukebox_song.minecraft.stal", 150.0f, 8);

    registerSong(
        ResourceLocation("minecraft:strad"), SoundEvents::MUSIC_DISC_STRAD, "jukebox_song.minecraft.strad", 188.0f, 9);

    registerSong(
        ResourceLocation("minecraft:ward"), SoundEvents::MUSIC_DISC_WARD, "jukebox_song.minecraft.ward", 251.0f, 10);

    registerSong(ResourceLocation("minecraft:11"), SoundEvents::MUSIC_DISC_11, "jukebox_song.minecraft.11", 71.0f, 11);

    registerSong(
        ResourceLocation("minecraft:wait"), SoundEvents::MUSIC_DISC_WAIT, "jukebox_song.minecraft.wait", 238.0f, 12);

    registerSong(ResourceLocation("minecraft:pigstep"),
        SoundEvents::MUSIC_DISC_PIGSTEP,
        "jukebox_song.minecraft.pigstep",
        149.0f,
        13);

    registerSong(ResourceLocation("minecraft:otherside"),
        SoundEvents::MUSIC_DISC_OTHERSIDE,
        "jukebox_song.minecraft.otherside",
        195.0f,
        14);

    registerSong(ResourceLocation("minecraft:5"), SoundEvents::MUSIC_DISC_5, "jukebox_song.minecraft.5", 178.0f, 15);

    registerSong(
        ResourceLocation("minecraft:relic"), SoundEvents::MUSIC_DISC_RELIC, "jukebox_song.minecraft.relic", 218.0f, 14);

    registerSong(ResourceLocation("minecraft:precipice"),
        SoundEvents::MUSIC_DISC_PRECIPICE,
        "jukebox_song.minecraft.precipice",
        299.0f,
        13);

    registerSong(ResourceLocation("minecraft:creator"),
        SoundEvents::MUSIC_DISC_CREATOR,
        "jukebox_song.minecraft.creator",
        176.0f,
        12);

    registerSong(ResourceLocation("minecraft:creator_music_box"),
        SoundEvents::MUSIC_DISC_CREATOR_MUSIC_BOX,
        "jukebox_song.minecraft.creator_music_box",
        73.0f,
        11);

    registerSong(
        ResourceLocation("minecraft:tears"), SoundEvents::MUSIC_DISC_TEARS, "jukebox_song.minecraft.tears", 175.0f, 10);

    registerSong(ResourceLocation("minecraft:lava_chicken"),
        SoundEvents::MUSIC_DISC_LAVA_CHICKEN,
        "jukebox_song.minecraft.lava_chicken",
        134.0f,
        9);

    s_initialized = true;
}

const JukeboxSong* JukeboxSongs::getSong(const ResourceLocation& id)
{
    auto it = s_songs.find(id);
    return it != s_songs.end() ? &it->second : nullptr;
}

const JukeboxSong* JukeboxSongs::getSongByComparatorOutput(i32 comparatorOutput)
{
    for (const auto& [id, song] : s_songs) {
        if (song.getComparatorOutput() == comparatorOutput) {
            return &song;
        }
    }
    return nullptr;
}

const JukeboxSong* JukeboxSongs::getSongBySoundEvent(const ResourceLocation& soundEventId)
{
    auto it = s_soundEventToSongId.find(soundEventId);
    if (it == s_soundEventToSongId.end()) {
        return nullptr;
    }
    return getSong(it->second);
}

void JukeboxSongs::registerSong(const ResourceLocation& songId,
    const ResourceLocation& soundEventId,
    std::string descriptionKey,
    f32 lengthInSeconds,
    i32 comparatorOutput)
{
    MC_ASSERT_RELEASE(comparatorOutput >= 1 && comparatorOutput <= 15);
    MC_ASSERT_RELEASE(lengthInSeconds > 0.0f);

    auto [it, inserted] = s_songs.emplace(
        songId, JukeboxSong(soundEventId, std::move(descriptionKey), lengthInSeconds, comparatorOutput));

    MC_ASSERT_RELEASE(inserted);

    // 建立声音事件到歌曲的反向映射
    s_soundEventToSongId.emplace(soundEventId, songId);
}

} // namespace mc
