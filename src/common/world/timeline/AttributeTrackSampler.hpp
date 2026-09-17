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
#include "common/world/attribute/EnvironmentAttributeLayer.hpp"
#include "common/world/attribute/LerpFunction.hpp"
#include "common/world/timeline/KeyframeTrack.hpp"
#include "common/world/timeline/KeyframeTrackSampler.hpp"

#include <functional>
#include <optional>

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 属性轨道采样器
 *
 * MC 1.21.11 class AttributeTrackSampler<Value, Argument>。
 * 实现 EnvironmentAttributeLayer.TimeBased<Value>，按 dayTime 采样 argument，
 * 再通过 modifier 将 argument 应用到基础值上。
 *
 * 缓存机制：同一 tickId 下复用 cachedArgument，避免重复采样。
 */
template <typename Value, typename Argument>
class AttributeTrackSampler : public EnvironmentAttributeLayer<Value>::TimeBased {
public:
    AttributeTrackSampler(std::optional<i32> periodTicks,
        AttributeModifier<Value, Argument> modifier,
        KeyframeTrack<Argument> argumentTrack,
        LerpFunction<Argument> lerp,
        std::function<i64()> dayTimeGetter)
        : m_modifier(std::move(modifier))
        , m_dayTimeGetter(std::move(dayTimeGetter))
        , m_argumentSampler(argumentTrack.bakeSampler(periodTicks, std::move(lerp)))
    {}

    Value applyTimeBased(Value value, int tick) override
    {
        if (!m_cachedArgument.has_value() || tick != m_cachedTickId) {
            m_cachedTickId = tick;
            m_cachedArgument = m_argumentSampler.sample(m_dayTimeGetter());
        }
        return m_modifier.apply(value, *m_cachedArgument);
    }

private:
    AttributeModifier<Value, Argument> m_modifier;
    KeyframeTrackSampler<Argument> m_argumentSampler;
    std::function<i64()> m_dayTimeGetter;
    int m_cachedTickId = -1;
    std::optional<Argument> m_cachedArgument;
};

} // namespace timeline
} // namespace world
} // namespace mc
