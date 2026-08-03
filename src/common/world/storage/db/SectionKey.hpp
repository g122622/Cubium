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
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace mc::world::storage {

/**
 * @brief Section标识符
 *
 * 用于唯一标识世界中的一个Section（16x16x16方块块）。
 * 支持范围：±8M 区块。
 *
 * 二进制布局（13字节）：
 * - dimensionId: 2字节（大端序）
 * - chunkX: 4字节（大端序）
 * - chunkZ: 4字节（大端序）
 * - sectionY: 1字节（有符号，-4到19对应Y=-64到320）
 * - padding: 2字节（保留）
 */
struct SectionKey {
    /// 区块X坐标
    i32 chunkX = 0;

    /// 区块Z坐标
    i32 chunkZ = 0;

    /// Section Y坐标（世界段坐标，MIN_SECTION_Y..MAX_SECTION_Y 即 -4..19，非数组索引）
    i8 sectionY = 0;

    /// 维度ID（0=主世界，1=下界，2=末地）
    DimensionId dimension = 0;

    SectionKey() = default;

    SectionKey(i32 cx, i32 cz, i8 sy, DimensionId dim)
        : chunkX(cx)
        , chunkZ(cz)
        , sectionY(sy)
        , dimension(dim)
    {}

    /**
     * @brief 从区块坐标和Section Y创建
     */
    static SectionKey fromChunk(i32 cx, i32 cz, i8 sy, DimensionId dim = 0) { return SectionKey(cx, cz, sy, dim); }

    /**
     * @brief 从方块坐标创建
     */
    static SectionKey fromBlock(i32 x, i32 y, i32 z, DimensionId dim = 0)
    {
        return SectionKey(x >> CHUNK_SHIFT,      // chunkX
            z >> CHUNK_SHIFT,                    // chunkZ
            static_cast<i8>(y >> SECTION_SHIFT), // sectionY
            dim);
    }

    /**
     * @brief 从ChunkPos创建
     */
    static SectionKey fromChunkPos(const ChunkPos& pos, i8 sy, DimensionId dim = 0)
    {
        return SectionKey(pos.x, pos.z, sy, dim);
    }

    /**
     * @brief 获取Section在世界中的最小Y坐标
     */
    [[nodiscard]] i32 minY() const noexcept { return static_cast<i32>(sectionY) * CHUNK_SECTION_HEIGHT; }

    /**
     * @brief 获取Section在世界中的最大Y坐标
     */
    [[nodiscard]] i32 maxY() const noexcept { return minY() + CHUNK_SECTION_HEIGHT - 1; }

    /**
     * @brief 序列化为二进制（13字节）
     */
    [[nodiscard]] std::array<u8, 13> serialize() const noexcept
    {
        std::array<u8, 13> data{};

        // dimensionId (2字节, 大端序)
        data[0] = static_cast<u8>(dimension >> 8);
        data[1] = static_cast<u8>(dimension & 0xFF);

        // chunkX (4字节, 大端序)
        data[2] = static_cast<u8>(chunkX >> 24);
        data[3] = static_cast<u8>(chunkX >> 16);
        data[4] = static_cast<u8>(chunkX >> 8);
        data[5] = static_cast<u8>(chunkX & 0xFF);

        // chunkZ (4字节, 大端序)
        data[6] = static_cast<u8>(chunkZ >> 24);
        data[7] = static_cast<u8>(chunkZ >> 16);
        data[8] = static_cast<u8>(chunkZ >> 8);
        data[9] = static_cast<u8>(chunkZ & 0xFF);

        // sectionY (1字节)
        data[10] = static_cast<u8>(sectionY);

        // padding (2字节, 保留)
        data[11] = 0;
        data[12] = 0;

        return data;
    }

    /**
     * @brief 从二进制反序列化
     */
    static SectionKey deserialize(const u8* data) noexcept
    {
        SectionKey key;

        // dimensionId
        key.dimension = static_cast<DimensionId>((static_cast<i32>(data[0]) << 8) | data[1]);

        // chunkX
        key.chunkX = (static_cast<i32>(data[2]) << 24) | (static_cast<i32>(data[3]) << 16) |
            (static_cast<i32>(data[4]) << 8) | static_cast<i32>(data[5]);

        // chunkZ
        key.chunkZ = (static_cast<i32>(data[6]) << 24) | (static_cast<i32>(data[7]) << 16) |
            (static_cast<i32>(data[8]) << 8) | static_cast<i32>(data[9]);

        // sectionY
        key.sectionY = static_cast<i8>(data[10]);

        return key;
    }

    /**
     * @brief 序列化为RocksDB键（vector形式）
     */
    [[nodiscard]] std::vector<u8> toKey() const
    {
        auto arr = serialize();
        return std::vector<u8>(arr.begin(), arr.end());
    }

    /**
     * @brief 比较运算符
     */
    bool operator==(const SectionKey& other) const noexcept
    {
        return chunkX == other.chunkX && chunkZ == other.chunkZ && sectionY == other.sectionY &&
            dimension == other.dimension;
    }

    bool operator!=(const SectionKey& other) const noexcept { return !(*this == other); }

    bool operator<(const SectionKey& other) const noexcept
    {
        if (dimension != other.dimension) return dimension < other.dimension;
        if (chunkX != other.chunkX) return chunkX < other.chunkX;
        if (chunkZ != other.chunkZ) return chunkZ < other.chunkZ;
        return sectionY < other.sectionY;
    }

    /**
     * @brief 计算哈希值（用于unordered_map/unordered_set）
     */
    struct Hash {
        size_t operator()(const SectionKey& key) const noexcept
        {
            size_t h = 0;
            h ^= std::hash<i32>{}(key.chunkX) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<i32>{}(key.chunkZ) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<i8>{}(key.sectionY) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<DimensionId>{}(key.dimension) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
};

} // namespace mc::world::storage
