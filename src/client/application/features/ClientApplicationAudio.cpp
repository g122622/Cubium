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

#include "client/application/ClientApplication.hpp"

#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/ocean/BubbleColumnBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <cmath>

using namespace mc::trace;

namespace mc::client {

Result<void> ClientApplication::initializeAudio()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeSoundSystem");

    spdlog::info("Initializing sound system...");

    m_audioService = std::make_unique<sound::AudioService>(m_resourcePackList, m_settings);

    auto soundInitResult = m_audioService->initialize();
    if (soundInitResult.failed()) {
        spdlog::warn(
            "Failed to initialize sound engine: {}. Audio will be disabled.", soundInitResult.error().toString());
        m_audioService.reset();
    } else {
        // 添加环境音效处理器
        spdlog::info("Sound system initialized successfully");

        // 设置 UI 音效回调，供 Widget::playUiSound() 使用
        ui::kagero::widget::Widget::setUiSoundCallback([this](const std::string& soundEventId) {
            if (m_audioService) {
                auto sound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createGlobal(ResourceLocation(soundEventId),
                        sound::SoundCategory::Master,
                        0.25f, // UI 按钮默认音量
                        1.0f   // 默认音调
                        ));
                m_audioService->play(std::move(sound));
            }
        });
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
        // 处理游泳声
        if (m_player->shouldPlaySwimSound()) {
            auto swimSound = std::make_unique<sound::SoundInstance>(
                sound::SoundInstance::createLocated(ResourceLocation("minecraft:entity.player.swim"),
                    sound::SoundCategory::Players,
                    m_player->x(),
                    m_player->y(),
                    m_player->z(),
                    m_player->swimSoundVolume() * 0.15f,                        // 音量系数
                    1.0f + (m_random.nextFloat() - m_random.nextFloat()) * 0.4f // 随机音调变化
                    ));
            m_audioService->play(std::move(swimSound));
        }
        // 处理脚步声
        else if (m_player->shouldPlayStepSound()) {
            // 获取脚下方块的 BlockState 来选择正确的声音
            const auto* blockState = m_world.getBlockState(
                m_player->stepSoundPos().x, m_player->stepSoundPos().y, m_player->stepSoundPos().z);

            if (blockState) {
                const auto& soundType = blockState->getSoundType();
                const auto& stepSoundId = soundType.getStepSound();

                auto stepSound = std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(stepSoundId,
                    sound::SoundCategory::Players,
                    m_player->x(),
                    m_player->y(),
                    m_player->z(),
                    soundType.getVolume() * 0.15f,                              // 音量系数
                    soundType.getPitch() * (0.8f + m_random.nextFloat() * 0.4f) // 随机音调变化
                    ));
                m_audioService->play(std::move(stepSound));
            } else {
                // 默认使用石头脚步声
                auto stepSound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(ResourceLocation("minecraft:block.stone.step"),
                        sound::SoundCategory::Players,
                        m_player->x(),
                        m_player->y(),
                        m_player->z(),
                        0.15f,
                        1.0f + (m_random.nextFloat() - m_random.nextFloat()) * 0.4f));
                m_audioService->play(std::move(stepSound));
            }
        }
    }

    // 更新声音系统听者位置
    if (m_audioService) {
        m_audioService->updateListener(m_camera.position(), m_camera.forward(), m_camera.up());
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

    // 玩家眼睛位置的方块坐标（多个音效查询共用）
    const i32 eyeBlockX = static_cast<i32>(std::floor(m_player->x()));
    const i32 eyeBlockY = static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight()));
    const i32 eyeBlockZ = static_cast<i32>(std::floor(m_player->z()));

    // 获取当前生物群系的水下雾颜色
    if (inWater) {
        const auto* biome = m_world.getBiomeAtBlock(eyeBlockX, eyeBlockY, eyeBlockZ);
        if (biome) {
            waterFogColor = biome->effects().waterFogColor();
        }
    }

    m_renderer->updateLiquidState(inWater, inLava, waterFogColor);

    // 入水/出水音效触发
    if (m_audioService && inWater && !m_wasPlayerInWater) {
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Entity, "EnterWater");
        // 入水音效
        auto enterSound = std::make_unique<sound::SoundInstance>(
            sound::SoundInstance::createGlobal(ResourceLocation("minecraft:ambient.underwater.enter"),
                sound::SoundCategory::Ambient,
                1.0f, // 音量
                1.0f  // 音调
                ));
        m_audioService->play(std::move(enterSound));
    } else if (m_audioService && !inWater && m_wasPlayerInWater) {
        // 出水音效
        auto exitSound = std::make_unique<sound::SoundInstance>(
            sound::SoundInstance::createGlobal(ResourceLocation("minecraft:ambient.underwater.exit"),
                sound::SoundCategory::Ambient,
                1.0f, // 音量
                1.0f  // 音调
                ));
        m_audioService->play(std::move(exitSound));
    }

    // 更新水下环境音效处理器
    if (m_audioService) {
        const auto* biome = m_world.getBiomeAtBlock(eyeBlockX, eyeBlockY, eyeBlockZ);
        m_audioService->setBiomeId(biome ? static_cast<u32>(biome->id()) : 0u);
        m_audioService->setUnderwater(inWater);

        // 更新群系环境音效处理器的光照等级和玩家位置
        // 用于心境音效的触发计算
        // 玩家眼睛位置光照
        const u8 skyLight = m_world.getSkyLight(eyeBlockX, eyeBlockY, eyeBlockZ);
        const u8 blockLight = m_world.getBlockLight(eyeBlockX, eyeBlockY, eyeBlockZ);

        // 心境音效采样位置：在玩家周围随机采样一个位置，查询该位置的光照
        // 与 MC 原版 BiomeAmbientSoundsHandler.tick() 行为一致：
        //   - 仅在群系配置了 moodSound 时才采样
        //   - 采样范围 blockSearchExtent 从群系配置读取（不同群系可不同）
        //   - 同一采样位置同时用于光照查询（驱动心境计时器）和声音播放位置计算
        //     （音频线程直接复用主线程采样的位置，避免双线程独立随机导致位置不一致）
        i32 moodBx = 0;
        i32 moodBy = 0;
        i32 moodBz = 0;
        u8 moodSkyLight = 0;
        u8 moodBlockLight = 0;
        if (biome) {
            const auto& ambientSounds = biome->ambientSounds();
            if (ambientSounds.moodSound().has_value()) {
                const i32 extent = ambientSounds.moodSound()->blockSearchExtent();
                moodBx = eyeBlockX + m_moodRng.nextInt(extent * 2 + 1) - extent;
                moodBy = eyeBlockY + m_moodRng.nextInt(extent * 2 + 1) - extent;
                moodBz = eyeBlockZ + m_moodRng.nextInt(extent * 2 + 1) - extent;
                moodSkyLight = m_world.getSkyLight(moodBx, moodBy, moodBz);
                moodBlockLight = m_world.getBlockLight(moodBx, moodBy, moodBz);
            }
        }
        m_audioService->setAmbientLightLevel(skyLight, blockLight, moodSkyLight, moodBlockLight);
        m_audioService->setAmbientPlayerPosition(
            m_player->x(), m_player->y() + m_player->eyeHeight(), m_player->z(), moodBx, moodBy, moodBz);

        // 更新音乐状态
        // 维度: 0=主世界, -1=下界, 1=末地
        const i32 dimension = static_cast<i32>(m_player->dimension());
        // 创造模式检查
        const bool inCreative = entity::GameModeUtils::isCreative(m_player->gameMode());
        // Boss战检查（当前未实现Boss战检测系统）
        const bool inBossFight = false;

        // 获取生物群系音乐
        std::optional<world::biome::BiomeMusic> biomeMusic;
        if (biome) {
            biomeMusic = biome->ambientSounds().music();
        }

        // 判断是否在海洋或河流生物群系中（水下音乐只在海洋/河流中播放）
        const bool inOceanOrRiverBiome = biome ? (world::biome::BiomeTags::IS_OCEAN().contains(biome->id()) ||
                                                     world::biome::BiomeTags::IS_RIVER().contains(biome->id()))
                                               : false;

        m_audioService->updateMusicState(dimension, inCreative, inBossFight, inOceanOrRiverBiome, biomeMusic);

        // 更新气泡柱状态
        // 检测玩家碰撞箱范围内是否有气泡柱
        bool inBubbleColumn = false;
        bool bubbleColumnDrag = false;
        // 玩家脚底位置的方块坐标
        const i32 footBlockY = static_cast<i32>(std::floor(m_player->y()));
        const auto* blockState = m_world.getBlockState(eyeBlockX, footBlockY, eyeBlockZ);
        if (blockState != nullptr && blockState->is(VanillaBlocks::BUBBLE_COLUMN)) {
            inBubbleColumn = true;
            // 获取气泡柱的 drag 属性
            const auto& bubbleColumn = static_cast<const mc::blocks::BubbleColumnBlock&>(blockState->owner());
            bubbleColumnDrag = bubbleColumn.isDrag(*blockState);
        }
        m_audioService->setBubbleColumnState(inBubbleColumn, bubbleColumnDrag);

        // 更新天气音效状态（雨声/雷声）
        const f32 rainStrength = m_world.weather().rainStrength();
        const f32 thunderStrength = m_world.weather().thunderStrength();
        const bool canSeeSky = m_world.canSeeSky(BlockPos(eyeBlockX, eyeBlockY, eyeBlockZ));
        m_audioService->updateWeatherState(rainStrength, thunderStrength, canSeeSky);
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
    bool isPaused = !m_mouseCaptured; // 鼠标未捕获时认为游戏暂停
    m_audioService->setPaused(isPaused);

    // 检查是否在菜单界面（ScreenManager 有屏幕打开）
    bool inMenu = ScreenManager::instance().hasScreen();
    m_audioService->setInMenu(inMenu);
}

void ClientApplication::shutdownAudio()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdownAudio");

    // 清除 UI 音效回调，避免悬空指针
    ui::kagero::widget::Widget::setUiSoundCallback(nullptr);

    if (!m_audioService) {
        return;
    }

    m_audioService->shutdown();
    m_audioService.reset();
}

} // namespace mc::client
