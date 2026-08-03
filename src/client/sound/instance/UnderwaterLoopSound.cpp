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

#include "client/sound/instance/UnderwaterLoopSound.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundEvents.hpp"
#include <algorithm>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

UnderwaterLoopSound::UnderwaterLoopSound()
    : TickableSound(SoundEvents::AMBIENT_UNDERWATER_LOOP,
          SoundCategory::Ambient,
          glm::vec3{0.0f, 0.0f, 0.0f}, // 位置不重要，全局声音
          0.0f,                        // 初始音量为0
          1.0f,                        // 音调
          true,                        // 循环
          AttenuationType::None,       // 无衰减（全局声音）
          16.0f                        // 衰减距离（无意义，全局声音）
      )
{}

void UnderwaterLoopSound::tick()
{
    // 在水中时增加计数，不在水中时更快减少

    if (m_canSwim) {
        ++m_ticksInWater;
    } else {
        m_ticksInWater -= 2;
    }

    // 先检查是否应该停止，再 clamp
    // 当计数器变为负数时停止声音
    if (m_ticksInWater < 0) {
        markDone();
        return;
    }

    // 限制在 [0, FADE_TICKS] 范围内
    m_ticksInWater = std::clamp(m_ticksInWater, 0, FADE_TICKS);

    // 计算音量：ticksInWater / FADE_TICKS
    // 音量范围 [0.0, 1.0]
    f32 volume = static_cast<f32>(m_ticksInWater) / static_cast<f32>(FADE_TICKS);
    volume = std::clamp(volume, 0.0f, 1.0f);
    setVolume(volume);
}

} // namespace mc::client::sound
