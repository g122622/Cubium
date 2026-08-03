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

#include "client/sound/SoundLoader.hpp"
#include "client/sound/backend/AudioBuffer.hpp"
#include "client/sound/backend/IAudioBackend.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::client::sound {

/**
 * @brief 音频缓冲区缓存管理器
 *
 * 管理音频缓冲区的创建、缓存和生命周期。
 * 避免重复加载相同的音频文件，支持预加载机制。
 *
 * 线程安全：所有公共方法都是线程安全的。
 *
 * 使用示例:
 * @code
 * AudioBufferCache cache;
 *
 * // 获取或创建缓冲区（自动缓存）
 * auto result = cache.getOrCreate(location, backend, loader);
 * if (result.success()) {
 *     auto buffer = result.value();
 *     source->setBuffer(buffer);
 * }
 *
 * // 预加载常用音频
 * std::vector<ResourceLocation> preloadList = {...};
 * cache.preload(preloadList, backend, loader);
 *
 * // 定期清理未使用的缓冲区
 * cache.cleanupUnused();
 * @endcode
 */
class AudioBufferCache {
public:
    /**
     * @brief 构造音频缓冲区缓存
     */
    AudioBufferCache() = default;

    /**
     * @brief 析构函数
     */
    ~AudioBufferCache() = default;

    // 禁止拷贝
    AudioBufferCache(const AudioBufferCache&) = delete;
    AudioBufferCache& operator=(const AudioBufferCache&) = delete;

    // 允许移动
    AudioBufferCache(AudioBufferCache&&) noexcept = default;
    AudioBufferCache& operator=(AudioBufferCache&&) noexcept = default;

    // ========================================================================
    // 缓冲区管理
    // ========================================================================

    /**
     * @brief 获取或创建音频缓冲区
     *
     * 如果缓冲区已存在且有效，直接返回缓存的缓冲区。
     * 否则加载音频数据并创建新缓冲区。
     *
     * @param location 音频文件资源位置
     * @param backend 音频后端
     * @param loader 音频加载器
     * @return 音频缓冲区，或错误
     */
    [[nodiscard]] Result<std::shared_ptr<IAudioBuffer>> getOrCreate(
        const ResourceLocation& location, IAudioBackend& backend, SoundLoader& loader);

    /**
     * @brief 预加载音频文件
     *
     * 加载指定的音频文件到缓存中。
     * 预加载的缓冲区会被永久保留（直到调用 clear）。
     *
     * @param locations 音频文件资源位置列表
     * @param backend 音频后端
     * @param loader 音频加载器
     * @return 成功加载的数量
     */
    size_t preload(const std::vector<ResourceLocation>& locations, IAudioBackend& backend, SoundLoader& loader);

    /**
     * @brief 清理未使用的缓冲区
     *
     * 移除引用计数为 1 的缓冲区（仅被缓存持有）。
     * 预加载的缓冲区不会被清理。
     */
    void cleanupUnused();

    /**
     * @brief 清空所有缓存
     */
    void clear();

    // ========================================================================
    // 统计信息
    // ========================================================================

    /**
     * @brief 获取缓存中的缓冲区数量
     */
    [[nodiscard]] size_t getCacheSize() const;

    /**
     * @brief 获取预加载的缓冲区数量
     */
    [[nodiscard]] size_t getPreloadSize() const;

private:
    /**
     * @brief 缓存条目
     */
    struct CacheEntry {
        std::weak_ptr<IAudioBuffer> buffer; ///< 弱引用，允许缓冲区被释放
        bool isPreloaded = false;           ///< 是否为预加载缓冲区
    };

    /// 缓存映射（资源位置 -> 缓冲区）
    std::unordered_map<ResourceLocation, CacheEntry> m_cache;

    /// 预加载的缓冲区（强引用，永久保留）
    std::vector<std::shared_ptr<IAudioBuffer>> m_preloadedBuffers;

    /// 缓存互斥锁
    mutable std::mutex m_mutex;
};

} // namespace mc::client::sound
