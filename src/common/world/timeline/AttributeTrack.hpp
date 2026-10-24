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

#include "common/world/attribute/AttributeModifier.hpp"
#include "common/world/timeline/EasingType.hpp"
#include "common/world/timeline/KeyframeTrack.hpp"

#include <memory>
#include <optional>
#include <utility>

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 属性轨道
 *
 * MC 1.21.11 record AttributeTrack<Value, Argument>。
 * 将一个属性修改器与关键帧轨道绑定，用于在时间线上驱动该属性。
 *
 * 修改器以 shared_ptr 持有：Timeline 需存入 std::any（要求可拷贝），
 * 而修改器是多态对象（抽象基类），故用共享所有权。
 *
 * easing 为空表示沿用关键帧轨道自带的缓动；非空时（vanilla 数据包把 `ease`
 * 写在轨道级别、由 Timeline.Builder.addModifierTrack 显式传入）覆盖轨道缓动。
 */
template <typename Value, typename Argument>
class AttributeTrack {
public:
    AttributeTrack(std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> modifier,
        KeyframeTrack<Argument> argumentTrack)
        : m_modifier(std::move(modifier))
        , m_argumentTrack(std::move(argumentTrack))
    {}

    AttributeTrack(std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> modifier,
        KeyframeTrack<Argument> argumentTrack,
        std::optional<EasingType> easing)
        : m_modifier(std::move(modifier))
        , m_argumentTrack(std::move(argumentTrack))
        , m_easing(std::move(easing))
    {}

    [[nodiscard]] const attribute::AttributeModifier<Value, Argument>& modifier() const noexcept
    {
        return *m_modifier;
    }

    /// 获取修改器的共享所有权，供采样器在轨道之外独立持有时使用
    [[nodiscard]] std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> modifierPtr() const noexcept
    {
        return m_modifier;
    }

    [[nodiscard]] const KeyframeTrack<Argument>& argumentTrack() const noexcept
    {
        return m_argumentTrack;
    }

    /// 轨道级缓动覆盖（空表示沿用关键帧轨道自带缓动）
    [[nodiscard]] const std::optional<EasingType>& easingOverride() const noexcept
    {
        return m_easing;
    }

    /// 返回覆盖了轨道级缓动后的关键帧轨道（无覆盖时原样返回）
    [[nodiscard]] KeyframeTrack<Value> effectiveArgumentTrack() const
    {
        // 采样得到的参数在时间线上以"属性的值表示"出现，因此关键帧轨道按 Value 暴露
        std::vector<Keyframe<Value>> keyframes;
        keyframes.reserve(m_argumentTrack.keyframes().size());
        for (const auto& keyframe : m_argumentTrack.keyframes()) {
            keyframes.emplace_back(keyframe.ticks(), static_cast<Value>(keyframe.value()));
        }

        if (!m_easing.has_value()) {
            return KeyframeTrack<Value>(std::move(keyframes), m_argumentTrack.easingType());
        }
        return KeyframeTrack<Value>(std::move(keyframes), *m_easing);
    }

private:
    std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> m_modifier;
    KeyframeTrack<Argument> m_argumentTrack;
    std::optional<EasingType> m_easing;
};

} // namespace timeline
} // namespace world
} // namespace mc
