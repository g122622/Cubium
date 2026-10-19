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
#include "common/util/assert/AssertAll.hpp"
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
#include <memory>
#include <optional>
#include <type_traits>
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
 * C++ 用 std::any 存储异构 AttributeTrack<Value, Argument>，以 EnvironmentAttribute 地址为键
 * （属性均为静态对象，地址稳定）。
 */
class Timeline {
public:
    Timeline() = default;

    Timeline(std::optional<i32> periodTicks, std::unordered_map<const void*, std::any> tracks)
        : m_periodTicks(periodTicks)
        , m_tracks(std::move(tracks))
    {}

    [[nodiscard]] const std::optional<i32>& periodTicks() const noexcept { return m_periodTicks; }

    /**
     * @brief 判断时间线是否包含某个属性的轨道
     */
    [[nodiscard]] bool hasTrack(const void* attribute) const { return m_tracks.contains(attribute); }

    /**
     * @brief 创建轨道采样器
     *
     * 轨道来自 addModifierTrack 注册的修改器+关键帧组合：采样得到的 argument 经修改器
     * 应用到属性的基础值上。改为覆写轨道（addTrack）时，参数类型同为 Value。
     *
     * @tparam Value 属性值类型（如 Activity）
     * @tparam Argument 修改器参数类型（覆写轨道下与 Value 相同）
     * @param envAttr 环境属性
     * @param dayTimeGetter 获取当前 dayTime 的函数
     * @return 采样器
     */
    template <typename Value, typename Argument>
    [[nodiscard]] std::unique_ptr<AttributeTrackSampler<Value, Argument>> createTrackSampler(
        const attribute::EnvironmentAttribute<Value>& envAttr, std::function<i64()> dayTimeGetter) const
    {
        const auto it = m_tracks.find(static_cast<const void*>(&envAttr));
        MC_ASSERT_RELEASE_MSG(it != m_tracks.end(), "Timeline::createTrackSampler: attribute track not found");

        using TrackType = AttributeTrack<Value, Argument>;
        const auto* track = std::any_cast<TrackType>(&it->second);
        MC_ASSERT_RELEASE_MSG(track != nullptr, "Timeline::createTrackSampler: attribute track type mismatch");

        return std::make_unique<AttributeTrackSampler<Value, Value>>(m_periodTicks,
            track->modifierPtr(),
            track->effectiveArgumentTrack(),
            track->modifier().argumentKeyframeLerp(),
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
        Builder& addModifierTrack(const attribute::EnvironmentAttribute<Value>& envAttr,
            std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> modifier,
            std::function<void(typename KeyframeTrack<Value>::Builder&)> consumer)
        {
            return addModifierTrack(envAttr, std::move(modifier), std::nullopt, std::move(consumer));
        }

        /**
         * @brief 添加带轨道级缓动的修改器轨道
         *
         * vanilla 数据包把 `ease` 写在轨道上（而非关键帧内），因此缓动需要单独传入；
         * 传 std::nullopt 表示沿用关键帧轨道自带缓动。
         *
         * @tparam Value 属性值类型
         * @tparam Argument 修改器参数类型
         * @param envAttr 环境属性
         * @param modifier 修改器
         * @param easing 轨道级缓动（覆盖关键帧轨道的缓动）
         * @param consumer 关键帧构建回调
         */
        template <typename Value, typename Argument>
        Builder& addModifierTrack(const attribute::EnvironmentAttribute<Value>& envAttr,
            std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> modifier,
            std::optional<EasingType> easing,
            std::function<void(typename KeyframeTrack<Value>::Builder&)> consumer)
        {
            // 关键帧值以"属性的值表示"给出：Value 与 Argument 不同时在这里完成转换
            typename KeyframeTrack<Value>::Builder builder;
            consumer(builder);

            const KeyframeTrack<Value> consumedTrack = builder.build();
            std::vector<Keyframe<Argument>> keyframes;
            for (const auto& keyframe : consumedTrack.keyframes()) {
                keyframes.emplace_back(keyframe.ticks(), static_cast<Argument>(keyframe.value()));
            }
            MC_ASSERT_RELEASE_MSG(!keyframes.empty(), "Timeline::addModifierTrack: track has no keyframes");
            m_tracks[static_cast<const void*>(&envAttr)] =
                std::make_any<AttributeTrack<Value, Argument>>(std::move(modifier),
                    KeyframeTrack<Argument>(std::move(keyframes), EasingType::LINEAR),
                    std::move(easing));
            return *this;
        }

        /**
         * @brief 添加覆写轨道（使用 OverrideModifier）
         */
        template <typename Value>
        Builder& addTrack(const attribute::EnvironmentAttribute<Value>& envAttr,
            std::function<void(typename KeyframeTrack<Value>::Builder&)> consumer)
        {
            return addModifierTrack<Value, Value>(
                envAttr, std::make_shared<attribute::OverrideModifier<Value>>(), std::move(consumer));
        }

        [[nodiscard]] Timeline build() { return Timeline(m_periodTicks, std::move(m_tracks)); }

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
