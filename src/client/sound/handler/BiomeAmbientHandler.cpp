#include "client/sound/handler/BiomeAmbientHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"

#include <random>

namespace mc::client::sound {

BiomeAmbientHandler::BiomeAmbientHandler()
    : m_rng(std::random_device{}())
{
}

void BiomeAmbientHandler::tick(SoundEngine& engine) {
    // 减少延迟计时器
    if (m_delayTimer > 0) {
        --m_delayTimer;
        return;
    }

    // 检查是否需要播放新的环境音
    if (m_currentBiomeId == 0) {
        return;
    }

    // 获取群系环境音效
    auto soundLocation = getBiomeAmbientSound(m_currentBiomeId);
    if (!soundLocation.has_value()) {
        return;
    }

    // 播放环境音效
    auto sound = std::make_unique<SoundInstance>(
        SoundInstance::createGlobal(
            soundLocation.value(),
            SoundCategory::Ambient,
            0.5f,  // 较低音量
            1.0f   // 正常音调
        )
    );

    engine.play(std::move(sound));

    // 设置下次播放延迟（600-1200 ticks，约30-60秒）
    std::uniform_int_distribution<u32> dist(600, 1200);
    m_delayTimer = dist(m_rng);

    m_lastPlayedBiomeId = m_currentBiomeId;
}

Optional<ResourceLocation> BiomeAmbientHandler::getBiomeAmbientSound(u32 biomeId) {
    // TODO: 从群系注册表获取实际的环境音效
    // 当前返回一些示例环境音效

    // 基于群系ID返回不同的环境音效
    // 注意：这些是占位符，实际应该从群系配置读取
    switch (biomeId) {
        // 沼泽群系
        case 6:  // swamp
            return ResourceLocation("minecraft:ambient.cave");
        // 沙漠群系
        case 2:  // desert
            return ResourceLocation("minecraft:ambient.weather.rain");
        // 海洋群系
        case 0:  // ocean
            return ResourceLocation("minecraft:ambient.underwater.enter");
        // 下界群系
        default:
            if (biomeId >= 100 && biomeId < 200) {
                return ResourceLocation("minecraft:ambient.nether_wasteland.mood");
            }
            // 默认洞穴环境音
            return ResourceLocation("minecraft:ambient.cave");
    }
}

} // namespace mc::client::sound
