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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundTypes.hpp"
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace mc::sound {

/**
 * @brief 声音事件定义
 *
 * 声音事件是注册表中的条目，对应一个或多个声音文件。
 * 例如: "minecraft:block.stone.break" 是一个声音事件。
 *
 * 声音事件本身不包含音频数据，它只是一个标识符。
 * 客户端通过 SoundHandler 解析声音事件获取实际的声音定义。
 *
 * 参考: net.minecraft.util.SoundEvent
 *
 * 使用示例:
 * @code
 * // 创建声音事件
 * SoundEvent breakSound(ResourceLocation("minecraft:block.stone.break"));
 *
 * // 获取事件 ID
 * const ResourceLocation& id = breakSound.getId();
 *
 * // 设置衰减距离
 * breakSound.setAttenuationDistance(32.0f);
 * @endcode
 */
class SoundEvent {
public:
    /**
     * @brief 默认构造函数
     *
     * 创建一个空的声音事件（用于延迟初始化）。
     */
    SoundEvent() = default;

    /**
     * @brief 构造声音事件
     *
     * @param id 声音事件的资源位置（如 "minecraft:block.stone.break"）
     *
     * @note id 应该是有效的资源位置，但不验证声音是否存在
     */
    explicit SoundEvent(ResourceLocation id);

    /**
     * @brief 构造声音事件（从字符串解析）
     *
     * @param idString 声音事件 ID 字符串（如 "minecraft:block.stone.break"）
     */
    explicit SoundEvent(std::string_view idString);

    // 拷贝和移动
    SoundEvent(const SoundEvent&) = default;
    SoundEvent(SoundEvent&&) noexcept = default;
    SoundEvent& operator=(const SoundEvent&) = default;
    SoundEvent& operator=(SoundEvent&&) noexcept = default;

    // 比较
    [[nodiscard]] bool operator==(const SoundEvent& other) const { return m_id == other.m_id; }
    [[nodiscard]] bool operator!=(const SoundEvent& other) const { return m_id != other.m_id; }
    [[nodiscard]] bool operator<(const SoundEvent& other) const { return m_id < other.m_id; }

    // ========================================================================
    // 属性访问
    // ========================================================================

    /**
     * @brief 获取声音事件ID
     *
     * @return 声音事件的资源位置
     */
    [[nodiscard]] const ResourceLocation& getId() const noexcept { return m_id; }

    /**
     * @brief 检查声音事件是否有效（非空）
     *
     * @return true 如果声音事件有有效的 ID
     */
    [[nodiscard]] bool isValid() const noexcept { return !m_id.path().empty(); }

    /**
     * @brief 获取距离衰减距离
     *
     * 声音的可听距离（格）。
     * 默认值：16.0 格
     *
     * @return 衰减距离
     */
    [[nodiscard]] f32 getAttenuationDistance() const noexcept { return m_attenuationDistance; }

    /**
     * @brief 设置距离衰减距离
     *
     * @param distance 衰减距离（格），必须 > 0
     *
     * @note 更大的值表示声音可以在更远的距离被听到
     *       唱片机通常使用更大的衰减距离（如 64 格）
     */
    void setAttenuationDistance(f32 distance) noexcept;

    /**
     * @brief 获取字符串表示
     *
     * @return 声音事件 ID 的字符串表示
     */
    [[nodiscard]] std::string toString() const { return m_id.toString(); }

    // ========================================================================
    // 预定义的声音事件工厂方法
    // ========================================================================

    /**
     * @brief 创建空声音事件
     *
     * 用于表示"无声音"或默认值。
     *
     * @return 空声音事件
     */
    [[nodiscard]] static SoundEvent empty() { return SoundEvent(); }

private:
    ResourceLocation m_id;
    f32 m_attenuationDistance = DEFAULT_ATTENUATION_DISTANCE;
};

} // namespace mc::sound

// std::hash 特化
namespace std {
template <>
struct hash<mc::sound::SoundEvent> {
    size_t operator()(const mc::sound::SoundEvent& event) const noexcept { return event.getId().hash(); }
};
} // namespace std
