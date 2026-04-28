#include "client/sound/handler/BiomeAmbientHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include <cmath>

namespace mc::client::sound {

BiomeAmbientHandler::BiomeAmbientHandler()
    : m_rng(std::random_device{}())
{
}

void BiomeAmbientHandler::tick(SoundEngine& engine) {
    // 获取当前群系的环境音效配置
    const Biome& biome = BiomeRegistry::instance().get(static_cast<BiomeId>(m_currentBiomeId));
    const world::biome::BiomeAmbientSounds& ambientSounds = biome.ambientSounds();

    // === 1. 处理附加音效 (Additions Sound) ===
    // 参考: BiomeSoundHandler.tick() lines 64-69
    // 每tick检查概率
    std::optional<world::biome::SoundAdditionsAmbience> additionsOpt = ambientSounds.additionsSound();
    if (additionsOpt.has_value()) {
        const world::biome::SoundAdditionsAmbience& additions = additionsOpt.value();
        std::uniform_real_distribution<f64> dist(0.0, 1.0);
        if (dist(m_rng) < additions.tickChance()) {
            SoundInstance sound = SoundInstance::createGlobal(
                additions.soundEvent(),
                SoundCategory::Ambient,
                1.0f,
                1.0f
            );
            engine.play(std::make_unique<SoundInstance>(std::move(sound)));
        }
    }

    // === 2. 处理心境音效 (Mood Sound) ===
    // 参考: BiomeSoundHandler.tick() lines 70-97
    // 心境音效在黑暗环境中触发
    std::optional<world::biome::MoodSoundAmbience> moodOpt = ambientSounds.moodSound();
    if (moodOpt.has_value()) {
        const world::biome::MoodSoundAmbience& mood = moodOpt.value();

        // 随机选择一个采样位置
        // 参考: block_search_extent 用于随机偏移范围
        i32 extent = mood.blockSearchExtent();
        std::uniform_int_distribution<i32> offsetDist(-extent, extent);

        // 计算光照影响
        // 参考: lines 82-88
        // 如果天空光 > 0，减少计时器
        // 如果天空光 = 0，根据方块光减少计时器
        if (m_skyLight > 0) {
            // 在有天空光的地方，计时器减少更快
            m_moodTimer -= static_cast<f32>(m_skyLight) / 15.0f * 0.001f;
        } else {
            // 在完全黑暗的地方，根据方块光减少
            // tick_delay (6000) 作为除数，越大则积累越慢
            m_moodTimer -= static_cast<f32>(m_blockLight - 1) / static_cast<f32>(mood.tickDelay());
        }

        // 计时器达到阈值时播放心境音效
        if (m_moodTimer >= 1.0f) {
            // 计算播放位置：在玩家附近随机位置，加上偏移
            f64 dx = static_cast<f64>(offsetDist(m_rng));
            f64 dy = static_cast<f64>(offsetDist(m_rng));
            f64 dz = static_cast<f64>(offsetDist(m_rng));

            // 计算距离并添加偏移
            f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

            // 如果距离太近，使用默认偏移
            if (distance < 0.001) {
                dx = 1.0;
                dy = 0.0;
                dz = 0.0;
                distance = 1.0;
            }

            f64 totalDistance = distance + mood.offset();

            // 归一化方向向量并计算最终位置
            f64 soundX = m_playerX + (dx / distance) * totalDistance;
            f64 soundY = m_playerY + (dy / distance) * totalDistance;
            f64 soundZ = m_playerZ + (dz / distance) * totalDistance;

            SoundInstance sound = SoundInstance::createLocated(
                mood.soundEvent(),
                SoundCategory::Ambient,
                static_cast<f32>(soundX),
                static_cast<f32>(soundY),
                static_cast<f32>(soundZ),
                1.0f,
                1.0f
            );
            engine.play(std::make_unique<SoundInstance>(std::move(sound)));

            // 重置计时器
            m_moodTimer = 0.0f;
        } else {
            // 确保计时器不会变成负数太多
            m_moodTimer = std::max(m_moodTimer, 0.0f);
        }
    }

    // === 3. 循环音效 (Loop Sound) ===
    // 循环音效需要特殊处理：需要 SoundEngine 支持 TickableSound 接口
    // 当前实现仅支持 Additions Sound 和 Mood Sound
    // 循环音效功能待后续 TickableSound 实现后补充
}

} // namespace mc::client::sound
