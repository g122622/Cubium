#include "../ClientApplication.hpp"

#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"

#include <algorithm>
#include <cmath>

namespace mc::client {

Result<void> ClientApplication::initializeAudio()
{
    MC_TRACE_EVENT("client.initialization", "InitializeSoundSystem");

    spdlog::info("Initializing sound system...");

    // 添加内部资源包到 ResourcePackList（用于 sounds.json）
    // 内部资源包具有最低优先级（-1），外部资源包可以覆盖
    auto builtinPackResult = m_resourcePackList.addPack(
        std::filesystem::path("resources/data/minecraft"), true, -1);
    if (builtinPackResult.success() && builtinPackResult.value().initialized) {
        spdlog::info("Added built-in resources pack to sound system");
    } else {
        spdlog::warn("Failed to add built-in resources pack: {}",
                     builtinPackResult.success() ? builtinPackResult.value().error : builtinPackResult.error().toString());
    }

    m_audioService = std::make_unique<sound::AudioService>(m_resourcePackList, m_settings);

    auto soundInitResult = m_audioService->initialize();
    if (soundInitResult.failed()) {
        spdlog::warn("Failed to initialize sound engine: {}. Audio will be disabled.",
                    soundInitResult.error().toString());
        m_audioService.reset();
    } else {
        // 添加环境音效处理器
        spdlog::info("Sound system initialized successfully");
    }

    return Result<void>::ok();
}

void ClientApplication::updatePlayerAudio()
{
    if (!m_player) {
        return;
    }

    // 处理脚步声和游泳声
    // updateMoveDistance 在 Player::updatePhysics 中调用
    // 这里检查是否需要播放音频
    if (m_audioService && !m_player->isSilent()) {
        // 澶勭悊娓告吵澹?
        if (m_player->shouldPlaySwimSound()) {
            auto swimSound = std::make_unique<sound::SoundInstance>(
                sound::SoundInstance::createLocated(
                    ResourceLocation("minecraft:entity.player.swim"),
                    sound::SoundCategory::Players,
                    m_player->x(), m_player->y(), m_player->z(),
                    m_player->swimSoundVolume() * 0.15f,  // MC 音量系数
                    1.0f + (m_random.nextFloat() - m_random.nextFloat()) * 0.4f  // 随机音调变化
                )
            );
            m_audioService->play(std::move(swimSound));
        }
        // 澶勭悊鑴氭澹?
        else if (m_player->shouldPlayStepSound()) {
            // 获取脚下方块的 BlockState 来选择正确的声音
            const auto* blockState = m_world.getBlockState(
                m_player->stepSoundPos().x,
                m_player->stepSoundPos().y,
                m_player->stepSoundPos().z
            );

            if (blockState) {
                const auto& soundType = blockState->getSoundType();
                const auto& stepSoundId = soundType.getStepSound();

                auto stepSound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(
                        stepSoundId,
                        sound::SoundCategory::Players,
                        m_player->x(), m_player->y(), m_player->z(),
                        soundType.getVolume() * 0.15f,  // MC 音量系数
                        soundType.getPitch() * (0.8f + m_random.nextFloat() * 0.4f)  // 随机音调变化
                    )
                );
                m_audioService->play(std::move(stepSound));
            } else {
                // 榛樿浣跨敤鐭冲ご鑴氭澹?
                auto stepSound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(
                        ResourceLocation("minecraft:block.stone.step"),
                        sound::SoundCategory::Players,
                        m_player->x(), m_player->y(), m_player->z(),
                        0.15f,
                        1.0f + (m_random.nextFloat() - m_random.nextFloat()) * 0.4f
                    )
                );
                m_audioService->play(std::move(stepSound));
            }
        }
    }

    // 更新声音系统听者位置
    if (m_audioService) {
        m_audioService->updateListener(
            m_camera.position(),
            m_camera.forward(),
            m_camera.up()
        );
    }
}

void ClientApplication::updateWorldAudio()
{
    if (!m_renderer || !m_player) {
        return;
    }

    bool inWater = m_player->isInWater();
    bool inLava = m_player->isInLava();
    u32 waterFogColor = world::biome::BiomeEffects::DEFAULT_WATER_FOG_COLOR;

    // 获取当前生物群系的水下雾颜色
    if (inWater) {
        const auto* biome = m_world.getBiomeAtBlock(
            static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
            static_cast<i32>(std::floor(m_player->z()))
        );
        if (biome) {
            waterFogColor = biome->waterFogColor();
        }
    }

    m_renderer->updateLiquidState(inWater, inLava, waterFogColor);

    // 入水/出水音效触发
    if (m_audioService && inWater && !m_wasPlayerInWater) {
        MC_TRACE_INSTANT("client.entity", "EnterWater");
        // 入水音效
        auto enterSound = std::make_unique<sound::SoundInstance>(
            sound::SoundInstance::createGlobal(
                ResourceLocation("minecraft:ambient.underwater.enter"),
                sound::SoundCategory::Ambient,
                1.0f,  // 音量
                1.0f   // 音调
            )
        );
        m_audioService->play(std::move(enterSound));
    } else if (m_audioService && !inWater && m_wasPlayerInWater) {
        // 出水音效
        auto exitSound = std::make_unique<sound::SoundInstance>(
            sound::SoundInstance::createGlobal(
                ResourceLocation("minecraft:ambient.underwater.exit"),
                sound::SoundCategory::Ambient,
                1.0f,  // 音量
                1.0f   // 音调
            )
        );
        m_audioService->play(std::move(exitSound));
    }

    // 更新水下环境音效处理器
    if (m_audioService) {
        const auto* biome = m_world.getBiomeAtBlock(
            static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
            static_cast<i32>(std::floor(m_player->z()))
        );
        m_audioService->setBiomeId(biome ? static_cast<u32>(biome->id()) : 0u);
        m_audioService->setUnderwater(inWater);
    }

    m_wasPlayerInWater = inWater;
    m_wasPlayerInLava = inLava;
}

void ClientApplication::updateAudioPauseState()
{
    if (!m_audioService) {
        return;
    }

    // 检查是否暂停（游戏暂停时不更新声音）
    bool isPaused = !m_mouseCaptured;  // 鼠标未捕获时认为游戏暂停
    m_audioService->setPaused(isPaused);
}

void ClientApplication::shutdownAudio()
{
    if (!m_audioService) {
        return;
    }

    m_audioService->shutdown();
    m_audioService.reset();
}

} // namespace mc::client