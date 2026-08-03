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

#include "client/sound/handler/BiomeAmbientHandler.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeAmbientSounds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <utility>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

namespace {

// ============================================================================
// BiomeLoopSound - 群系循环音效
// ============================================================================

/**
 * @brief 群系循环音效
 *
 * 循环音效会持续播放，带有淡入淡出效果。
 * 当群系切换时，旧群系的循环音效淡出，新群系的循环音效淡入。
 */
class BiomeLoopSound : public TickableSound {
public:
    explicit BiomeLoopSound(const ResourceLocation& soundEvent)
        : TickableSound(
              soundEvent, SoundCategory::Ambient, glm::vec3(0.0f), 0.0f, 1.0f, true, AttenuationType::None, 16.0f)
        , m_fadeDirection(0) // 开始时静音
        , m_fadeTicks(0)
    {
        // AttenuationType::None 使声音成为全局声音
    }

    void tick() override
    {
        // 淡入淡出逻辑
        if (m_fadeDirection < 0) {
            // 淡出
            m_fadeTicks += m_fadeDirection;
            if (m_fadeTicks <= -40) {
                markDone();
                return;
            }
        } else {
            // 淡入
            m_fadeTicks += m_fadeDirection;
        }

        // 音量 = clamp(fadeTicks / 40.0, 0.0, 1.0)
        f32 volume = std::clamp(static_cast<f32>(m_fadeTicks) / 40.0f, 0.0f, 1.0f);
        setVolume(volume);
    }

    [[nodiscard]] bool canBeSilent() const override { return true; }

    /**
     * @brief 开始淡出
     */
    void startFadeOut()
    {
        m_fadeDirection = -1;
        m_fadeTicks = std::min(m_fadeTicks, 40); // 限制最大淡出起始点
    }

    /**
     * @brief 开始淡入
     */
    void startFadeIn()
    {
        m_fadeDirection = 1;
        m_fadeTicks = std::max(m_fadeTicks, 0); // 确保从非负开始
    }

private:
    i32 m_fadeDirection; // 1 = 淡入, -1 = 淡出
    i32 m_fadeTicks;     // 淡入淡出计数器
};

} // anonymous namespace

// ============================================================================
// BiomeAmbientHandler 实现
// ============================================================================

BiomeAmbientHandler::BiomeAmbientHandler()
    : m_rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()))
{}

BiomeAmbientHandler::~BiomeAmbientHandler() = default;

void BiomeAmbientHandler::tick(SoundEngine& engine)
{
    // 清理已完成的循环音效
    for (auto it = m_loopSounds.begin(); it != m_loopSounds.end();) {
        if (!engine.isPlaying(it->second)) {
            it = m_loopSounds.erase(it);
        } else {
            ++it;
        }
    }

    // 获取当前群系的环境音效配置
    const Biome& biome = BiomeRegistry::instance().get(static_cast<BiomeId>(m_currentBiomeId));
    const world::biome::BiomeAmbientSounds& ambientSounds = biome.ambientSounds();

    // 检查群系是否变化
    if (static_cast<u32>(biome.id()) != m_previousBiomeId) {
        m_previousBiomeId = static_cast<u32>(biome.id());

        // 群系变化，淡出所有现有循环音效
        for (auto& [biomeId, soundId] : m_loopSounds) {
            ISoundInstance* sound = engine.getSoundInstance(soundId);
            if (sound) {
                auto* loopSound = dynamic_cast<BiomeLoopSound*>(sound);
                if (loopSound) {
                    loopSound->startFadeOut();
                }
            }
        }

        // 更新心境音效和附加音效配置
        m_currentMoodSound = ambientSounds.moodSound();
        m_currentAdditionsSound = ambientSounds.additionsSound();

        // 如果新群系有循环音效，创建并开始淡入
        if (ambientSounds.loopSound().has_value()) {
            auto sound = std::make_unique<BiomeLoopSound>(ambientSounds.loopSound().value());
            sound->startFadeIn();
            SoundInstanceId soundId = engine.play(std::move(sound));
            if (soundId != 0) {
                m_loopSounds[m_currentBiomeId] = soundId;
            }
        }
    }

    // === 1. 处理循环音效 (Loop Sound) ===
    // 检查是否有循环音效需要淡入
    if (ambientSounds.loopSound().has_value()) {
        auto it = m_loopSounds.find(m_currentBiomeId);
        if (it == m_loopSounds.end() || !engine.isPlaying(it->second)) {
            // 创建新的循环音效
            auto sound = std::make_unique<BiomeLoopSound>(ambientSounds.loopSound().value());
            sound->startFadeIn();
            SoundInstanceId soundId = engine.play(std::move(sound));
            if (soundId != 0) {
                m_loopSounds[m_currentBiomeId] = soundId;
            }
        } else {
            // 已存在，确保正在淡入
            ISoundInstance* sound = engine.getSoundInstance(it->second);
            if (sound) {
                auto* loopSound = dynamic_cast<BiomeLoopSound*>(sound);
                if (loopSound) {
                    loopSound->startFadeIn();
                }
            }
        }
    }

    // === 2. 处理附加音效 (Additions Sound) ===
    // 每tick检查概率
    if (m_currentAdditionsSound.has_value()) {
        const world::biome::SoundAdditionsAmbience& additions = m_currentAdditionsSound.value();
        if (m_rng.nextDouble() < additions.tickChance()) {
            SoundInstance sound =
                SoundInstance::createGlobal(additions.soundEvent(), SoundCategory::Ambient, 1.0f, 1.0f);
            engine.play(std::make_unique<SoundInstance>(std::move(sound)));
        }
    }

    // === 3. 处理心境音效 (Mood Sound) ===
    if (m_currentMoodSound.has_value()) {
        const world::biome::MoodSoundAmbience& mood = m_currentMoodSound.value();

        // 使用主线程已采样的心境位置（与光照采样位置一致，对齐 MC 原版
        // BiomeAmbientSoundsHandler.tick() 在同一 tick 中用同一 blockpos
        // 同时查询光照和计算声音播放位置的行为）
        const i32 bx = m_moodBx;
        const i32 by = m_moodBy;
        const i32 bz = m_moodBz;

        // 心境计时器逻辑
        // 使用采样位置的光照，而非玩家位置的光照（与 MC 原版一致）
        if (m_moodSkyLight > 0) {
            // 在有天空光的地方，计时器减少
            m_moodTimer -= static_cast<f32>(m_moodSkyLight) / static_cast<f32>(game::MAX_LIGHT_LEVEL) * 0.001f;
        } else {
            // 在完全黑暗的地方，根据方块光调整
            if (m_moodBlockLight > 0) {
                // 有方块光，计时器减少
                m_moodTimer -= static_cast<f32>(m_moodBlockLight - 1) / static_cast<f32>(mood.tickDelay());
            } else {
                // 完全黑暗，计时器增加
                m_moodTimer += 1.0f / static_cast<f32>(mood.tickDelay());
            }
        }

        // 计时器达到阈值时播放心境音效
        if (m_moodTimer >= 1.0f) {
            // 计算播放位置
            f64 dx = static_cast<f64>(bx) + 0.5 - m_playerX;
            f64 dy = static_cast<f64>(by) + 0.5 - m_playerY;
            f64 dz = static_cast<f64>(bz) + 0.5 - m_playerZ;
            f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

            // 如果距离太近，使用默认方向
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

            SoundInstance sound = SoundInstance::createLocated(mood.soundEvent(),
                SoundCategory::Ambient,
                static_cast<f32>(soundX),
                static_cast<f32>(soundY),
                static_cast<f32>(soundZ),
                1.0f,
                1.0f);
            engine.play(std::make_unique<SoundInstance>(std::move(sound)));

            // 重置计时器
            m_moodTimer = 0.0f;
        } else {
            // 确保计时器不会变成负数太多
            m_moodTimer = std::max(m_moodTimer, 0.0f);
        }
    }
}

void BiomeAmbientHandler::stopAll()
{
    m_loopSounds.clear();
}

} // namespace mc::client::sound
