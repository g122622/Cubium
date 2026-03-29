#pragma once

#include "client/sound/SoundPool.hpp"
#include "client/sound/SoundLoader.hpp"
#include "client/sound/backend/IAudioBackend.hpp"
#include "client/sound/backend/AudioBuffer.hpp"
#include "client/sound/resource/SoundRegistry.hpp"
#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/core/Result.hpp"
#include "common/util/math/random/Random.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace mc {

// 前向声明
class ResourcePackList;

namespace client {

// 前向声明
class ClientSettings;

namespace sound {

// 从 mc::sound 引入类型
using ::mc::sound::SoundInstanceId;
using ::mc::sound::AttenuationType;
using ::mc::sound::DEFAULT_ATTENUATION_DISTANCE;

// 前向声明
class SoundHandler;

/**
 * @brief 声音引擎主类
 *
 * 管理声音播放、通道分配、听者更新等核心功能。
 * 使用 OpenAL 作为音频后端。
 *
 * 参考: net.minecraft.client.audio.SoundEngine
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
 * - 所有方法应在主线程调用
 * - 如需跨线程播放声音，使用线程安全队列
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
    void updateListener(const glm::vec3& position,
                        const glm::vec3& forward,
                        const glm::vec3& up);

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
    [[nodiscard]] f32 calculateVolume(const ISoundInstance& sound) const;

    /**
     * @brief 计算实际音调
     *
     * @param sound 声音实例
     * @return 实际音调（限制在0.5-2.0）
     */
    [[nodiscard]] f32 calculatePitch(const ISoundInstance& sound) const;

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
    [[nodiscard]] bool resolveSoundDefinition(
        SoundDefinition& soundDef,
        u32 depth,
        f32& outVolume,
        f32& outPitch
    ) const;

    /**
     * @brief 检查声音是否在可听范围内
     *
     * @param sound 声音实例
     * @param attenuationDistance 衰减距离
     * @return true 如果声音在可听范围内
     */
    [[nodiscard]] bool isInRange(const ISoundInstance& sound, f32 attenuationDistance) const;

    /**
     * @brief 更新声音位置
     *
     * @param channel 活动通道
     * @param sound 声音实例
     */
    void updateSoundPosition(ActiveChannel& channel, const ISoundInstance& sound);

    /**
     * @brief 更新所有延迟声音
     */
    void updateDelayedSounds();

    /// 声音处理器
    SoundHandler& m_handler;

    /// 客户端设置
    ClientSettings& m_settings;

    /// 音频后端
    std::unique_ptr<IAudioBackend> m_backend;

    /// 声音加载器
    std::unique_ptr<SoundLoader> m_loader;

    /// 音频缓冲区管理器
    std::unique_ptr<AudioBufferManager> m_bufferManager;

    /// 声音池
    SoundPool m_pool;

    /// 活动通道
    std::unordered_map<SoundInstanceId, ActiveChannel> m_channels;

    /// 延迟声音
    std::vector<std::pair<std::unique_ptr<ISoundInstance>, u32>> m_delayedSounds;

    /// 环境音效处理器
    std::vector<std::unique_ptr<IAmbientSoundHandler>> m_ambientHandlers;

    /// 是否已加载
    bool m_loaded = false;

    /// 是否暂停
    bool m_paused = false;

    /// 下一个声音ID
    SoundInstanceId m_nextSoundId = 1;

    /// 听者位置（用于距离剔除）
    glm::vec3 m_listenerPosition{0.0f};

    /// 随机数生成器（用于声音选择）
    mutable math::Random m_rng;
};

} // namespace mc::client::sound
} // namespace mc::client
} // namespace mc
