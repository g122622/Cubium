#pragma once

#include "../../core/Types.hpp"
#include "../../core/Constants.hpp"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <limits>

namespace mc::math {

// ============================================================================
// 基本数学函数
// ============================================================================

/**
 * @brief 度转弧度
 */
[[nodiscard]] inline constexpr f32 toRadians(f32 degrees) noexcept
{
    return degrees * DEG_TO_RAD;
}

/**
 * @brief 弧度转度
 */
[[nodiscard]] inline constexpr f32 toDegrees(f32 radians) noexcept
{
    return radians * RAD_TO_DEG;
}

/**
 * @brief 数值限制在范围内
 */
template<typename T>
[[nodiscard]] inline constexpr T clamp(T value, T minVal, T maxVal) noexcept
{
    return std::clamp(value, minVal, maxVal);
}

/**
 * @brief 线性插值
 */
[[nodiscard]] inline constexpr f32 lerp(f32 a, f32 b, f32 t) noexcept
{
    return a + (b - a) * t;
}

/**
 * @brief 平滑插值 (smoothstep)
 */
[[nodiscard]] inline constexpr f32 smoothstep(f32 edge0, f32 edge1, f32 x) noexcept
{
    const f32 t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief 平方
 */
template<typename T>
[[nodiscard]] inline constexpr T square(T x) noexcept
{
    return x * x;
}

/**
 * @brief 立方
 */
template<typename T>
[[nodiscard]] inline constexpr T cube(T x) noexcept
{
    return x * x * x;
}

/**
 * @brief 判断是否接近零
 */
[[nodiscard]] inline bool isZero(f32 x, f32 epsilon = EPSILON) noexcept
{
    return std::abs(x) < epsilon;
}

/**
 * @brief 判断两个浮点数是否近似相等
 */
[[nodiscard]] inline bool approxEqual(f32 a, f32 b, f32 epsilon = EPSILON) noexcept
{
    return std::abs(a - b) < epsilon;
}

/**
 * @brief 快速逆平方根近似
 * TODO 现在CPU有硬件指令rsqrtss，或直接使用1.0f / sqrtf(value)（现代编译器优化后已很快）。
 *
 * 适用于正数输入，返回 $1 / \sqrt{x}$ 的快速近似值。
 */
[[nodiscard]] inline f32 fastInverseSqrt(f32 value) noexcept
{
    // const f32 halfValue = value * 0.5f;
    // u32 bits = 0;
    // std::memcpy(&bits, &value, sizeof(f32));
    // bits = 0x5f3759dfu - (bits >> 1);
    // std::memcpy(&value, &bits, sizeof(f32));
    // value = value * (1.5f - halfValue * value * value);
    // return value;

    return 1.0f / std::sqrt(value);
}

/**
 * @brief 向上取整到整数
 */
template<typename T>
[[nodiscard]] inline T ceilTo(f32 x) noexcept
{
    return static_cast<T>(std::ceil(x));
}

/**
 * @brief 向上取整到整数 (f64 版本)
 */
template<typename T>
[[nodiscard]] inline T ceilTo(f64 x) noexcept
{
    return static_cast<T>(std::ceil(x));
}

/**
 * @brief 向下取整到整数
 */
template<typename T>
[[nodiscard]] inline T floorTo(f32 x) noexcept
{
    return static_cast<T>(std::floor(x));
}

/**
 * @brief 向下取整到整数 (f64 版本)
 */
template<typename T>
[[nodiscard]] inline T floorTo(f64 x) noexcept
{
    return static_cast<T>(std::floor(x));
}

/**
 * @brief 四舍五入到整数
 */
template<typename T>
[[nodiscard]] inline T roundTo(f32 x) noexcept
{
    return static_cast<T>(std::round(x));
}

/**
 * @brief 四舍五入到整数 (f64 版本)
 */
template<typename T>
[[nodiscard]] inline T roundTo(f64 x) noexcept
{
    return static_cast<T>(std::round(x));
}

// ============================================================================
// 区块相关计算
// ============================================================================

/**
 * @brief 世界坐标转区块坐标
 */
[[nodiscard]] inline ChunkCoord toChunkCoord(f32 worldCoord) noexcept
{
    return static_cast<ChunkCoord>(std::floor(worldCoord / static_cast<f32>(world::CHUNK_WIDTH)));
}

/**
 * @brief 世界坐标转区块坐标（整数版本）
 */
[[nodiscard]] inline constexpr ChunkCoord toChunkCoord(BlockCoord worldCoord) noexcept
{
    return worldCoord >= 0
               ? worldCoord / world::CHUNK_WIDTH
               : (worldCoord - world::CHUNK_WIDTH + 1) / world::CHUNK_WIDTH;
}

/**
 * @brief 区块坐标转世界坐标（区块原点）
 */
[[nodiscard]] inline constexpr BlockCoord toWorldCoord(ChunkCoord chunkCoord) noexcept
{
    return chunkCoord * world::CHUNK_WIDTH;
}

/**
 * @brief 世界坐标转区块内坐标
 */
[[nodiscard]] inline BlockCoord toLocalCoord(f32 worldCoord) noexcept
{
    return static_cast<BlockCoord>(std::floor(worldCoord)) & world::CHUNK_MASK;
}

/**
 * @brief 世界坐标转区块内坐标（整数版本）
 */
[[nodiscard]] inline constexpr BlockCoord toLocalCoord(BlockCoord worldCoord) noexcept
{
    return worldCoord & world::CHUNK_MASK;
}

/**
 * @brief 区块坐标转换为64位唯一ID
 */
[[nodiscard]] inline constexpr u64 chunkPosToId(ChunkCoord x, ChunkCoord z) noexcept
{
    const u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFFLL);
    const u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFFLL);
    return (ux << 32) | uz;
}

/**
 * @brief 从64位ID解包区块坐标
 */
inline void idToChunkPos(u64 id, ChunkCoord& x, ChunkCoord& z) noexcept
{
    x = static_cast<ChunkCoord>(static_cast<i32>((id >> 32) & 0xFFFFFFFF));
    z = static_cast<ChunkCoord>(static_cast<i32>(id & 0xFFFFFFFF));
}

// ============================================================================
// 角度处理函数
// ============================================================================

/**
 * @brief 将角度规范化到 [-180, 180) 范围
 *
 * @param degrees 输入角度（度）
 * @return 规范化后的角度
 */
[[nodiscard]] inline f32 wrapDegrees(f32 degrees) noexcept
{
    degrees = std::fmod(degrees, 360.0f);
    if (degrees >= 180.0f) {
        degrees -= 360.0f;
    } else if (degrees < -180.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

/**
 * @brief 将角度规范化到 [0, 360) 范围
 *
 * @param degrees 输入角度（度）
 * @return 规范化后的角度
 */
[[nodiscard]] inline f32 wrapDegreesPositive(f32 degrees) noexcept
{
    degrees = std::fmod(degrees, 360.0f);
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

/**
 * @brief 限制角度变化量
 *
 * 计算从 sourceAngle 到 targetAngle 的最短路径，
 * 并限制最大变化量为 maximumChange。
 *
 * @param sourceAngle 起始角度（度）
 * @param targetAngle 目标角度（度）
 * @param maximumChange 最大变化量（度）
 * @return 调整后的角度
 */
[[nodiscard]] inline f32 clampAngle(f32 sourceAngle, f32 targetAngle, f32 maximumChange) noexcept
{
    f32 diff = wrapDegrees(targetAngle - sourceAngle);
    if (diff > maximumChange) {
        diff = maximumChange;
    } else if (diff < -maximumChange) {
        diff = -maximumChange;
    }
    return wrapDegreesPositive(sourceAngle + diff);
}

/**
 * @brief 计算两点之间的水平距离平方
 *
 * @param x1 第一个点的X坐标
 * @param z1 第一个点的Z坐标
 * @param x2 第二个点的X坐标
 * @param z2 第二个点的Z坐标
 * @return 水平距离的平方
 */
[[nodiscard]] inline f32 distanceHorizontalSq(f32 x1, f32 z1, f32 x2, f32 z2) noexcept
{
    const f32 dx = x2 - x1;
    const f32 dz = z2 - z1;
    return dx * dx + dz * dz;
}

/**
 * @brief 计算两点之间的距离平方
 *
 * @param x1 第一个点的X坐标
 * @param y1 第一个点的Y坐标
 * @param z1 第一个点的Z坐标
 * @param x2 第二个点的X坐标
 * @param y2 第二个点的Y坐标
 * @param z2 第二个点的Z坐标
 * @return 距离的平方
 */
[[nodiscard]] inline f32 distanceSq(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2) noexcept
{
    const f32 dx = x2 - x1;
    const f32 dy = y2 - y1;
    const f32 dz = z2 - z1;
    return dx * dx + dy * dy + dz * dz;
}

/**
 * @brief 角度插值（处理角度环绕）
 *
 * 参考 MC 1.16.5 ModelUtils.func_228283_a_
 * 用于平滑过渡两个角度值，考虑角度环绕问题。
 *
 * @param current 当前角度（弧度）
 * @param target 目标角度（弧度）
 * @param factor 插值因子（0-1，值越大越接近目标）
 * @return 插值后的角度
 */
[[nodiscard]] inline f32 lerpAngleRadians(f32 current, f32 target, f32 factor) noexcept
{
    f32 diff = target - current;
    // 规范化到 [-PI, PI)
    while (diff < -static_cast<f32>(PI)) {
        diff += static_cast<f32>(PI * 2.0);
    }
    while (diff >= static_cast<f32>(PI)) {
        diff -= static_cast<f32>(PI * 2.0);
    }
    return current + factor * diff;
}

} // namespace mc::math
