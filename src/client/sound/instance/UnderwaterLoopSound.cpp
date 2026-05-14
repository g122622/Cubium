#include "client/sound/instance/UnderwaterLoopSound.hpp"
#include "common/sound/SoundEvents.hpp"
#include <algorithm>

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
    // 参考: UnderwaterAmbientSounds.UnderWaterSound.tick()
    // 在水中时增加计数，不在水中时更快减少

    if (m_canSwim) {
        ++m_ticksInWater;
    } else {
        m_ticksInWater -= 2;
    }

    // MC 1.16.5: 先检查是否应该停止，再 clamp
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
