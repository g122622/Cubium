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

#include "../ClientApplication.hpp"

#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/blocks/ocean/BubbleColumnBlock.hpp"

#include <algorithm>
#include <cmath>

namespace mc::client {

Result<void> ClientApplication::initializeAudio()
{
    MC_TRACE_EVENT("client.initialization", "InitializeSoundSystem");

    spdlog::info("Initializing sound system...");

    // 添加内部资源包到 ResourcePackList（用于声音事件定义）
    // 内置包具有最低优先级（-1），外部资源包可以覆盖
    auto builtinPackResult = m_resourcePackList.addPack(m_gameDirectory.builtinPackDir(), true, -1);
    if (builtinPackResult.success() && builtinPackResult.value().initialized) {
        spdlog::info("Added built-in resource pack to sound system");
    } else {
        spdlog::warn("Failed to add built-in resource pack: {}",
            builtinPackResult.success() ? builtinPackResult.value().error : builtinPackResult.error().toString());
    }

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
        // 参考 MC 1.16.5 Widget.playDownSound(): 音量 0.25F，无衰减
        ui::kagero::widget::Widget::setUiSoundCallback([this](const std::string& soundEventId) {
            if (m_audioService) {
                auto sound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createGlobal(ResourceLocation(soundEventId),
                        sound::SoundCategory::Master,
                        0.25f, // MC 1.16.5 UI 按钮默认音量
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
                    m_player->swimSoundVolume() * 0.15f,                        // MC 音量系数
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
                    soundType.getVolume() * 0.15f,                              // MC 音量系数
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

    // 获取当前生物群系的水下雾颜色
    if (inWater) {
        const auto* biome = m_world.getBiomeAtBlock(static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
            static_cast<i32>(std::floor(m_player->z())));
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
        const auto* biome = m_world.getBiomeAtBlock(static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
            static_cast<i32>(std::floor(m_player->z())));
        m_audioService->setBiomeId(biome ? static_cast<u32>(biome->id()) : 0u);
        m_audioService->setUnderwater(inWater);

        // 更新群系环境音效处理器的光照等级和玩家位置
        // 用于心境音效的触发计算
        const u8 skyLight = m_world.getSkyLight(static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
            static_cast<i32>(std::floor(m_player->z())));
        const u8 blockLight = m_world.getBlockLight(static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
            static_cast<i32>(std::floor(m_player->z())));
        m_audioService->setAmbientLightLevel(skyLight, blockLight);
        m_audioService->setAmbientPlayerPosition(m_player->x(), m_player->y() + m_player->eyeHeight(), m_player->z());

        // 更新音乐状态
        // 维度: 0=主世界, -1=下界, 1=末地
        const i32 dimension = static_cast<i32>(m_player->dimension());
        // 创造模式检查
        const bool inCreative = entity::GameModeUtils::isCreative(m_player->gameMode());
        // Boss战检查（当前未实现Boss战检测系统）
        const bool inBossFight = false;

        // 获取生物群系音乐（MC 1.16.5: 下界各生物群系有专属音乐）
        std::optional<world::biome::BiomeMusic> biomeMusic;
        if (biome) {
            biomeMusic = biome->getMusic();
        }

        m_audioService->updateMusicState(dimension, inCreative, inBossFight, biomeMusic);

        // 更新气泡柱状态
        // 检测玩家碰撞箱范围内是否有气泡柱
        bool inBubbleColumn = false;
        bool bubbleColumnDrag = false;
        const auto* blockState = m_world.getBlockState(static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y())),
            static_cast<i32>(std::floor(m_player->z())));
        if (blockState != nullptr && VanillaBlocks::BUBBLE_COLUMN != nullptr &&
            blockState->is(VanillaBlocks::BUBBLE_COLUMN)) {
            inBubbleColumn = true;
            // 获取气泡柱的 drag 属性
            const auto* bubbleColumn = dynamic_cast<const mc::blocks::BubbleColumnBlock*>(&blockState->owner());
            if (bubbleColumn != nullptr) {
                bubbleColumnDrag = bubbleColumn->isDrag(*blockState);
            }
        }
        m_audioService->setBubbleColumnState(inBubbleColumn, bubbleColumnDrag);

        // 更新天气音效状态（雨声/雷声）
        const f32 rainStrength = m_world.weather().rainStrength();
        const f32 thunderStrength = m_world.weather().thunderStrength();
        const bool canSeeSky = m_world.canSeeSky(BlockPos(static_cast<i32>(std::floor(m_player->x())),
            static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
            static_cast<i32>(std::floor(m_player->z()))));
        m_audioService->updateWeatherState(rainStrength, thunderStrength, m_player->y(), canSeeSky);
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
    // 清除 UI 音效回调，避免悬空指针
    ui::kagero::widget::Widget::setUiSoundCallback(nullptr);

    if (!m_audioService) {
        return;
    }

    m_audioService->shutdown();
    m_audioService.reset();
}

} // namespace mc::client