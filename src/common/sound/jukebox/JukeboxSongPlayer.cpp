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

#include "JukeboxSongPlayer.hpp"

#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/jukebox/JukeboxSong.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <utility>

namespace mc {

JukeboxSongPlayer::JukeboxSongPlayer(OnSongChanged onSongChanged, BlockPos blockPos)
    : m_blockPos(blockPos)
    , m_onSongChanged(std::move(onSongChanged))
{}

void JukeboxSongPlayer::play(IWorld& world, const JukeboxSong& song)
{
    m_song = &song;
    m_ticksSinceSongStarted = 0;

    // 广播播放事件：data 为比较器输出信号强度
    // 参考 MC 1.21.11: JukeboxSongPlayer.play() 使用注册表ID，
    // 我们的实现使用比较器输出信号强度，客户端根据此值确定播放哪首曲目
    world.playEvent(world::WorldEvents::PLAY_RECORD_SOUND, m_blockPos, song.getComparatorOutput());

    if (m_onSongChanged) {
        m_onSongChanged();
    }
}

void JukeboxSongPlayer::stop(IWorld& world)
{
    if (m_song == nullptr) {
        return;
    }

    m_song = nullptr;
    m_ticksSinceSongStarted = 0;

    // 触发 JUKEBOX_STOP_PLAY 游戏事件，通知附近的幽匿感测体
    // 参考 MC: JukeboxSongPlayer.stop() 中先触发 gameEvent 再触发 levelEvent
    world.gameEvent(gameevent::GameEvents::JUKEBOX_STOP_PLAY, m_blockPos, nullptr);

    // 广播停止事件
    world.playEvent(world::WorldEvents::STOP_RECORD_SOUND, m_blockPos, 0);

    if (m_onSongChanged) {
        m_onSongChanged();
    }
}

void JukeboxSongPlayer::setSongWithoutPlaying(const JukeboxSong& song, i64 ticksSinceSongStarted)
{
    // 只有当歌曲尚未播放完毕时才恢复播放状态
    if (!song.hasFinished(ticksSinceSongStarted)) {
        m_song = &song;
        m_ticksSinceSongStarted = ticksSinceSongStarted;
    }
}

void JukeboxSongPlayer::tick(IWorld& world)
{
    if (m_song == nullptr) {
        return;
    }

    // 检查歌曲是否播放完毕
    if (m_song->hasFinished(m_ticksSinceSongStarted)) {
        stop(world);
        return;
    }

    // 每20tick（1秒）触发音符粒子效果和游戏事件
    // 参考 MC 1.21.11: JukeboxSongPlayer.tick()
    if (shouldEmitJukeboxPlayingEvent()) {
        // 触发 JUKEBOX_PLAY 游戏事件，通知附近的幽匿感测体
        // 参考 MC: JukeboxSongPlayer.tick() 中每 20 tick 触发 gameEvent(JUKEBOX_PLAY)
        world.gameEvent(gameevent::GameEvents::JUKEBOX_PLAY, m_blockPos, nullptr);

        spawnMusicParticles(world);
    }

    ++m_ticksSinceSongStarted;
}

bool JukeboxSongPlayer::shouldEmitJukeboxPlayingEvent() const
{
    return m_ticksSinceSongStarted % PLAY_EVENT_INTERVAL_TICKS == 0;
}

void JukeboxSongPlayer::spawnMusicParticles(IWorld& world)
{
    // 参考 MC 1.21.11: JukeboxSongPlayer.spawnMusicParticles()
    // 生成音符粒子，位置在唱片机上方1.2格
    // 粒子颜色由随机值（0-3）/24 决定
    const f32 x = static_cast<f32>(m_blockPos.x) + 0.5f;
    const f32 y = static_cast<f32>(m_blockPos.y) + 1.2f;
    const f32 z = static_cast<f32>(m_blockPos.z) + 0.5f;

    // 使用世界随机数生成音符颜色，与 MC 原版一致
    // MC 原版: world.random.nextInt(4) / 24.0f
    const f32 colorData = static_cast<f32>(world.getRandom().nextInt(4)) / 24.0f;

    world.addParticle(particle::ParticleTypeId::Note, Vector3(x, y, z), Vector3(colorData, 0.0f, 0.0f));
}

} // namespace mc
