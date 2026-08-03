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
#include "common/world/chunk/base/ChunkPos.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace mc::world::chunk {
/**
 * @brief 区块唯一标识符
 *
 * 用于标识区块在世界中的唯一位置，包含维度信息
 */
struct ChunkId {
    ChunkCoord x;
    ChunkCoord z;
    i32 dimension; // 0=主世界, 1=下界, 2=末地

    ChunkId()
        : x(0)
        , z(0)
        , dimension(0)
    {}
    ChunkId(ChunkCoord x, ChunkCoord z, i32 dim)
        : x(x)
        , z(z)
        , dimension(dim)
    {}

    /**
     * @brief 编码为64位唯一ID
     *
     * 编码格式：高16位=维度，中间24位=X，低24位=Z
     * 支持坐标范围 -8388608 到 8388607
     */
    [[nodiscard]] u64 toId() const noexcept
    {
        u64 dim = static_cast<u64>(static_cast<u32>(dimension) & 0xFFFF);
        u64 dx = static_cast<u64>(static_cast<u32>(x) & 0xFFFFFF);
        u64 dz = static_cast<u64>(static_cast<u32>(z) & 0xFFFFFF);
        return (dim << 48) | (dx << 24) | dz;
    }

    /**
     * @brief 从64位ID解码
     */
    [[nodiscard]] static ChunkId fromId(u64 id) noexcept
    {
        ChunkId cid;
        cid.dimension = static_cast<i32>(static_cast<u16>(id >> 48));
        // 处理24位有符号数
        u32 ux = static_cast<u32>((id >> 24) & 0xFFFFFF);
        u32 uz = static_cast<u32>(id & 0xFFFFFF);
        // 如果最高位为1，扩展为负数
        cid.x = (ux & 0x800000) ? static_cast<ChunkCoord>(ux | 0xFF000000) : static_cast<ChunkCoord>(ux);
        cid.z = (uz & 0x800000) ? static_cast<ChunkCoord>(uz | 0xFF000000) : static_cast<ChunkCoord>(uz);
        return cid;
    }

    /**
     * @brief 转换为区块位置（不含维度）
     */
    [[nodiscard]] ChunkPos chunkPos() const noexcept { return ChunkPos(x, z); }

    bool operator==(const ChunkId& other) const noexcept
    {
        return x == other.x && z == other.z && dimension == other.dimension;
    }

    bool operator!=(const ChunkId& other) const noexcept { return !(*this == other); }

    bool operator<(const ChunkId& other) const noexcept
    {
        if (dimension != other.dimension) return dimension < other.dimension;
        if (x != other.x) return x < other.x;
        return z < other.z;
    }
};

} // namespace mc::world::chunk

namespace mc {
using ChunkId = world::chunk::ChunkId;
} // namespace mc

// 哈希支持
namespace std {
template <>
struct hash<mc::world::chunk::ChunkId> {
    size_t operator()(const mc::world::chunk::ChunkId& id) const noexcept { return static_cast<size_t>(id.toId()); }
};
} // namespace std
