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

#include "client/sound/resource/SoundDefinition.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::client::sound {

/**
 * @brief 声音注册表
 *
 * 存储和管理所有声音事件定义。
 * 支持从多个资源包加载和合并声音定义。
 *
 * 线程安全：所有公共方法都是线程安全的。
 *
 * 使用示例:
 * @code
 * SoundRegistry registry;
 *
 * // 注册声音事件
 * SoundEventDefinition breakSound("minecraft:block.stone.break");
 * breakSound.sounds.push_back(SoundDefinition("minecraft:dig/stone1"));
 * breakSound.sounds.push_back(SoundDefinition("minecraft:dig/stone2"));
 * registry.registerSoundEvent(std::move(breakSound));
 *
 * // 获取声音事件定义
 * const SoundEventDefinition* def = registry.getSoundEvent(
 *     ResourceLocation("minecraft:block.stone.break")
 * );
 *
 * // 随机选择声音
 * if (def) {
 *     mc::math::Random rng;
 *     const SoundDefinition* sound = def->selectSound(rng);
 * }
 * @endcode
 */
class SoundRegistry {
public:
    /**
     * @brief 默认构造函数
     */
    SoundRegistry() = default;

    /**
     * @brief 析构函数
     */
    ~SoundRegistry() = default;

    // 禁止拷贝
    SoundRegistry(const SoundRegistry&) = delete;
    SoundRegistry& operator=(const SoundRegistry&) = delete;

    // 允许移动
    SoundRegistry(SoundRegistry&&) noexcept = default;
    SoundRegistry& operator=(SoundRegistry&&) noexcept = default;

    // ========================================================================
    // 声音事件管理
    // ========================================================================

    /**
     * @brief 注册声音事件定义
     *
     * 如果已存在同名声音事件：
     * - 如果 replace=true，则完全替换
     * - 如果 replace=false，则追加声音到现有定义
     *
     * @param definition 声音事件定义
     */
    void registerSoundEvent(SoundEventDefinition definition);

    /**
     * @brief 获取声音事件定义
     *
     * @param id 声音事件ID
     * @return 声音事件定义，不存在返回 nullptr
     */
    [[nodiscard]] const SoundEventDefinition* getSoundEvent(const ResourceLocation& id) const;

    /**
     * @brief 检查声音事件是否存在
     *
     * @param id 声音事件ID
     * @return true 如果存在
     */
    [[nodiscard]] bool hasSoundEvent(const ResourceLocation& id) const;

    /**
     * @brief 获取所有声音事件ID
     *
     * @return 所有注册的声音事件ID列表
     */
    [[nodiscard]] std::vector<ResourceLocation> getAllSoundEventIds() const;

    /**
     * @brief 获取声音事件数量
     */
    [[nodiscard]] size_t getSoundEventCount() const;

    // ========================================================================
    // 批量操作
    // ========================================================================

    /**
     * @brief 从另一个注册表合并声音事件
     *
     * 遵循资源包覆盖规则：后加载的资源包可以覆盖先加载的。
     *
     * @param other 源注册表
     */
    void merge(const SoundRegistry& other);

    /**
     * @brief 清空所有声音事件
     */
    void clear();

    // ========================================================================
    // 预加载管理
    // ========================================================================

    /**
     * @brief 获取需要预加载的声音列表
     *
     * @return 所有标记为 preload=true 的声音定义
     */
    [[nodiscard]] std::vector<ResourceLocation> getPreloadSounds() const;

private:
    /// 声音事件映射表
    std::unordered_map<ResourceLocation, SoundEventDefinition> m_soundEvents;

    /// 互斥锁（用于线程安全）
    mutable std::mutex m_mutex;
};

} // namespace mc::client::sound
