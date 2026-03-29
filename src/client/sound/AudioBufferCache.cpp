#include "client/sound/AudioBufferCache.hpp"

#include <spdlog/spdlog.h>

namespace mc::client::sound {

// ============================================================================
// AudioBufferWrapper 实现
// ============================================================================

AudioBufferWrapper::AudioBufferWrapper(AudioBufferId id, AudioFormat format, f32 duration, IAudioBackend* backend)
    : m_id(id)
    , m_format(format)
    , m_duration(duration)
    , m_backend(backend)
    , m_valid(id != 0)
{
    // 计算样本数量
    if (format.sampleRate > 0 && format.channels > 0 && format.bitsPerSample > 0) {
        m_sampleCount = static_cast<size_t>(duration * format.sampleRate);
    }
}

AudioBufferWrapper::~AudioBufferWrapper() {
    if (m_valid && m_backend && m_id != 0) {
        m_backend->destroyBuffer(m_id);
        m_valid = false;
    }
}

AudioBufferWrapper::AudioBufferWrapper(AudioBufferWrapper&& other) noexcept
    : m_id(other.m_id)
    , m_format(other.m_format)
    , m_duration(other.m_duration)
    , m_sampleCount(other.m_sampleCount)
    , m_backend(other.m_backend)
    , m_valid(other.m_valid)
{
    other.m_id = 0;
    other.m_valid = false;
    other.m_backend = nullptr;
}

AudioBufferWrapper& AudioBufferWrapper::operator=(AudioBufferWrapper&& other) noexcept {
    if (this != &other) {
        // 销毁当前缓冲区
        if (m_valid && m_backend && m_id != 0) {
            m_backend->destroyBuffer(m_id);
        }

        m_id = other.m_id;
        m_format = other.m_format;
        m_duration = other.m_duration;
        m_sampleCount = other.m_sampleCount;
        m_backend = other.m_backend;
        m_valid = other.m_valid;

        other.m_id = 0;
        other.m_valid = false;
        other.m_backend = nullptr;
    }
    return *this;
}

// ============================================================================
// AudioBufferCache 实现
// ============================================================================

Result<std::shared_ptr<IAudioBuffer>> AudioBufferCache::getOrCreate(
    const ResourceLocation& location,
    IAudioBackend& backend,
    SoundLoader& loader
) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查缓存中是否存在
    auto it = m_cache.find(location);
    if (it != m_cache.end()) {
        // 尝试获取弱引用
        auto buffer = it->second.buffer.lock();
        if (buffer && buffer->isValid()) {
            // 缓存命中，返回现有缓冲区
            return buffer;
        }
        // 缓冲区已失效，移除旧条目
        m_cache.erase(it);
    }

    // 加载音频数据
    auto loadResult = loader.load(location);
    if (!loadResult.success()) {
        return Error(loadResult.error().code(),
                     fmt::format("Failed to load audio: {}", loadResult.error().message()));
    }

    AudioData& audioData = loadResult.value();

    // 创建音频缓冲区
    auto bufferResult = backend.createBuffer(audioData);
    if (!bufferResult.success()) {
        return Error(bufferResult.error().code(),
                     fmt::format("Failed to create buffer: {}", bufferResult.error().message()));
    }

    AudioBufferId bufferId = bufferResult.value();

    // 创建缓冲区包装器
    auto buffer = std::make_shared<AudioBufferWrapper>(
        bufferId,
        audioData.format,
        audioData.duration,
        &backend
    );

    // 存入缓存
    CacheEntry entry;
    entry.buffer = buffer;
    entry.isPreloaded = false;
    m_cache[location] = entry;

    spdlog::trace("[AudioBufferCache] Created buffer for: {}", location.toString());

    return buffer;
}

size_t AudioBufferCache::preload(
    const std::vector<ResourceLocation>& locations,
    IAudioBackend& backend,
    SoundLoader& loader
) {
    size_t successCount = 0;

    for (const auto& location : locations) {
        auto result = getOrCreate(location, backend, loader);
        if (result.success()) {
            auto buffer = result.value();

            std::lock_guard<std::mutex> lock(m_mutex);

            // 标记为预加载
            auto it = m_cache.find(location);
            if (it != m_cache.end()) {
                it->second.isPreloaded = true;
            }

            // 保存强引用
            m_preloadedBuffers.push_back(buffer);
            ++successCount;

            spdlog::trace("[AudioBufferCache] Preloaded: {}", location.toString());
        } else {
            spdlog::debug("[AudioBufferCache] Failed to preload {}: {}",
                          location.toString(), result.error().message());
        }
    }

    if (successCount > 0) {
        spdlog::info("[AudioBufferCache] Preloaded {}/{} audio files",
                     successCount, locations.size());
    }

    return successCount;
}

void AudioBufferCache::cleanupUnused() {
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t removedCount = 0;

    for (auto it = m_cache.begin(); it != m_cache.end(); ) {
        // 跳过预加载的缓冲区
        if (it->second.isPreloaded) {
            ++it;
            continue;
        }

        // 检查弱引用是否失效（引用计数为 1 或更少）
        auto buffer = it->second.buffer.lock();
        if (!buffer || buffer.use_count() <= 1) {
            it = m_cache.erase(it);
            ++removedCount;
        } else {
            ++it;
        }
    }

    if (removedCount > 0) {
        spdlog::trace("[AudioBufferCache] Cleaned up {} unused buffers", removedCount);
    }
}

void AudioBufferCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_cache.clear();
    m_preloadedBuffers.clear();

    spdlog::trace("[AudioBufferCache] Cleared all buffers");
}

size_t AudioBufferCache::getCacheSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.size();
}

size_t AudioBufferCache::getPreloadSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_preloadedBuffers.size();
}

} // namespace mc::client::sound
