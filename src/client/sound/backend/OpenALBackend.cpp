#include "client/sound/backend/OpenALBackend.hpp"

#include "common/core/Result.hpp"

#include <AL/al.h>
#include <AL/alc.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace mc::client::sound {

// ============================================================================
// OpenALSource 实现
// ============================================================================

OpenALSource::OpenALSource(AudioSourceId id, ALuint source)
    : m_id(id)
    , m_source(source)
{
}

OpenALSource::~OpenALSource() {
    // 注意：source 的删除由 OpenALBackend 管理
    // 这里不删除，因为源可能被移动
}

OpenALSource::OpenALSource(OpenALSource&& other) noexcept
    : m_id(other.m_id)
    , m_source(other.m_source)
    , m_buffer(std::move(other.m_buffer))
{
    other.m_id = 0;
    other.m_source = 0;
}

OpenALSource& OpenALSource::operator=(OpenALSource&& other) noexcept {
    if (this != &other) {
        m_id = other.m_id;
        m_source = other.m_source;
        m_buffer = std::move(other.m_buffer);
        other.m_id = 0;
        other.m_source = 0;
    }
    return *this;
}

AudioSourceState OpenALSource::getState() const noexcept {
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

void OpenALSource::setBuffer(std::shared_ptr<IAudioBuffer> buffer) {
    if (!isValid()) {
        spdlog::warn("[OpenALSource] setBuffer called on invalid source");
        return;
    }

    m_buffer = std::move(buffer);

    if (m_buffer) {
        auto* alBuffer = dynamic_cast<OpenALBuffer*>(m_buffer.get());
        if (alBuffer) {
            alSourcei(m_source, AL_BUFFER, static_cast<ALint>(alBuffer->getALBuffer()));
            checkError("setBuffer");
        } else {
            spdlog::warn("[OpenALSource] Buffer is not an OpenALBuffer");
        }
    } else {
        alSourcei(m_source, AL_BUFFER, 0);
        checkError("setBuffer(null)");
    }
}

void OpenALSource::play() {
    if (!isValid()) {
        spdlog::warn("[OpenALSource] play called on invalid source");
        return;
    }

    alSourcePlay(m_source);
    checkError("play");
}

void OpenALSource::pause() {
    if (!isValid()) {
        spdlog::warn("[OpenALSource] pause called on invalid source");
        return;
    }

    alSourcePause(m_source);
    checkError("pause");
}

void OpenALSource::stop() {
    if (!isValid()) {
        spdlog::warn("[OpenALSource] stop called on invalid source");
        return;
    }

    alSourceStop(m_source);
    checkError("stop");
}

void OpenALSource::rewind() {
    if (!isValid()) {
        spdlog::warn("[OpenALSource] rewind called on invalid source");
        return;
    }

    alSourceRewind(m_source);
    checkError("rewind");
}

void OpenALSource::setGain(f32 gain) {
    if (!isValid()) {
        return;
    }

    alSourcef(m_source, AL_GAIN, std::max(0.0f, gain));
    checkError("setGain");
}

f32 OpenALSource::getGain() const noexcept {
    if (!isValid()) {
        return 0.0f;
    }

    ALfloat gain;
    alGetSourcef(m_source, AL_GAIN, &gain);
    return gain;
}

void OpenALSource::setPitch(f32 pitch) {
    if (!isValid()) {
        return;
    }

    // OpenAL 要求 pitch > 0
    alSourcef(m_source, AL_PITCH, std::max(0.001f, pitch));
    checkError("setPitch");
}

f32 OpenALSource::getPitch() const noexcept {
    if (!isValid()) {
        return 1.0f;
    }

    ALfloat pitch;
    alGetSourcef(m_source, AL_PITCH, &pitch);
    return pitch;
}

void OpenALSource::setPosition(const glm::vec3& position) {
    if (!isValid()) {
        return;
    }

    alSource3f(m_source, AL_POSITION, position.x, position.y, position.z);
    checkError("setPosition");
}

glm::vec3 OpenALSource::getPosition() const noexcept {
    if (!isValid()) {
        return glm::vec3(0.0f);
    }

    ALfloat x, y, z;
    alGetSource3f(m_source, AL_POSITION, &x, &y, &z);
    return glm::vec3(x, y, z);
}

void OpenALSource::setVelocity(const glm::vec3& velocity) {
    if (!isValid()) {
        return;
    }

    alSource3f(m_source, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    checkError("setVelocity");
}

glm::vec3 OpenALSource::getVelocity() const noexcept {
    if (!isValid()) {
        return glm::vec3(0.0f);
    }

    ALfloat x, y, z;
    alGetSource3f(m_source, AL_VELOCITY, &x, &y, &z);
    return glm::vec3(x, y, z);
}

void OpenALSource::setDirection(const glm::vec3& direction) {
    if (!isValid()) {
        return;
    }

    alSource3f(m_source, AL_DIRECTION, direction.x, direction.y, direction.z);
    checkError("setDirection");
}

glm::vec3 OpenALSource::getDirection() const noexcept {
    if (!isValid()) {
        return glm::vec3(0.0f);
    }

    ALfloat x, y, z;
    alGetSource3f(m_source, AL_DIRECTION, &x, &y, &z);
    return glm::vec3(x, y, z);
}

void OpenALSource::setRelative(bool relative) {
    if (!isValid()) {
        return;
    }

    alSourcei(m_source, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
    checkError("setRelative");
}

bool OpenALSource::isRelative() const noexcept {
    if (!isValid()) {
        return false;
    }

    ALint relative;
    alGetSourcei(m_source, AL_SOURCE_RELATIVE, &relative);
    return relative == AL_TRUE;
}

void OpenALSource::setReferenceDistance(f32 distance) {
    if (!isValid()) {
        return;
    }

    alSourcef(m_source, AL_REFERENCE_DISTANCE, std::max(0.0f, distance));
    checkError("setReferenceDistance");
}

f32 OpenALSource::getReferenceDistance() const noexcept {
    if (!isValid()) {
        return 0.0f;
    }

    ALfloat distance;
    alGetSourcef(m_source, AL_REFERENCE_DISTANCE, &distance);
    return distance;
}

void OpenALSource::setMaxDistance(f32 distance) {
    if (!isValid()) {
        return;
    }

    alSourcef(m_source, AL_MAX_DISTANCE, std::max(0.0f, distance));
    checkError("setMaxDistance");
}

f32 OpenALSource::getMaxDistance() const noexcept {
    if (!isValid()) {
        return 0.0f;
    }

    ALfloat distance;
    alGetSourcef(m_source, AL_MAX_DISTANCE, &distance);
    return distance;
}

void OpenALSource::setLooping(bool looping) {
    if (!isValid()) {
        return;
    }

    alSourcei(m_source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    checkError("setLooping");
}

bool OpenALSource::isLooping() const noexcept {
    if (!isValid()) {
        return false;
    }

    ALint looping;
    alGetSourcei(m_source, AL_LOOPING, &looping);
    return looping == AL_TRUE;
}

void OpenALSource::queueBuffers(const AudioBufferId* buffers, size_t count) {
    if (!isValid() || buffers == nullptr || count == 0) {
        return;
    }

    // 收集 OpenAL buffer handles
    std::vector<ALuint> alBuffers;
    alBuffers.reserve(count);

    // 注意：这里需要从 AudioBufferManager 获取实际的 OpenALBuffer
    // 这是一个简化实现，实际使用时需要传入 BufferManager 或映射表
    // 暂时跳过，流式播放将在 SoundEngine 层实现
    spdlog::warn("[OpenALSource] queueBuffers: streaming not fully implemented");

    if (!alBuffers.empty()) {
        alSourceQueueBuffers(m_source, static_cast<ALsizei>(alBuffers.size()), alBuffers.data());
        checkError("queueBuffers");
    }
}

u32 OpenALSource::unqueueBuffers(AudioBufferId* buffers, size_t count) {
    if (!isValid() || buffers == nullptr || count == 0) {
        return 0;
    }

    std::vector<ALuint> alBuffers(count);
    ALsizei processed = 0;

    alSourceUnqueueBuffers(m_source, static_cast<ALsizei>(count), alBuffers.data());

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::warn("[OpenALSource] unqueueBuffers failed: {}", alGetString(error));
        return 0;
    }

    // 将 ALuint 转换回 AudioBufferId
    // 这是一个简化实现，需要实际的映射
    processed = static_cast<ALsizei>(count);
    for (ALsizei i = 0; i < processed; ++i) {
        buffers[i] = static_cast<AudioBufferId>(alBuffers[i]);
    }

    return static_cast<u32>(processed);
}

u32 OpenALSource::getProcessedBuffers() const noexcept {
    if (!isValid()) {
        return 0;
    }

    ALint processed;
    alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);
    return static_cast<u32>(processed);
}

u32 OpenALSource::getQueuedBuffers() const noexcept {
    if (!isValid()) {
        return 0;
    }

    ALint queued;
    alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
    return static_cast<u32>(queued);
}

bool OpenALSource::checkError(const char* operation) const {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        spdlog::warn("[OpenALSource] Error in {}: {} (0x{:X})",
                     operation, alGetString(error), static_cast<u32>(error));
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
{
}

OpenALBuffer::~OpenALBuffer() {
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

OpenALBuffer& OpenALBuffer::operator=(OpenALBuffer&& other) noexcept {
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

Result<std::unique_ptr<OpenALBuffer>> OpenALBuffer::create(
    AudioBufferId id,
    const AudioData& data
) {
    if (!data.isValid()) {
        return Error(ErrorCode::InvalidData, "Invalid audio data");
    }

    // 创建 OpenAL 缓冲区
    ALuint buffer;
    alGenBuffers(1, &buffer);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        return Error(ErrorCode::OutOfMemory,
                     fmt::format("Failed to create OpenAL buffer: {}", alGetString(error)));
    }

    // 获取 OpenAL 格式
    ALenum format = OpenALBuffer::getALFormat(data.format);
    if (format == 0) {
        alDeleteBuffers(1, &buffer);
        return Error(ErrorCode::InvalidData,
                     fmt::format("Unsupported audio format: {} channels, {} bits",
                                 data.format.channels, data.format.bitsPerSample));
    }

    // 上传音频数据
    alBufferData(buffer, format, data.samples.data(),
                 static_cast<ALsizei>(data.samples.size()),
                 static_cast<ALsizei>(data.format.sampleRate));

    error = alGetError();
    if (error != AL_NO_ERROR) {
        alDeleteBuffers(1, &buffer);
        return Error(ErrorCode::OperationFailed,
                     fmt::format("Failed to upload audio data: {}", alGetString(error)));
    }

    return std::make_unique<OpenALBuffer>(id, buffer, data.format, data.duration);
}

ALenum OpenALBuffer::getALFormat(const AudioFormat& format) {
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

OpenALBackend::~OpenALBackend() {
    if (m_initialized) {
        shutdown();
    }
}

Result<void> OpenALBackend::initialize() {
    if (m_initialized) {
        return Error(ErrorCode::InvalidState, "Audio backend already initialized");
    }

    spdlog::info("[OpenALBackend] Initializing OpenAL...");

    // 打开默认音频设备
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        return Error(ErrorCode::InitializationFailed,
                     "Failed to open OpenAL device");
    }

    // 创建上下文
    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context) {
        const char* deviceName = alcGetString(m_device, ALC_DEVICE_SPECIFIER);
        alcCloseDevice(m_device);
        m_device = nullptr;
        return Error(ErrorCode::InitializationFailed,
                     fmt::format("Failed to create OpenAL context for device: {}",
                                 deviceName ? deviceName : "unknown"));
    }

    // 激活上下文
    if (!alcMakeContextCurrent(m_context)) {
        alcDestroyContext(m_context);
        alcCloseDevice(m_device);
        m_context = nullptr;
        m_device = nullptr;
        return Error(ErrorCode::InitializationFailed,
                     "Failed to make OpenAL context current");
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

    spdlog::info("[OpenALBackend] Max mono sources: {}, max stereo sources: {}",
                 maxMonoSources, maxStereoSources);

    // 设置默认距离模型
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    checkALError("alDistanceModel");

    // 设置默认听者属性
    setListenerPosition(glm::vec3(0.0f));
    setListenerOrientation(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    setListenerVelocity(glm::vec3(0.0f));
    setListenerGain(1.0f);

    m_initialized = true;

    spdlog::info("[OpenALBackend] Initialization complete");
    return {};
}

void OpenALBackend::shutdown() {
    if (!m_initialized) {
        return;
    }

    spdlog::info("[OpenALBackend] Shutting down...");

    // 清理所有缓冲区
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffers.clear();
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

    m_initialized = false;
    spdlog::info("[OpenALBackend] Shutdown complete");
}

void OpenALBackend::setListenerPosition(const glm::vec3& position) {
    if (!m_initialized) {
        return;
    }

    alListener3f(AL_POSITION, position.x, position.y, position.z);
    checkALError("setListenerPosition");
    m_listenerPosition = position;
}

glm::vec3 OpenALBackend::getListenerPosition() const noexcept {
    return m_listenerPosition;
}

void OpenALBackend::setListenerOrientation(const glm::vec3& forward, const glm::vec3& up) {
    if (!m_initialized) {
        return;
    }

    ALfloat orientation[] = {
        forward.x, forward.y, forward.z,
        up.x, up.y, up.z
    };
    alListenerfv(AL_ORIENTATION, orientation);
    checkALError("setListenerOrientation");
    m_listenerForward = forward;
    m_listenerUp = up;
}

glm::vec3 OpenALBackend::getListenerForward() const noexcept {
    return m_listenerForward;
}

glm::vec3 OpenALBackend::getListenerUp() const noexcept {
    return m_listenerUp;
}

void OpenALBackend::setListenerVelocity(const glm::vec3& velocity) {
    if (!m_initialized) {
        return;
    }

    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    checkALError("setListenerVelocity");
    m_listenerVelocity = velocity;
}

glm::vec3 OpenALBackend::getListenerVelocity() const noexcept {
    return m_listenerVelocity;
}

void OpenALBackend::setListenerGain(f32 gain) {
    if (!m_initialized) {
        return;
    }

    alListenerf(AL_GAIN, std::max(0.0f, gain));
    checkALError("setListenerGain");
    m_listenerGain = gain;
}

f32 OpenALBackend::getListenerGain() const noexcept {
    return m_listenerGain;
}

Result<AudioBufferId> OpenALBackend::createBuffer(const AudioData& data) {
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

    // 存储缓冲区
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffers[id] = std::move(result.value());
    }

    return id;
}

void OpenALBackend::destroyBuffer(AudioBufferId id) {
    if (!m_initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_bufferMutex);
    m_buffers.erase(id);
}

bool OpenALBackend::hasBuffer(AudioBufferId id) const noexcept {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return m_buffers.find(id) != m_buffers.end();
}

Result<std::unique_ptr<IAudioSource>> OpenALBackend::createSource() {
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
        return Error(ErrorCode::OutOfMemory,
                     fmt::format("Failed to create OpenAL source: {}", alGetString(error)));
    }

    // 分配 ID
    AudioSourceId id = m_nextSourceId++;

    return std::make_unique<OpenALSource>(id, source);
}

u32 OpenALBackend::getAvailableSources() const noexcept {
    if (!m_initialized) {
        return 0;
    }

    // OpenAL 不直接提供查询已创建源数量的方法
    // 我们使用固定的最大源数量
    // 实际可用数量取决于 alGenSources 是否成功
    return MAX_SOURCES; // 简化实现
}

void OpenALBackend::process() {
    // 当前实现没有需要每帧处理的逻辑
    // 流式播放的处理将在 SoundEngine 层实现
}

String OpenALBackend::getDeviceName() const {
    if (!m_initialized || !m_device) {
        return "Not initialized";
    }

    const char* name = alcGetString(m_device, ALC_DEVICE_SPECIFIER);
    return name ? name : "Unknown";
}

String OpenALBackend::getDebugString() const {
    if (!m_initialized) {
        return "Not initialized";
    }

    String result;
    result += fmt::format("Device: {}\n", getDeviceName());
    result += fmt::format("Vendor: {}\n", alGetString(AL_VENDOR));
    result += fmt::format("Version: {}\n", alGetString(AL_VERSION));
    result += fmt::format("Renderer: {}\n", alGetString(AL_RENDERER));

    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        result += fmt::format("Buffers: {}\n", m_buffers.size());
    }

    result += fmt::format("Listener position: ({}, {}, {})\n",
                          m_listenerPosition.x, m_listenerPosition.y, m_listenerPosition.z);
    result += fmt::format("Listener gain: {}\n", m_listenerGain);

    return result;
}

String OpenALBackend::checkALError(const char* operation) const {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        String msg = fmt::format("[OpenALBackend] Error in {}: {} (0x{:X})",
                                 operation, alGetString(error), static_cast<u32>(error));
        spdlog::warn("{}", msg);
        return msg;
    }
    return "";
}

// ============================================================================
// 工厂函数
// ============================================================================

std::unique_ptr<IAudioBackend> createOpenALBackend() {
    return std::make_unique<OpenALBackend>();
}

} // namespace mc::client::sound
