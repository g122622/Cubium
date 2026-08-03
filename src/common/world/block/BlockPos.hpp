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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <string>
#include <tuple>

namespace mc {

/**
 * @brief 方块位置（整数坐标）
 *
 * 用于精确定位方块在世界中的位置
 *
 * 参考: net.minecraft.util.math.BlockPos
 */
class BlockPos {
public:
    BlockCoord x, y, z;

    // 构造函数
    BlockPos() noexcept
        : x(0)
        , y(0)
        , z(0)
    {}

    BlockPos(BlockCoord x_, BlockCoord y_, BlockCoord z_) noexcept
        : x(x_)
        , y(y_)
        , z(z_)
    {}

    explicit BlockPos(const Vector3& pos) noexcept
        : x(static_cast<BlockCoord>(std::floor(pos.x)))
        , y(static_cast<BlockCoord>(std::floor(pos.y)))
        , z(static_cast<BlockCoord>(std::floor(pos.z)))
    {}

    // 静态常量
    static BlockPos zero() { return {0, 0, 0}; }

    // 算术运算
    [[nodiscard]] BlockPos operator+(const BlockPos& other) const noexcept
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] BlockPos operator-(const BlockPos& other) const noexcept
    {
        return {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] BlockPos operator*(i32 scalar) const noexcept { return {x * scalar, y * scalar, z * scalar}; }

    BlockPos& operator+=(const BlockPos& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    BlockPos& operator-=(const BlockPos& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // 比较运算
    [[nodiscard]] bool operator==(const BlockPos& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] bool operator!=(const BlockPos& other) const noexcept { return !(*this == other); }

    // 排序支持
    [[nodiscard]] bool operator<(const BlockPos& other) const noexcept
    {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }

    // 转换为浮点位置
    [[nodiscard]] Vector3 toVector3() const noexcept
    {
        return {static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)};
    }

    // 获取方块中心点
    [[nodiscard]] Vector3 center() const noexcept
    {
        return {static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f, static_cast<f32>(z) + 0.5f};
    }

    /**
     * @brief 计算到另一个位置的曼哈顿距离
     *
     * 曼哈顿距离 = |x1-x2| + |y1-y2| + |z1-z2|
     *
     * @param other 另一个位置
     * @return 曼哈顿距离
     */
    [[nodiscard]] i32 manhattanDistance(const BlockPos& other) const noexcept
    {
        return std::abs(x - other.x) + std::abs(y - other.y) + std::abs(z - other.z);
    }

    /**
     * @brief 计算到另一个位置的欧几里得距离平方
     *
     * 使用 i64 避免大坐标溢出。
     *
     * @param other 另一个位置
     * @return 距离的平方
     */
    [[nodiscard]] i64 distanceSq(const BlockPos& other) const noexcept
    {
        const i64 dx = static_cast<i64>(x) - static_cast<i64>(other.x);
        const i64 dy = static_cast<i64>(y) - static_cast<i64>(other.y);
        const i64 dz = static_cast<i64>(z) - static_cast<i64>(other.z);
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 计算到另一个位置的水平距离平方（仅 XZ）
     *
     * 使用 i64 避免大坐标溢出。
     *
     * @param other 另一个位置
     * @return 水平距离的平方
     */
    [[nodiscard]] i64 distanceHorizontalSq(const BlockPos& other) const noexcept
    {
        const i64 dx = static_cast<i64>(x) - static_cast<i64>(other.x);
        const i64 dz = static_cast<i64>(z) - static_cast<i64>(other.z);
        return dx * dx + dz * dz;
    }

    // 获取相邻方块
    [[nodiscard]] BlockPos up(i32 offset = 1) const noexcept { return {x, y + offset, z}; }
    [[nodiscard]] BlockPos down(i32 offset = 1) const noexcept { return {x, y - offset, z}; }
    [[nodiscard]] BlockPos north(i32 offset = 1) const noexcept { return {x, y, z - offset}; }
    [[nodiscard]] BlockPos south(i32 offset = 1) const noexcept { return {x, y, z + offset}; }
    [[nodiscard]] BlockPos east(i32 offset = 1) const noexcept { return {x + offset, y, z}; }
    [[nodiscard]] BlockPos west(i32 offset = 1) const noexcept { return {x - offset, y, z}; }

    // 根据朝向获取相邻方块
    [[nodiscard]] BlockPos offset(BlockFace face, i32 distance = 1) const noexcept
    {
        switch (face) {
            case BlockFace::Top:
                return {x, y + distance, z};
            case BlockFace::Bottom:
                return {x, y - distance, z};
            case BlockFace::North:
                return {x, y, z - distance};
            case BlockFace::South:
                return {x, y, z + distance};
            case BlockFace::East:
                return {x + distance, y, z};
            case BlockFace::West:
                return {x - distance, y, z};
            default:
                return *this;
        }
    }

    /**
     * @brief 根据方向获取相邻方块
     *
     * @param dir 方向
     * @param distance 距离（默认1）
     * @return 相邻方块位置
     */
    [[nodiscard]] BlockPos offset(Direction dir, i32 distance = 1) const noexcept
    {
        return {x + Directions::xOffset(dir) * distance,
            y + Directions::yOffset(dir) * distance,
            z + Directions::zOffset(dir) * distance};
    }

    // 转换为64位唯一ID
    [[nodiscard]] u64 toId() const noexcept
    {
        // 将坐标打包为64位ID
        // Y: 8位 (-128 to 127, 或更高的位)
        // X: 28位
        // Z: 28位
        const u64 ux = static_cast<u64>(static_cast<i64>(x) & 0x0FFFFFFFLL);
        const u64 uy = static_cast<u64>(static_cast<i64>(y) & 0xFFLL);
        const u64 uz = static_cast<u64>(static_cast<i64>(z) & 0x0FFFFFFFLL);
        return (ux << 36) | (uy << 28) | uz;
    }

    // ========================================================================
    // 网络序列化打包 (asLong / fromLong)
    // 与 MC Java BlockPos.asLong 完全一致的位布局：
    //   X 占高 26 位 (bit 38..63)，Z 占中间 26 位 (bit 12..37)，Y 占低 12 位 (bit 0..11)
    //   PACKED_HORIZONTAL_LENGTH = 26, PACKED_Y_LENGTH = 12
    // 支持 X/Z: ±33554432 (2^25)，Y: ±2048 (2^11)
    // 用于 VibrationParticleOption 等网络协议中的方块位置编码
    // ========================================================================

    /// 水平坐标（X/Z）打包位宽，对应 MC Java BlockPos.PACKED_HORIZONTAL_LENGTH
    static constexpr i32 PACKED_HORIZONTAL_LENGTH = 26;
    /// Y 坐标打包位宽，对应 MC Java BlockPos.PACKED_Y_LENGTH
    static constexpr i32 PACKED_Y_LENGTH = 64 - 2 * PACKED_HORIZONTAL_LENGTH; // 12
    /// X 坐标在 packed long 中的位移，对应 MC Java BlockPos.X_OFFSET
    static constexpr i32 PACKED_X_OFFSET = PACKED_Y_LENGTH + PACKED_HORIZONTAL_LENGTH; // 38
    /// Z 坐标在 packed long 中的位移，对应 MC Java BlockPos.Z_OFFSET
    static constexpr i32 PACKED_Z_OFFSET = PACKED_Y_LENGTH; // 12

    /**
     * @brief 将方块位置打包为 64 位整数
     *
     * 位布局：X(高26位) | Z(中26位) | Y(低12位)
     * 对应 MC Java: BlockPos.asLong(int, int, int)
     *
     * @return 打包后的 64 位整数
     */
    [[nodiscard]] i64 asLong() const noexcept { return asLong(x, y, z); }

    /**
     * @brief 将三个整数坐标打包为 64 位整数
     *
     * 对应 MC Java: BlockPos.asLong(int x, int y, int z)
     */
    [[nodiscard]] static constexpr i64 asLong(i32 px, i32 py, i32 pz) noexcept
    {
        constexpr i64 XZ_MASK = (1LL << PACKED_HORIZONTAL_LENGTH) - 1;
        constexpr i64 Y_MASK = (1LL << PACKED_Y_LENGTH) - 1;
        return ((static_cast<i64>(px) & XZ_MASK) << PACKED_X_OFFSET) | (static_cast<i64>(py) & Y_MASK) |
            ((static_cast<i64>(pz) & XZ_MASK) << PACKED_Z_OFFSET);
    }

    /**
     * @brief 从 packed long 中提取 X 坐标
     *
     * 对应 MC Java: BlockPos.getX(long)
     */
    [[nodiscard]] static constexpr i32 getXFromLong(i64 packed) noexcept
    {
        // 算术右移实现符号扩展：先左移使 X 成为最高 26 位，再右移回低位
        return static_cast<i32>(
            packed << (64 - PACKED_X_OFFSET - PACKED_HORIZONTAL_LENGTH) >> (64 - PACKED_HORIZONTAL_LENGTH));
    }

    /**
     * @brief 从 packed long 中提取 Y 坐标
     *
     * 对应 MC Java: BlockPos.getY(long)
     */
    [[nodiscard]] static constexpr i32 getYFromLong(i64 packed) noexcept
    {
        return static_cast<i32>(packed << (64 - PACKED_Y_LENGTH) >> (64 - PACKED_Y_LENGTH));
    }

    /**
     * @brief 从 packed long 中提取 Z 坐标
     *
     * 对应 MC Java: BlockPos.getZ(long)
     */
    [[nodiscard]] static constexpr i32 getZFromLong(i64 packed) noexcept
    {
        return static_cast<i32>(
            packed << (64 - PACKED_Z_OFFSET - PACKED_HORIZONTAL_LENGTH) >> (64 - PACKED_HORIZONTAL_LENGTH));
    }

    /**
     * @brief 从 packed long 还原 BlockPos
     *
     * 对应 MC Java: BlockPos.of(long)
     */
    [[nodiscard]] static BlockPos fromLong(i64 packed) noexcept
    {
        return BlockPos(getXFromLong(packed), getYFromLong(packed), getZFromLong(packed));
    }

    // 区块坐标
    [[nodiscard]] ChunkCoord chunkX() const noexcept { return math::toChunkCoord(x); }
    [[nodiscard]] ChunkCoord chunkZ() const noexcept { return math::toChunkCoord(z); }

    // 区块内坐标
    [[nodiscard]] BlockCoord localX() const noexcept { return math::toLocalCoord(x); }
    [[nodiscard]] BlockCoord localZ() const noexcept { return math::toLocalCoord(z); }

    // 段坐标 (Y >> SECTION_SHIFT)，范围 [MIN_SECTION_Y, MAX_SECTION_Y]
    [[nodiscard]] i32 sectionCoord() const noexcept { return y >> world::SECTION_SHIFT; }

    // 转为字符串
    [[nodiscard]] std::string toString() const
    {
        return "BlockPos(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }

    // ========================================================================
    // 区域遍历方法
    // ========================================================================

    /**
     * @brief 遍历从当前位置到目标位置之间的所有方块位置
     *
     * @param end 结束位置
     * @param callback 回调函数，返回 false 可提前终止遍历
     */
    void forEachBetween(const BlockPos& end, std::function<bool(const BlockPos&)> callback) const
    {
        i32 minX = std::min(x, end.x);
        i32 maxX = std::max(x, end.x);
        i32 minY = std::min(y, end.y);
        i32 maxY = std::max(y, end.y);
        i32 minZ = std::min(z, end.z);
        i32 maxZ = std::max(z, end.z);

        BlockPos pos;
        for (i32 py = minY; py <= maxY; ++py) {
            pos.y = py;
            for (i32 pz = minZ; pz <= maxZ; ++pz) {
                pos.z = pz;
                for (i32 px = minX; px <= maxX; ++px) {
                    pos.x = px;
                    if (!callback(pos)) {
                        return;
                    }
                }
            }
        }
    }

    /**
     * @brief 遍历以当前位置为中心的立方体区域
     *
     * @param radiusX X方向半径
     * @param radiusY Y方向半径
     * @param radiusZ Z方向半径
     * @param callback 回调函数，返回 false 可提前终止遍历
     */
    void forEachInCube(i32 radiusX, i32 radiusY, i32 radiusZ, std::function<bool(const BlockPos&)> callback) const
    {
        BlockPos pos;
        for (i32 dy = -radiusY; dy <= radiusY; ++dy) {
            pos.y = y + dy;
            for (i32 dz = -radiusZ; dz <= radiusZ; ++dz) {
                pos.z = z + dz;
                for (i32 dx = -radiusX; dx <= radiusX; ++dx) {
                    pos.x = x + dx;
                    if (!callback(pos)) {
                        return;
                    }
                }
            }
        }
    }

    /**
     * @brief 遍历以当前位置为中心的正方体区域
     *
     * @param radius 半径
     * @param callback 回调函数，返回 false 可提前终止遍历
     */
    void forEachInCube(i32 radius, std::function<bool(const BlockPos&)> callback) const
    {
        forEachInCube(radius, radius, radius, callback);
    }

    /**
     * @brief 遍历当前位置周围的相邻方块（六个方向）
     *
     * @param callback 回调函数，返回 false 可提前终止遍历
     */
    void forEachNeighbor(std::function<bool(const BlockPos&)> callback) const
    {
        static constexpr Direction directions[] = {
            Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

        for (Direction dir : directions) {
            if (!callback(offset(dir))) {
                return;
            }
        }
    }

    /**
     * @brief 遍历当前位置周围的相邻方块（包括对角线，共26个）
     *
     * @param callback 回调函数，返回 false 可提前终止遍历
     */
    void forEachNeighborIncludingDiagonal(std::function<bool(const BlockPos&)> callback) const
    {
        BlockPos pos;
        for (i32 dy = -1; dy <= 1; ++dy) {
            pos.y = y + dy;
            for (i32 dz = -1; dz <= 1; ++dz) {
                pos.z = z + dz;
                for (i32 dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0 && dz == 0) continue;
                    pos.x = x + dx;
                    if (!callback(pos)) {
                        return;
                    }
                }
            }
        }
    }
};

/**
 * @brief 可变方块位置
 *
 * 用于迭代和遍历时避免频繁创建新的 BlockPos 对象。
 * 可以原地修改坐标值。
 *
 * 参考: net.minecraft.util.math.BlockPos.Mutable
 *
 * 用法示例:
 * @code
 * BlockPos::Mutable mutable;
 * for (int y = minY; y <= maxY; ++y) {
 *     mutable.setY(y);
 *     for (int z = minZ; z <= maxZ; ++z) {
 *         mutable.setZ(z);
 *         for (int x = minX; x <= maxX; ++x) {
 *             mutable.setX(x);
 *             // 使用 mutable 作为 BlockPos
 *         }
 *     }
 * }
 * @endcode
 */
class BlockPosMutable : public BlockPos {
public:
    BlockPosMutable() noexcept
        : BlockPos()
    {}
    BlockPosMutable(BlockCoord x_, BlockCoord y_, BlockCoord z_) noexcept
        : BlockPos(x_, y_, z_)
    {}
    explicit BlockPosMutable(const BlockPos& pos) noexcept
        : BlockPos(pos)
    {}

    /**
     * @brief 设置X坐标
     */
    BlockPosMutable& setX(BlockCoord newX) noexcept
    {
        x = newX;
        return *this;
    }

    /**
     * @brief 设置Y坐标
     */
    BlockPosMutable& setY(BlockCoord newY) noexcept
    {
        y = newY;
        return *this;
    }

    /**
     * @brief 设置Z坐标
     */
    BlockPosMutable& setZ(BlockCoord newZ) noexcept
    {
        z = newZ;
        return *this;
    }

    /**
     * @brief 设置所有坐标
     */
    BlockPosMutable& set(BlockCoord newX, BlockCoord newY, BlockCoord newZ) noexcept
    {
        x = newX;
        y = newY;
        z = newZ;
        return *this;
    }

    /**
     * @brief 从另一个 BlockPos 设置
     */
    BlockPosMutable& set(const BlockPos& pos) noexcept
    {
        x = pos.x;
        y = pos.y;
        z = pos.z;
        return *this;
    }

    /**
     * @brief 移动位置
     */
    BlockPosMutable& move(Direction dir, i32 distance = 1) noexcept
    {
        x += Directions::xOffset(dir) * distance;
        y += Directions::yOffset(dir) * distance;
        z += Directions::zOffset(dir) * distance;
        return *this;
    }

    /**
     * @brief 移动位置
     */
    BlockPosMutable& move(BlockFace face, i32 distance = 1) noexcept
    {
        switch (face) {
            case BlockFace::Top:
                y += distance;
                break;
            case BlockFace::Bottom:
                y -= distance;
                break;
            case BlockFace::North:
                z -= distance;
                break;
            case BlockFace::South:
                z += distance;
                break;
            case BlockFace::East:
                x += distance;
                break;
            case BlockFace::West:
                x -= distance;
                break;
            default:
                break;
        }
        return *this;
    }

    /**
     * @brief 移动位置
     */
    BlockPosMutable& move(i32 dx, i32 dy, i32 dz) noexcept
    {
        x += dx;
        y += dy;
        z += dz;
        return *this;
    }

    /**
     * @brief 转换为不可变 BlockPos
     */
    [[nodiscard]] BlockPos toImmutable() const noexcept { return BlockPos(x, y, z); }
};

} // namespace mc

// 哈希函数支持
namespace std {
template <>
struct hash<mc::BlockPos> {
    size_t operator()(const mc::BlockPos& pos) const noexcept { return static_cast<size_t>(pos.toId()); }
};
} // namespace std
