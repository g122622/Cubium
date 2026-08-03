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

#include "MapFrame.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include <string>

namespace mc::world::map {

namespace nbt_helper = mc::entity::serialization::nbt_helper;

MapFrame::MapFrame(BlockPos pos, i32 rotation, i32 entityId) noexcept
    : m_pos(pos)
    , m_rotation(rotation)
    , m_entityId(entityId)
{}

MapFrame MapFrame::fromNbt(const nbt::tags::compound_tag& tag)
{
    auto x = nbt_helper::tryGetInt(tag, "X").value_or(0);
    auto y = nbt_helper::tryGetInt(tag, "Y").value_or(0);
    auto z = nbt_helper::tryGetInt(tag, "Z").value_or(0);
    auto rotation = nbt_helper::tryGetInt(tag, "Rot").value_or(0);
    auto entityId = nbt_helper::tryGetInt(tag, "EntityId").value_or(-1);

    return MapFrame(BlockPos(x, y, z), rotation, entityId);
}

void MapFrame::toNbt(nbt::tags::compound_tag& tag) const
{
    tag.put("X", m_pos.x);
    tag.put("Y", m_pos.y);
    tag.put("Z", m_pos.z);
    tag.put("Rot", m_rotation);
    tag.put("EntityId", m_entityId);
}

std::string MapFrame::getId() const
{
    return getIdForPos(m_pos);
}

std::string MapFrame::getIdForPos(BlockPos pos)
{
    return "frame-" + std::to_string(pos.x) + "-" + std::to_string(pos.y) + "-" + std::to_string(pos.z);
}

} // namespace mc::world::map
