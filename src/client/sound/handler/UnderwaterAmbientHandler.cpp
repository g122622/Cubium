#include "client/sound/handler/UnderwaterAmbientHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <chrono>

namespace mc::client::sound {

UnderwaterAmbientHandler::UnderwaterAmbientHandler()
    : m_rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
}

void UnderwaterAmbientHandler::tick(SoundEngine& engine) {
    // 只在水下播放附加音效
    // 参考: UnderwaterAmbientSoundHandler.tick() lines 19-33
    // 概率检查每tick都进行，无冷却延迟
    if (!m_isUnderwater) {
        return;
    }

    // 使用 float 随机数 [0.0, 1.0)
    f32 f = m_rng.nextFloat();

    // 概率阈值（累积概率）
    // 超稀有: f < 0.0001 (0.01%)
    // 稀有:   f < 0.001  (0.1%, 但排除超稀有后为 0.09%)
    // 普通:   f < 0.01   (1%, 但排除稀有后为 0.9%)

    if (f < 0.0001f) {
        // 超稀有音效: ambient.underwater.loop.additions.ultra_rare
        auto sound = std::make_unique<SoundInstance>(
            SoundInstance::createGlobal(
                ResourceLocation("minecraft:ambient.underwater.loop.additions.ultra_rare"),
                SoundCategory::Ambient,
                1.0f,
                1.0f
            )
        );
        engine.play(std::move(sound));
    } else if (f < 0.001f) {
        // 稀有音效: ambient.underwater.loop.additions.rare
        auto sound = std::make_unique<SoundInstance>(
            SoundInstance::createGlobal(
                ResourceLocation("minecraft:ambient.underwater.loop.additions.rare"),
                SoundCategory::Ambient,
                1.0f,
                1.0f
            )
        );
        engine.play(std::move(sound));
    } else if (f < 0.01f) {
        // 普通音效: ambient.underwater.loop.additions
        auto sound = std::make_unique<SoundInstance>(
            SoundInstance::createGlobal(
                ResourceLocation("minecraft:ambient.underwater.loop.additions"),
                SoundCategory::Ambient,
                1.0f,
                1.0f
            )
        );
        engine.play(std::move(sound));
    }
}

} // namespace mc::client::sound
