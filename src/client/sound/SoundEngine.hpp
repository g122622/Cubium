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

#pragma once

#include "client/sound/AudioBufferCache.hpp"
#include "client/sound/SoundLoader.hpp"
#include "client/sound/SoundPool.hpp"
#include "client/sound/backend/AudioBuffer.hpp"
#include "client/sound/backend/IAudioBackend.hpp"
#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/resource/SoundDefinition.hpp"
#include "client/sound/resource/SoundRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundTypes.hpp"
#include "common/util/math/random/Random.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc {

// 前向声明

namespace client {

// 前向声明
class ClientSettings;

namespace sound {

// 从 mc::sound 引入类型
using ::mc::sound::AttenuationType;
using ::mc::sound::DEFAULT_ATTENUATION_DISTANCE;
using ::mc::sound::SoundInstanceId;

// 前向声明
class SoundHandler;

/**
 * @brief 声音引擎主类
 *
 * 管理声音播放、通道分配、听者更新等核心功能。
 * 使用 OpenAL 作为音频后端。
 *
 * 使用示例:
 * @code
 * SoundEngine engine(handler, settings);
 * auto result = engine.initialize();
 * if (!result.success()) {
 *     spdlog::error("Failed to init audio: {}", result.error().message);
 *     return;
 * }
 *
 * // 播放声音
 * auto sound = SoundInstance::createLocated(
 *     ResourceLocation("minecraft:block.stone.break"),
 *     SoundCategory::Blocks,
 *     pos.x, pos.y, pos.z
 * );
 * engine.play(std::move(sound));
 *
 * // 更新听者位置
 * engine.updateListener(playerPos, forward, up);
 *
 * // 每帧更新
 * engine.tick(isPaused);
 *
 * // 关闭引擎
 * engine.shutdown();
 * @endcode
 *
 * 线程安全说明:
 * - SoundEngine 本身不是线程安全的（内部含 OpenAL 上下文、可变状态、缓存等）。
 * - 当前工程中应由 AudioService 在"音频引擎线程"独占调用 SoundEngine 的所有方法。
 * - 主线程不得直接触碰 SoundEngine；需要播放/停止/更新 listener 等行为必须投递到 AudioService。
 */
class SoundEngine {
public:
    /**
     * @brief 构造声音引擎
     *
     * @param handler 声音资源处理器
     * @param settings 客户端设置
     */
    SoundEngine(SoundHandler& handler, ClientSettings& settings);

    ~SoundEngine();

    // 禁止拷贝
    SoundEngine(const SoundEngine&) = delete;
    SoundEngine& operator=(const SoundEngine&) = delete;

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 初始化声音引擎
     *
     * 初始化音频后端和内部状态。
     * 必须在使用其他方法之前调用。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize();

    /**
     * @brief 关闭声音引擎
     *
     * 停止所有声音，释放音频资源。
     */
    void shutdown();

    /**
     * @brief 检查引擎是否已加载
     */
    [[nodiscard]] bool isLoaded() const noexcept { return m_loaded; }

    // ========================================================================
    // 播放控制
    // ========================================================================

    /**
     * @brief 播放声音
     *
     * @param sound 声音实例
     * @return 声音实例ID，失败返回 0
     */
    SoundInstanceId play(std::unique_ptr<ISoundInstance> sound);

    /**
     * @brief 延迟播放声音
     *
     * @param sound 声音实例
     * @param delayTicks 延迟的游戏 ticks
     */
    void playDelayed(std::unique_ptr<ISoundInstance> sound, u32 delayTicks);

    /**
     * @brief 在下一 tick 播放声音
     *
     * 用于 TickableSound 的声音切换（如蜜蜂从飞行声切换到愤怒声）。
     * 声音会在下一个 tick 周期开始时播放。
     *
     * @param sound 声音实例
     */
    void playOnNextTick(std::unique_ptr<ISoundInstance> sound);

    /**
     * @brief 停止指定声音
     *
     * @param id 声音实例ID
     */
    void stop(SoundInstanceId id);

    /**
     * @brief 停止指定声音事件的所有实例
     *
     * @param soundEventId 声音事件ID
     */
    void stop(const ResourceLocation& soundEventId);

    /**
     * @brief 停止指定类别的所有声音
     *
     * @param category 声音类别
     */
    void stop(SoundCategory category);

    /**
     * @brief 停止所有声音
     */
    void stopAll();

    /**
     * @brief 暂停所有声音
     */
    void pause();

    /**
     * @brief 恢复所有声音
     */
    void resume();

    /**
     * @brief 检查声音是否正在播放
     *
     * @param id 声音实例ID
     * @return 是否正在播放
     */
    [[nodiscard]] bool isPlaying(SoundInstanceId id) const;

    // ========================================================================
    // 听者管理
    // ========================================================================

    /**
     * @brief 更新听者（玩家相机）位置和方向
     *
     * @param position 世界坐标
     * @param forward 前方向向量（单位向量）
     * @param up 上方向向量（单位向量）
     */
    void updateListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    /**
     * @brief 设置听者速度
     *
     * 用于多普勒效应计算。
     *
     * @param velocity 速度向量
     */
    void setListenerVelocity(const glm::vec3& velocity);

    // ========================================================================
    // 音量控制
    // ========================================================================

    /**
     * @brief 设置类别音量
     *
     * @param category 声音类别
     * @param volume 音量 (0.0-1.0)
     */
    void setVolume(SoundCategory category, f32 volume);

    /**
     * @brief 获取类别音量
     *
     * @param category 声音类别
     * @return 音量 (0.0-1.0)
     */
    [[nodiscard]] f32 getVolume(SoundCategory category) const;

    // ========================================================================
    // 更新
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * @param isPaused 游戏是否暂停
     */
    void tick(bool isPaused);

    // ========================================================================
    // 环境音效处理器
    // ========================================================================

    /**
     * @brief 添加环境音效处理器
     *
     * @param handler 环境音效处理器
     */
    void addAmbientHandler(std::unique_ptr<IAmbientSoundHandler> handler);

    // ========================================================================
    // 访问器
    // ========================================================================

    /**
     * @brief 获取声音处理器
     */
    [[nodiscard]] SoundHandler& getHandler() noexcept { return m_handler; }

    /**
     * @brief 获取声音处理器（const版本）
     */
    [[nodiscard]] const SoundHandler& getHandler() const noexcept { return m_handler; }

    /**
     * @brief 获取音频后端
     */
    [[nodiscard]] IAudioBackend* getBackend() noexcept { return m_backend.get(); }

    /**
     * @brief 获取音频后端（const版本）
     */
    [[nodiscard]] const IAudioBackend* getBackend() const noexcept { return m_backend.get(); }

    /**
     * @brief 获取声音实例
     *
     * @param id 声音实例ID
     * @return 声音实例指针，不存在返回 nullptr
     */
    [[nodiscard]] ISoundInstance* getSoundInstance(SoundInstanceId id);

    /**
     * @brief 获取声音实例（const版本）
     *
     * @param id 声音实例ID
     * @return 声音实例指针，不存在返回 nullptr
     */
    [[nodiscard]] const ISoundInstance* getSoundInstance(SoundInstanceId id) const;

private:
    /**
     * @brief 活动声音通道
     */
    struct ActiveChannel {
        SoundInstanceId soundId;
        std::unique_ptr<IAudioSource> source;
        std::shared_ptr<IAudioBuffer> buffer;
        AudioBufferId bufferId = 0;
        bool isPaused = false;
    };

    /**
     * @brief 计算实际音量
     *
     * 音量 = 声音音量 * 类别音量 * 主音量
     *
     * @param sound 声音实例
     * @return 实际音量
     */
    [[nodiscard]] f32 _calculateVolume(const ISoundInstance& sound) const;

    /**
     * @brief 计算实际音调
     *
     * @param sound 声音实例
     * @return 实际音调（限制在0.5-2.0）
     */
    [[nodiscard]] f32 _calculatePitch(const ISoundInstance& sound) const;

    /**
     * @brief 解析声音定义（处理事件引用）
     *
     * 如果声音定义是事件引用，则递归解析直到获得文件引用。
     * 同时累积事件引用中的音量和音调修正。
     *
     * @param soundDef 声音定义（可能被修改）
     * @param depth 当前递归深度（防止无限循环）
     * @param outVolume 累积的音量修正（输出）
     * @param outPitch 累积的音调修正（输出）
     * @return 是否成功解析
     */
    [[nodiscard]] bool _resolveSoundDefinition(
        SoundDefinition& soundDef, u32 depth, f32& outVolume, f32& outPitch) const;

    /**
     * @brief 检查声音是否在可听范围内
     *
     * @param sound 声音实例
     * @param attenuationDistance 衰减距离
     * @return true 如果声音在可听范围内
     */
    [[nodiscard]] bool _isInRange(const ISoundInstance& sound, f32 attenuationDistance) const;

    /**
     * @brief 更新声音位置
     *
     * @param channel 活动通道
     * @param sound 声音实例
     */
    void _updateSoundPosition(ActiveChannel& channel, const ISoundInstance& sound);

    /**
     * @brief 更新所有延迟声音
     */
    void _updateDelayedSounds();

    /// 声音处理器
    SoundHandler& m_handler;

    /// 客户端设置
    ClientSettings& m_settings;

    /// 音频后端
    std::unique_ptr<IAudioBackend> m_backend;

    /// 声音加载器
    std::unique_ptr<SoundLoader> m_loader;

    /// 音频缓冲区缓存（避免重复解码相同音频）
    AudioBufferCache m_bufferCache;

    /// 声音池
    SoundPool m_pool;

    /// 活动通道
    std::unordered_map<SoundInstanceId, ActiveChannel> m_channels;

    /// 延迟声音
    std::vector<std::pair<std::unique_ptr<ISoundInstance>, u32>> m_delayedSounds;

    /// 下一tick播放的声音队列（用于TickableSound的声音切换）
    std::vector<std::unique_ptr<ISoundInstance>> m_playOnNextTickQueue;

    /// 环境音效处理器
    std::vector<std::unique_ptr<IAmbientSoundHandler>> m_ambientHandlers;

    /// 是否已加载
    bool m_loaded = false;

    /// 是否暂停
    bool m_paused = false;

    /// 听者位置（用于距离剔除）
    glm::vec3 m_listenerPosition{0.0f};

    /// 随机数生成器（用于声音选择）
    mutable math::Random m_rng;

    /// 缓冲区清理计数器（每N帧清理一次未使用的缓冲区）
    u32 m_bufferCleanupCounter = 0;

    /// 缓冲区清理间隔（帧数）
    static constexpr u32 BUFFER_CLEANUP_INTERVAL = 600; // 约30秒（假设60fps）
};

} // namespace sound
} // namespace client
} // namespace mc
