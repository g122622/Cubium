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
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/base/types/BlockPos.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <tuple>

namespace mc::world::chunk {

/**
 * @brief 区块位置
 *
 * 用于标识区块在世界中的位置 (X, Z)
 */
class ChunkPos {
public:
    ChunkCoord x, z;

    // 构造函数
    ChunkPos() noexcept
        : x(0)
        , z(0)
    {}

    ChunkPos(ChunkCoord x, ChunkCoord z) noexcept
        : x(x)
        , z(z)
    {}

    explicit ChunkPos(const BlockPos& pos) noexcept
        : x(pos.chunkX())
        , z(pos.chunkZ())
    {}

    explicit ChunkPos(const Vector3& pos) noexcept
        : x(math::toChunkCoord(pos.x))
        , z(math::toChunkCoord(pos.z))
    {}

    // 静态常量
    static ChunkPos zero()
    {
        return {0, 0};
    }

    // 算术运算
    [[nodiscard]] ChunkPos operator+(const ChunkPos& other) const noexcept
    {
        return {x + other.x, z + other.z};
    }

    [[nodiscard]] ChunkPos operator-(const ChunkPos& other) const noexcept
    {
        return {x - other.x, z - other.z};
    }

    [[nodiscard]] ChunkPos operator*(i32 scalar) const noexcept
    {
        return {x * scalar, z * scalar};
    }

    // 比较运算
    [[nodiscard]] bool operator==(const ChunkPos& other) const noexcept
    {
        return x == other.x && z == other.z;
    }

    [[nodiscard]] bool operator!=(const ChunkPos& other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] bool operator<(const ChunkPos& other) const noexcept
    {
        return std::tie(x, z) < std::tie(other.x, other.z);
    }

    // 区块在世界中的原点坐标
    [[nodiscard]] BlockCoord worldX() const noexcept
    {
        return x * mc::world::CHUNK_WIDTH;
    }

    [[nodiscard]] BlockCoord worldZ() const noexcept
    {
        return z * mc::world::CHUNK_WIDTH;
    }

    // 区块中心坐标
    [[nodiscard]] Vector3 center(f32 y = 0.0f) const noexcept
    {
        return {static_cast<f32>(worldX()) + mc::world::CHUNK_WIDTH / 2.0f,
            y,
            static_cast<f32>(worldZ()) + mc::world::CHUNK_WIDTH / 2.0f};
    }

    // 转换为64位唯一ID
    [[nodiscard]] u64 toId() const noexcept
    {
        return math::chunkPosToId(x, z);
    }

    // 从64位ID创建
    [[nodiscard]] static ChunkPos fromId(u64 id) noexcept
    {
        ChunkPos pos;
        math::idToChunkPos(id, pos.x, pos.z);
        return pos;
    }

    /**
     * @brief 按原版 `ChunkPos.asLong` 的布局打包成 64 位
     *
     * **X 在低 32 位、Z 在高 32 位**。注意与 `toId()` 的布局不同：`toId()` 是项目内部
     * 使用的标识，而这里是原版序列化用的布局，两者不可混用。
     */
    [[nodiscard]] i64 asLong() const noexcept
    {
        return (static_cast<i64>(x) & 0xFFFFFFFFLL) | ((static_cast<i64>(z) & 0xFFFFFFFFLL) << 32);
    }

    /**
     * @brief 从原版 `ChunkPos.asLong` 的布局解出 X，对应原版 `ChunkPos.getX(long)`
     *
     * 取**低** 32 位。
     */
    [[nodiscard]] static i32 getX(i64 packed) noexcept
    {
        return static_cast<i32>(packed & 0xFFFFFFFFLL);
    }

    /**
     * @brief 从原版 `ChunkPos.asLong` 的布局解出 Z，对应原版 `ChunkPos.getZ(long)`
     *
     * 逻辑右移 32 位后再取低 32 位。
     */
    [[nodiscard]] static i32 getZ(i64 packed) noexcept
    {
        return static_cast<i32>((packed >> 32) & 0xFFFFFFFFLL);
    }

    /**
     * @brief 所在区域文件的 X 坐标，对应原版 `ChunkPos.getRegionX`
     *
     * 每个区域 32×32 区块，用算术右移 5 位——对负数同样向下取整，不能用 `/`。
     */
    [[nodiscard]] i32 getRegionX() const noexcept
    {
        return x >> 5;
    }

    /**
     * @brief 所在区域文件的 Z 坐标，对应原版 `ChunkPos.getRegionZ`
     */
    [[nodiscard]] i32 getRegionZ() const noexcept
    {
        return z >> 5;
    }

    /**
     * @brief 在区域文件内的本地 X，对应原版 `ChunkPos.getRegionLocalX`
     *
     * 用按位与 31——对负数同样得到非负结果（`-1 & 31 == 31`）。
     */
    [[nodiscard]] i32 getRegionLocalX() const noexcept
    {
        return x & 31;
    }

    /**
     * @brief 在区域文件内的本地 Z，对应原版 `ChunkPos.getRegionLocalZ`
     */
    [[nodiscard]] i32 getRegionLocalZ() const noexcept
    {
        return z & 31;
    }

    /**
     * @brief 区块坐标的稳定哈希，对应原版 `ChunkPos.hash(int, int)`
     *
     * 两个 LCG 混合后异或：X 直接代入，Z 先与 `-559038737` 异或再代入。
     * 两个乘加都在 **32 位无符号**下进行并自然回绕——用有符号会因溢出产生 UB。
     */
    [[nodiscard]] static i32 hash(i32 chunkX, i32 chunkZ) noexcept
    {
        const u32 i = 1664525U * static_cast<u32>(chunkX) + 1013904223U;
        const u32 j = 1664525U * (static_cast<u32>(chunkZ) ^ 0xDEADBEEFU) + 1013904223U;
        return static_cast<i32>(i ^ j);
    }

    // 计算与另一个区块的曼哈顿距离
    [[nodiscard]] i32 manhattanDistance(const ChunkPos& other) const noexcept
    {
        return std::abs(x - other.x) + std::abs(z - other.z);
    }

    // 计算与另一个区块的切比雪夫距离（棋盘距离）
    [[nodiscard]] i32 chebyshevDistance(const ChunkPos& other) const noexcept
    {
        return std::max(std::abs(x - other.x), std::abs(z - other.z));
    }

    // 检查是否在指定半径内
    [[nodiscard]] bool inRange(const ChunkPos& center, i32 radius) const noexcept
    {
        return chebyshevDistance(center) <= radius;
    }

    // 获取相邻区块
    [[nodiscard]] ChunkPos north() const noexcept
    {
        return {x, z - 1};
    }

    [[nodiscard]] ChunkPos south() const noexcept
    {
        return {x, z + 1};
    }

    [[nodiscard]] ChunkPos east() const noexcept
    {
        return {x + 1, z};
    }

    [[nodiscard]] ChunkPos west() const noexcept
    {
        return {x - 1, z};
    }
};

} // namespace mc::world::chunk

namespace mc {
using ChunkPos = world::chunk::ChunkPos;
} // namespace mc

// 哈希函数支持
namespace std {
template <>
struct hash<mc::world::chunk::ChunkPos> {
    size_t operator()(const mc::world::chunk::ChunkPos& pos) const noexcept
    {
        return static_cast<size_t>(pos.toId());
    }
};
} // namespace std
