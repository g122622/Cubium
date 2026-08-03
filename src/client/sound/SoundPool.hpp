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

#include "client/sound/instance/ISoundInstance.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::client::sound {

/**
 * @brief 声音池
 *
 * 管理活动声音实例的生命周期。
 * 负责声音实例的创建、存储、查找和销毁。
 *
 * 参考: net.minecraft.client.audio.SoundEngine 中的声音管理部分
 *
 * 使用示例:
 * @code
 * SoundPool pool;
 *
 * // 添加声音
 * auto sound = std::make_unique<SoundInstance>(...);
 * SoundInstanceId id = pool.add(std::move(sound));
 *
 * // 查询声音
 * ISoundInstance* sound = pool.get(id);
 * if (sound && !sound->isDone()) {
 *     // 使用声音
 * }
 *
 * // 按类别停止
 * pool.stopByCategory(SoundCategory::Music);
 *
 * // 每帧清理已完成的声音
 * pool.tick();
 * @endcode
 */
class SoundPool {
public:
    SoundPool() = default;
    ~SoundPool() = default;

    // 禁止拷贝
    SoundPool(const SoundPool&) = delete;
    SoundPool& operator=(const SoundPool&) = delete;

    // 允许移动
    SoundPool(SoundPool&&) noexcept = default;
    SoundPool& operator=(SoundPool&&) noexcept = default;

    // ========================================================================
    // 声音管理
    // ========================================================================

    /**
     * @brief 添加声音实例
     *
     * @param sound 声音实例
     * @return 声音实例ID
     */
    SoundInstanceId add(std::unique_ptr<ISoundInstance> sound);

    /**
     * @brief 获取声音实例
     *
     * @param id 声音实例ID
     * @return 声音实例，不存在返回 nullptr
     */
    [[nodiscard]] ISoundInstance* get(SoundInstanceId id);

    /**
     * @brief 获取声音实例（const版本）
     *
     * @param id 声音实例ID
     * @return 声音实例，不存在返回 nullptr
     */
    [[nodiscard]] const ISoundInstance* get(SoundInstanceId id) const;

    /**
     * @brief 移除声音实例
     *
     * @param id 声音实例ID
     * @return 是否成功移除
     */
    bool remove(SoundInstanceId id);

    /**
     * @brief 移除所有声音实例
     */
    void clear();

    // ========================================================================
    // 查询
    // ========================================================================

    /**
     * @brief 检查声音是否存在
     *
     * @param id 声音实例ID
     * @return 是否存在
     */
    [[nodiscard]] bool has(SoundInstanceId id) const { return m_sounds.contains(id); }

    /**
     * @brief 获取声音数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_sounds.size(); }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return m_sounds.empty(); }

    // ========================================================================
    // 类别管理
    // ========================================================================

    /**
     * @brief 获取指定类别的所有声音ID
     *
     * @param category 声音类别
     * @return 声音ID列表
     */
    [[nodiscard]] std::vector<SoundInstanceId> getByCategory(SoundCategory category) const;

    /**
     * @brief 移除指定类别的所有声音
     *
     * @param category 声音类别
     * @return 移除的声音数量
     */
    size_t removeByCategory(SoundCategory category);

    /**
     * @brief 获取指定声音事件的所有声音ID
     *
     * @param soundEventId 声音事件ID
     * @return 声音ID列表
     */
    [[nodiscard]] std::vector<SoundInstanceId> getBySoundEvent(const ResourceLocation& soundEventId) const;

    /**
     * @brief 移除指定声音事件的所有声音
     *
     * @param soundEventId 声音事件ID
     * @return 移除的声音数量
     */
    size_t removeBySoundEvent(const ResourceLocation& soundEventId);

    // ========================================================================
    // 更新
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 清理已完成的声音实例。
     *
     * @return 清理的声音数量
     */
    size_t tick();

    // ========================================================================
    // 迭代器
    // ========================================================================

    /**
     * @brief 获取所有活动声音
     */
    [[nodiscard]] const std::unordered_map<SoundInstanceId, std::unique_ptr<ISoundInstance>>& getAll() const noexcept
    {
        return m_sounds;
    }

private:
    /// 下一个声音ID
    SoundInstanceId m_nextId = 1;

    /// 声音实例映射
    std::unordered_map<SoundInstanceId, std::unique_ptr<ISoundInstance>> m_sounds;

    /// 声音事件到声音ID的映射
    std::unordered_multimap<ResourceLocation, SoundInstanceId> m_soundEventMap;

    /// 类别到声音ID的映射
    std::unordered_multimap<SoundCategory, SoundInstanceId> m_categoryMap;
};

} // namespace mc::client::sound
