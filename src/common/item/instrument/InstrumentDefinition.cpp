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

#include "common/item/instrument/InstrumentDefinition.hpp"

#include "common/core/Types.hpp"
#include "common/util/text/ITextComponent.hpp"

#include <cmath>
#include <utility>

namespace mc {
namespace item::instrument {

InstrumentDefinition::InstrumentDefinition(
    ResourceLocation soundEvent, f32 useDuration, f32 range, std::unique_ptr<text::ITextComponent> description)
    : m_soundEvent(std::move(soundEvent))
    , m_useDuration(useDuration)
    , m_range(range)
    , m_description(std::move(description))
{}

InstrumentDefinition::InstrumentDefinition(const InstrumentDefinition& other)
    : m_soundEvent(other.m_soundEvent)
    , m_useDuration(other.m_useDuration)
    , m_range(other.m_range)
    , m_description(other.m_description != nullptr ? other.m_description->deepCopy() : nullptr)
{}

InstrumentDefinition& InstrumentDefinition::operator=(const InstrumentDefinition& other)
{
    if (this == &other) {
        return *this;
    }
    InstrumentDefinition copy(other);
    *this = std::move(copy);
    return *this;
}

i32 InstrumentDefinition::useDurationTicks() const noexcept
{
    // 与 vanilla 一致：Mth.floor(use_duration * 20.0F)，秒换算为 tick 后向下取整。
    return static_cast<i32>(std::floor(static_cast<f64>(m_useDuration) * 20.0));
}

f32 InstrumentDefinition::volume() const noexcept
{
    // vanilla InstrumentItem.play：音量 = range / 16（默认 range=16 时为 1.0）。
    return m_range / 16.0f;
}

} // namespace item::instrument
} // namespace mc
