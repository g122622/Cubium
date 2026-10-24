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

#include "common/item/instrument/InstrumentTags.hpp"

#include <algorithm>

namespace mc::item::instrument {

InstrumentTags& InstrumentTags::instance() noexcept
{
    static InstrumentTags s_instance;
    return s_instance;
}

InstrumentTag& InstrumentTags::REGULAR_GOAT_HORNS()
{
    return instance().registerTag(ResourceLocation("minecraft:regular_goat_horns"));
}

InstrumentTag& InstrumentTags::SCREAMING_GOAT_HORNS()
{
    return instance().registerTag(ResourceLocation("minecraft:screaming_goat_horns"));
}

InstrumentTag& InstrumentTags::GOAT_HORNS()
{
    return instance().registerTag(ResourceLocation("minecraft:goat_horns"));
}

InstrumentTag& InstrumentTags::registerTag(const ResourceLocation& id)
{
    const auto it = std::find_if(m_tags.begin(), m_tags.end(), [&id](const InstrumentTag& tag) {
        return tag.id() == id;
    });
    if (it != m_tags.end()) {
        return *it;
    }
    m_tags.emplace_back(id);
    return m_tags.back();
}

const InstrumentTag* InstrumentTags::getTag(const ResourceLocation& id) const
{
    const auto it = std::find_if(m_tags.begin(), m_tags.end(), [&id](const InstrumentTag& tag) {
        return tag.id() == id;
    });
    return it == m_tags.end() ? nullptr : &*it;
}

void InstrumentTags::forEachTag(const std::function<void(InstrumentTag&)>& callback)
{
    for (auto& tag : m_tags) {
        callback(tag);
    }
}

void InstrumentTags::clear()
{
    m_tags.clear();
}

void initializeVanillaInstrumentTags()
{
    InstrumentTags::instance().clear();
    InstrumentTags::REGULAR_GOAT_HORNS();
    InstrumentTags::SCREAMING_GOAT_HORNS();
    InstrumentTags::GOAT_HORNS();
}

} // namespace mc::item::instrument