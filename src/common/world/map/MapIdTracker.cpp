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

#include "MapIdTracker.hpp"
#include "entity/serialization/NbtHelper.hpp"

namespace mc::world::map {

namespace nbt_helper = mc::entity::serialization::nbt_helper;

i32 MapIdTracker::getNextId()
{
    return m_nextMapId++;
}

void MapIdTracker::readFromNbt(const nbt::tags::compound_tag& tag)
{
    m_nextMapId = nbt_helper::tryGetInt(tag, "map").value_or(-1) + 1;
}

void MapIdTracker::writeToNbt(nbt::tags::compound_tag& tag) const
{
    tag.put("map", m_nextMapId);
}

} // namespace mc::world::map
