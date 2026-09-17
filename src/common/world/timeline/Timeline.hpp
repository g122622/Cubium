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
#include "common/world/attribute/AttributeModifier.hpp"
#include "common/world/attribute/EnvironmentAttribute.hpp"
#include "common/world/attribute/LerpFunction.hpp"
#include "common/world/timeline/AttributeTrack.hpp"
#include "common/world/timeline/AttributeTrackSampler.hpp"
#include "common/world/timeline/EasingType.hpp"
#include "common/world/timeline/Keyframe.hpp"
#include "common/world/timeline/KeyframeTrack.hpp"
#include "common/world/timeline/KeyframeTrackSampler.hpp"

#include <any>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 时间线
 *
 * MC 1.21.11 class Timeline。
 * 持有一组属性轨道（AttributeTrack），按 periodTicks 循环。
 * 通过 createTrackSampler 创建采样器，按 dayTime 查询属性值。
 *
 * 类型擦除：MC 用 Java 通配符泛型 Map<EnvironmentAttribute<?>, AttributeTrack<?, ?>>。
 * C++ 用 std::any 存储异构 AttributeTrack<Value, Argument>，key 用 EnvironmentAttribute 地址。
 */
class Timeline {
public:
    Timeline() = default;

    Timeline(std::optional<i32> periodTicks,
        std::unordered_map<const void*, std::any> tracks)
        : m_periodTicks(periodTicks)
        , m_tracks(std::move(tracks))
    {}

    const std::optional<i32>& periodTicks() const noexcept { return m_periodTicks; }

    /**
     * @brief 创建轨道采样器
     *
     * @tparam Value 属性值类型（如 Activity）
     * @tparam Argument 修改器参数类型（OverrideModifier 时与 Value 相同）
     * @param envAttr 环境属性
     * @param dayTimeGetter 获取当前 dayTime 的函数
     * @return 采样器
     */
    template <typename Value, typename Argument>
    std::unique_ptr<AttributeTrackSampler<Value, Argument>> createTrackSampler(
        const EnvironmentAttribute<Value>& envAttr,
        std::function<i64()> dayTimeGetter) const
    {
        const auto it = m_tracks.find(static_cast<const void*>(&envAttr));
        MC_ASSERT(it != m_tracks.end());

        const auto* track = std::any_cast<AttributeTrack<Value, Argument>>(&it->second);
        MC_ASSERT(track != nullptr);

        return std::make_unique<AttributeTrackSampler<Value, Argument>>(
            m_periodTicks,
            track->modifier(),
            track->argumentTrack(),
            envAttr.type().keyframeLerp().template cast<Argument>(),
            std::move(dayTimeGetter));
    }

    /**
     * @brief 时间线构建器
     */
    class Builder {
    public:
        Builder& setPeriodTicks(i32 periodTicks)
        {
            m_periodTicks = periodTicks;
            return *this;
        }

        /**
         * @brief 添加修改器轨道
         *
         * @tparam Value 属性值类型
         * @tparam Argument 修改器参数类型
         */
        template <typename Value, typename Argument>
        Builder& addModifierTrack(const EnvironmentAttribute<Value>& envAttr,
            AttributeModifier<Value, Argument> modifier,
            std::function<void(typename KeyframeTrack<Argument>::Builder&)> consumer)
        {
            typename KeyframeTrack<Argument>::Builder builder;
            consumer(builder);
            KeyframeTrack<Argument> track = builder.build();

            m_tracks[static_cast<const void*>(&envAttr)] = std::make_any<AttributeTrack<Value, Argument>>(
                std::move(modifier), std::move(track));
            return *this;
        }

        /**
         * @brief 添加覆写轨道（使用 OverrideModifier）
         */
        template <typename Value>
        Builder& addTrack(const EnvironmentAttribute<Value>& envAttr,
            std::function<void(typename KeyframeTrack<Value>::Builder&)> consumer)
        {
            return addModifierTrack<Value, Value>(envAttr,
                AttributeModifier<Value, Value>::Override{},
                consumer);
        }

        Timeline build()
        {
            return Timeline(m_periodTicks, std::move(m_tracks));
        }

    private:
        std::optional<i32> m_periodTicks;
        std::unordered_map<const void*, std::any> m_tracks;
    };

private:
    std::optional<i32> m_periodTicks;
    std::unordered_map<const void*, std::any> m_tracks;
};

} // namespace timeline
} // namespace world
} // namespace mc
