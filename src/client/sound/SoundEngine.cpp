#include "client/sound/SoundEngine.hpp"
#include "client/sound/SoundHandler.hpp"
#include "client/sound/resource/SoundRegistry.hpp"
#include "client/sound/resource/SoundDefinition.hpp"
#include "client/settings/ClientSettings.hpp"
#include "common/util/math/random/Random.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace mc::client::sound {

SoundEngine::SoundEngine(SoundHandler& handler, ClientSettings& settings)
    : m_handler(handler)
    , m_settings(settings)
{
}

SoundEngine::~SoundEngine() {
    if (m_loaded) {
        shutdown();
    }
}

Result<void> SoundEngine::initialize() {
    if (m_loaded) {
        return Error(ErrorCode::InvalidState, "Sound engine already initialized");
    }

    spdlog::info("[SoundEngine] Initializing sound engine...");

    // 创建音频后端
    m_backend = createOpenALBackend();
    if (!m_backend) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to create audio backend");
    }

    // 初始化音频后端
    auto result = m_backend->initialize();
    if (!result.success()) {
        m_backend.reset();
        return result;
    }

    // 创建声音加载器
    m_loader = std::make_unique<SoundLoader>(m_handler.getResourcePacks());

    // 创建缓冲区管理器（简化实现，不使用后端创建缓冲区）
    m_bufferManager = std::make_unique<AudioBufferManager>();

    m_loaded = true;
    spdlog::info("[SoundEngine] Sound engine initialized successfully");

    return {};
}

void SoundEngine::shutdown() {
    if (!m_loaded) {
        return;
    }

    spdlog::info("[SoundEngine] Shutting down sound engine...");

    // 停止所有声音
    stopAll();

    // 清理资源
    m_channels.clear();
    m_delayedSounds.clear();
    m_ambientHandlers.clear();
    m_pool.clear();
    m_bufferManager.reset();
    m_loader.reset();

    // 关闭音频后端
    if (m_backend) {
        m_backend->shutdown();
        m_backend.reset();
    }

    m_loaded = false;
    spdlog::info("[SoundEngine] Sound engine shutdown complete");
}

SoundInstanceId SoundEngine::play(std::unique_ptr<ISoundInstance> sound) {
    if (!m_loaded || !sound) {
        return 0;
    }

    // 获取声音事件定义
    const SoundEventDefinition* eventDef = m_handler.getRegistry().getSoundEvent(sound->getSoundEventId());
    if (!eventDef) {
        spdlog::debug("[SoundEngine] Sound event not found: {}", sound->getSoundEventId().toString());
        return 0;
    }

    // 选择随机声音
    mc::math::Random rng;
    const SoundDefinition* soundDef = eventDef->selectSound(rng);
    if (!soundDef) {
        spdlog::debug("[SoundEngine] No sounds available for: {}", sound->getSoundEventId().toString());
        return 0;
    }

    // 计算音量
    f32 volume = calculateVolume(*sound);
    if (volume <= 0.0f) {
        // 静音
        return 0;
    }

    // 计算音调
    f32 pitch = calculatePitch(*sound);

    // 检查是否在听者范围内（对于位置声音）
    if (!sound->isGlobal()) {
        // TODO: 实现距离检查
    }

    // 加载音频数据
    auto loadResult = m_loader->load(soundDef->location);
    if (!loadResult.success()) {
        spdlog::debug("[SoundEngine] Failed to load sound: {} - {}",
                      soundDef->location.toString(), loadResult.error().message());
        return 0;
    }

    AudioData& audioData = loadResult.value();

    // 创建音频缓冲区
    auto bufferResult = m_backend->createBuffer(audioData);
    if (!bufferResult.success()) {
        spdlog::debug("[SoundEngine] Failed to create buffer: {}", bufferResult.error().message());
        return 0;
    }

    AudioBufferId bufferId = bufferResult.value();

    // 创建音频源
    auto sourceResult = m_backend->createSource();
    if (!sourceResult.success()) {
        m_backend->destroyBuffer(bufferId);
        spdlog::debug("[SoundEngine] Failed to create source: {}", sourceResult.error().message());
        return 0;
    }

    auto source = std::move(sourceResult.value());

    // 配置音频源
    source->setGain(volume * soundDef->volume);
    source->setPitch(pitch * soundDef->pitch);
    source->setLooping(sound->isLooping());

    // 设置位置（对于非全局声音）
    if (!sound->isGlobal()) {
        source->setPosition(sound->getPosition());

        if (sound->getAttenuationType() == AttenuationType::Linear) {
            f32 attenuationDist = sound->getAttenuationDistance() * std::max(volume, 1.0f);
            source->setLinearAttenuation(attenuationDist);
        } else {
            source->setNoAttenuation();
        }

        source->setRelative(false);
    } else {
        // 全局声音：相对于听者
        source->setPosition(glm::vec3(0.0f));
        source->setNoAttenuation();
        source->setRelative(true);
    }

    // 设置缓冲区
    // 注意：这里需要创建一个包装器来持有 buffer ID
    // 为简化实现，我们暂时使用原始指针
    // TODO: 实现 IAudioBuffer 包装器

    // 添加到声音池
    SoundInstanceId soundId = m_pool.add(std::move(sound));
    if (soundId == 0) {
        m_backend->destroyBuffer(bufferId);
        return 0;
    }

    // 创建活动通道
    ActiveChannel channel;
    channel.soundId = soundId;
    channel.source = std::move(source);
    channel.isPaused = false;

    m_channels[soundId] = std::move(channel);

    // 开始播放
    m_channels[soundId].source->play();

    spdlog::trace("[SoundEngine] Playing sound: {} (id={})",
                  eventDef->location.toString(), soundId);

    return soundId;
}

void SoundEngine::playDelayed(std::unique_ptr<ISoundInstance> sound, u32 delayTicks) {
    if (!sound) {
        return;
    }

    m_delayedSounds.emplace_back(std::move(sound), delayTicks);
}

void SoundEngine::stop(SoundInstanceId id) {
    if (!m_loaded) {
        return;
    }

    auto it = m_channels.find(id);
    if (it != m_channels.end()) {
        it->second.source->stop();
        m_channels.erase(it);
    }

    m_pool.remove(id);
}

void SoundEngine::stop(const ResourceLocation& soundEventId) {
    if (!m_loaded) {
        return;
    }

    auto ids = m_pool.getBySoundEvent(soundEventId);
    for (SoundInstanceId id : ids) {
        stop(id);
    }
}

void SoundEngine::stop(SoundCategory category) {
    if (!m_loaded) {
        return;
    }

    auto ids = m_pool.getByCategory(category);
    for (SoundInstanceId id : ids) {
        stop(id);
    }
}

void SoundEngine::stopAll() {
    if (!m_loaded) {
        return;
    }

    // 停止所有通道
    for (auto& [id, channel] : m_channels) {
        if (channel.source) {
            channel.source->stop();
        }
    }

    m_channels.clear();
    m_pool.clear();
}

void SoundEngine::pause() {
    if (!m_loaded || m_paused) {
        return;
    }

    m_paused = true;

    for (auto& [id, channel] : m_channels) {
        if (channel.source && !channel.isPaused) {
            channel.source->pause();
            channel.isPaused = true;
        }
    }
}

void SoundEngine::resume() {
    if (!m_loaded || !m_paused) {
        return;
    }

    m_paused = false;

    for (auto& [id, channel] : m_channels) {
        if (channel.source && channel.isPaused) {
            channel.source->play();
            channel.isPaused = false;
        }
    }
}

bool SoundEngine::isPlaying(SoundInstanceId id) const {
    if (!m_loaded) {
        return false;
    }

    auto it = m_channels.find(id);
    if (it == m_channels.end()) {
        return false;
    }

    return it->second.source->getState() == AudioSourceState::Playing;
}

void SoundEngine::updateListener(const glm::vec3& position,
                                  const glm::vec3& forward,
                                  const glm::vec3& up) {
    if (!m_loaded || !m_backend) {
        return;
    }

    m_backend->setListenerPosition(position);
    m_backend->setListenerOrientation(forward, up);
}

void SoundEngine::setListenerVelocity(const glm::vec3& velocity) {
    if (!m_loaded || !m_backend) {
        return;
    }

    m_backend->setListenerVelocity(velocity);
}

void SoundEngine::setVolume(SoundCategory category, f32 volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    m_settings.setVolumeForCategory(category, volume);

    // 如果是主音量，更新听者增益
    if (category == SoundCategory::Master && m_backend) {
        m_backend->setListenerGain(volume);
    }

    // 更新活动声音的音量
    auto ids = m_pool.getByCategory(category);
    for (SoundInstanceId id : ids) {
        auto it = m_channels.find(id);
        if (it != m_channels.end()) {
            const ISoundInstance* sound = m_pool.get(id);
            if (sound && it->second.source) {
                it->second.source->setGain(calculateVolume(*sound));
            }
        }
    }
}

f32 SoundEngine::getVolume(SoundCategory category) const {
    return m_settings.getVolumeForCategory(category);
}

void SoundEngine::tick(bool isPaused) {
    if (!m_loaded) {
        return;
    }

    // 处理暂停/恢复
    if (isPaused && !m_paused) {
        pause();
    } else if (!isPaused && m_paused) {
        resume();
    }

    if (m_paused) {
        return;
    }

    // 更新延迟声音
    updateDelayedSounds();

    // 更新活动声音
    std::vector<SoundInstanceId> finishedSounds;

    for (auto& [id, channel] : m_channels) {
        // 更新可更新声音
        ISoundInstance* sound = m_pool.get(id);
        if (sound) {
            sound->tick();

            // 更新位置
            if (!sound->isGlobal() && channel.source) {
                channel.source->setPosition(sound->getPosition());
            }

            // 检查是否完成
            if (sound->isDone()) {
                finishedSounds.push_back(id);
                continue;
            }
        }

        // 检查播放状态
        if (channel.source) {
            AudioSourceState state = channel.source->getState();
            if (state == AudioSourceState::Stopped) {
                finishedSounds.push_back(id);
            }
        }
    }

    // 清理已完成的声音
    for (SoundInstanceId id : finishedSounds) {
        auto it = m_channels.find(id);
        if (it != m_channels.end()) {
            m_channels.erase(it);
        }
        m_pool.remove(id);
    }

    // 调用环境音效处理器
    for (auto& handler : m_ambientHandlers) {
        handler->tick(*this);
    }

    // 更新音频后端
    m_backend->process();
}

void SoundEngine::addAmbientHandler(std::unique_ptr<IAmbientSoundHandler> handler) {
    m_ambientHandlers.push_back(std::move(handler));
}

f32 SoundEngine::calculateVolume(const ISoundInstance& sound) const {
    f32 volume = sound.getVolume();

    // 乘以类别音量
    volume *= m_settings.getVolumeForCategory(sound.getCategory());

    // 乘以主音量（如果不是主类别）
    if (sound.getCategory() != SoundCategory::Master) {
        volume *= m_settings.getVolumeForCategory(SoundCategory::Master);
    }

    return std::max(0.0f, volume);
}

f32 SoundEngine::calculatePitch(const ISoundInstance& sound) const {
    f32 pitch = sound.getPitch();

    // 限制音调范围
    return std::clamp(pitch, 0.5f, 2.0f);
}

void SoundEngine::updateSoundPosition(ActiveChannel& channel, const ISoundInstance& sound) {
    if (!channel.source || sound.isGlobal()) {
        return;
    }

    channel.source->setPosition(sound.getPosition());
}

void SoundEngine::updateDelayedSounds() {
    std::vector<std::pair<std::unique_ptr<ISoundInstance>, u32>> remaining;

    for (auto& [sound, delay] : m_delayedSounds) {
        if (delay == 0) {
            // 延迟结束，播放声音
            play(std::move(sound));
        } else {
            // 减少延迟
            --delay;
            remaining.push_back({std::move(sound), delay});
        }
    }

    m_delayedSounds = std::move(remaining);
}

} // namespace mc::client::sound
