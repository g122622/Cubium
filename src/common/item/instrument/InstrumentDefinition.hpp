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

#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <string>

namespace mc {
namespace text {
class ITextComponent;
} // namespace text

namespace item::instrument {

/**
 * @brief 乐器定义（对应 vanilla `net.minecraft.world.item.Instrument`）
 *
 * 描述一种可吹奏的乐器：音效事件、吹奏时长、可听距离与显示名。
 * 山羊角等乐器物品通过 `instrument` 数据组件引用乐器注册名，吹奏时按此定义播放音效。
 *
 * 字段对应数据包 `data/<namespace>/instrument/<path>.json`：
 * @code
 * {
 *   "description": { "translate": "instrument.minecraft.ponder_goat_horn" },
 *   "range": 256.0,
 *   "sound_event": "minecraft:item.goat_horn.sound.0",
 *   "use_duration": 7.0
 * }
 * @endcode
 */
class InstrumentDefinition {
public:
    /**
     * @brief 构造乐器
     *
     * @param soundEvent 音效事件资源位置
     * @param useDuration 吹奏时长（秒）；使用时长与冷却按其乘以 20 取整
     * @param range 可听距离（格）；播放音量取 range/16
     * @param description 显示名组件
     */
    InstrumentDefinition(
        ResourceLocation soundEvent, f32 useDuration, f32 range, std::unique_ptr<text::ITextComponent> description);

    InstrumentDefinition(const InstrumentDefinition& other);
    InstrumentDefinition& operator=(const InstrumentDefinition& other);
    InstrumentDefinition(InstrumentDefinition&& other) noexcept = default;
    InstrumentDefinition& operator=(InstrumentDefinition&& other) noexcept = default;
    ~InstrumentDefinition() = default;

    /**
     * @brief 音效事件资源位置
     */
    [[nodiscard]] const ResourceLocation& soundEvent() const noexcept
    {
        return m_soundEvent;
    }

    /**
     * @brief 吹奏时长（秒）
     */
    [[nodiscard]] f32 useDuration() const noexcept
    {
        return m_useDuration;
    }

    /**
     * @brief 可听距离（格）
     */
    [[nodiscard]] f32 range() const noexcept
    {
        return m_range;
    }

    /**
     * @brief 显示名组件
     */
    [[nodiscard]] const text::ITextComponent* description() const noexcept
    {
        return m_description.get();
    }

    /**
     * @brief 吹奏时长换算为使用时长（tick，`floor(useDuration * 20)`）
     */
    [[nodiscard]] i32 useDurationTicks() const noexcept;

    /**
     * @brief 播放音量倍率（`range / 16`）
     */
    [[nodiscard]] f32 volume() const noexcept;

private:
    ResourceLocation m_soundEvent;
    f32 m_useDuration;
    f32 m_range;
    std::unique_ptr<text::ITextComponent> m_description;
};

} // namespace item::instrument
} // namespace mc