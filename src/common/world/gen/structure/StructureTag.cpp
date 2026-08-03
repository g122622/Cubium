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

#include "StructureTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <utility>
#include <vector>

namespace mc::world::gen::structure {

// ============================================================================
// StructureTag 实现
// ============================================================================

StructureTag::StructureTag(ResourceLocation id, bool replace) noexcept
    : m_id(std::move(id))
    , m_replace(replace)
{}

void StructureTag::add(ResourceLocation structureId)
{
    m_structureIds.insert(std::move(structureId));
}

void StructureTag::addAll(const std::vector<ResourceLocation>& structureIds)
{
    for (const auto& id : structureIds) {
        m_structureIds.insert(id);
    }
}

bool StructureTag::contains(const ResourceLocation& structureId) const noexcept
{
    return m_structureIds.contains(structureId);
}

void StructureTag::clear() noexcept
{
    m_structureIds.clear();
}

} // namespace mc::world::gen::structure
