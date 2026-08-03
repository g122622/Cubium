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

#include "client/sound/AudioBufferCache.hpp"
#include "client/sound/SoundLoader.hpp"
#include "client/sound/backend/AudioBuffer.hpp"
#include "client/sound/backend/IAudioBackend.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc::client::sound {

// ============================================================================
// AudioBufferCache 实现
// ============================================================================

Result<std::shared_ptr<IAudioBuffer>> AudioBufferCache::getOrCreate(
    const ResourceLocation& location, IAudioBackend& backend, SoundLoader& loader)
{
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
        return Error(loadResult.error().code(), fmt::format("Failed to load audio: {}", loadResult.error().message()));
    }

    AudioData& audioData = loadResult.value();

    // 创建音频缓冲区
    auto bufferResult = backend.createBuffer(audioData);
    if (!bufferResult.success()) {
        return Error(
            bufferResult.error().code(), fmt::format("Failed to create buffer: {}", bufferResult.error().message()));
    }

    AudioBufferId bufferId = bufferResult.value();
    auto buffer = backend.getBuffer(bufferId);
    if (!buffer || !buffer->isValid()) {
        backend.destroyBuffer(bufferId);
        return Error(ErrorCode::OperationFailed,
            fmt::format("Failed to retrieve backend buffer after creation: {}", location.toString()));
    }

    // 存入缓存
    CacheEntry entry;
    entry.buffer = buffer;
    entry.isPreloaded = false;
    m_cache[location] = entry;

    return buffer;
}

size_t AudioBufferCache::preload(
    const std::vector<ResourceLocation>& locations, IAudioBackend& backend, SoundLoader& loader)
{
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
        }
    }

    if (successCount > 0) {
        spdlog::info("[AudioBufferCache] Preloaded {}/{} audio files", successCount, locations.size());
    }

    return successCount;
}

void AudioBufferCache::cleanupUnused()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t removedCount = 0;

    for (auto it = m_cache.begin(); it != m_cache.end();) {
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
        spdlog::info("[AudioBufferCache] Cleaned up {} unused buffers", removedCount);
    }
}

void AudioBufferCache::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_cache.clear();
    m_preloadedBuffers.clear();
}

size_t AudioBufferCache::getCacheSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.size();
}

size_t AudioBufferCache::getPreloadSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_preloadedBuffers.size();
}

} // namespace mc::client::sound
