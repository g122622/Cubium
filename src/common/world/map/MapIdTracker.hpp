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
 * LIABILITY, WHETER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "core/Types.hpp"
#include "util/nbt/Nbt.hpp"

namespace mc::world::map {

/**
 * @brief 地图ID追踪器
 *
 * 管理地图ID的自增分配，确保每个新地图获得唯一ID。
 * 对应MC中的MapIdTracker，存储在data/idcounts.dat中。
 */
class MapIdTracker {
public:
    MapIdTracker() = default;

    /**
     * @brief 获取下一个地图ID并递增计数器
     */
    [[nodiscard]] i32 getNextId();

    /**
     * @brief 从NBT读取
     */
    void readFromNbt(const nbt::tags::compound_tag& tag);

    /**
     * @brief 写入NBT
     */
    void writeToNbt(nbt::tags::compound_tag& tag) const;

private:
    i32 m_nextMapId = 0;
};

} // namespace mc::world::map
