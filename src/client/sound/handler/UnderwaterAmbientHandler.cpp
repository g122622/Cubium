#include "client/sound/handler/UnderwaterAmbientHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <random>

namespace mc::client::sound {

UnderwaterAmbientHandler::UnderwaterAmbientHandler()
    : m_rng(std::random_device{}())
{
}

void UnderwaterAmbientHandler::tick(SoundEngine& engine) {
    // 减少延迟计时器
    if (m_delayTimer > 0) {
        --m_delayTimer;
        return;
    }

    // 只在水下播放
    if (!m_isUnderwater) {
        return;
    }

    // 播放水下环境音效
    // 使用水下循环声音
    auto sound = std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(
            ResourceLocation("minecraft:ambient.underwater.loop"),
            SoundCategory::Ambient,
            1.0f,  // 正常音量
            1.0f   // 正常音调
        )
    );

    engine.play(std::move(sound));

    // 设置下次播放延迟（200-400 ticks，约10-20秒）
    std::uniform_int_distribution<u32> dist(200, 400);
    m_delayTimer = dist(m_rng);
}

} // namespace mc::client::sound
