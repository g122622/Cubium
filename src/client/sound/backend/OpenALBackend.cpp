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

#include "client/sound/backend/OpenALBackend.hpp"
#include "common/core/Result.hpp"
#include "common/profiler/TraceEvents.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

using namespace mc::trace;

namespace mc::client::sound {

// ============================================================================
// OpenALSource 实现
// ============================================================================

OpenALSource::OpenALSource(AudioSourceId id,
    ALuint source,
    OnDestroyCallback onDestroy,
    LookupALBufferCallback lookupALBuffer,
    LookupBufferIdCallback lookupBufferId)
    : m_id(id)
    , m_source(source)
    , m_onDestroy(std::move(onDestroy))
    , m_lookupALBuffer(std::move(lookupALBuffer))
    , m_lookupBufferId(std::move(lookupBufferId))
{}

OpenALSource::~OpenALSource()
{
    // 由音频源对象负责自身 OpenAL 资源释放（RAII）
    // 仅在上下文可用时删除，避免关闭阶段误调用
    if (m_source != 0 && alcGetCurrentContext() != nullptr) {
        alDeleteSources(1, &m_source);
    }
    m_source = 0;

    // 通知后端释放源计数
    if (m_onDestroy) {
        m_onDestroy();
    }
}

OpenALSource::OpenALSource(OpenALSource&& other) noexcept
    : m_id(other.m_id)
    , m_source(other.m_source)
    , m_buffer(std::move(other.m_buffer))
    , m_onDestroy(std::move(other.m_onDestroy))
    , m_lookupALBuffer(std::move(other.m_lookupALBuffer))
    , m_lookupBufferId(std::move(other.m_lookupBufferId))
{
    other.m_id = 0;
    other.m_source = 0;
    other.m_onDestroy = nullptr;
    other.m_lookupALBuffer = nullptr;
    other.m_lookupBufferId = nullptr;
}

OpenALSource& OpenALSource::operator=(OpenALSource&& other) noexcept
{
    if (this != &other) {
        // 先释放当前资源
        if (m_source != 0 && alcGetCurrentContext() != nullptr) {
            alDeleteSources(1, &m_source);
        }

        // 通知后端释放当前源的计数（如果不是从自身移动）
        if (m_onDestroy) {
            m_onDestroy();
        }

        m_id = other.m_id;
        m_source = other.m_source;
        m_buffer = std::move(other.m_buffer);
        m_onDestroy = std::move(other.m_onDestroy);
        m_lookupALBuffer = std::move(other.m_lookupALBuffer);
        m_lookupBufferId = std::move(other.m_lookupBufferId);

        other.m_id = 0;
        other.m_source = 0;
        other.m_onDestroy = nullptr;
        other.m_lookupALBuffer = nullptr;
        other.m_lookupBufferId = nullptr;
    }
    return *this;
}

AudioSourceState OpenALSource::getState() const noexcept
{
    if (!isValid()) {
        return AudioSourceState::Initial;
    }

    ALint state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);

    switch (state) {
        case AL_INITIAL:
            return AudioSourceState::Initial;
        case AL_PLAYING:
            return AudioSourceState::Playing;
        case AL_PAUSED:
            return AudioSourceState::Paused;
        case AL_STOPPED:
            return AudioSourceState::Stopped;
        default:
            return AudioSourceState::Initial;
    }
}

void OpenALSource::setBuffer(std::shared_ptr<IAudioBuffer> buffer)
{
    if (!isValid()) {
        spdlog::warn("[OpenALSource] setBuffer called on invalid source");
        return;
    }

    m_buffer = std::move(buffer);

    if (m_buffer) {
        auto* alBuffer = dynamic_cast<OpenALBuffer*>(m_buffer.get());
        if (alBuffer) {
            alSourcei(m_source, AL_BUFFER, static_cast<ALint>(alBuffer->getALBuffer()));
            _checkError("setBuffer");
        } else {
            spdlog::error("[OpenALSource] Buffer is not an OpenALBuffer");
        }
    } else {
        alSourcei(m_source, AL_BUFFER, 0);
        _checkError("setBuffer(null)");
    }
}

void OpenALSource::play()
{
    if (!isValid()) {
        spdlog::error("[OpenALSource] play called on invalid source");
        return;
    }

    spdlog::info("[OpenALSource] Playing source (id={}), source handle: {}", m_id, m_source);
    alSourcePlay(m_source);
    _checkError("play");
}

void OpenALSource::pause()
{
    if (!isValid()) {
        spdlog::warn("[OpenALSource] pause called on invalid source");
        return;
    }

    alSourcePause(m_source);
    _checkError("pause");
}

void OpenALSource::stop()
{
    if (!isValid()) {
        spdlog::error("[OpenALSource] stop called on invalid source");
        return;
    }

    alSourceStop(m_source);
    _checkError("stop");
}

void OpenALSource::rewind()
{
    if (!isValid()) {
        spdlog::error("[OpenALSource] rewind called on invalid source");
        return;
    }

    alSourceRewind(m_source);
    _checkError("rewind");
}

void OpenALSource::setGain(f32 gain)
{
    if (!isValid()) {
        return;
    }

    alSourcef(m_source, AL_GAIN, std::max(0.0f, gain));
    _checkError("setGain");
}

f32 OpenALSource::getGain() const noexcept
{
    if (!isValid()) {
        return 0.0f;
    }

    ALfloat gain;
    alGetSourcef(m_source, AL_GAIN, &gain);
    return gain;
}

void OpenALSource::setPitch(f32 pitch)
{
    if (!isValid()) {
        return;
    }

    // OpenAL 要求 pitch > 0
    alSourcef(m_source, AL_PITCH, std::max(0.001f, pitch));
    _checkError("setPitch");
}

f32 OpenALSource::getPitch() const noexcept
{
    if (!isValid()) {
        return 1.0f;
    }

    ALfloat pitch;
    alGetSourcef(m_source, AL_PITCH, &pitch);
    return pitch;
}

void OpenALSource::setPosition(const glm::vec3& position)
{
    if (!isValid()) {
        return;
    }

    alSource3f(m_source, AL_POSITION, position.x, position.y, position.z);
    _checkError("setPosition");
}

glm::vec3 OpenALSource::getPosition() const noexcept
{
    if (!isValid()) {
        return glm::vec3(0.0f);
    }

    ALfloat x, y, z;
    alGetSource3f(m_source, AL_POSITION, &x, &y, &z);
    return glm::vec3(x, y, z);
}

void OpenALSource::setVelocity(const glm::vec3& velocity)
{
    if (!isValid()) {
        return;
    }

    alSource3f(m_source, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    _checkError("setVelocity");
}

glm::vec3 OpenALSource::getVelocity() const noexcept
{
    if (!isValid()) {
        return glm::vec3(0.0f);
    }

    ALfloat x, y, z;
    alGetSource3f(m_source, AL_VELOCITY, &x, &y, &z);
    return glm::vec3(x, y, z);
}

void OpenALSource::setDirection(const glm::vec3& direction)
{
    if (!isValid()) {
        return;
    }

    alSource3f(m_source, AL_DIRECTION, direction.x, direction.y, direction.z);
    _checkError("setDirection");
}

glm::vec3 OpenALSource::getDirection() const noexcept
{
    if (!isValid()) {
        return glm::vec3(0.0f);
    }

    ALfloat x, y, z;
    alGetSource3f(m_source, AL_DIRECTION, &x, &y, &z);
    return glm::vec3(x, y, z);
}

void OpenALSource::setRelative(bool relative)
{
    if (!isValid()) {
        return;
    }

    alSourcei(m_source, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
    _checkError("setRelative");
}

bool OpenALSource::isRelative() const noexcept
{
    if (!isValid()) {
        return false;
    }

    ALint relative;
    alGetSourcei(m_source, AL_SOURCE_RELATIVE, &relative);
    return relative == AL_TRUE;
}

void OpenALSource::setReferenceDistance(f32 distance)
{
    if (!isValid()) {
        return;
    }

    alSourcef(m_source, AL_REFERENCE_DISTANCE, std::max(0.0f, distance));
    _checkError("setReferenceDistance");
}

f32 OpenALSource::getReferenceDistance() const noexcept
{
    if (!isValid()) {
        return 0.0f;
    }

    ALfloat distance;
    alGetSourcef(m_source, AL_REFERENCE_DISTANCE, &distance);
    return distance;
}

void OpenALSource::setMaxDistance(f32 distance)
{
    if (!isValid()) {
        return;
    }

    alSourcef(m_source, AL_MAX_DISTANCE, std::max(0.0f, distance));
    _checkError("setMaxDistance");
}

f32 OpenALSource::getMaxDistance() const noexcept
{
    if (!isValid()) {
        return 0.0f;
    }

    ALfloat distance;
    alGetSourcef(m_source, AL_MAX_DISTANCE, &distance);
    return distance;
}

void OpenALSource::setLooping(bool looping)
{
    if (!isValid()) {
        return;
    }

    alSourcei(m_source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    _checkError("setLooping");
}

bool OpenALSource::isLooping() const noexcept
{
    if (!isValid()) {
        return false;
    }

    ALint looping;
    alGetSourcei(m_source, AL_LOOPING, &looping);
    return looping == AL_TRUE;
}

void OpenALSource::queueBuffers(const AudioBufferId* buffers, size_t count)
{
    if (!isValid() || buffers == nullptr || count == 0) {
        return;
    }

    // 收集 OpenAL buffer 句柄
    // 应用层 AudioBufferId 与 OpenAL 内部 ALuint 是两套独立的命名空间，
    // 必须通过 backend 维护的正向映射（AudioBufferId -> OpenALBuffer -> ALuint）翻译。
    // 若查找回调缺失（旧代码路径），仅打印警告并放弃本次入队。
    if (!m_lookupALBuffer) {
        spdlog::warn("[OpenALSource] queueBuffers: buffer lookup callback not configured");
        return;
    }

    std::vector<ALuint> alBuffers;
    alBuffers.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        AudioBufferId id = buffers[i];
        if (id == 0) {
            // 0 表示无效 ID，跳过以避免破坏后续入队顺序
            spdlog::warn("[OpenALSource] queueBuffers: encountered invalid buffer id at index {}", i);
            continue;
        }

        ALuint alBuffer = m_lookupALBuffer(id);
        if (alBuffer == 0) {
            // 找不到对应的 OpenALBuffer，跳过该 ID
            spdlog::warn("[OpenALSource] queueBuffers: buffer id {} not found in backend", id);
            continue;
        }

        alBuffers.push_back(alBuffer);
    }

    if (alBuffers.empty()) {
        // 没有可入队的有效 buffer，避免对空数组调用 alSourceQueueBuffers
        return;
    }

    alSourceQueueBuffers(m_source, static_cast<ALsizei>(alBuffers.size()), alBuffers.data());
    _checkError("queueBuffers");
}

u32 OpenALSource::unqueueBuffers(AudioBufferId* buffers, size_t count)
{
    if (!isValid() || buffers == nullptr || count == 0) {
        return 0;
    }

    // 先查询 OpenAL 实际已处理的 buffer 数量，取 min(processed, count) 作为本次出队数量。
    // 直接按 count 出队在 processed < count 时会触发 AL_INVALID_VALUE。
    ALint processed = 0;
    alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
    if (processed <= 0) {
        return 0;
    }

    const size_t toUnqueue = std::min(static_cast<size_t>(processed), count);
    if (toUnqueue == 0) {
        return 0;
    }

    std::vector<ALuint> alBuffers(toUnqueue);
    alSourceUnqueueBuffers(m_source, static_cast<ALsizei>(toUnqueue), alBuffers.data());

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::error("[OpenALSource] unqueueBuffers failed: {}", alGetString(error));
        return 0;
    }

    // 把 OpenAL ALuint 句柄反向翻译回应用层 AudioBufferId。
    // 若反向查找回调缺失（旧代码路径），无法正确翻译，对应位置写 0 并打印警告。
    if (!m_lookupBufferId) {
        spdlog::warn("[OpenALSource] unqueueBuffers: buffer id lookup callback not configured");
        for (size_t i = 0; i < toUnqueue; ++i) {
            buffers[i] = 0;
        }
        return static_cast<u32>(toUnqueue);
    }

    for (size_t i = 0; i < toUnqueue; ++i) {
        AudioBufferId id = m_lookupBufferId(alBuffers[i]);
        if (id == 0) {
            // 反向映射缺失通常意味着 buffer 已被 destroyBuffer 销毁，
            // 但 OpenAL 仍在队列中持有句柄——理论上不应该发生，记录警告。
            spdlog::warn("[OpenALSource] unqueueBuffers: alBuffer {} not found in reverse map", alBuffers[i]);
        }
        buffers[i] = id;
    }

    return static_cast<u32>(toUnqueue);
}

u32 OpenALSource::getProcessedBuffers() const noexcept
{
    if (!isValid()) {
        return 0;
    }

    ALint processed;
    alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
    return static_cast<u32>(processed);
}

u32 OpenALSource::getQueuedBuffers() const noexcept
{
    if (!isValid()) {
        return 0;
    }

    ALint queued;
    alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
    return static_cast<u32>(queued);
}

bool OpenALSource::_checkError(const char* operation) const
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::warn("[OpenALSource] Error in {}: {} (0x{:X})", operation, alGetString(error), static_cast<u32>(error));
        return true;
    }
    return false;
}

// ============================================================================
// OpenALBuffer 实现
// ============================================================================

OpenALBuffer::OpenALBuffer(AudioBufferId id, ALuint buffer, const AudioFormat& format, f32 duration)
    : m_id(id)
    , m_buffer(buffer)
    , m_format(format)
    , m_duration(duration)
    , m_sampleCount(static_cast<size_t>(format.sampleRate * duration))
{}

OpenALBuffer::~OpenALBuffer()
{
    if (m_buffer != 0) {
        alDeleteBuffers(1, &m_buffer);
        m_buffer = 0;
    }
}

OpenALBuffer::OpenALBuffer(OpenALBuffer&& other) noexcept
    : m_id(other.m_id)
    , m_buffer(other.m_buffer)
    , m_format(other.m_format)
    , m_duration(other.m_duration)
    , m_sampleCount(other.m_sampleCount)
{
    other.m_id = 0;
    other.m_buffer = 0;
}

OpenALBuffer& OpenALBuffer::operator=(OpenALBuffer&& other) noexcept
{
    if (this != &other) {
        // 删除当前缓冲区
        if (m_buffer != 0) {
            alDeleteBuffers(1, &m_buffer);
        }

        m_id = other.m_id;
        m_buffer = other.m_buffer;
        m_format = other.m_format;
        m_duration = other.m_duration;
        m_sampleCount = other.m_sampleCount;

        other.m_id = 0;
        other.m_buffer = 0;
    }
    return *this;
}

Result<std::unique_ptr<OpenALBuffer>> OpenALBuffer::create(AudioBufferId id, const AudioData& data)
{
    if (!data.isValid()) {
        return Error(ErrorCode::InvalidData, "Invalid audio data");
    }

    // 创建 OpenAL 缓冲区
    ALuint buffer;
    alGenBuffers(1, &buffer);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        return Error(ErrorCode::OutOfMemory, fmt::format("Failed to create OpenAL buffer: {}", alGetString(error)));
    }

    // 获取 OpenAL 格式
    ALenum format = OpenALBuffer::getALFormat(data.format);
    if (format == 0) {
        alDeleteBuffers(1, &buffer);
        return Error(ErrorCode::InvalidData,
            fmt::format(
                "Unsupported audio format: {} channels, {} bits", data.format.channels, data.format.bitsPerSample));
    }

    // 上传音频数据
    alBufferData(buffer,
        format,
        data.samples.data(),
        static_cast<ALsizei>(data.samples.size()),
        static_cast<ALsizei>(data.format.sampleRate));

    error = alGetError();
    if (error != AL_NO_ERROR) {
        alDeleteBuffers(1, &buffer);
        return Error(ErrorCode::OperationFailed, fmt::format("Failed to upload audio data: {}", alGetString(error)));
    }

    return std::make_unique<OpenALBuffer>(id, buffer, data.format, data.duration);
}

ALenum OpenALBuffer::getALFormat(const AudioFormat& format)
{
    if (format.channels == 1) {
        if (format.bitsPerSample == 8) {
            return AL_FORMAT_MONO8;
        } else if (format.bitsPerSample == 16) {
            return AL_FORMAT_MONO16;
        }
    } else if (format.channels == 2) {
        if (format.bitsPerSample == 8) {
            return AL_FORMAT_STEREO8;
        } else if (format.bitsPerSample == 16) {
            return AL_FORMAT_STEREO16;
        }
    }
    return 0; // 不支持的格式
}

// ============================================================================
// OpenALBackend 实现
// ============================================================================

OpenALBackend::OpenALBackend() = default;

OpenALBackend::~OpenALBackend()
{
    if (m_initialized) {
        shutdown();
    }
}

Result<void> OpenALBackend::initialize()
{
    if (m_initialized) {
        return Error(ErrorCode::InvalidState, "Audio backend already initialized");
    }

    spdlog::info("[OpenALBackend] Initializing OpenAL...");

    // 打开默认音频设备
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "OpenAL_OpenDevice", "phase", "open_device");
        m_device = alcOpenDevice(nullptr);
    }
    if (!m_device) {
        return Error(ErrorCode::InitializationFailed, "Failed to open OpenAL device");
    }

    // 创建上下文
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "OpenAL_CreateContext", "phase", "create_context");
        m_context = alcCreateContext(m_device, nullptr);
    }
    if (!m_context) {
        const char* deviceName = alcGetString(m_device, ALC_DEVICE_SPECIFIER);
        alcCloseDevice(m_device);
        m_device = nullptr;
        return Error(ErrorCode::InitializationFailed,
            fmt::format("Failed to create OpenAL context for device: {}", deviceName ? deviceName : "unknown"));
    }

    // 激活上下文
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Sound, "OpenAL_MakeContextCurrent", "phase", "make_context_current");
        if (!alcMakeContextCurrent(m_context)) {
            alcDestroyContext(m_context);
            alcCloseDevice(m_device);
            m_context = nullptr;
            m_device = nullptr;
            return Error(ErrorCode::InitializationFailed, "Failed to make OpenAL context current");
        }
    }

    // 打印设备信息
    const char* deviceName = alcGetString(m_device, ALC_DEVICE_SPECIFIER);
    const char* vendor = alGetString(AL_VENDOR);
    const char* version = alGetString(AL_VERSION);
    const char* renderer = alGetString(AL_RENDERER);

    spdlog::info("[OpenALBackend] Device: {}", deviceName ? deviceName : "unknown");
    spdlog::info("[OpenALBackend] Vendor: {}", vendor ? vendor : "unknown");
    spdlog::info("[OpenALBackend] Version: {}", version ? version : "unknown");
    spdlog::info("[OpenALBackend] Renderer: {}", renderer ? renderer : "unknown");

    // 查询最大源数量
    ALint maxMonoSources, maxStereoSources;
    alcGetIntegerv(m_device, ALC_MONO_SOURCES, 1, &maxMonoSources);
    alcGetIntegerv(m_device, ALC_STEREO_SOURCES, 1, &maxStereoSources);

    spdlog::info("[OpenALBackend] Max mono sources: {}, max stereo sources: {}", maxMonoSources, maxStereoSources);

    // 根据设备属性确定最大源数量
    // ALC_MONO_SOURCES 和 ALC_STEREO_SOURCES 是提示属性，某些驱动可能返回 0
    // 参考 MC Library.getChannelCount() 的做法，使用总源数作为上限
    constexpr u32 MIN_SOURCES = 16;
    if (maxMonoSources > 0 || maxStereoSources > 0) {
        m_maxSources = static_cast<u32>(maxMonoSources) + static_cast<u32>(maxStereoSources);
        // 确保不超过合理上限
        m_maxSources = std::min(m_maxSources, ::mc::sound::MAX_CONCURRENT_SOUNDS);
        // 确保至少有最小源数量
        m_maxSources = std::max(m_maxSources, MIN_SOURCES);
    } else {
        // 驱动未提供信息，使用默认值
        m_maxSources = ::mc::sound::MAX_CONCURRENT_SOUNDS;
    }

    spdlog::info("[OpenALBackend] Max sources: {}", m_maxSources);

    // 设置默认距离模型
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    static_cast<void>(_checkALError("alDistanceModel"));

    // 设置默认听者属性
    setListenerPosition(glm::vec3(0.0f));
    setListenerOrientation(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    setListenerVelocity(glm::vec3(0.0f));
    setListenerGain(1.0f);

    m_initialized = true;

    spdlog::info("[OpenALBackend] Initialization complete");
    return {};
}

void OpenALBackend::shutdown()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("[OpenALBackend] Shutting down...");

    // 清理所有缓冲区（正向/反向映射一并清空）
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffers.clear();
        m_alBufferToId.clear();
    }

    // 停止所有源并清理上下文
    if (m_context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        m_context = nullptr;
    }

    // 关闭设备
    if (m_device) {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }

    // 重置状态
    m_activeSourceCount.store(0, std::memory_order::relaxed);
    m_maxSources = ::mc::sound::MAX_CONCURRENT_SOUNDS;
    m_initialized = false;
    spdlog::info("[OpenALBackend] Shutdown complete");
}

void OpenALBackend::setListenerPosition(const glm::vec3& position)
{
    if (!m_initialized) {
        return;
    }

    alListener3f(AL_POSITION, position.x, position.y, position.z);
    static_cast<void>(_checkALError("setListenerPosition"));
    m_listenerPosition = position;
}

glm::vec3 OpenALBackend::getListenerPosition() const noexcept
{
    return m_listenerPosition;
}

void OpenALBackend::setListenerOrientation(const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_initialized) {
        return;
    }

    ALfloat orientation[] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};
    alListenerfv(AL_ORIENTATION, orientation);
    static_cast<void>(_checkALError("setListenerOrientation"));
    m_listenerForward = forward;
    m_listenerUp = up;
}

glm::vec3 OpenALBackend::getListenerForward() const noexcept
{
    return m_listenerForward;
}

glm::vec3 OpenALBackend::getListenerUp() const noexcept
{
    return m_listenerUp;
}

void OpenALBackend::setListenerVelocity(const glm::vec3& velocity)
{
    if (!m_initialized) {
        return;
    }

    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    static_cast<void>(_checkALError("setListenerVelocity"));
    m_listenerVelocity = velocity;
}

glm::vec3 OpenALBackend::getListenerVelocity() const noexcept
{
    return m_listenerVelocity;
}

void OpenALBackend::setListenerGain(f32 gain)
{
    if (!m_initialized) {
        return;
    }

    alListenerf(AL_GAIN, std::max(0.0f, gain));
    static_cast<void>(_checkALError("setListenerGain"));
    m_listenerGain = gain;
}

f32 OpenALBackend::getListenerGain() const noexcept
{
    return m_listenerGain;
}

Result<AudioBufferId> OpenALBackend::createBuffer(const AudioData& data)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "Audio backend not initialized");
    }

    if (!data.isValid()) {
        return Error(ErrorCode::InvalidData, "Invalid audio data");
    }

    // 分配新 ID
    AudioBufferId id = m_nextBufferId++;

    // 创建缓冲区
    auto result = OpenALBuffer::create(id, data);
    if (!result.success()) {
        return result.error();
    }

    std::shared_ptr<OpenALBuffer> buffer = result.value();

    // 存储缓冲区（同时维护正向/反向映射，保持两者一致）
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        ALuint alBuffer = buffer->getALBuffer();
        m_buffers[id] = std::move(buffer);
        m_alBufferToId[alBuffer] = id;
    }

    return id;
}

void OpenALBackend::destroyBuffer(AudioBufferId id)
{
    if (!m_initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_bufferMutex);

    auto it = m_buffers.find(id);
    if (it == m_buffers.end()) {
        return;
    }

    // 同步删除反向映射，保持正向/反向映射一致
    ALuint alBuffer = it->second->getALBuffer();
    m_alBufferToId.erase(alBuffer);
    m_buffers.erase(it);
}

bool OpenALBackend::hasBuffer(AudioBufferId id) const noexcept
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return m_buffers.find(id) != m_buffers.end();
}

std::shared_ptr<IAudioBuffer> OpenALBackend::getBuffer(AudioBufferId id) const
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    auto it = m_buffers.find(id);
    if (it == m_buffers.end()) {
        return nullptr;
    }

    return it->second;
}

ALuint OpenALBackend::_lookupALBuffer(AudioBufferId id) const
{
    if (id == 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_bufferMutex);
    auto it = m_buffers.find(id);
    if (it == m_buffers.end()) {
        return 0;
    }
    return it->second->getALBuffer();
}

AudioBufferId OpenALBackend::_lookupBufferId(ALuint alBuffer) const
{
    if (alBuffer == 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_bufferMutex);
    auto it = m_alBufferToId.find(alBuffer);
    if (it == m_alBufferToId.end()) {
        return 0;
    }
    return it->second;
}

Result<std::unique_ptr<IAudioSource>> OpenALBackend::createSource()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "Audio backend not initialized");
    }

    // 检查可用源数量
    if (getAvailableSources() == 0) {
        return Error(ErrorCode::ResourceExhausted, "No available audio sources");
    }

    // 创建 OpenAL source
    ALuint source;
    alGenSources(1, &source);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        return Error(ErrorCode::OutOfMemory, fmt::format("Failed to create OpenAL source: {}", alGetString(error)));
    }

    // 分配 ID
    AudioSourceId id = m_nextSourceId++;

    // 递增活跃源计数
    m_activeSourceCount.fetch_add(1, std::memory_order::relaxed);

    // 创建源销毁回调，当源被销毁时递减活跃源计数
    auto onDestroy = [this]() { m_activeSourceCount.fetch_sub(1, std::memory_order::relaxed); };

    // 创建缓冲区查找回调，供 OpenALSource 在流式播放 queue/unqueue 时
    // 在 AudioBufferId 与 OpenAL ALuint 句柄之间转换。
    // 回调内部加锁访问 m_buffers / m_alBufferToId，OpenALSource 调用时无需持锁。
    auto lookupALBuffer = [this](AudioBufferId id) { return this->_lookupALBuffer(id); };
    auto lookupBufferId = [this](ALuint alBuffer) { return this->_lookupBufferId(alBuffer); };

    std::unique_ptr<IAudioSource> audioSource = std::make_unique<OpenALSource>(
        id, source, std::move(onDestroy), std::move(lookupALBuffer), std::move(lookupBufferId));
    return audioSource;
}

u32 OpenALBackend::getAvailableSources() const noexcept
{
    if (!m_initialized) {
        return 0;
    }

    u32 active = m_activeSourceCount.load(std::memory_order::relaxed);
    return active >= m_maxSources ? 0 : m_maxSources - active;
}

u32 OpenALBackend::getActiveSourceCount() const noexcept
{
    return m_activeSourceCount.load(std::memory_order::relaxed);
}

void OpenALBackend::process()
{
    // 流式播放的缓冲区队列管理在 SoundEngine 层通过 ActiveChannel 实现，
    // 后端层无需每帧处理逻辑
}

std::string OpenALBackend::getDeviceName() const
{
    if (!m_initialized || !m_device) {
        return "Not initialized";
    }

    const char* name = alcGetString(m_device, ALC_DEVICE_SPECIFIER);
    return name ? name : "Unknown";
}

std::string OpenALBackend::getDebugString() const
{
    if (!m_initialized) {
        return "Not initialized";
    }

    std::string result;
    result += fmt::format("Device: {}\n", getDeviceName());
    result += fmt::format("Vendor: {}\n", alGetString(AL_VENDOR));
    result += fmt::format("Version: {}\n", alGetString(AL_VERSION));
    result += fmt::format("Renderer: {}\n", alGetString(AL_RENDERER));

    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        result += fmt::format("Buffers: {}\n", m_buffers.size());
    }

    u32 activeSources = m_activeSourceCount.load(std::memory_order::relaxed);
    result += fmt::format("Sources: {}/{}\n", activeSources, m_maxSources);
    result += fmt::format(
        "Listener position: ({}, {}, {})\n", m_listenerPosition.x, m_listenerPosition.y, m_listenerPosition.z);
    result += fmt::format("Listener gain: {}\n", m_listenerGain);

    return result;
}

std::string OpenALBackend::_checkALError(const char* operation) const
{
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::string msg = fmt::format(
            "[OpenALBackend] Error in {}: {} (0x{:X})", operation, alGetString(error), static_cast<u32>(error));
        spdlog::warn("{}", msg);
        return msg;
    }
    return "";
}

// ============================================================================
// 工厂函数
// ============================================================================

std::unique_ptr<IAudioBackend> createOpenALBackend()
{
    return std::make_unique<OpenALBackend>();
}

} // namespace mc::client::sound
