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

#include "client/sound/SoundEngine.hpp"
#include "client/settings/ClientSettings.hpp"
#include "client/sound/SoundHandler.hpp"
#include "client/sound/resource/SoundDefinition.hpp"
#include "client/sound/resource/SoundRegistry.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>

using namespace mc::trace;

namespace mc::client::sound {

SoundEngine::SoundEngine(SoundHandler& handler, ClientSettings& settings)
    : m_handler(handler)
    , m_settings(settings)
    , m_rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()))
{}

SoundEngine::~SoundEngine()
{
    if (m_loaded) {
        shutdown();
    }
}

Result<void> SoundEngine::initialize()
{
    if (m_loaded) {
        return Error(ErrorCode::InvalidState, "Sound engine already initialized");
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "SoundEngine_Initialize", "phase", "begin");
    }

    spdlog::info("[SoundEngine] Initializing sound engine...");

    // 加载声音事件定义（sounds.json）
    spdlog::info("[SoundEngine] Loading sound events from resource packs...");
    auto startLoad = std::chrono::steady_clock::now();

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "SoundHandler_Reload", "phase", "reload");
        auto reloadResult = m_handler.reload();
        if (!reloadResult.success()) {
            spdlog::warn("[SoundEngine] Failed to load sound events: {}", reloadResult.error().message());
            // 继续初始化，只是没有声音事件可用
        } else {
            spdlog::info("[SoundEngine] Loaded {} sound events", m_handler.getSoundEventCount());
        }
    }

    auto endLoad = std::chrono::steady_clock::now();
    auto loadMs = std::chrono::duration_cast<std::chrono::milliseconds>(endLoad - startLoad).count();
    spdlog::info("[SoundEngine] Sound events loaded in {} ms", loadMs);

    // 创建音频后端
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "OpenAL_Create", "phase", "create_backend");
        m_backend = createOpenALBackend();
    }
    if (!m_backend) {
        return Error(ErrorCode::InitializationFailed, "Failed to create audio backend");
    }

    // 初始化音频后端
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "OpenAL_Initialize", "phase", "initialize_backend");
        auto result = m_backend->initialize();
        if (!result.success()) {
            m_backend.reset();
            return result;
        }
    }

    // 创建声音加载器
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "SoundLoader_Create", "phase", "create_loader");
        m_loader = std::make_unique<SoundLoader>(m_handler.getResourcePacks());
    }

    m_loaded = true;
    spdlog::info("[SoundEngine] Sound engine initialized successfully");

    return {};
}

void SoundEngine::shutdown()
{
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
    m_bufferCache.clear();
    m_loader.reset();

    // 关闭音频后端
    m_backend->shutdown();
    m_backend.reset();

    m_loaded = false;
    spdlog::info("[SoundEngine] Sound engine shutdown complete");
}

SoundInstanceId SoundEngine::play(std::unique_ptr<ISoundInstance> sound)
{
    MC_ASSERT_RELEASE(sound);
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Client.Sound, "SoundEngine::play", "sound_event", sound->getSoundEventId().toString());
    if (!m_loaded) {
        return 0;
    }

    // 获取声音事件定义
    const SoundEventDefinition* eventDef = m_handler.getRegistry().getSoundEvent(sound->getSoundEventId());
    if (!eventDef) {
        spdlog::warn("[SoundEngine] Sound event not found: {}", sound->getSoundEventId().toString());
        return 0;
    }

    // 选择随机声音（使用成员变量 m_rng 确保每次选择不同）
    const SoundDefinition* soundDef = eventDef->selectSound(m_rng);
    if (!soundDef) {
        return 0;
    }

    // 复制声音定义（可能需要修改用于事件引用解析）
    SoundDefinition resolvedDef = *soundDef;

    // 解析事件引用（如果有）
    f32 eventVolume = 1.0f;
    f32 eventPitch = 1.0f;
    if (!_resolveSoundDefinition(resolvedDef, 0, eventVolume, eventPitch)) {
        spdlog::warn("[SoundEngine] Failed to resolve sound definition: {}", soundDef->location.toString());
        return 0;
    }

    // 计算音量
    f32 volume = _calculateVolume(*sound);
    if (volume <= 0.0f) {
        // 静音
        return 0;
    }

    // 计算音调
    f32 pitch = _calculatePitch(*sound);

    // 计算衰减距离（考虑音量）
    f32 attenuationDistance = resolvedDef.attenuationDistance * std::max(volume, 1.0f);

    // 检查是否在听者范围内（对于位置声音）
    if (!sound->isGlobal() && !_isInRange(*sound, attenuationDistance)) {
        // 超出可听范围，不播放
        return 0;
    }

    // 加载音频数据
    auto loadResult = m_loader->load(resolvedDef.location);
    if (!loadResult.success()) {
        spdlog::warn("[SoundEngine] Failed to load sound: {} - {}",
            resolvedDef.location.toString(),
            loadResult.error().message());
        return 0;
    }

    // 使用缓冲区缓存获取或创建缓冲区
    auto bufferResult = m_bufferCache.getOrCreate(resolvedDef.location, *m_backend, *m_loader);
    if (!bufferResult.success()) {
        spdlog::warn("[SoundEngine] Failed to get/create buffer: {}", bufferResult.error().message());
        return 0;
    }

    auto buffer = bufferResult.value();

    // 创建音频源
    auto sourceResult = m_backend->createSource();
    if (!sourceResult.success()) {
        // 注意：缓冲区由缓存管理，不需要手动销毁
        spdlog::warn("[SoundEngine] Failed to create source: {}", sourceResult.error().message());
        return 0;
    }

    auto source = sourceResult.value();

    // 配置音频源
    // 音量 = 实例音量 * 事件音量修正 * 定义音量
    source->setGain(volume * eventVolume * resolvedDef.volume);
    // 音调 = 实例音调 * 事件音调修正 * 定义音调
    source->setPitch(pitch * eventPitch * resolvedDef.pitch);
    source->setLooping(sound->isLooping());

    // 设置位置（对于非全局声音）
    if (!sound->isGlobal()) {
        source->setPosition(sound->getPosition());

        if (sound->getAttenuationType() == AttenuationType::Linear) {
            f32 attenuationDist = resolvedDef.attenuationDistance * std::max(volume, 1.0f);
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

    // 绑定缓冲区
    source->setBuffer(buffer);

    // 添加到声音池
    SoundInstanceId soundId = m_pool.add(std::move(sound));
    if (soundId == 0) {
        // 注意：缓冲区由缓存管理，不需要手动销毁
        return 0;
    }

    // 创建活动通道
    // 注意：bufferId 为 0，因为缓冲区由缓存管理，不需要手动销毁
    ActiveChannel channel;
    channel.soundId = soundId;
    channel.source = std::move(source);
    channel.buffer = std::move(buffer);
    channel.bufferId = 0; // 缓冲区由缓存管理
    channel.isPaused = false;

    m_channels[soundId] = std::move(channel);

    // 开始播放
    m_channels[soundId].source->play();

    spdlog::info("[SoundEngine] Playing sound: {} -> {} (id={})",
        eventDef->location.toString(),
        resolvedDef.location.toString(),
        soundId);

    return soundId;
}

void SoundEngine::playDelayed(std::unique_ptr<ISoundInstance> sound, u32 delayTicks)
{
    if (!sound) {
        return;
    }

    m_delayedSounds.emplace_back(std::move(sound), delayTicks);
}

void SoundEngine::playOnNextTick(std::unique_ptr<ISoundInstance> sound)
{
    if (!sound) {
        return;
    }

    m_playOnNextTickQueue.push_back(std::move(sound));
}

void SoundEngine::stop(SoundInstanceId id)
{
    if (!m_loaded) {
        return;
    }

    auto it = m_channels.find(id);
    if (it != m_channels.end()) {
        it->second.source->stop();
        // 注意：缓冲区由缓存管理，不需要手动销毁
        // bufferId 为 0 时表示缓冲区来自缓存
        m_channels.erase(it);
    }

    m_pool.remove(id);
}

void SoundEngine::stop(const ResourceLocation& soundEventId)
{
    if (!m_loaded) {
        return;
    }

    auto ids = m_pool.getBySoundEvent(soundEventId);
    for (SoundInstanceId id : ids) {
        stop(id);
    }
}

void SoundEngine::stop(SoundCategory category)
{
    if (!m_loaded) {
        return;
    }

    auto ids = m_pool.getByCategory(category);
    for (SoundInstanceId id : ids) {
        stop(id);
    }
}

void SoundEngine::stopAll()
{
    if (!m_loaded) {
        return;
    }

    // 停止所有通道
    for (auto& [id, channel] : m_channels) {
        channel.source->stop();
    }

    m_channels.clear();
    m_pool.clear();
}

void SoundEngine::pause()
{
    if (!m_loaded || m_paused) {
        return;
    }

    m_paused = true;

    for (auto& [id, channel] : m_channels) {
        if (!channel.isPaused) {
            channel.source->pause();
            channel.isPaused = true;
        }
    }
}

void SoundEngine::resume()
{
    if (!m_loaded || !m_paused) {
        return;
    }

    m_paused = false;

    for (auto& [id, channel] : m_channels) {
        if (channel.isPaused) {
            channel.source->play();
            channel.isPaused = false;
        }
    }
}

bool SoundEngine::isPlaying(SoundInstanceId id) const
{
    if (!m_loaded) {
        return false;
    }

    auto it = m_channels.find(id);
    if (it == m_channels.end()) {
        return false;
    }

    return it->second.source->getState() == AudioSourceState::Playing;
}

void SoundEngine::updateListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_loaded) {
        return;
    }

    // 存储听者位置（用于距离剔除）
    m_listenerPosition = position;

    m_backend->setListenerPosition(position);
    m_backend->setListenerOrientation(forward, up);
}

void SoundEngine::setListenerVelocity(const glm::vec3& velocity)
{
    if (!m_loaded) {
        return;
    }

    m_backend->setListenerVelocity(velocity);
}

void SoundEngine::setVolume(SoundCategory category, f32 volume)
{
    volume = std::clamp(volume, 0.0f, 1.0f);
    m_settings.setVolumeForCategory(category, volume);

    // 如果是主音量，更新听者增益
    if (category == SoundCategory::Master) {
        m_backend->setListenerGain(volume);
    }

    // 更新活动声音的音量
    auto ids = m_pool.getByCategory(category);
    for (SoundInstanceId id : ids) {
        auto it = m_channels.find(id);
        if (it != m_channels.end()) {
            ISoundInstance* sound = m_pool.get(id);
            MC_ASSERT_RELEASE(sound != nullptr);
            it->second.source->setGain(_calculateVolume(*sound));
        }
    }
}

f32 SoundEngine::getVolume(SoundCategory category) const
{
    return m_settings.getVolumeForCategory(category);
}

void SoundEngine::tick(bool isPaused)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "SoundEngine::tick", "isPaused", isPaused);

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

    // 处理 playOnNextTick 队列（用于 TickableSound 的声音切换）
    for (auto& sound : m_playOnNextTickQueue) {
        if (sound && sound->canBeSilent()) {
            play(std::move(sound));
        }
    }
    m_playOnNextTickQueue.clear();

    // 更新延迟声音
    _updateDelayedSounds();

    // 更新活动声音
    std::vector<SoundInstanceId> finishedSounds;

    for (auto& [id, channel] : m_channels) {
        // 更新可更新声音
        ISoundInstance* sound = m_pool.get(id);
        if (sound) {
            sound->tick();

            // 检查是否完成
            if (sound->isDone()) {
                finishedSounds.push_back(id);
                continue;
            }

            // 动态更新音量和音调
            f32 volume = _calculateVolume(*sound);
            f32 pitch = _calculatePitch(*sound);
            channel.source->setGain(volume);
            channel.source->setPitch(pitch);

            // 更新位置
            _updateSoundPosition(channel, *sound);
        }

        // 检查播放状态
        AudioSourceState state = channel.source->getState();
        if (state == AudioSourceState::Stopped) {
            finishedSounds.push_back(id);
        }
    }

    // 清理已完成的声音
    for (SoundInstanceId id : finishedSounds) {
        auto it = m_channels.find(id);
        if (it != m_channels.end()) {
            // 注意：缓冲区由缓存管理，不需要手动销毁
            m_channels.erase(it);
        }
        m_pool.remove(id);
    }

    // 调用环境音效处理器
    for (auto& handler : m_ambientHandlers) {
        handler->tick(*this);
    }

    // 定期清理未使用的缓冲区（每 BUFFER_CLEANUP_INTERVAL 帧清理一次）
    ++m_bufferCleanupCounter;
    if (m_bufferCleanupCounter >= BUFFER_CLEANUP_INTERVAL) {
        m_bufferCache.cleanupUnused();
        m_bufferCleanupCounter = 0;
    }

    // 更新音频后端
    m_backend->process();
}

void SoundEngine::addAmbientHandler(std::unique_ptr<IAmbientSoundHandler> handler)
{
    m_ambientHandlers.push_back(std::move(handler));
}

ISoundInstance* SoundEngine::getSoundInstance(SoundInstanceId id)
{
    return m_pool.get(id);
}

const ISoundInstance* SoundEngine::getSoundInstance(SoundInstanceId id) const
{
    return m_pool.get(id);
}

f32 SoundEngine::_calculateVolume(const ISoundInstance& sound) const
{
    f32 volume = sound.getVolume();

    // 乘以类别音量
    volume *= m_settings.getVolumeForCategory(sound.getCategory());

    // 乘以主音量（如果不是主类别）
    if (sound.getCategory() != SoundCategory::Master) {
        volume *= m_settings.getVolumeForCategory(SoundCategory::Master);
    }

    return std::max(0.0f, volume);
}

f32 SoundEngine::_calculatePitch(const ISoundInstance& sound) const
{
    f32 pitch = sound.getPitch();

    // 限制音调范围
    return std::clamp(pitch, 0.5f, 2.0f);
}

void SoundEngine::_updateSoundPosition(ActiveChannel& channel, const ISoundInstance& sound)
{
    if (sound.isGlobal()) {
        return;
    }

    channel.source->setPosition(sound.getPosition());
}

void SoundEngine::_updateDelayedSounds()
{
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

bool SoundEngine::_resolveSoundDefinition(SoundDefinition& soundDef, u32 depth, f32& outVolume, f32& outPitch) const
{
    // 防止无限递归（最大深度 16）
    constexpr u32 MAX_RESOLVE_DEPTH = 16;
    if (depth > MAX_RESOLVE_DEPTH) {
        spdlog::warn("[SoundEngine] Max event reference depth exceeded for: {}", soundDef.location.toString());
        return false;
    }

    // 如果是文件引用，直接返回成功
    if (soundDef.type == SoundType::File) {
        // 转换为 OGG 文件路径
        soundDef.location = soundDef.toOggLocation();
        return true;
    }

    // 事件引用：查找被引用的声音事件
    const SoundEventDefinition* refEvent = m_handler.getRegistry().getSoundEvent(soundDef.location);
    if (!refEvent) {
        spdlog::warn("[SoundEngine] Referenced sound event not found: {}", soundDef.location.toString());
        return false;
    }

    // 累积当前事件引用的音量和音调修正
    outVolume *= soundDef.volume;
    outPitch *= soundDef.pitch;

    // 从被引用的事件中随机选择一个声音（使用成员变量 m_rng）
    const SoundDefinition* selectedSound = refEvent->selectSound(m_rng);
    if (!selectedSound) {
        return false;
    }

    // 复制选中的声音定义
    soundDef = *selectedSound;

    // 递归解析（如果是嵌套的事件引用）
    return _resolveSoundDefinition(soundDef, depth + 1, outVolume, outPitch);
}

bool SoundEngine::_isInRange(const ISoundInstance& sound, f32 attenuationDistance) const
{
    // 全局声音始终在范围内
    if (sound.isGlobal()) {
        return true;
    }

    // 计算声音到听者的距离平方
    glm::vec3 soundPos = sound.getPosition();
    glm::vec3 diff = soundPos - m_listenerPosition;
    f32 distanceSq = glm::dot(diff, diff);
    f32 attenuationDistSq = attenuationDistance * attenuationDistance;

    // 在衰减距离内即可听到
    return distanceSq < attenuationDistSq;
}

} // namespace mc::client::sound
