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

#pragma once

#include "JukeboxSong.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <unordered_map>

namespace mc {

/**
 * @brief 唱片机歌曲注册表
 *
 * 管理所有唱片机歌曲的定义。每首歌曲通过 ResourceLocation 标识，
 * 存储声音事件ID、描述翻译键、歌曲长度和比较器输出信号强度。
 *
 * 参考: net.minecraft.world.item.JukeboxSongs
 */
class JukeboxSongs {
public:
    /**
     * @brief 初始化注册表，注册所有原版唱片机歌曲
     *
     * 必须在使用任何查询方法之前调用。
     */
    static void initialize();

    /**
     * @brief 根据资源位置查找歌曲
     * @param id 歌曲的资源位置
     * @return 歌曲指针，如果未找到返回 nullptr
     */
    [[nodiscard]] static const JukeboxSong* getSong(const ResourceLocation& id);

    /**
     * @brief 根据比较器输出信号强度查找歌曲
     *
     * 注意：多个唱片可能共享相同的比较器输出值（如 otherside 和 relic 都是 14），
     * 此方法返回第一个匹配的歌曲。
     *
     * @param comparatorOutput 比较器输出信号强度 (1-15)
     * @return 歌曲指针，如果未找到返回 nullptr
     */
    [[nodiscard]] static const JukeboxSong* getSongByComparatorOutput(i32 comparatorOutput);

    /**
     * @brief 根据声音事件ID查找歌曲
     * @param soundEventId 声音事件的资源位置
     * @return 歌曲指针，如果未找到返回 nullptr
     */
    [[nodiscard]] static const JukeboxSong* getSongBySoundEvent(const ResourceLocation& soundEventId);

    /**
     * @brief 检查注册表是否已初始化
     * @return 如果已初始化返回 true
     */
    [[nodiscard]] static bool isInitialized() noexcept { return s_initialized; }

private:
    /**
     * @brief 注册一首歌曲
     * @param songId 歌曲ID (如 "minecraft:13")
     * @param soundEventId 声音事件ID (如 "minecraft:music_disc.13")
     * @param descriptionKey 描述翻译键 (如 "jukebox_song.minecraft.13")
     * @param lengthInSeconds 歌曲长度（秒）
     * @param comparatorOutput 比较器输出信号强度 (1-15)
     */
    static void registerSong(const ResourceLocation& songId,
        const ResourceLocation& soundEventId,
        std::string descriptionKey,
        f32 lengthInSeconds,
        i32 comparatorOutput);

    static bool s_initialized;

    /// 歌曲ID -> 歌曲定义的映射
    static std::unordered_map<ResourceLocation, JukeboxSong> s_songs;

    /// 声音事件ID -> 歌曲ID的映射（用于从MusicDiscItem查找歌曲）
    static std::unordered_map<ResourceLocation, ResourceLocation> s_soundEventToSongId;
};

} // namespace mc
