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

#include "common/core/Types.hpp"
#include "common/world/timeline/EasingType.hpp"
#include "common/world/timeline/Keyframe.hpp"
#include "common/world/timeline/KeyframeTrackSampler.hpp"

#include <vector>

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 关键帧轨道
 *
 * MC 1.21.11 record KeyframeTrack<T>(List<Keyframe<T>> keyframes, EasingType easingType)。
 * 持有一组按 ticks 升序排列的关键帧，以及缓动类型。
 */
template <typename T>
class KeyframeTrack {
public:
    KeyframeTrack(std::vector<Keyframe<T>> keyframes, EasingType easingType)
        : m_keyframes(std::move(keyframes))
        , m_easingType(easingType)
    {}

    const std::vector<Keyframe<T>>& keyframes() const noexcept { return m_keyframes; }
    const EasingType& easingType() const noexcept { return m_easingType; }

    KeyframeTrackSampler<T> bakeSampler(std::optional<i32> periodTicks,
        LerpFunction<T> lerp) const
    {
        return KeyframeTrackSampler<T>(*this, periodTicks, std::move(lerp));
    }

    /**
     * @brief 关键帧轨道构建器
     */
    class Builder {
    public:
        Builder() = default;

        Builder& addKeyframe(i32 ticks, T value)
        {
            m_keyframes.emplace_back(ticks, std::move(value));
            return *this;
        }

        Builder& setEasing(EasingType easing)
        {
            m_easing = easing;
            return *this;
        }

        KeyframeTrack<T> build()
        {
            return KeyframeTrack<T>(std::move(m_keyframes), m_easing);
        }

    private:
        std::vector<Keyframe<T>> m_keyframes;
        EasingType m_easing = EasingType::LINEAR;
    };

private:
    std::vector<Keyframe<T>> m_keyframes;
    EasingType m_easingType;
};

} // namespace timeline
} // namespace world
} // namespace mc
