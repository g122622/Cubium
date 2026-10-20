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

#include "common/item/instrument/InstrumentTag.hpp"

#include <algorithm>
#include <utility>

namespace mc::item::instrument {

InstrumentTag::InstrumentTag(ResourceLocation id, bool replace)
    : m_id(std::move(id))
    , m_replace(replace)
{}

void InstrumentTag::add(const ResourceLocation& instrumentId)
{
    const auto it = std::lower_bound(m_members.begin(), m_members.end(), instrumentId);
    if (it != m_members.end() && *it == instrumentId) {
        return;
    }
    m_members.insert(it, instrumentId);
}

void InstrumentTag::addAll(const std::vector<ResourceLocation>& instrumentIds)
{
    for (const auto& instrumentId : instrumentIds) {
        add(instrumentId);
    }
}

bool InstrumentTag::contains(const ResourceLocation& instrumentId) const
{
    return std::binary_search(m_members.begin(), m_members.end(), instrumentId);
}

void InstrumentTag::clear()
{
    m_members.clear();
}

} // namespace mc::item::instrument