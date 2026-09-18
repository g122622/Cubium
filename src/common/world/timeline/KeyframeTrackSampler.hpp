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
#include "common/util/math/MathUtils.hpp"
#include "common/world/attribute/LerpFunction.hpp"
#include "common/world/timeline/EasingType.hpp"
#include "common/world/timeline/Keyframe.hpp"

#include <optional>
#include <vector>

namespace mc {
namespace world {
namespace timeline {

template <typename T>
class KeyframeTrack;

/**
 * @brief 关键帧轨道采样器
 *
 * MC 1.21.11 class KeyframeTrackSampler<T>。
 * 将关键帧轨道烘焙为段（Segment）列表，支持 period 回绕采样。
 *
 * 烘焙逻辑（bakeSegments）：
 * - 单 keyframe：返回单个 CONSTANT 段
 * - 多 keyframe + period：先加"回绕段"（from=last, fromTicks=last.ticks - period,
 *   to=first, toTicks=first.ticks），再加正常段，最后加"延伸段"
 *   （from=last, fromTicks=last.ticks, to=first, toTicks=first.ticks + period）
 */
template <typename T>
class KeyframeTrackSampler {
public:
    KeyframeTrackSampler(const KeyframeTrack<T>& track, std::optional<i32> periodTicks, attribute::LerpFunction<T> lerp)
        : m_periodTicks(periodTicks)
        , m_lerp(std::move(lerp))
        , m_segments(bakeSegments(track, periodTicks))
    {}

    /**
     * @brief 采样指定时间点的值
     *
     * @param tick 时间（gameTime 或 dayTime）
     */
    T sample(i64 tick) const
    {
        const i64 j = loopTicks(tick);
        const Segment<T>& segment = getSegmentAt(j);

        if (j <= static_cast<i64>(segment.fromTicks)) {
            return segment.fromValue;
        } else if (j >= static_cast<i64>(segment.toTicks)) {
            return segment.toValue;
        } else {
            const float f =
                static_cast<float>(j - segment.fromTicks) / static_cast<float>(segment.toTicks - segment.fromTicks);
            const float f1 = segment.easing.apply(f);
            return m_lerp.apply(f1, segment.fromValue, segment.toValue);
        }
    }

private:
    /// 采样段：定义一个 [fromTicks, toTicks] 区间内的插值
    template <typename U>
    struct Segment {
        EasingType easing;
        U fromValue;
        i32 fromTicks;
        U toValue;
        i32 toTicks;

        Segment(EasingType easing, U fromValue, i32 fromTicks, U toValue, i32 toTicks)
            : easing(easing)
            , fromValue(std::move(fromValue))
            , fromTicks(fromTicks)
            , toValue(std::move(toValue))
            , toTicks(toTicks)
        {}
    };

    static std::vector<Segment<T>> bakeSegments(const KeyframeTrack<T>& track, std::optional<i32> periodTicks)
    {
        const std::vector<Keyframe<T>>& list = track.keyframes();

        if (list.size() == 1) {
            const T& value = list[0].value();
            return {Segment<T>(EasingType::CONSTANT, value, 0, value, 0)};
        }

        std::vector<Segment<T>> result;

        if (periodTicks.has_value()) {
            const Keyframe<T>& first = list[0];
            const Keyframe<T>& last = list[list.size() - 1];
            // 回绕段：跨 period 边界从 last 回到 first
            result.emplace_back(
                track.easingType(), last.value(), last.ticks() - *periodTicks, first.value(), first.ticks());
            addSegmentsFromKeyframes(track, list, result);
            // 延伸段：从 last 延伸到下一周期的 first
            result.emplace_back(
                track.easingType(), last.value(), last.ticks(), first.value(), first.ticks() + *periodTicks);
        } else {
            addSegmentsFromKeyframes(track, list, result);
        }

        return result;
    }

    static void addSegmentsFromKeyframes(
        const KeyframeTrack<T>& track, const std::vector<Keyframe<T>>& list, std::vector<Segment<T>>& out)
    {
        for (size_t i = 0; i + 1 < list.size(); ++i) {
            const Keyframe<T>& k0 = list[i];
            const Keyframe<T>& k1 = list[i + 1];
            out.emplace_back(track.easingType(), k0.value(), k0.ticks(), k1.value(), k1.ticks());
        }
    }

    const Segment<T>& getSegmentAt(i64 i) const
    {
        for (const auto& segment : m_segments) {
            if (i < static_cast<i64>(segment.toTicks)) {
                return segment;
            }
        }
        return m_segments.back();
    }

    i64 loopTicks(i64 i) const
    {
        if (!m_periodTicks.has_value()) {
            return i;
        }
        return math::floorMod(i, static_cast<i64>(*m_periodTicks));
    }

    std::optional<i32> m_periodTicks;
    attribute::LerpFunction<T> m_lerp;
    std::vector<Segment<T>> m_segments;
};

} // namespace timeline
} // namespace world
} // namespace mc
