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
            // 本工程 Release 下开了 -ffast-math，它会把下面那次除法改写成乘倒数。两者在 IEEE 754
            // 下不等价：49f/50f 得 0x3f7ae148，而 49f*(1/50f) 得 0x3f7ae147。进度差 1 ulp 会顺着
            // 缓动放大成可见的插值误差（timeline_sample 差分测试的 three_out_bounce@11 即此），
            // 因此除法必须落在关闭了倒数优化的独立函数里。
            //
            // 不能在此处直接写 `#pragma clang fp reciprocal(off)`：该 pragma 只能在文件作用域或
            // 复合语句**开头**生效，写在 else 体里会报 "only at file scope or at the start of a
            // compound statement"，而本头文件被上百个 TU 包含，会直接卡住整个构建。
            return _interpolate(j, segment);
        }
    }

private:
    /// 段内线性插值：除法按 IEEE 754 执行，不得被 -ffast-math 改写成乘倒数。
    ///
    /// 两条编译器路径：
    /// - clang-cl：局部 `#pragma clang fp` 不可用，用 noinline 让实现与应用 -ffast-math 的
    ///   调用方隔离开，避免内联后被重新优化成乘倒数。
    /// - MSVC：cl 支持局部 `#pragma float_control(precise, on)`，内联即可，零额外开销。
    ///   （MSVC 不认 `__attribute__((noinline))`，所以两条路径必须分开写。）
#if defined(_MSC_VER) && !defined(__clang__)
    T _interpolate(i64 j, const Segment<T>& segment) const
    {
#pragma float_control(precise, on)
        const f32 f = static_cast<f32>(j - segment.fromTicks) / static_cast<f32>(segment.toTicks - segment.fromTicks);
        const f32 f1 = segment.easing.apply(f);
        const T result = m_lerp.apply(f1, segment.fromValue, segment.toValue);
#pragma float_control(precise, pop)
        return result;
    }
#else
    [[gnu::noinline]] T _interpolate(i64 j, const Segment<T>& segment) const
    {
        // TODO: 若将来能按目标为 timeline 单独去掉 -ffast-math（或在文件作用域写下该 pragma），
        //       可去掉这里的 noinline 调用开销。
        const f32 f = static_cast<f32>(j - segment.fromTicks) / static_cast<f32>(segment.toTicks - segment.fromTicks);
        const f32 f1 = segment.easing.apply(f);
        return m_lerp.apply(f1, segment.fromValue, segment.toValue);
    }
#endif

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
