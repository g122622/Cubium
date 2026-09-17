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

#pragma once

#include "common/core/Types.hpp"
#include "world/block/BlockPos.hpp"
#include <cstddef>
#include <functional>

namespace mc {

// 前向声明
class Block;

/**
 * @brief 方块事件数据
 *
 * 表示一个待处理的方块事件，包含位置、方块类型和两个事件参数。
 * 服务端将方块事件加入队列，每tick处理时验证方块是否仍匹配，
 * 匹配则执行事件并广播给客户端；客户端收到后直接执行。
 *
 * 参考 MC Java: BlockEventData
 */
struct BlockEventData {
    BlockPos pos;
    const Block* block;
    i32 paramA;
    i32 paramB;

    bool operator==(const BlockEventData& other) const noexcept
    {
        return pos == other.pos && block == other.block && paramA == other.paramA && paramB == other.paramB;
    }

    bool operator!=(const BlockEventData& other) const noexcept { return !(*this == other); }
};

} // namespace mc

// BlockEventData 的哈希特化，用于去重队列
template <>
struct std::hash<mc::BlockEventData> {
    size_t operator()(const mc::BlockEventData& event) const noexcept
    {
        size_t h = std::hash<mc::BlockPos>{}(event.pos);
        h ^= reinterpret_cast<size_t>(event.block) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= static_cast<size_t>(event.paramA) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= static_cast<size_t>(event.paramB) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
