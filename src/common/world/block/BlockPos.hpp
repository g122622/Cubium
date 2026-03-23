#pragma once

#include "../../core/Types.hpp"
#include "../../core/Constants.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/Direction.hpp"

#include <cstdint>
#include <functional>
#include <tuple>

namespace mc {

/**
 * @brief 方块位置（整数坐标）
 *
 * 用于精确定位方块在世界中的位置
 */
class BlockPos {
public:
    BlockCoord x, y, z;

    // 构造函数
    BlockPos() noexcept
        : x(0)
        , y(0)
        , z(0)
    {
    }

    BlockPos(BlockCoord x, BlockCoord y, BlockCoord z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }

    explicit BlockPos(const Vector3& pos) noexcept
        : x(static_cast<BlockCoord>(std::floor(pos.x)))
        , y(static_cast<BlockCoord>(std::floor(pos.y)))
        , z(static_cast<BlockCoord>(std::floor(pos.z)))
    {
    }

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

    [[nodiscard]] BlockPos operator*(i32 scalar) const noexcept
    {
        return {x * scalar, y * scalar, z * scalar};
    }

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

    [[nodiscard]] bool operator!=(const BlockPos& other) const noexcept
    {
        return !(*this == other);
    }

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
            case BlockFace::Top:    return {x, y + distance, z};
            case BlockFace::Bottom: return {x, y - distance, z};
            case BlockFace::North:  return {x, y, z - distance};
            case BlockFace::South:  return {x, y, z + distance};
            case BlockFace::East:   return {x + distance, y, z};
            case BlockFace::West:   return {x - distance, y, z};
            default: return *this;
        }
    }

    /**
     * @brief 根据方向获取相邻方块
     *
     * @param dir 方向
     * @param distance 距离（默认1）
     * @return 相邻方块位置
     */
    [[nodiscard]] BlockPos offset(Direction dir, i32 distance = 1) const noexcept {
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

    // 区块坐标
    [[nodiscard]] ChunkCoord chunkX() const noexcept { return math::toChunkCoord(x); }
    [[nodiscard]] ChunkCoord chunkZ() const noexcept { return math::toChunkCoord(z); }

    // 区块内坐标
    [[nodiscard]] BlockCoord localX() const noexcept { return math::toLocalCoord(x); }
    [[nodiscard]] BlockCoord localZ() const noexcept { return math::toLocalCoord(z); }

    // 区块段索引 (Y / 16)
    [[nodiscard]] i32 sectionIndex() const noexcept
    {
        return y / world::CHUNK_SECTION_HEIGHT;
    }

    // 转为字符串
    [[nodiscard]] std::string toString() const
    {
        return "BlockPos(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }
};

} // namespace mc

// 哈希函数支持
namespace std {
template<>
struct hash<mc::BlockPos> {
    size_t operator()(const mc::BlockPos& pos) const noexcept
    {
        return static_cast<size_t>(pos.toId());
    }
};
} // namespace std
