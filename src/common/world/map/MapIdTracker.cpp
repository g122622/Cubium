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
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "entity/serialization/NbtHelper.hpp"

namespace mc::world::map {

namespace nbt_helper = mc::entity::serialization::nbt_helper;

i32 MapIdTracker::getNextId() noexcept
{
    // 对齐 MC 1.16.5 MapIdTracker#getNextId：返回 usedIds["map"]+1 并写入。
    // 本项目用 m_nextMapId 表示“下一个待分配的ID”（等价于原版 usedIds["map"]+1），
    // 因此先返回再自增即可。
    return m_nextMapId++;
}

void MapIdTracker::readFromNbt(const nbt::tags::compound_tag& tag)
{
    // 原版磁盘格式：tag["map"] 存储的是“最后已分配的ID”（默认 -1，见
    // MC 1.16.5 MapIdTracker.usedIds.defaultReturnValue(-1) /
    // MC 1.21.11 MapIndex CODEC optionalFieldOf("map", -1)）。
    // 转换为本项目的 m_nextMapId（下一个待分配ID）需 +1。
    m_nextMapId = nbt_helper::tryGetInt(tag, "map").value_or(-1) + 1;
}

void MapIdTracker::writeToNbt(nbt::tags::compound_tag& tag) const
{
    // 写入“最后已分配的ID”以与原版 idcounts.dat 格式兼容：
    // m_nextMapId（下一个待分配ID）- 1 = 最后已分配的ID。
    tag.put("map", m_nextMapId - 1);
}

} // namespace mc::world::map
