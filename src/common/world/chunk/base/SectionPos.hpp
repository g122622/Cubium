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

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>

namespace mc::world::chunk {

/**
 * @brief 区块段位置
 *
 * 用于标识区块中的一个16x16x16的段
 */
class SectionPos {
public:
    ChunkCoord x;
    i32 y; // 段Y坐标（世界段坐标，范围 MIN_SECTION_Y..MAX_SECTION_Y，非数组索引）
    ChunkCoord z;

    SectionPos() noexcept
        : x(0)
        , y(0)
        , z(0)
    {}

    SectionPos(ChunkCoord x, i32 y, ChunkCoord z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {}

    explicit SectionPos(const BlockPos& pos) noexcept
        : x(pos.chunkX())
        , y(pos.sectionCoord())
        , z(pos.chunkZ())
    {}

    /**
     * @brief 从方块位置创建区块段位置
     */
    [[nodiscard]] static SectionPos fromBlockPos(const BlockPos& pos) noexcept
    {
        return SectionPos(pos.chunkX(), pos.sectionCoord(), pos.chunkZ());
    }

    /**
     * @brief 从区块位置创建区块段位置
     */
    [[nodiscard]] static SectionPos fromChunkPos(ChunkCoord chunkX, i32 sectionY, ChunkCoord chunkZ) noexcept
    {
        return SectionPos(chunkX, sectionY, chunkZ);
    }

    /**
     * @brief 从长整型编码创建
     */
    [[nodiscard]] static SectionPos fromLong(i64 packed) noexcept
    {
        return SectionPos(static_cast<ChunkCoord>(packed >> 42),
            static_cast<i32>((packed << 44) >> 44),
            static_cast<ChunkCoord>((packed << 22) >> 42));
    }

    /**
     * @brief 转换为长整型编码
     */
    [[nodiscard]] i64 toLong() const noexcept
    {
        i64 lx = static_cast<i64>(x) & 0x3FFFFFLL;
        i64 lz = static_cast<i64>(z) & 0x3FFFFFLL;
        i64 ly = static_cast<i64>(y) & 0xFFFFFLL;
        return (lx << 42) | (lz << 20) | ly;
    }

    [[nodiscard]] bool operator==(const SectionPos& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] bool operator!=(const SectionPos& other) const noexcept { return !(*this == other); }

    [[nodiscard]] bool operator<(const SectionPos& other) const noexcept
    {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }

    /**
     * @brief 获取区块X坐标
     */
    [[nodiscard]] ChunkCoord chunkX() const noexcept { return x; }

    /**
     * @brief 获取区块Z坐标
     */
    [[nodiscard]] ChunkCoord chunkZ() const noexcept { return z; }

    /**
     * @brief 转换为世界坐标
     */
    [[nodiscard]] i32 worldX() const noexcept { return x << mc::world::CHUNK_SHIFT; }
    [[nodiscard]] i32 worldY() const noexcept { return y << mc::world::SECTION_SHIFT; }
    [[nodiscard]] i32 worldZ() const noexcept { return z << mc::world::CHUNK_SHIFT; }

    /**
     * @brief 获取区块段内的局部坐标
     */
    [[nodiscard]] static i32 mask(i32 coord) noexcept { return coord & mc::world::CHUNK_MASK; }

    /**
     * @brief 向指定方向偏移
     */
    [[nodiscard]] SectionPos offset(i32 dx, i32 dy, i32 dz) const noexcept
    {
        return SectionPos(x + dx, y + dy, z + dz);
    }

    /**
     * @brief 向指定方向偏移
     */
    [[nodiscard]] SectionPos offset(Direction dir) const noexcept
    {
        switch (dir) {
            case Direction::Down:
                return SectionPos(x, y - 1, z);
            case Direction::Up:
                return SectionPos(x, y + 1, z);
            case Direction::North:
                return SectionPos(x, y, z - 1);
            case Direction::South:
                return SectionPos(x, y, z + 1);
            case Direction::West:
                return SectionPos(x - 1, y, z);
            case Direction::East:
                return SectionPos(x + 1, y, z);
            default:
                return *this;
        }
    }

    /**
     * @brief 转换为区块列位置（不含Y坐标）
     */
    [[nodiscard]] i64 toColumnLong() const noexcept
    {
        i64 lx = static_cast<i64>(x) & 0x3FFFFFLL;
        i64 lz = static_cast<i64>(z) & 0x3FFFFFLL;
        return (lx << 42) | (lz << 20);
    }

    /**
     * @brief 获取该段的最小世界Y坐标
     */
    [[nodiscard]] i32 minY() const noexcept { return y * mc::world::CHUNK_SECTION_HEIGHT; }

    /**
     * @brief 获取该段的最大世界Y坐标
     */
    [[nodiscard]] i32 maxY() const noexcept { return (y + 1) * mc::world::CHUNK_SECTION_HEIGHT - 1; }

    /**
     * @brief 获取区块位置（不含Y坐标）
     */
    [[nodiscard]] ChunkPos chunkPos() const noexcept { return {x, z}; }
};

} // namespace mc::world::chunk

namespace mc {
using SectionPos = world::chunk::SectionPos;
} // namespace mc

// 哈希函数支持
namespace std {
template <>
struct hash<mc::world::chunk::SectionPos> {
    size_t operator()(const mc::world::chunk::SectionPos& pos) const noexcept
    {
        size_t h1 = std::hash<mc::ChunkCoord>{}(pos.x);
        size_t h2 = std::hash<mc::i32>{}(pos.y);
        size_t h3 = std::hash<mc::ChunkCoord>{}(pos.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std
