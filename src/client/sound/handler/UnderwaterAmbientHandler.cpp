#include "client/sound/handler/UnderwaterAmbientHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/sound/instance/UnderwaterLoopSound.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"

#include <chrono>

namespace mc::client::sound {

UnderwaterAmbientHandler::UnderwaterAmbientHandler()
    : m_rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()))
{}

UnderwaterAmbientHandler::~UnderwaterAmbientHandler() = default;

void UnderwaterAmbientHandler::setUnderwater(bool underwater)
{
    // 检测状态变化
    bool wasUnderwater = m_isUnderwater;
    m_isUnderwater = underwater;

    // 从非水下变为水下：播放入水音效
    // 注意：循环音效的启动在 tick() 中处理
    // 因为需要访问 SoundEngine
}

void UnderwaterAmbientHandler::tick(SoundEngine& engine)
{
    // 检测进入水状态变化
    if (m_isUnderwater && !m_wasUnderwater) {
        // 玩家刚进入水 - 播放入水音效
        // 参考: ClientPlayerEntity.updateEyesInWaterPlayer()
        auto enterSound = std::make_unique<SoundInstance>(
            SoundInstance::createGlobal(SoundEvents::AMBIENT_UNDERWATER_ENTER, SoundCategory::Ambient, 1.0f, 1.0f));
        engine.play(std::move(enterSound));

        // 启动水下循环音效
        auto loopSound = std::make_unique<UnderwaterLoopSound>();
        m_underwaterLoopSoundId = engine.play(std::move(loopSound));
    }

    // 检测离开水状态变化
    if (!m_isUnderwater && m_wasUnderwater) {
        // 玩家刚离开水 - 播放出书音效
        auto exitSound = std::make_unique<SoundInstance>(
            SoundInstance::createGlobal(SoundEvents::AMBIENT_UNDERWATER_EXIT, SoundCategory::Ambient, 1.0f, 1.0f));
        engine.play(std::move(exitSound));

        // 水下循环音效会自动淡出（通过 setCanSwim(false)）
        // 但我们需要通知它
        if (m_underwaterLoopSoundId != 0) {
            ISoundInstance* sound = engine.getSoundInstance(m_underwaterLoopSoundId);
            if (sound) {
                auto* loopSound = dynamic_cast<UnderwaterLoopSound*>(sound);
                if (loopSound) {
                    loopSound->setCanSwim(false);
                }
            }
        }
        m_underwaterLoopSoundId = 0;
    }

    // 更新上一帧状态
    m_wasUnderwater = m_isUnderwater;

    // 更新水下循环音效状态
    if (m_isUnderwater && m_underwaterLoopSoundId != 0) {
        ISoundInstance* sound = engine.getSoundInstance(m_underwaterLoopSoundId);
        if (sound) {
            auto* loopSound = dynamic_cast<UnderwaterLoopSound*>(sound);
            if (loopSound) {
                loopSound->setCanSwim(true);
            }
        }
    }

    // 检查循环音效是否已结束
    if (m_underwaterLoopSoundId != 0 && !engine.isPlaying(m_underwaterLoopSoundId)) {
        m_underwaterLoopSoundId = 0;
    }

    // 如果在水下但没有循环音效，创建一个新的
    if (m_isUnderwater && m_underwaterLoopSoundId == 0) {
        auto loopSound = std::make_unique<UnderwaterLoopSound>();
        loopSound->setCanSwim(true);
        m_underwaterLoopSoundId = engine.play(std::move(loopSound));
    }

    // 播放附加音效（只在水下播放）
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
        auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(
            SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS_ULTRA_RARE, SoundCategory::Ambient, 1.0f, 1.0f));
        engine.play(std::move(sound));
    } else if (f < 0.001f) {
        // 稀有音效: ambient.underwater.loop.additions.rare
        auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(
            SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS_RARE, SoundCategory::Ambient, 1.0f, 1.0f));
        engine.play(std::move(sound));
    } else if (f < 0.01f) {
        // 普通音效: ambient.underwater.loop.additions
        auto sound = std::make_unique<SoundInstance>(SoundInstance::createGlobal(
            SoundEvents::AMBIENT_UNDERWATER_LOOP_ADDITIONS, SoundCategory::Ambient, 1.0f, 1.0f));
        engine.play(std::move(sound));
    }
}

} // namespace mc::client::sound
