#include "client/sound/backend/AudioBuffer.hpp"

#include "common/resource/ResourcePackList.hpp"

#include <algorithm>
#include <fmt/format.h>

namespace mc::client::sound {

// ============================================================================
// AudioData 实现
// ============================================================================

AudioData::AudioData(AudioFormat format, std::vector<u8> data)
    : format(format)
    , samples(std::move(data))
{
    duration = calculateDuration();
}

size_t AudioData::sampleCount() const noexcept {
    if (format.channels == 0 || format.bitsPerSample == 0) {
        return 0;
    }

    size_t bytesPerSample = format.bitsPerSample / 8;
    return samples.size() / (format.channels * bytesPerSample);
}

f32 AudioData::calculateDuration() const noexcept {
    if (format.sampleRate == 0 || format.channels == 0) {
        return 0.0f;
    }

    size_t bytesPerSample = format.bitsPerSample / 8;
    if (bytesPerSample == 0) {
        return 0.0f;
    }

    size_t totalSamples = samples.size() / (format.channels * bytesPerSample);
    return static_cast<f32>(totalSamples) / static_cast<f32>(format.sampleRate);
}

// ============================================================================
// AudioBufferManager 实现
// ============================================================================

Result<std::shared_ptr<IAudioBuffer>> AudioBufferManager::load(
    const ResourceLocation& location
) {
    // 检查缓存
    auto it = m_buffers.find(location);
    if (it != m_buffers.end()) {
        // 返回已缓存的缓冲区
        auto buffer = it->second.lock();
        if (buffer) {
            return buffer;
        }
        // 弱引用已过期，移除并重新加载
        m_buffers.erase(it);
    }

    // TODO: 实现音频加载
    // 需要:
    // 1. 从 ResourcePackList 加载 OGG 文件
    // 2. 解码 OGG Vorbis 数据
    // 3. 创建 AudioData
    // 4. 调用 IAudioBackend::createBuffer()
    //
    // 这将在 SoundLoader 中实现，AudioBufferManager 需要
    // 对 IAudioBackend 和 ResourcePackList 的引用

    return Error(ErrorCode::Unsupported,
                 fmt::format("Audio loading not yet implemented for: {}", location.toString()));
}

std::shared_ptr<IAudioBuffer> AudioBufferManager::get(
    const ResourceLocation& location
) const {
    auto it = m_buffers.find(location);
    if (it != m_buffers.end()) {
        return it->second.lock();
    }
    return nullptr;
}

bool AudioBufferManager::has(const ResourceLocation& location) const {
    auto it = m_buffers.find(location);
    if (it != m_buffers.end()) {
        return !it->second.expired();
    }
    return false;
}

bool AudioBufferManager::unload(const ResourceLocation& location) {
    return m_buffers.erase(location) > 0;
}

void AudioBufferManager::unloadAll() {
    m_buffers.clear();
}

size_t AudioBufferManager::cacheSize() const noexcept {
    size_t count = 0;
    for (const auto& [key, weakBuffer] : m_buffers) {
        if (!weakBuffer.expired()) {
            ++count;
        }
    }
    return count;
}

size_t AudioBufferManager::bufferCount() const noexcept {
    return m_buffers.size();
}

} // namespace mc::client::sound
