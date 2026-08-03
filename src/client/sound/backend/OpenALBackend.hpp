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

#include "client/sound/backend/AudioBuffer.hpp"
#include "client/sound/backend/IAudioBackend.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundTypes.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

// 前向声明，用于 OpenALSource 持有 backend 反向指针
class OpenALBackend;

/**
 * @brief OpenAL 音频源实现
 *
 * 封装 OpenAL source 对象，实现 IAudioSource 接口。
 *
 * 注意：此类不是线程安全的，所有调用应在音频线程进行。
 */
class OpenALSource : public IAudioSource {
public:
    /**
     * @brief 源销毁回调类型
     *
     * 当源被销毁时调用，用于通知后端释放源计数。
     */
    using OnDestroyCallback = std::function<void()>;

    /**
     * @brief 缓冲区查找回调类型
     *
     * 用于流式播放接口（queueBuffers / unqueueBuffers）在 AudioBufferId 与 OpenAL
     * ALuint 句柄之间转换。回调由 OpenALBackend 在创建源时绑定，避免 OpenALSource
     * 直接依赖 OpenALBackend 的具体类型，保持接口抽象层次清晰。
     *
     * @param id 应用层缓冲区 ID
     * @return 对应的 OpenAL ALuint 句柄，找不到返回 0
     */
    using LookupALBufferCallback = std::function<ALuint(AudioBufferId)>;

    /**
     * @brief 反向查找回调类型
     *
     * 用于 unqueueBuffers 把 OpenAL ALuint 句柄反向翻译回 AudioBufferId。
     *
     * @param alBuffer OpenAL 缓冲区句柄
     * @return 对应的应用层缓冲区 ID，找不到返回 0
     */
    using LookupBufferIdCallback = std::function<AudioBufferId(ALuint)>;

    /**
     * @brief 构造 OpenAL 音频源
     *
     * @param id 音频源 ID
     * @param source OpenAL source 句柄
     * @param onDestroy 源销毁时的回调（可选）
     * @param lookupALBuffer 由 AudioBufferId 查找 ALuint 句柄的回调（流式播放必需）
     * @param lookupBufferId 由 ALuint 句柄反向查找 AudioBufferId 的回调（流式播放必需）
     */
    OpenALSource(AudioSourceId id,
        ALuint source,
        OnDestroyCallback onDestroy = nullptr,
        LookupALBufferCallback lookupALBuffer = nullptr,
        LookupBufferIdCallback lookupBufferId = nullptr);
    ~OpenALSource() override;

    // 禁止拷贝
    OpenALSource(const OpenALSource&) = delete;
    OpenALSource& operator=(const OpenALSource&) = delete;

    // 允许移动
    OpenALSource(OpenALSource&& other) noexcept;
    OpenALSource& operator=(OpenALSource&& other) noexcept;

    // ========================================================================
    // 基本属性
    // ========================================================================

    [[nodiscard]] AudioSourceId getId() const noexcept override { return m_id; }
    [[nodiscard]] AudioSourceState getState() const noexcept override;

    // ========================================================================
    // 缓冲区绑定
    // ========================================================================

    void setBuffer(std::shared_ptr<IAudioBuffer> buffer) override;
    [[nodiscard]] std::shared_ptr<IAudioBuffer> getBuffer() const noexcept override { return m_buffer; }

    // ========================================================================
    // 播放控制
    // ========================================================================

    void play() override;
    void pause() override;
    void stop() override;
    void rewind() override;

    // ========================================================================
    // 音量和音调
    // ========================================================================

    void setGain(f32 gain) override;
    [[nodiscard]] f32 getGain() const noexcept override;

    void setPitch(f32 pitch) override;
    [[nodiscard]] f32 getPitch() const noexcept override;

    // ========================================================================
    // 空间音频
    // ========================================================================

    void setPosition(const glm::vec3& position) override;
    [[nodiscard]] glm::vec3 getPosition() const noexcept override;

    void setVelocity(const glm::vec3& velocity) override;
    [[nodiscard]] glm::vec3 getVelocity() const noexcept override;

    void setDirection(const glm::vec3& direction) override;
    [[nodiscard]] glm::vec3 getDirection() const noexcept override;

    void setRelative(bool relative) override;
    [[nodiscard]] bool isRelative() const noexcept override;

    // ========================================================================
    // 距离衰减
    // ========================================================================

    void setReferenceDistance(f32 distance) override;
    [[nodiscard]] f32 getReferenceDistance() const noexcept override;

    void setMaxDistance(f32 distance) override;
    [[nodiscard]] f32 getMaxDistance() const noexcept override;

    // ========================================================================
    // 循环播放
    // ========================================================================

    void setLooping(bool looping) override;
    [[nodiscard]] bool isLooping() const noexcept override;

    // ========================================================================
    // 流式播放
    // ========================================================================

    void queueBuffers(const AudioBufferId* buffers, size_t count) override;
    u32 unqueueBuffers(AudioBufferId* buffers, size_t count) override;
    [[nodiscard]] u32 getProcessedBuffers() const noexcept override;
    [[nodiscard]] u32 getQueuedBuffers() const noexcept override;

    // ========================================================================
    // OpenAL 特定方法
    // ========================================================================

    /**
     * @brief 获取 OpenAL source 句柄
     */
    [[nodiscard]] ALuint getALSource() const noexcept { return m_source; }

    /**
     * @brief 检查源是否有效
     */
    [[nodiscard]] bool isValid() const noexcept { return m_source != 0; }

private:
    /**
     * @brief 检查 OpenAL 错误
     *
     * @param operation 操作名称（用于日志）
     * @return 是否有错误
     */
    bool _checkError(const char* operation) const;

    AudioSourceId m_id;
    ALuint m_source;
    std::shared_ptr<IAudioBuffer> m_buffer;
    OnDestroyCallback m_onDestroy;
    LookupALBufferCallback m_lookupALBuffer; ///< AudioBufferId -> ALuint 查找回调
    LookupBufferIdCallback m_lookupBufferId; ///< ALuint -> AudioBufferId 反向查找回调
};

/**
 * @brief OpenAL 音频缓冲区实现
 *
 * 封装 OpenAL buffer 对象，实现 IAudioBuffer 接口。
 */
class OpenALBuffer : public IAudioBuffer {
public:
    /**
     * @brief 构造 OpenAL 音频缓冲区
     *
     * @param id 缓冲区 ID
     * @param buffer OpenAL buffer 句柄
     * @param format 音频格式
     * @param duration 音频时长
     */
    OpenALBuffer(AudioBufferId id, ALuint buffer, const AudioFormat& format, f32 duration);
    ~OpenALBuffer() override;

    // 禁止拷贝
    OpenALBuffer(const OpenALBuffer&) = delete;
    OpenALBuffer& operator=(const OpenALBuffer&) = delete;

    // 允许移动
    OpenALBuffer(OpenALBuffer&& other) noexcept;
    OpenALBuffer& operator=(OpenALBuffer&& other) noexcept;

    [[nodiscard]] AudioBufferId getId() const noexcept override { return m_id; }
    [[nodiscard]] const AudioFormat& getFormat() const noexcept override { return m_format; }
    [[nodiscard]] f32 getDuration() const noexcept override { return m_duration; }
    [[nodiscard]] size_t getSampleCount() const noexcept override { return m_sampleCount; }
    [[nodiscard]] bool isValid() const noexcept override { return m_buffer != 0; }

    /**
     * @brief 获取 OpenAL buffer 句柄
     */
    [[nodiscard]] ALuint getALBuffer() const noexcept { return m_buffer; }

    /**
     * @brief 将音频格式转换为 OpenAL 格式
     *
     * @param format 音频格式
     * @return OpenAL 格式枚举，不支持返回 0
     */
    [[nodiscard]] static ALenum getALFormat(const AudioFormat& format);

    /**
     * @brief 从音频数据创建 OpenAL 缓冲区
     *
     * @param id 缓冲区 ID
     * @param data 音频数据
     * @return 缓冲区，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<OpenALBuffer>> create(AudioBufferId id, const AudioData& data);

private:
    AudioBufferId m_id;
    ALuint m_buffer;
    AudioFormat m_format;
    f32 m_duration;
    size_t m_sampleCount = 0;
};

/**
 * @brief OpenAL 音频后端实现
 *
 * 使用 OpenAL Soft 实现音频播放功能。
 *
 * 使用示例:
 * @code
 * auto backend = createOpenALBackend();
 * auto result = backend->initialize();
 * if (!result.success()) {
 *     spdlog::error("Failed to init audio: {}", result.error().message);
 *     return;
 * }
 *
 * // 创建缓冲区和源
 * auto bufferResult = backend->createBuffer(audioData);
 * auto source = backend->createSource();
 * source->setBuffer(bufferResult.value());
 * source->play();
 * @endcode
 *
 * 线程安全说明:
 * - initialize() 和 shutdown() 必须在主线程调用
 * - 其他方法可以从任何线程调用，但建议在音频线程统一调用
 *
 * @see IAudioBackend
 */
class OpenALBackend : public IAudioBackend {
public:
    OpenALBackend();
    ~OpenALBackend() override;

    // 禁止拷贝
    OpenALBackend(const OpenALBackend&) = delete;
    OpenALBackend& operator=(const OpenALBackend&) = delete;

    // ========================================================================
    // 生命周期
    // ========================================================================

    [[nodiscard]] Result<void> initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const noexcept override { return m_initialized; }

    // ========================================================================
    // 听者控制
    // ========================================================================

    void setListenerPosition(const glm::vec3& position) override;
    [[nodiscard]] glm::vec3 getListenerPosition() const noexcept override;

    void setListenerOrientation(const glm::vec3& forward, const glm::vec3& up) override;
    [[nodiscard]] glm::vec3 getListenerForward() const noexcept override;
    [[nodiscard]] glm::vec3 getListenerUp() const noexcept override;

    void setListenerVelocity(const glm::vec3& velocity) override;
    [[nodiscard]] glm::vec3 getListenerVelocity() const noexcept override;

    void setListenerGain(f32 gain) override;
    [[nodiscard]] f32 getListenerGain() const noexcept override;

    // ========================================================================
    // 音频缓冲区管理
    // ========================================================================

    [[nodiscard]] Result<AudioBufferId> createBuffer(const AudioData& data) override;
    void destroyBuffer(AudioBufferId id) override;
    [[nodiscard]] bool hasBuffer(AudioBufferId id) const noexcept override;
    [[nodiscard]] std::shared_ptr<IAudioBuffer> getBuffer(AudioBufferId id) const override;

    // ========================================================================
    // 音频源管理
    // ========================================================================

    [[nodiscard]] Result<std::unique_ptr<IAudioSource>> createSource() override;
    [[nodiscard]] u32 getAvailableSources() const noexcept override;
    [[nodiscard]] u32 getMaxSources() const noexcept override { return m_maxSources; }
    [[nodiscard]] u32 getActiveSourceCount() const noexcept override;

    // ========================================================================
    // 更新
    // ========================================================================

    void process() override;

    // ========================================================================
    // 调试信息
    // ========================================================================

    [[nodiscard]] std::string getDeviceName() const override;
    [[nodiscard]] std::string getDebugString() const override;

private:
    /**
     * @brief 检查 OpenAL 错误
     *
     * @param operation 操作名称
     * @return 错误信息，无错误返回空
     */
    [[nodiscard]] std::string _checkALError(const char* operation) const;

    /**
     * @brief 由 AudioBufferId 查找 OpenAL ALuint 句柄
     *
     * 供 OpenALSource::queueBuffers 使用，将应用层 ID 翻译为 OpenAL 内部句柄。
     * 调用方在音频线程，内部加锁访问 m_buffers。
     *
     * @param id 应用层缓冲区 ID
     * @return OpenAL 缓冲区句柄，找不到返回 0
     */
    [[nodiscard]] ALuint _lookupALBuffer(AudioBufferId id) const;

    /**
     * @brief 由 OpenAL ALuint 句柄反向查找 AudioBufferId
     *
     * 供 OpenALSource::unqueueBuffers 使用，将 OpenAL 内部句柄翻译回应用层 ID。
     * 调用方在音频线程，内部加锁访问 m_alBufferToId。
     *
     * @param alBuffer OpenAL 缓冲区句柄
     * @return 应用层缓冲区 ID，找不到返回 0
     */
    [[nodiscard]] AudioBufferId _lookupBufferId(ALuint alBuffer) const;

    /// 最大音频源数量（从设备属性查询，回退到默认值）
    u32 m_maxSources = ::mc::sound::MAX_CONCURRENT_SOUNDS;

    /// 当前活跃的音频源数量
    std::atomic<u32> m_activeSourceCount{0};

    /// 初始化状态
    bool m_initialized = false;

    /// OpenAL 设备
    ALCdevice* m_device = nullptr;

    /// OpenAL 上下文
    ALCcontext* m_context = nullptr;

    /// 下一个缓冲区 ID
    AudioBufferId m_nextBufferId = 1;

    /// 下一个源 ID
    AudioSourceId m_nextSourceId = 1;

    /// 缓冲区映射（AudioBufferId -> OpenALBuffer）
    std::unordered_map<AudioBufferId, std::shared_ptr<OpenALBuffer>> m_buffers;

    /// 反向缓冲区映射（OpenAL ALuint 句柄 -> AudioBufferId）
    /// 用于流式播放时 unqueueBuffers 把 OpenAL 句柄翻译回应用层 ID
    std::unordered_map<ALuint, AudioBufferId> m_alBufferToId;

    /// 缓冲区互斥锁（同时保护 m_buffers 与 m_alBufferToId）
    mutable std::mutex m_bufferMutex;

    /// 听者位置
    glm::vec3 m_listenerPosition{0.0f};

    /// 听者前方向
    glm::vec3 m_listenerForward{0.0f, 0.0f, -1.0f};

    /// 听者上方向
    glm::vec3 m_listenerUp{0.0f, 1.0f, 0.0f};

    /// 听者速度
    glm::vec3 m_listenerVelocity{0.0f};

    /// 听者音量
    f32 m_listenerGain = 1.0f;
};

/**
 * @brief 创建 OpenAL 音频后端
 *
 * @return OpenAL 后端实例
 */
[[nodiscard]] std::unique_ptr<IAudioBackend> createOpenALBackend();

} // namespace mc::client::sound
