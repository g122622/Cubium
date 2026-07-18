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
 */

#include "client/resource/atlas/AtlasSource.hpp"

namespace mc::client::resource::atlas {

void SpriteSourceOutput::add(const ResourceLocation& spriteName, SpriteLoader loader)
{
    auto it = m_index.find(spriteName);
    if (it != m_index.end()) {
        // 后执行覆盖先执行：替换已有条目的 loader，保留位置
        m_entries[it->second].loader = std::move(loader);
        m_entries[it->second].removed = false;
        return;
    }
    m_index[spriteName] = m_entries.size();
    m_entries.push_back(Entry{spriteName, std::move(loader), false});
}

void SpriteSourceOutput::removeAll(const IdentifierPattern& pattern)
{
    for (auto& entry : m_entries) {
        if (!entry.removed && pattern.matches(entry.name)) {
            entry.removed = true;
            m_index.erase(entry.name);
        }
    }
}

std::vector<std::pair<ResourceLocation, SpriteLoader>> SpriteSourceOutput::build() const
{
    std::vector<std::pair<ResourceLocation, SpriteLoader>> result;
    result.reserve(m_entries.size());
    for (const auto& entry : m_entries) {
        if (!entry.removed) {
            result.emplace_back(entry.name, entry.loader);
        }
    }
    return result;
}

size_t SpriteSourceOutput::size() const
{
    size_t count = 0;
    for (const auto& entry : m_entries) {
        if (!entry.removed) {
            ++count;
        }
    }
    return count;
}

} // namespace mc::client::resource::atlas
