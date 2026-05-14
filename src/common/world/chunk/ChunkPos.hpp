#pragma once

#include "../../core/Constants.hpp"
#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include "../../util/math/MathUtils.hpp"
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
    static ChunkPos zero() { return {0, 0}; }

    // 算术运算
    [[nodiscard]] ChunkPos operator+(const ChunkPos& other) const noexcept { return {x + other.x, z + other.z}; }

    [[nodiscard]] ChunkPos operator-(const ChunkPos& other) const noexcept { return {x - other.x, z - other.z}; }

    [[nodiscard]] ChunkPos operator*(i32 scalar) const noexcept { return {x * scalar, z * scalar}; }

    // 比较运算
    [[nodiscard]] bool operator==(const ChunkPos& other) const noexcept { return x == other.x && z == other.z; }

    [[nodiscard]] bool operator!=(const ChunkPos& other) const noexcept { return !(*this == other); }

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
        return {static_cast<f32>(worldX()) + world::CHUNK_WIDTH / 2.0f,
            y,
            static_cast<f32>(worldZ()) + world::CHUNK_WIDTH / 2.0f};
    }

    // 转换为64位唯一ID
    [[nodiscard]] u64 toId() const noexcept { return math::chunkPosToId(x, z); }

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

} // namespace mc

// 哈希函数支持
namespace std {
template <>
struct hash<mc::ChunkPos> {
    size_t operator()(const mc::ChunkPos& pos) const noexcept { return static_cast<size_t>(pos.toId()); }
};
} // namespace std

// 包含 SectionPos（已迁移到单独文件）
#include "SectionPos.hpp"
