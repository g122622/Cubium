/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "EntityTypeTag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// EntityTypeTag 实现
// ============================================================================

EntityTypeTag::EntityTypeTag(ResourceLocation id) noexcept
    : m_id(std::move(id))
{}

void EntityTypeTag::add(const ResourceLocation& entityTypeId)
{
    m_entityTypeIds.insert(entityTypeId);
}

void EntityTypeTag::addAll(const std::vector<ResourceLocation>& entityTypeIds)
{
    for (const auto& id : entityTypeIds) {
        m_entityTypeIds.insert(id);
    }
}

bool EntityTypeTag::contains(const ResourceLocation& entityTypeId) const noexcept
{
    return m_entityTypeIds.find(entityTypeId) != m_entityTypeIds.end();
}

bool EntityTypeTag::contains(const std::string& entityTypeId) const
{
    return contains(ResourceLocation(entityTypeId));
}

bool EntityTypeTag::contains(const entity::EntityType& entityType) const
{
    return contains(entityType.name());
}

void EntityTypeTag::clear()
{
    m_entityTypeIds.clear();
}

} // namespace mc
