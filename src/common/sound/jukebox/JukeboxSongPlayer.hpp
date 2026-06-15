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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "JukeboxSong.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>

namespace mc {

class IWorld;

/**
 * @brief 唱片机歌曲播放器
 *
 * 管理唱片机中歌曲的播放状态，包括：
 * - 播放/停止歌曲
 * - 跟踪播放进度
 * - 自动检测歌曲播放完毕并停止
 * - 每20tick触发音符粒子和游戏事件
 *
 * 参考: net.minecraft.world.item.JukeboxSongPlayer
 */
class JukeboxSongPlayer {
public:
    /**
     * @brief 歌曲变化回调类型
     *
     * 当歌曲开始播放或停止播放时触发，用于通知外部状态更新
     * （如更新方块状态、标记方块实体为脏数据等）。
     */
    using OnSongChanged = std::function<void()>;

    /**
     * @brief 构造歌曲播放器
     * @param onSongChanged 歌曲变化回调
     * @param blockPos 唱片机方块位置
     */
    JukeboxSongPlayer(OnSongChanged onSongChanged, BlockPos blockPos);

    ~JukeboxSongPlayer() = default;

    // ========== 播放控制 ==========

    /**
     * @brief 开始播放歌曲
     *
     * 设置当前歌曲，重置播放进度，并广播播放事件给所有客户端。
     * 触发 onSongChanged 回调。
     *
     * @param world 世界引用
     * @param song 要播放的歌曲
     */
    void play(IWorld& world, const JukeboxSong& song);

    /**
     * @brief 停止播放歌曲
     *
     * 如果当前正在播放歌曲，则停止播放，广播停止事件，
     * 并触发 onSongChanged 回调。
     *
     * @param world 世界引用
     */
    void stop(IWorld& world);

    /**
     * @brief 设置歌曲但不开始播放
     *
     * 用于从存档加载时恢复播放状态。只有当歌曲尚未播放完毕时
     * 才会设置歌曲和进度。
     *
     * @param song 歌曲
     * @param ticksSinceSongStarted 已播放的tick数
     */
    void setSongWithoutPlaying(const JukeboxSong& song, i64 ticksSinceSongStarted);

    // ========== 状态查询 ==========

    /**
     * @brief 是否正在播放
     * @return 如果当前正在播放歌曲返回 true
     */
    [[nodiscard]] bool isPlaying() const noexcept { return m_song != nullptr; }

    /**
     * @brief 获取当前歌曲
     * @return 当前歌曲指针，如果未在播放返回 nullptr
     */
    [[nodiscard]] const JukeboxSong* getSong() const noexcept { return m_song; }

    /**
     * @brief 获取歌曲开始后经过的tick数
     * @return 经过的tick数
     */
    [[nodiscard]] i64 getTicksSinceSongStarted() const noexcept { return m_ticksSinceSongStarted; }

    // ========== Tick 更新 ==========

    /**
     * @brief 每tick调用
     *
     * 检查歌曲是否播放完毕，如果完毕则自动停止。
     * 每20tick（1秒）触发音符粒子效果。
     *
     * @param world 世界引用
     */
    void tick(IWorld& world);

    /// 播放事件间隔（tick）
    static constexpr i32 PLAY_EVENT_INTERVAL_TICKS = 20;

private:
    /**
     * @brief 检查是否应该触发音符粒子和游戏事件
     * @return 如果应该触发返回 true
     */
    [[nodiscard]] bool shouldEmitJukeboxPlayingEvent() const;

    /**
     * @brief 生成音符粒子
     * @param world 世界引用
     */
    void spawnMusicParticles(IWorld& world);

    const JukeboxSong* m_song = nullptr; ///< 当前播放的歌曲（非拥有指针）
    i64 m_ticksSinceSongStarted = 0;     ///< 歌曲开始后经过的tick数
    BlockPos m_blockPos;                 ///< 唱片机方块位置
    OnSongChanged m_onSongChanged;       ///< 歌曲变化回调
};

} // namespace mc
