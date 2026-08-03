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

#include "ElytraSound.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundEvents.hpp"
#include <algorithm>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

ElytraSound::ElytraSound(const ClientEntity& player)
    : TickableSound(SoundEvents::ITEM_ELYTRA_FLYING,
          SoundCategory::Players,
          glm::vec3(player.x(), player.y(), player.z()),
          0.1f, // 初始音量
          1.0f, // 音调
          true, // 循环
          AttenuationType::Linear,
          16.0f // 衰减距离
          )
    , m_player(player)
{}

void ElytraSound::tick()
{
    ++m_time;

    // 检查玩家是否正在鞘翅飞行
    // 条件：玩家未被移除且（初始延迟未结束 或 正在鞍翅飞行）
    bool isFallFlying = m_player.isFallFlying();

    if (!m_player.isRemoved() && (m_time <= 20 || isFallFlying)) {
        // 更新位置
        setPosition(glm::vec3(m_player.x(), m_player.y(), m_player.z()));

        // 计算速度平方
        auto vel = m_player.velocity();
        f32 speedSquared = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;

        if (speedSquared >= 1.0e-7f) {
            // 音量与速度成正比，上限为 1.0
            f32 volume = std::clamp(speedSquared / 4.0f, 0.0f, 1.0f);
            setVolume(volume);
        } else {
            setVolume(0.0f);
        }

        // 渐入效果
        // 开始阶段，time < 20 时音量为 0
        // time 在 20-40 之间时音量逐渐增加
        if (m_time < 20) {
            setVolume(0.0f);
        } else if (m_time < 40) {
            f32 currentVolume = getVolume();
            f32 fadeIn = static_cast<f32>(m_time - 20) / 20.0f;
            setVolume(currentVolume * fadeIn);
        }

        // 音调计算
        // 音量较高时提升音调
        f32 volume = getVolume();
        if (volume > 0.8f) {
            setPitch(1.0f + (volume - 0.8f));
        } else {
            setPitch(1.0f);
        }
    } else {
        // 不再飞行或玩家被移除，停止播放
        markDone();
    }
}

} // namespace mc::client::sound
