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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>

namespace mc {

/**
 * @brief 唱片机歌曲定义
 *
 * 存储一首唱片机歌曲的元数据，包括声音事件、描述翻译键、
 * 歌曲长度（秒）和比较器输出信号强度。
 *
 * 参考: net.minecraft.world.item.JukeboxSong
 */
class JukeboxSong {
public:
    /**
     * @brief 构造唱片机歌曲
     * @param soundEventId 声音事件ID (如 "minecraft:music_disc.13")
     * @param descriptionKey 描述翻译键 (如 "jukebox_song.minecraft.13")
     * @param lengthInSeconds 歌曲长度（秒）
     * @param comparatorOutput 比较器输出信号强度 (1-15)
     */
    JukeboxSong(ResourceLocation soundEventId, std::string descriptionKey, f32 lengthInSeconds, i32 comparatorOutput);

    ~JukeboxSong() = default;

    // ========== 访问器 ==========

    /**
     * @brief 获取声音事件ID
     * @return 声音事件的资源位置
     */
    [[nodiscard]] const ResourceLocation& getSoundEventId() const noexcept { return m_soundEventId; }

    /**
     * @brief 获取描述翻译键
     * @return 翻译键字符串
     */
    [[nodiscard]] const std::string& getDescriptionKey() const noexcept { return m_descriptionKey; }

    /**
     * @brief 获取歌曲长度（秒）
     * @return 歌曲长度
     */
    [[nodiscard]] f32 getLengthInSeconds() const noexcept { return m_lengthInSeconds; }

    /**
     * @brief 获取比较器输出信号强度
     * @return 信号强度 (1-15)
     */
    [[nodiscard]] i32 getComparatorOutput() const noexcept { return m_comparatorOutput; }

    // ========== 计算方法 ==========

    /**
     * @brief 计算歌曲长度（tick）
     *
     * 将秒数转换为游戏 tick，1 秒 = 20 tick，向上取整。
     *
     * @return 歌曲长度（tick）
     */
    [[nodiscard]] i32 lengthInTicks() const;

    /**
     * @brief 检查歌曲是否已播放完毕
     *
     * 当 ticksSinceSongStarted >= lengthInTicks() + SONG_END_PADDING_TICKS 时，
     * 认为歌曲播放完毕。额外的 20 tick (1秒) 填充确保音频完整播放。
     *
     * @param ticksSinceSongStarted 歌曲开始后经过的 tick 数
     * @return 如果歌曲已播放完毕返回 true
     */
    [[nodiscard]] bool hasFinished(i64 ticksSinceSongStarted) const;

    /// 歌曲结束后的填充 tick 数（1秒缓冲）
    static constexpr i32 SONG_END_PADDING_TICKS = 20;

private:
    ResourceLocation m_soundEventId; ///< 声音事件ID
    std::string m_descriptionKey;    ///< 描述翻译键
    f32 m_lengthInSeconds;           ///< 歌曲长度（秒）
    i32 m_comparatorOutput;          ///< 比较器输出信号强度 (1-15)
};

} // namespace mc
