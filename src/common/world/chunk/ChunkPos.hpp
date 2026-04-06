#pragma once

#include "../../core/Types.hpp"
#include "../../core/Constants.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../util/Direction.hpp"
#include "../block/BlockPos.hpp"

#include <cstdint>
#include <functional>
#include <tuple>

namespace mc {

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
    {
    }

    ChunkPos(ChunkCoord x, ChunkCoord z) noexcept
        : x(x)
        , z(z)
    {
    }

    explicit ChunkPos(const BlockPos& pos) noexcept
        : x(pos.chunkX())
        , z(pos.chunkZ())
    {
    }

    explicit ChunkPos(const Vector3& pos) noexcept
        : x(math::toChunkCoord(pos.x))
        , z(math::toChunkCoord(pos.z))
    {
    }

    // 静态常量
    static ChunkPos zero() { return {0, 0}; }

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
    [[nodiscard]] BlockCoord worldX() const noexcept { return x * world::CHUNK_WIDTH; }
    [[nodiscard]] BlockCoord worldZ() const noexcept { return z * world::CHUNK_WIDTH; }

    // 区块中心坐标
    [[nodiscard]] Vector3 center(f32 y = 0.0f) const noexcept
    {
        return {
            static_cast<f32>(worldX()) + world::CHUNK_WIDTH / 2.0f,
            y,
            static_cast<f32>(worldZ()) + world::CHUNK_WIDTH / 2.0f
        };
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
    [[nodiscard]] ChunkPos north() const noexcept { return {x, z - 1}; }
    [[nodiscard]] ChunkPos south() const noexcept { return {x, z + 1}; }
    [[nodiscard]] ChunkPos east() const noexcept { return {x + 1, z}; }
    [[nodiscard]] ChunkPos west() const noexcept { return {x - 1, z}; }
};

/**
 * @brief 区块段位置
 *
 * 用于标识区块中的一个16x16x16的段
 * TODO URGENT 将该类作为单独文件
 */
class SectionPos {
public:
    ChunkCoord x;
    i32 y; // 段Y坐标 (0-15 for y=0-255)
    ChunkCoord z;

    SectionPos() noexcept
        : x(0)
        , y(0)
        , z(0)
    {
    }

    SectionPos(ChunkCoord x, i32 y, ChunkCoord z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }

    explicit SectionPos(const BlockPos& pos) noexcept
        : x(pos.chunkX())
        , y(pos.sectionIndex())
        , z(pos.chunkZ())
    {
    }

    /**
     * @brief 从方块位置创建区块段位置
     */
    [[nodiscard]] static SectionPos fromBlockPos(const BlockPos& pos) {
        return SectionPos(pos.chunkX(), pos.sectionIndex(), pos.chunkZ());
    }

    /**
     * @brief 从区块位置创建区块段位置
     */
    [[nodiscard]] static SectionPos fromChunkPos(ChunkCoord chunkX, i32 sectionY, ChunkCoord chunkZ) {
        return SectionPos(chunkX, sectionY, chunkZ);
    }

    /**
     * @brief 从长整型编码创建
     */
    [[nodiscard]] static SectionPos fromLong(i64 packed) {
        return SectionPos(
            static_cast<ChunkCoord>(packed >> 42),
            static_cast<i32>((packed << 44) >> 44),
            static_cast<ChunkCoord>((packed << 22) >> 42));
    }

    /**
     * @brief 转换为长整型编码
     */
    [[nodiscard]] i64 toLong() const {
        i64 lx = static_cast<i64>(x) & 0x3FFFFFLL;
        i64 lz = static_cast<i64>(z) & 0x3FFFFFLL;
        i64 ly = static_cast<i64>(y) & 0xFFFFFLL;
        return (lx << 42) | (lz << 20) | ly;
    }

    [[nodiscard]] bool operator==(const SectionPos& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] bool operator!=(const SectionPos& other) const noexcept
    {
        return !(*this == other);
    }

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
    [[nodiscard]] i32 worldX() const noexcept { return x << 4; }
    [[nodiscard]] i32 worldY() const noexcept { return y << 4; }
    [[nodiscard]] i32 worldZ() const noexcept { return z << 4; }

    /**
     * @brief 获取区块段内的局部坐标
     */
    [[nodiscard]] static i32 mask(i32 coord) {
        return coord & 0xF;
    }

    /**
     * @brief 向指定方向偏移
     */
    [[nodiscard]] SectionPos offset(i32 dx, i32 dy, i32 dz) const {
        return SectionPos(x + dx, y + dy, z + dz);
    }

    /**
     * @brief 向指定方向偏移
     */
    [[nodiscard]] SectionPos offset(Direction dir) const {
        switch (dir) {
            case Direction::Down:    return SectionPos(x, y - 1, z);
            case Direction::Up:      return SectionPos(x, y + 1, z);
            case Direction::North:   return SectionPos(x, y, z - 1);
            case Direction::South:   return SectionPos(x, y, z + 1);
            case Direction::West:    return SectionPos(x - 1, y, z);
            case Direction::East:    return SectionPos(x + 1, y, z);
            default:                 return *this;
        }
    }

    /**
     * @brief 转换为区块列位置（不含Y坐标）
     */
    [[nodiscard]] i64 toColumnLong() const {
        i64 lx = static_cast<i64>(x) & 0x3FFFFFLL;
        i64 lz = static_cast<i64>(z) & 0x3FFFFFLL;
        return (lx << 42) | (lz << 20);
    }

    // 世界Y坐标范围
    [[nodiscard]] i32 minY() const noexcept
    {
        return y * world::CHUNK_SECTION_HEIGHT;
    }

    [[nodiscard]] i32 maxY() const noexcept
    {
        return (y + 1) * world::CHUNK_SECTION_HEIGHT - 1;
    }

    // 区块位置
    [[nodiscard]] ChunkPos chunkPos() const noexcept
    {
        return {x, z};
    }
};

} // namespace mc

// 哈希函数支持
namespace std {
template<>
struct hash<mc::ChunkPos> {
    size_t operator()(const mc::ChunkPos& pos) const noexcept
    {
        return static_cast<size_t>(pos.toId());
    }
};

template<>
struct hash<mc::SectionPos> {
    size_t operator()(const mc::SectionPos& pos) const noexcept
    {
        size_t h1 = std::hash<mc::ChunkCoord>{}(pos.x);
        size_t h2 = std::hash<mc::i32>{}(pos.y);
        size_t h3 = std::hash<mc::ChunkCoord>{}(pos.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std
