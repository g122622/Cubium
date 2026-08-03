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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundTypes.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::AudioBufferId;
using ::mc::sound::AudioSourceId;

/**
 * @brief 音频源状态
 *
 * 参考: OpenAL AL_SOURCE_STATE
 */
enum class AudioSourceState : u8 {
    Initial, ///< 初始状态（未播放）
    Playing, ///< 正在播放
    Paused,  ///< 已暂停
    Stopped  ///< 已停止
};

/**
 * @brief 音频源接口
 *
 * 表示一个正在播放或待播放的音频源。
 * 每个音频源可以绑定一个音频缓冲区，并控制播放属性。
 *
 * 注意：这是音频后端资源的抽象接口。
 * 具体实现由 IAudioBackend::createSource() 创建。
 */
class IAudioSource {
public:
    virtual ~IAudioSource() = default;

    // ========================================================================
    // 基本属性
    // ========================================================================

    /**
     * @brief 获取音频源 ID
     */
    [[nodiscard]] virtual AudioSourceId getId() const noexcept = 0;

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] virtual AudioSourceState getState() const noexcept = 0;

    // ========================================================================
    // 缓冲区绑定
    // ========================================================================

    /**
     * @brief 绑定音频缓冲区
     *
     * 将音频缓冲区绑定到此音频源。
     * 只能绑定静态缓冲区（非流式）。
     *
     * @param buffer 音频缓冲区
     */
    virtual void setBuffer(std::shared_ptr<IAudioBuffer> buffer) = 0;

    /**
     * @brief 获取当前绑定的缓冲区
     */
    [[nodiscard]] virtual std::shared_ptr<IAudioBuffer> getBuffer() const noexcept = 0;

    // ========================================================================
    // 播放控制
    // ========================================================================

    /**
     * @brief 开始播放
     *
     * 如果音频源已暂停，则从暂停位置继续播放。
     * 如果音频源已停止，则从头开始播放。
     */
    virtual void play() = 0;

    /**
     * @brief 暂停播放
     */
    virtual void pause() = 0;

    /**
     * @brief 停止播放
     *
     * 停止播放并将播放位置重置到开头。
     */
    virtual void stop() = 0;

    /**
     * @brief 重绕
     *
     * 停止播放并将播放位置重置到开头。
     */
    virtual void rewind() = 0;

    // ========================================================================
    // 音量和音调
    // ========================================================================

    /**
     * @brief 设置音量
     *
     * @param gain 音量倍率 (0.0 = 静音, 1.0 = 正常, >1.0 = 放大)
     */
    virtual void setGain(f32 gain) = 0;

    /**
     * @brief 获取音量
     */
    [[nodiscard]] virtual f32 getGain() const noexcept = 0;

    /**
     * @brief 设置音调
     *
     * @param pitch 音调倍率 (0.5 = 低八度, 1.0 = 正常, 2.0 = 高八度)
     */
    virtual void setPitch(f32 pitch) = 0;

    /**
     * @brief 获取音调
     */
    [[nodiscard]] virtual f32 getPitch() const noexcept = 0;

    // ========================================================================
    // 空间音频
    // ========================================================================

    /**
     * @brief 设置位置
     *
     * @param position 世界坐标位置
     */
    virtual void setPosition(const glm::vec3& position) = 0;

    /**
     * @brief 获取位置
     */
    [[nodiscard]] virtual glm::vec3 getPosition() const noexcept = 0;

    /**
     * @brief 设置速度
     *
     * 用于多普勒效应计算。
     *
     * @param velocity 速度向量
     */
    virtual void setVelocity(const glm::vec3& velocity) = 0;

    /**
     * @brief 获取速度
     */
    [[nodiscard]] virtual glm::vec3 getVelocity() const noexcept = 0;

    /**
     * @brief 设置方向
     *
     * 用于定向声音（如圆锥形声音）。
     *
     * @param direction 方向向量
     */
    virtual void setDirection(const glm::vec3& direction) = 0;

    /**
     * @brief 获取方向
     */
    [[nodiscard]] virtual glm::vec3 getDirection() const noexcept = 0;

    /**
     * @brief 设置是否为相对源
     *
     * 相对源的位置是相对于听者的。
     *
     * @param relative 是否为相对源
     */
    virtual void setRelative(bool relative) = 0;

    /**
     * @brief 是否为相对源
     */
    [[nodiscard]] virtual bool isRelative() const noexcept = 0;

    // ========================================================================
    // 距离衰减
    // ========================================================================

    /**
     * @brief 设置参考距离
     *
     * 在此距离内音量不衰减。
     *
     * @param distance 参考距离
     */
    virtual void setReferenceDistance(f32 distance) = 0;

    /**
     * @brief 获取参考距离
     */
    [[nodiscard]] virtual f32 getReferenceDistance() const noexcept = 0;

    /**
     * @brief 设置最大距离
     *
     * 超过此距离后音量停止衰减。
     *
     * @param distance 最大距离
     */
    virtual void setMaxDistance(f32 distance) = 0;

    /**
     * @brief 获取最大距离
     */
    [[nodiscard]] virtual f32 getMaxDistance() const noexcept = 0;

    /**
     * @brief 设置线性衰减距离
     *
     * 便捷方法，设置参考距离为0，最大距离为指定值。
     * 等效于线性衰减模型。
     *
     * @param distance 衰减距离
     */
    virtual void setLinearAttenuation(f32 distance)
    {
        setReferenceDistance(0.0f);
        setMaxDistance(distance);
    }

    /**
     * @brief 禁用衰减
     *
     * 设置参考距离和最大距离为很大的值。
     */
    virtual void setNoAttenuation()
    {
        setReferenceDistance(10000.0f);
        setMaxDistance(10000.0f);
    }

    // ========================================================================
    // 循环播放
    // ========================================================================

    /**
     * @brief 设置是否循环播放
     *
     * @param looping 是否循环
     */
    virtual void setLooping(bool looping) = 0;

    /**
     * @brief 是否循环播放
     */
    [[nodiscard]] virtual bool isLooping() const noexcept = 0;

    // ========================================================================
    // 流式播放
    // ========================================================================

    /**
     * @brief 入队缓冲区
     *
     * 用于流式播放。将一个或多个缓冲区入队到播放队列。
     *
     * @param buffers 缓冲区 ID 数组
     * @param count 缓冲区数量
     */
    virtual void queueBuffers(const AudioBufferId* buffers, size_t count) = 0;

    /**
     * @brief 出队缓冲区
     *
     * 获取已处理完的缓冲区。
     *
     * @param buffers 接收缓冲区 ID 的数组
     * @param count 数组容量
     * @return 实际出队的缓冲区数量
     */
    virtual u32 unqueueBuffers(AudioBufferId* buffers, size_t count) = 0;

    /**
     * @brief 获取已处理的缓冲区数量
     *
     * 这些缓冲区已经播放完毕，可以出队并重新填充。
     */
    [[nodiscard]] virtual u32 getProcessedBuffers() const noexcept = 0;

    /**
     * @brief 获取排队的缓冲区数量
     *
     * 当前排队等待播放的缓冲区数量。
     */
    [[nodiscard]] virtual u32 getQueuedBuffers() const noexcept = 0;
};

/**
 * @brief 音频后端接口
 *
 * 抽象底层音频 API，便于未来替换实现（如 XAudio2、SDL Audio）。
 * 当前使用 OpenAL 作为默认实现。
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
 * // 设置听者位置
 * backend->setListenerPosition(playerPos);
 * backend->setListenerOrientation(forward, up);
 *
 * // 创建音频源
 * auto source = backend->createSource();
 * source->setBuffer(buffer);
 * source->setPosition(soundPos);
 * source->play();
 * @endcode
 */
class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 初始化音频后端
     *
     * 初始化音频设备、上下文等资源。
     * 必须在使用其他方法之前调用。
     *
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> initialize() = 0;

    /**
     * @brief 关闭音频后端
     *
     * 释放所有资源，停止所有声音。
     */
    virtual void shutdown() = 0;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] virtual bool isInitialized() const noexcept = 0;

    // ========================================================================
    // 听者控制
    // ========================================================================

    /**
     * @brief 设置听者位置
     *
     * @param position 世界坐标位置
     */
    virtual void setListenerPosition(const glm::vec3& position) = 0;

    /**
     * @brief 获取听者位置
     */
    [[nodiscard]] virtual glm::vec3 getListenerPosition() const noexcept = 0;

    /**
     * @brief 设置听者方向
     *
     * @param forward 前方向向量（单位向量）
     * @param up 上方向向量（单位向量）
     */
    virtual void setListenerOrientation(const glm::vec3& forward, const glm::vec3& up) = 0;

    /**
     * @brief 获取听者前方向
     */
    [[nodiscard]] virtual glm::vec3 getListenerForward() const noexcept = 0;

    /**
     * @brief 获取听者上方向
     */
    [[nodiscard]] virtual glm::vec3 getListenerUp() const noexcept = 0;

    /**
     * @brief 设置听者速度
     *
     * 用于多普勒效应计算。
     *
     * @param velocity 速度向量
     */
    virtual void setListenerVelocity(const glm::vec3& velocity) = 0;

    /**
     * @brief 获取听者速度
     */
    [[nodiscard]] virtual glm::vec3 getListenerVelocity() const noexcept = 0;

    /**
     * @brief 设置主音量
     *
     * @param gain 主音量 (0.0 = 静音, 1.0 = 正常)
     */
    virtual void setListenerGain(f32 gain) = 0;

    /**
     * @brief 获取主音量
     */
    [[nodiscard]] virtual f32 getListenerGain() const noexcept = 0;

    // ========================================================================
    // 音频缓冲区管理
    // ========================================================================

    /**
     * @brief 创建音频缓冲区
     *
     * @param data 音频数据
     * @return 缓冲区 ID，或错误
     */
    [[nodiscard]] virtual Result<AudioBufferId> createBuffer(const AudioData& data) = 0;

    /**
     * @brief 销毁音频缓冲区
     *
     * @param id 缓冲区 ID
     */
    virtual void destroyBuffer(AudioBufferId id) = 0;

    /**
     * @brief 检查缓冲区是否存在
     */
    [[nodiscard]] virtual bool hasBuffer(AudioBufferId id) const noexcept = 0;

    /**
     * @brief 获取音频缓冲区对象
     *
     * 用于将后端内部创建的缓冲区绑定到音频源。
     * 不存在时返回 nullptr。
     *
     * @param id 缓冲区 ID
     */
    [[nodiscard]] virtual std::shared_ptr<IAudioBuffer> getBuffer(AudioBufferId id) const = 0;

    // ========================================================================
    // 音频源管理
    // ========================================================================

    /**
     * @brief 创建音频源
     *
     * @return 音频源，或错误
     */
    [[nodiscard]] virtual Result<std::unique_ptr<IAudioSource>> createSource() = 0;

    /**
     * @brief 获取可用音频源数量
     *
     * @return 可创建的音频源数量
     */
    [[nodiscard]] virtual u32 getAvailableSources() const noexcept = 0;

    /**
     * @brief 获取最大音频源数量
     */
    [[nodiscard]] virtual u32 getMaxSources() const noexcept = 0;

    /**
     * @brief 获取当前活跃的音频源数量
     *
     * 活跃源指已通过 createSource() 创建但尚未销毁的源。
     *
     * @return 当前活跃的音频源数量
     */
    [[nodiscard]] virtual u32 getActiveSourceCount() const noexcept = 0;

    // ========================================================================
    // 更新
    // ========================================================================

    /**
     * @brief 处理音频更新
     *
     * 每帧调用一次，处理流式播放等。
     */
    virtual void process() = 0;

    // ========================================================================
    // 调试信息
    // ========================================================================

    /**
     * @brief 获取设备名称
     */
    [[nodiscard]] virtual std::string getDeviceName() const = 0;

    /**
     * @brief 获取调试字符串
     */
    [[nodiscard]] virtual std::string getDebugString() const = 0;
};

/**
 * @brief 创建 OpenAL 音频后端
 *
 * @return OpenAL 音频后端实例
 */
[[nodiscard]] std::unique_ptr<IAudioBackend> createOpenALBackend();

} // namespace mc::client::sound
