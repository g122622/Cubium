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
#include "common/world/attribute/EnvironmentAttributeLayer.hpp"
#include "common/world/attribute/LerpFunction.hpp"
#include "common/world/timeline/KeyframeTrack.hpp"
#include "common/world/timeline/KeyframeTrackSampler.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 属性轨道采样器
 *
 * MC 1.21.11 class AttributeTrackSampler<Value, Argument>。
 * 实现 EnvironmentAttributeLayer.TimeBased，按 dayTime 采样 argument，
 * 再通过 modifier 将 argument 应用到基础值上。
 *
 * 值/参数异构（如灰度混合）时，修改器必须是 `TypedModifierAdapter`：采样值先转换成
 * 修改器参数，再执行底层修改器。
 *
 * 缓存机制：同一 tick 序号下复用 cachedArgument，避免重复采样关键帧轨道。
 */
template <typename Value, typename Argument>
class AttributeTrackSampler : public attribute::EnvironmentAttributeLayer<Value>::TimeBased {
public:
    AttributeTrackSampler(std::optional<i32> periodTicks,
        std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> modifier,
        const KeyframeTrack<Argument>& argumentTrack,
        attribute::LerpFunction<Argument> lerp,
        std::function<i64()> dayTimeGetter)
        : m_modifier(std::move(modifier))
        , m_argumentSampler(argumentTrack.bakeSampler(periodTicks, std::move(lerp)))
        , m_dayTimeGetter(std::move(dayTimeGetter))
    {}

    /**
     * @brief 按时间应用属性修改
     *
     * @param value 基础值
     * @param tick 当前 tick 序号（仅用于缓存判定，同一序号内复用采样结果）
     */
    Value applyTimeBased(Value value, i64 tick) const override
    {
        // 负序号表示不缓存：直接采样，不污染缓存（用于拷贝后的一次性求值）
        if (tick < 0) {
            return _applyArgument(
                std::move(value), m_argumentSampler.sample(m_dayTimeGetter()), std::is_same<Value, Argument>{});
        }

        if (!m_cachedArgument.has_value() || tick != m_cachedTickId) {
            m_cachedTickId = tick;
            m_cachedArgument = m_argumentSampler.sample(m_dayTimeGetter());
        }
        // 注意：cachedArgument 需保持有效以服务同一 tick 内的后续查询，此处按值拷贝而非移动
        return _applyArgument(std::move(value), *m_cachedArgument, std::is_same<Value, Argument>{});
    }

    /// 清空采样缓存：下一次求值重新采样 dayTime 对应的关键帧轨道
    void invalidateTickCache() const override
    {
        m_cachedTickId = -1;
        m_cachedArgument.reset();
    }

private:
    /// 值/参数同型：直接以采样值作为参数
    Value _applyArgument(Value value, const Argument& argument, std::true_type) const
    {
        return m_modifier->apply(std::move(value), argument);
    }

    /// 值/参数异构：修改器必须是适配器，由适配器转换后再执行底层修改器
    Value _applyArgument(Value value, const Argument& argument, std::false_type) const
    {
        const auto* adapter = dynamic_cast<const attribute::TypedModifierAdapter<Value, Argument>*>(m_modifier.get());
        MC_ASSERT_RELEASE_MSG(
            adapter != nullptr, "AttributeTrackSampler: heterogeneous track requires adapter modifier");
        return adapter->applyArgumentValue(std::move(value), argument);
    }

    std::shared_ptr<const attribute::AttributeModifier<Value, Argument>> m_modifier;
    KeyframeTrackSampler<Argument> m_argumentSampler;
    std::function<i64()> m_dayTimeGetter;
    mutable i64 m_cachedTickId = -1;
    mutable std::optional<Argument> m_cachedArgument;
};

} // namespace timeline
} // namespace world
} // namespace mc