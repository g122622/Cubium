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

#include "MathConstants.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

namespace mc::math {

// ============================================================================
// 度数/弧度转换常量
// ============================================================================

/// 度转弧度乘数
constexpr f32 DEG_TO_RAD = PI / 180.0f;

/// 弧度转度乘数
constexpr f32 RAD_TO_DEG = 180.0f / PI;

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
 * @brief 数值限制在范围内，对应原版 `Mth.clamp`
 *
 * 原版按类型分了四条重载，语义并不相同：
 *
 * ```
 * int/long   : Math.min(Math.max(v, min), max)
 * float/double: v < min ? min : Math.min(v, max)
 * ```
 *
 * 两者只在 `min > max` 这种畸形区间下才会分道扬镳：
 * `clamp(5, 10, 0)` 整数版得 0（先 max 抬到 10、再 min 压回 0），
 * 浮点版得 10（`5 < 10` 直接返回 min）。这里按原版分别实现。
 *
 * **不要换成 `std::clamp`**：它在 `min > max` 时是未定义行为，实际会返回 `min`，
 * 与两种原版语义都对不上。
 */
template <typename T>
[[nodiscard]] inline constexpr T clamp(T value, T minVal, T maxVal) noexcept
{
    if constexpr (std::is_floating_point_v<T>) {
        return value < minVal ? minVal : std::min(value, maxVal);
    } else {
        return std::min(std::max(value, minVal), maxVal);
    }
}

/**
 * @brief 线性插值 (f32 版本)
 */
// 不能保留 constexpr：这里必须用 volatile 强制乘积先落回标量存储。
//
// 启用了 +fma 的目标（-march=native 在本机 raptorlake 上会带上）会把 `a + (b-a)*t`
// 编译成单条 fma，乘积不再舍入到 f32，与原版 JVM 的逐步舍入差 1 ulp。以
// a=0.864000499、b=-8.08599949、t=0.206599995 为例：融合得 -0.985069394，
// 逐步舍入得 -0.985069454。这个差值会一路传到噪声采样与地形高度。
//
// 没有 constexpr 调用方（全库查过），去掉 constexpr 不影响任何现有代码。
[[nodiscard]] inline f32 lerp(f32 a, f32 b, f32 t) noexcept
{
    const volatile f32 delta = (b - a) * t;
    return a + delta;
}

/**
 * @brief 线性插值 (f64 版本)
 *
 * 同 f32 版：禁止 FMA 收缩，保留原版的逐步舍入。
 */
[[nodiscard]] inline f64 lerp(f64 a, f64 b, f64 t) noexcept
{
    const volatile f64 delta = (b - a) * t;
    return a + delta;
}

/**
 * @brief 三线性插值 (Trilinear Interpolation)
 *
 * 沿三个轴向依次进行线性插值，用于 3D 噪声生成等场景。
 *
 * 插值顺序：X 轴 → Z 轴 → Y 轴
 *
 * 8 个角点的编号约定（参考 Perlin 噪声）：
 * - v0: (0, 0, 0)  v1: (1, 0, 0)  沿 X 轴：t1
 * - v2: (0, 0, 1)  v3: (1, 0, 1)  沿 Z 轴：t2
 * - v4: (0, 1, 0)  v5: (1, 1, 0)  沿 Y 轴：t3
 * - v6: (0, 1, 1)  v7: (1, 1, 1)
 *
 * @param t1 X 轴插值因子 [0, 1]
 * @param t2 Z 轴插值因子 [0, 1]
 * @param t3 Y 轴插值因子 [0, 1]
 * @param v0 角点 (0, 0, 0) 的值
 * @param v1 角点 (1, 0, 0) 的值
 * @param v2 角点 (0, 0, 1) 的值
 * @param v3 角点 (1, 0, 1) 的值
 * @param v4 角点 (0, 1, 0) 的值
 * @param v5 角点 (1, 1, 0) 的值
 * @param v6 角点 (0, 1, 1) 的值
 * @param v7 角点 (1, 1, 1) 的值
 * @return 插值结果
 *
 * 三线性插值，插值顺序为 X → Y → Z，与 MC 的 Mth.lerp3 一致。
 *
 * @param sx X 方向插值因子（smoothstep 后的值）
 * @param sy Y 方向插值因子
 * @param sz Z 方向插值因子
 * @param d0 角点 (0,0,0) 的值
 * @param d1 角点 (1,0,0) 的值
 * @param d2 角点 (0,1,0) 的值
 * @param d3 角点 (1,1,0) 的值
 * @param d4 角点 (0,0,1) 的值
 * @param d5 角点 (1,0,1) 的值
 * @param d6 角点 (0,1,1) 的值
 * @param d7 角点 (1,1,1) 的值
 * @return 插值结果
 */
[[nodiscard]] inline f32 lerp3(
    f32 sx, f32 sy, f32 sz, f32 d0, f32 d1, f32 d2, f32 d3, f32 d4, f32 d5, f32 d6, f32 d7) noexcept
{
    // X 轴插值
    const f32 x0 = lerp(d0, d1, sx); // (0,0,0)→(1,0,0)
    const f32 x1 = lerp(d2, d3, sx); // (0,1,0)→(1,1,0)
    const f32 x2 = lerp(d4, d5, sx); // (0,0,1)→(1,0,1)
    const f32 x3 = lerp(d6, d7, sx); // (0,1,1)→(1,1,1)

    // Y 轴插值
    const f32 y0 = lerp(x0, x1, sy); // z=0 平面
    const f32 y1 = lerp(x2, x3, sy); // z=1 平面

    // Z 轴插值
    return lerp(y0, y1, sz);
}

/**
 * @brief f64 版本的三线性插值，用于 NoiseInterpolator.fillingCell 模式
 *
 * MC 的 Mth.lerp3 使用 f64 参数。
 * 参数顺序: d0=(0,0,0), d1=(1,0,0), d2=(0,1,0), d3=(1,1,0),
 *           d4=(0,0,1), d5=(1,0,1), d6=(0,1,1), d7=(1,1,1)
 */
[[nodiscard]] inline f64 lerp3(
    f64 sx, f64 sy, f64 sz, f64 d0, f64 d1, f64 d2, f64 d3, f64 d4, f64 d5, f64 d6, f64 d7) noexcept
{
    // X 轴插值
    const f64 x0 = lerp(d0, d1, sx);
    const f64 x1 = lerp(d2, d3, sx);
    const f64 x2 = lerp(d4, d5, sx);
    const f64 x3 = lerp(d6, d7, sx);

    // Y 轴插值
    const f64 y0 = lerp(x0, x1, sy);
    const f64 y1 = lerp(x2, x3, sy);

    // Z 轴插值
    return lerp(y0, y1, sz);
}

/**
 * @brief 线性插值，但将插值因子限制在 [0, 1] 范围内
 * @param a 起始值
 * @param b 目标值
 * @param t 插值因子（会被 clamp 到 [0, 1]）
 * @return 插值结果
 *
 * 参考 MC 1.16.5: MathHelper.clampedLerp
 */
[[nodiscard]] inline f32 clampedLerp(f32 a, f32 b, f32 t) noexcept
{
    return lerp(a, b, clamp(t, 0.0f, 1.0f));
}

/**
 * @brief 线性插值，将输入值从输入范围映射到输出范围
 * @param outputMin 输出最小值
 * @param outputMax 输出最大值
 * @param inputMin 输入最小值
 * @param inputMax 输入最大值
 * @param inputValue 输入值
 * @return 插值结果
 *
 * 参考 MC 1.16.5: LinearPosTest 使用的映射插值
 */
[[nodiscard]] inline f32 mappedLerp(f32 outputMin, f32 outputMax, f32 inputMin, f32 inputMax, f32 inputValue) noexcept
{
    if (inputMax <= inputMin) {
        return outputMin;
    }
    f32 t = (inputValue - inputMin) / (inputMax - inputMin);
    return lerp(outputMin, outputMax, clamp(t, 0.0f, 1.0f));
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
 *
 * 整数入参按有符号回绕计算：原版的 `int * int` 溢出是定义良好的回绕，而 C++ 的有符号
 * 溢出是未定义行为，编译器可以假定它不发生并据此优化。已知 `Mth.square` 会被喂进
 * 坐标量级的数值，这里显式经无符号乘再转回，保证与 Java 一致。
 */
template <typename T>
[[nodiscard]] inline constexpr T square(T x) noexcept
{
    if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        using U = std::make_unsigned_t<T>;
        return static_cast<T>(static_cast<U>(static_cast<U>(x) * static_cast<U>(x)));
    } else {
        return x * x;
    }
}

/**
 * @brief 线性映射，将值从 [fromMin, fromMax] 映射到 [toMin, toMax]
 * 与 mappedLerp 不同，此函数不 clamp 输入值
 *
 * 参考 MC 1.21: Mth.map
 */
[[nodiscard]] inline f64 map(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax) noexcept
{
    return toMin + (value - fromMin) * (toMax - toMin) / (fromMax - fromMin);
}

[[nodiscard]] inline f32 map(f32 value, f32 fromMin, f32 fromMax, f32 toMin, f32 toMax) noexcept
{
    return toMin + (value - fromMin) * (toMax - toMin) / (fromMax - fromMin);
}

/**
 * @brief 钳位线性映射，将值从 [fromMin, fromMax] 映射到 [toMin, toMax] 并 clamp
 *
 * 参考 MC 1.21: Mth.clampedMap
 */
[[nodiscard]] inline f64 clampedMap(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax) noexcept
{
    f64 t = (value - fromMin) / (fromMax - fromMin);
    if (t < 0.0) {
        t = 0.0;
    }
    if (t > 1.0) {
        t = 1.0;
    }
    // 同 lerp：乘积先落回标量存储，禁止 FMA 收缩。
    const volatile f64 delta = t * (toMax - toMin);
    return toMin + delta;
}

/**
 * @brief 立方
 */
template <typename T>
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
 * @brief 快速逆平方根（一次位模式初猜 + 一次牛顿迭代）
 *
 * 计算 $1 / \sqrt{x}$ 的近似值，相对误差约 1e-4 量级——它**不是**正确舍入的结果，
 * 也不能用 `1.0f / std::sqrt(x)` 替代：
 *
 * - 结构地形适配（Beardifier 的胡须曲线）直接使用本近似值参与乘除，原版也如此；
 *   换成精确值会让结构边缘的密度贡献相差千分之一量级；
 * - 必须用 double 的位模式与魔数 `6910469410427058090`（即原版 Mth.fastInvSqrt 的常量），
 *   换成 float 版本（0x5f3759df）会得到另一个近似值。
 *
 * @param value 正数输入值
 * @return $1 / \sqrt{value}$ 的近似值
 */
[[nodiscard]] inline f64 fastInvSqrt(f64 value) noexcept
{
    const f64 half = 0.5 * value;
    u64 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = 6910469410427058090ULL - (bits >> 1); // 位模式魔数：把「除法」变成「减去指数的一半」
    f64 estimate = 0.0;
    std::memcpy(&estimate, &bits, sizeof(estimate));
    return estimate * (1.5 - half * estimate * estimate); // 一次牛顿迭代
}

/**
 * @brief 正弦（复刻 MC 的 Mth.sin 查表实现）
 *
 * 原版 Mth 不是直接调 Math.sin，而是预计算一张 65536 项的表再按量化索引取用：
 *   SIN[i] = (f32)Math.sin(i / 10430.378350470453)
 *   sin(v) = SIN[(i32)((long)(v * 10430.378350470453) & 65535L)]
 *
 * 两者差异约为 f32 精度量级（约 1e-7），单独看可忽略；但世界生成中它参与
 * 半径、位置等几何量的计算并被反复放大，与 std::sin 混用会让生成结果逐位分叉。
 * 凡是原版写 `Mth.sin` 的地方都必须用本函数，原版写 `Math.sin` 的地方才用 std::sin。
 *
 * @param value 弧度值
 * @return 量化查表得到的 f32 正弦值
 */
[[nodiscard]] f32 mthSin(f64 value) noexcept;

/**
 * @brief 余弦（复刻 MC 的 Mth.cos 查表实现）
 *
 * 原版实现为 `SIN[(i32)((long)(v * SCALE + 16384.0) & 65535L)]`，
 * 即在同一张表上偏移四分之一周期（65536 / 4 = 16384）。
 *
 * @param value 弧度值
 * @return 量化查表得到的 f32 余弦值
 */
[[nodiscard]] f32 mthCos(f64 value) noexcept;

/**
 * @brief 向上取整到整数
 */
template <typename T>
[[nodiscard]] inline T ceilTo(f32 x) noexcept
{
    return static_cast<T>(std::ceil(x));
}

/**
 * @brief 向上取整到整数 (f64 版本)
 */
template <typename T>
[[nodiscard]] inline T ceilTo(f64 x) noexcept
{
    return static_cast<T>(std::ceil(x));
}

/**
 * @brief 向下取整到整数
 */
template <typename T>
[[nodiscard]] inline T floorTo(f32 x) noexcept
{
    return static_cast<T>(std::floor(x));
}

/**
 * @brief 向下取整到整数 (f64 版本)
 */
template <typename T>
[[nodiscard]] inline T floorTo(f64 x) noexcept
{
    return static_cast<T>(std::floor(x));
}

/**
 * @brief 四舍五入到整数
 */
template <typename T>
[[nodiscard]] inline T roundTo(f32 x) noexcept
{
    return static_cast<T>(std::round(x));
}

/**
 * @brief 四舍五入到整数 (f64 版本)
 */
template <typename T>
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
    return worldCoord >= 0 ? worldCoord / world::CHUNK_WIDTH
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
 * @brief MC 1.16.5 clampedRotate - 限制角度旋转速度
 *
 * 与 clampAngle 不同，此函数不包装结果到 [0, 360)，
 * 而是直接返回 sourceAngle + clamped(diff)。
 * 这是 MC 1.16.5 中 LookController 和 MovementController 使用的方法。
 *
 * @param from 当前角度（度）
 * @param to 目标角度（度）
 * @param maxDelta 最大角度变化（度）
 * @return 限制后的角度
 */
[[nodiscard]] inline f32 clampedRotate(f32 from, f32 to, f32 maxDelta) noexcept
{
    f32 diff = wrapDegrees(to - from);
    f32 clamped = std::clamp(diff, -maxDelta, maxDelta);
    return from + clamped;
}

/**
 * @brief MC 1.16.5 approachTargetAngle (MathHelper.func_219800_b)
 *
 * 将角度从当前值向目标值接近，限制最大变化量。
 * 与 clampedRotate 不同，此函数的结果更接近目标值。
 *
 * 参考 MC 1.16.5: f32 approachTargetAngle(f32 from, f32 to, f32 max)
 * 用于 LookController 在有导航路径时限制头部角度。
 *
 * 注意：MC 原版不包装结果到 [0, 360)，结果可能为负值或大于 360。
 *
 * @param sourceAngle 当前角度（度）
 * @param targetAngle 目标角度（度）
 * @param maximumChange 最大角度变化（度）
 * @return 调整后的角度（不包装到 [0, 360)）
 */
[[nodiscard]] inline f32 approachTargetAngle(f32 sourceAngle, f32 targetAngle, f32 maximumChange) noexcept
{
    f32 diff = wrapDegrees(targetAngle - sourceAngle);
    if (diff > maximumChange) {
        diff = maximumChange;
    } else if (diff < -maximumChange) {
        diff = -maximumChange;
    }
    // MC 1.16.5: 不包装结果，直接返回
    return targetAngle - diff;
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
 * @brief 地板除运算 (MC MathHelper.floorDiv)
 *
 * 与 C++ 的 / 运算符不同，结果总是向负无穷方向取整。
 * 例如: floorDiv(-1, 40) = -1 (而 -1 / 40 = 0 在 C++ 中向零取整)
 *
 * 参考 MC 1.16.5: MathHelper.floorDiv(i32, i32)
 *
 * @param value 被除数
 * @param divisor 除数（必须非零）
 * @return 地板除结果
 */
[[nodiscard]] inline constexpr i32 floorDiv(i32 value, i32 divisor) noexcept
{
    // MC 1.16.5: Math.floorDiv(i32, i32) 实现
    // 向负无穷方向取整
    i32 q = value / divisor;
    i32 r = value % divisor;
    return (r != 0 && ((r > 0) != (divisor > 0))) ? (q - 1) : q;
}

/**
 * @brief 地板除运算 (64位版本)
 *
 * @param value 被除数
 * @param divisor 除数（必须非零）
 * @return 地板除结果
 */
[[nodiscard]] inline constexpr i64 floorDiv(i64 value, i64 divisor) noexcept
{
    i64 q = value / divisor;
    i64 r = value % divisor;
    return (r != 0 && ((r > 0) != (divisor > 0))) ? (q - 1) : q;
}

/**
 * @brief 地板模运算 (MC MathHelper.floorMod)
 *
 * 与 C++ 的 % 运算符不同，结果总是与除数同号。
 * 例如: floorMod(-1, 40) = 39 (而 -1 % 40 = -1)
 *
 * 参考 MC 1.16.5: MathHelper.floorMod(i32, i32)
 *
 * @param value 被除数
 * @param divisor 除数（必须为正数）
 * @return 地板模结果 [0, divisor)
 */
[[nodiscard]] inline i64 floorMod(i64 value, i64 divisor) noexcept
{
    // MC 1.16.5: Math.floorMod(i32, i32) 实现
    // 结果与 divisor 同号
    const i64 result = value % divisor;
    return (result < 0) ? (result + divisor) : result;
}

/**
 * @brief 地板模运算 (32位版本)
 *
 * @param value 被除数
 * @param divisor 除数（必须为正数）
 * @return 地板模结果 [0, divisor)
 */
[[nodiscard]] inline i32 floorMod(i32 value, i32 divisor) noexcept
{
    const i32 result = value % divisor;
    return (result < 0) ? (result + divisor) : result;
}

/**
 * @brief 地板模运算 (浮点版本)
 *
 * 参考 MC 1.16.5: MathHelper.func_226168_f_(f64, f64)
 * 用于计算周期性动画时间
 *
 * @param value 被除数
 * @param divisor 除数（必须为正数）
 * @return 地板模结果 [0, divisor)
 */
[[nodiscard]] inline f32 floorMod(f32 value, f32 divisor) noexcept
{
    f32 result = std::fmod(value, divisor);
    return (result < 0.0f) ? (result + divisor) : result;
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
    while (diff < -PI) {
        diff += TWO_PI;
    }
    while (diff >= PI) {
        diff -= TWO_PI;
    }
    return current + factor * diff;
}

// ============================================================================
// 区块/方块位置哈希
// ============================================================================

/**
 * @brief 坐标 → 位置种子（`Mth.getSeed(i32,i32,i32)` 的逐位复刻）
 *
 * 原算法（1.8 起的历代版本一致）：
 * @code
 * long l = (long)(x * 3129871) ^ (long) z * 116129781L ^ (long) y;
 * l = l * l * 42317861L + l * 11L;
 * return l >> 16;
 * @endcode
 *
 * 【两处整型回绕必须逐位复刻，否则坐标稍大即与原版发散】
 * - `x * 3129871` 是 Java 的 **int32** 乘法：先在 32 位内回绕，再符号扩展为 long。
 *   按 64 位乘法计算时 |x| ≥ 687 就会发散，而方块世界坐标几乎必然越界。
 * - `l * l * 42317861L + l * 11L` 是 Java 的 **long** 回绕运算。
 * - 末尾 `>> 16` 是 Java 的算术右移（负数补符号位）。
 * C++ 的有符号溢出是 UB，故全程用无符号运算显式回绕。
 *
 * 该值被 `RandomSource.create(long)`（= LegacyRandomSource）消费，是结构放置中
 * 「按位置取随机」类逻辑（如调色板选择）的随机源。
 *
 * @return 位置种子（i64，与 Java long 同语义）
 */
[[nodiscard]] inline i64 mthGetSeed(i32 x, i32 y, i32 z) noexcept
{
    const i32 xTerm = static_cast<i32>(static_cast<u32>(x) * 3129871u);
    u64 i = static_cast<u64>(static_cast<i64>(xTerm)) ^ static_cast<u64>(static_cast<i64>(z)) * 116129781ULL ^
        static_cast<u64>(static_cast<i64>(y));
    i = i * i * 42317861ULL + i * 11ULL;
    return static_cast<i64>(i) >> 16;
}

/**
 * @brief 计算方块位置的确定性哈希值（用于结构完整度等随机数种子）
 *
 * 取值与 mthGetSeed 相同，仅返回类型为 u64 以便直接作随机源种子。
 *
 * @param x X坐标
 * @param y Y坐标
 * @param z Z坐标
 * @return 64位哈希值，可用作随机数种子
 */
[[nodiscard]] inline u64 hashBlockPos(i32 x, i32 y, i32 z) noexcept
{
    return static_cast<u64>(mthGetSeed(x, y, z));
}

/**
 * @brief 计算方块位置的确定性随机种子（MC 1.16.5 兼容）
 *
 * 参考 MC 1.16.5: MathHelper.getPositionRandom
 * 用于结构完整度、规则处理器等需要基于位置确定性随机的场景
 *
 * @param x X坐标
 * @param y Y坐标
 * @param z Z坐标
 * @return 可用于 Random 构造函数的种子值
 */
[[nodiscard]] inline u64 getPositionRandom(i32 x, i32 y, i32 z) noexcept
{
    return hashBlockPos(x, y, z);
}

/**
 * @brief 计算方块位置的确定性随机种子（仅 XZ 坐标）
 *
 * 参考 MC 1.16.5: 用于某些不需要 Y 坐标的场景
 *
 * @param x X坐标
 * @param z Z坐标
 * @return 可用于 Random 构造函数的种子值
 */
[[nodiscard]] inline u64 getPositionRandomXZ(i32 x, i32 z) noexcept
{
    // 与 mthGetSeed 相同的回绕语义（省去 y 项）
    const i32 xTerm = static_cast<i32>(static_cast<u32>(x) * 3129871u);
    u64 i = static_cast<u64>(static_cast<i64>(xTerm)) ^ static_cast<u64>(static_cast<i64>(z)) * 116129781ULL;
    i = i * i * 42317861ULL + i * 11ULL;
    return static_cast<u64>(static_cast<i64>(i) >> 16);
}

// ============================================================================
// 帧率无关的衰减/插值函数
// ============================================================================

/**
 * @brief 计算指数衰减的帧率无关纠正因子
 *
 * 使用指数衰减公式计算纠正因子，使纠正速度与帧率无关。
 * 无论帧率高低，每秒的纠正总量保持一致。
 *
 * 数学公式: correctionFactor = 1 - (1 - ratePerSecond)^deltaTime
 *
 * 例如，ratePerSecond = 0.5 表示每秒纠正约 50% 的差值：
 * - 60 FPS 时: 每帧纠正因子 ≈ 0.0115
 * - 30 FPS 时: 每帧纠正因子 ≈ 0.0228
 * - 无论帧率如何，1秒内总纠正量都约为 50%
 *
 * @param ratePerSecond 每秒的衰减/纠正速率 [0, 1]
 * @param deltaTime 帧间隔时间（秒）
 * @return 该帧的纠正因子 [0, 1]
 *
 * @note 当 deltaTime 为 0 时返回 0
 * @note 当 ratePerSecond 为 0 时返回 0
 * @note 当 ratePerSecond 为 1 时返回 1（立即纠正）
 */
[[nodiscard]] inline f32 exponentialDecayFactor(f32 ratePerSecond, f32 deltaTime) noexcept
{
    // 边界情况处理
    if (deltaTime <= 0.0f || ratePerSecond <= 0.0f) {
        return 0.0f;
    }
    if (ratePerSecond >= 1.0f) {
        return 1.0f;
    }
    // 公式: 1 - (1 - r)^dt
    // 确保结果在 [0, 1] 范围内（浮点精度保护）
    return std::clamp(1.0f - std::pow(1.0f - ratePerSecond, deltaTime), 0.0f, 1.0f);
}

/**
 * @brief murmur3 32 位混洗，对应原版 `Mth.murmurHash3Mixer`
 *
 * 用于把坐标/种子打散成伪随机值。移位必须用无符号右移，有符号右移会把符号位带下来，
 * 结果完全不同。
 */
[[nodiscard]] inline constexpr i32 murmurHash3Mixer(i32 value) noexcept
{
    u32 hash = static_cast<u32>(value);
    hash ^= hash >> 16U;
    hash *= 0x85EBCA6BU; // -2048144789 的无符号形式
    hash ^= hash >> 13U;
    hash *= 0xC2B2AE35U; // -1028477387 的无符号形式
    hash ^= hash >> 16U;
    return static_cast<i32>(hash);
}

/**
 * @brief 快速立方根倒数，对应原版 `Mth.fastInvCubeRoot`
 *
 * 先用整数魔数近似，再做一次牛顿迭代。整数部分用带符号除法（向零取整），与无符号
 * 移位的结果不同，负数输入要特别注意。
 */
[[nodiscard]] inline f32 fastInvCubeRoot(f32 value) noexcept
{
    i32 bits = static_cast<i32>(std::bit_cast<u32>(value));
    bits = 1419967116 - bits / 3;
    f32 approx = std::bit_cast<f32>(static_cast<u32>(bits));
    // 原版迭代两次：第一次把误差压到千分位，第二次才进入 f32 的精度量级。
    // 只做一次会让结果与 Java 差出若干 ulp 到千分之一。
    approx = 0.6666667f * approx + 1.0f / (3.0f * approx * approx * value);
    return 0.6666667f * approx + 1.0f / (3.0f * approx * approx * value);
}

/**
 * @brief Catmull-Rom 样条插值，对应原版 `Mth.catmullrom`
 *
 * 五个参数依次为插值因子、p0、p1、p2、p3；曲线穿过 p1 与 p2。
 */
[[nodiscard]] inline f32 catmullrom(f32 delta, f32 p0, f32 p1, f32 p2, f32 p3) noexcept
{
    return 0.5f *
        (2.0f * p1 + (p2 - p0) * delta + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * delta * delta +
            (3.0f * p1 - p0 - 3.0f * p2 + p3) * delta * delta * delta);
}

/**
 * @brief 向下量化到步长的整数倍，对应原版 `Mth.quantize`
 *
 * 先做浮点除法、再向下取整、再乘回步长。
 */
[[nodiscard]] inline i32 quantize(f64 value, i32 step) noexcept
{
    return static_cast<i32>(std::floor(value / static_cast<f64>(step))) * step;
}

/**
 * @brief 三态符号，对应原版 `Mth.sign`
 *
 * 零返回 0。`-0.0 == 0.0` 为真，因此负零同样返回 0；NaN 走 `> 0.0` 为假的分支返回 -1，
 * 与原版一致。
 */
[[nodiscard]] inline i32 sign(f64 value) noexcept
{
    if (value == 0.0) {
        return 0;
    }
    return value > 0.0 ? 1 : -1;
}

/**
 * @brief 棋盘距离，对应原版 `Mth.chessboardDistance`
 *
 * 两个方向差值的绝对值取大者。
 */
[[nodiscard]] inline i32 chessboardDistance(i32 x1, i32 z1, i32 x2, i32 z2) noexcept
{
    const i32 dx = x2 - x1;
    const i32 dz = z2 - z1;
    const i32 absX = dx < 0 ? -dx : dx;
    const i32 absZ = dz < 0 ? -dz : dz;
    return absX > absZ ? absX : absZ;
}

/**
 * @brief 是否为另一个数的整数倍，对应原版 `Mth.isMultipleOf`
 */
[[nodiscard]] inline bool isMultipleOf(i32 value, i32 divisor) noexcept
{
    return value % divisor == 0;
}

/**
 * @brief 容纳 count 个物品所需的正方形边长，对应原版 `Mth.smallestSquareSide`
 *
 * 对数量开方后向上取整；负数在原版抛异常，这里用断言等价表达。
 */
[[nodiscard]] inline i32 smallestSquareSide(i32 count) noexcept
{
    MC_ASSERT_RELEASE_MSG(count >= 0, "smallestSquareSide requires a non-negative count");
    return static_cast<i32>(std::ceil(std::sqrt(static_cast<f64>(count))));
}

/**
 * @brief 射线是否与轴对齐包围盒相交，对应原版 `Mth.rayIntersectsAABB`
 *
 * 先用三轴的「中心距超出半宽且朝外」做快速排除，再对三对轴做叉积判定。三次叉积的
 * 比较方向不同：前两次用 `>`，最后一次用 `<`——最后一次是在已经排除了前两次的前提下
 * 判断是否真正穿过，边界行为与前者不对称。
 *
 * @param ox/oy/oz 射线起点
 * @param dx/dy/dz 射线方向（不要求归一化）
 * @param minX/minY/minZ/maxX/maxY/maxZ 包围盒的边界（f64，与原版 AABB 一致）
 */
[[nodiscard]] inline bool rayIntersectsAABB(
    f64 ox, f64 oy, f64 oz, f64 dx, f64 dy, f64 dz, f64 minX, f64 minY, f64 minZ, f64 maxX, f64 maxY, f64 maxZ) noexcept
{
    // X 轴：起点到中心的距离超出半宽，且方向朝外 → 整条射线都在盒外
    const f64 centerX = (minX + maxX) * 0.5;
    const f64 halfX = (maxX - minX) * 0.5;
    const f64 deltaX = ox - centerX;
    if (std::abs(deltaX) > halfX && deltaX * dx >= 0.0) {
        return false;
    }

    const f64 centerY = (minY + maxY) * 0.5;
    const f64 halfY = (maxY - minY) * 0.5;
    const f64 deltaY = oy - centerY;
    if (std::abs(deltaY) > halfY && deltaY * dy >= 0.0) {
        return false;
    }

    const f64 centerZ = (minZ + maxZ) * 0.5;
    const f64 halfZ = (maxZ - minZ) * 0.5;
    const f64 deltaZ = oz - centerZ;
    if (std::abs(deltaZ) > halfZ && deltaZ * dz >= 0.0) {
        return false;
    }

    const f64 absDx = std::abs(dx);
    const f64 absDy = std::abs(dy);
    const f64 absDz = std::abs(dz);

    // 三次叉积：前两次用 >（超出即不相交），最后一次用 <（在界内才相交）。
    f64 cross = dy * deltaZ - dz * deltaY;
    if (std::abs(cross) > halfY * absDz + halfZ * absDy) {
        return false;
    }

    cross = dz * deltaX - dx * deltaZ;
    if (std::abs(cross) > halfX * absDz + halfZ * absDx) {
        return false;
    }

    cross = dx * deltaY - dy * deltaX;
    return std::abs(cross) < halfX * absDy + halfY * absDx;
}

/**
 * @brief HSV 转 ARGB 打包值，对应原版 `Mth.hsvToArgb`
 *
 * 色相按 60 度一档切成六段，每段走不同的分量排列。几个容易写错的地方：
 *
 * - 段号用 `(i32)(hue * 6.0f) % 6`，`(i32)` 是**向零截断**而非向下取整；
 *   色相为负时段号为负，会落到 switch 的 default 分支；
 * - 各分量乘 255 后同样先截断再夹到 `[0, 255]`；
 * - 结果是 `alpha << 24 | r << 16 | g << 8 | b`，alpha 不做夹取。
 */
[[nodiscard]] inline u32 hsvToArgb(f32 hue, f32 saturation, f32 value, i32 alpha) noexcept
{
    const i32 sector = static_cast<i32>(hue * 6.0f) % 6;
    const f32 fraction = hue * 6.0f - static_cast<f32>(sector);
    const f32 p = value * (1.0f - saturation);
    const f32 q = value * (1.0f - fraction * saturation);
    const f32 t = value * (1.0f - (1.0f - fraction) * saturation);

    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    switch (sector) {
        case 0:
            r = value;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = value;
            b = p;
            break;
        case 2:
            r = p;
            g = value;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = value;
            break;
        case 4:
            r = t;
            g = p;
            b = value;
            break;
        case 5:
            r = value;
            g = p;
            b = q;
            break;
        default:
            // 色相为负时段号为负，原版在这里抛异常；调用方应先归一化色相。
            MC_ASSERT_RELEASE_MSG(false, "hsvToArgb received an out-of-range hue");
            break;
    }

    const auto toByte = [](f32 channel) {
        return static_cast<u32>(clamp(static_cast<i32>(channel * 255.0f), 0, 255));
    };
    return (static_cast<u32>(alpha) << 24U) | (toByte(r) << 16U) | (toByte(g) << 8U) | toByte(b);
}

/**
 * @brief HSV 转 RGB 打包值（alpha 恒为 0），对应原版 `Mth.hsvToRgb`
 *
 * 注意 alpha 是 0 而不是 255——原版就是这么写的，调用方负责补 alpha。
 */
[[nodiscard]] inline u32 hsvToRgb(f32 hue, f32 saturation, f32 value) noexcept
{
    return hsvToArgb(hue, saturation, value, 0);
}

// ============================================================================
// 位运算与整数工具
// ============================================================================

/**
 * @brief 是否为 2 的整数次幂，对应原版 `Mth.isPowerOfTwo`
 *
 * 注意 0 不算——`0 & -1 == 0` 会让朴素的写法把 0 误判成 2 的幂，所以要先单独排除。
 */
[[nodiscard]] inline constexpr bool isPowerOfTwo(i32 value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}

/**
 * @brief 最小的、不小于 value 的 2 的整数次幂，对应原版 `Mth.smallestEncompassingPowerOfTwo`
 *
 * 用「把最高位以下全部填 1、再 +1」的做法。value 为 1 时返回 1；value 为 0 或负数时
 * 走满五轮填充得到 -1、再加 1 得 0——原版同样如此，调用方需自行保证入参为正。
 */
[[nodiscard]] inline constexpr i32 smallestEncompassingPowerOfTwo(i32 value) noexcept
{
    i32 result = value - 1;
    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    result |= result >> 8;
    result |= result >> 16;
    return result + 1;
}

/**
 * @brief `value` 向上取整到 `divisor` 的整数倍，对应原版 `Mth.roundToward`
 *
 * 经 `positiveCeilDiv` 实现，因此除数为负时行为与原版一致。
 */
[[nodiscard]] inline i32 roundToward(i32 value, i32 divisor) noexcept;

/**
 * @brief 向正无穷取整的除法，对应原版 `Mth.positiveCeilDiv`
 *
 * 原版写成 `-Math.floorDiv(-a, b)`：先取负、做向下取整除法、再取负。不能简单用
 * `(a + b - 1) / b`——那样在 a 为负、b 为正时会因为 C++ 的向零截断而算错。
 */
[[nodiscard]] inline i32 positiveCeilDiv(i32 a, i32 b) noexcept
{
    // floorDiv(-a, b) 用向零截断加修正表达
    const i32 negated = -a;
    i32 quotient = negated / b;
    if ((negated % b != 0) && ((negated < 0) != (b < 0))) {
        --quotient;
    }
    return -quotient;
}

inline i32 roundToward(i32 value, i32 divisor) noexcept
{
    return positiveCeilDiv(value, divisor) * divisor;
}

/**
 * @brief 向上取整的二进制对数，对应原版 `Mth.ceillog2`
 *
 * 先把 value 补齐到 2 的幂，再用 de Bruijn 序列查最高位的位置。表里 32 项按
 * `value * 125613361 >> 27` 的取值排布，索引再 `& 31`。
 *
 * value 为 0 时结果无意义（原版同样如此），调用方需保证入参为正。
 */
[[nodiscard]] inline i32 ceillog2(i32 value) noexcept
{
    static constexpr i32 DE_BRUIJN_BIT_POSITION[32] = {0,
        1,
        28,
        2,
        29,
        14,
        24,
        3,
        30,
        22,
        20,
        15,
        25,
        17,
        4,
        8,
        31,
        27,
        13,
        23,
        21,
        19,
        16,
        7,
        26,
        12,
        18,
        6,
        11,
        5,
        10,
        9};
    const i32 adjusted = isPowerOfTwo(value) ? value : smallestEncompassingPowerOfTwo(value);
    return DE_BRUIJN_BIT_POSITION[static_cast<i32>((static_cast<i64>(adjusted) * 125613361LL) >> 27) & 31];
}

/**
 * @brief 向下取整的二进制对数，对应原版 `Mth.log2`
 */
[[nodiscard]] inline i32 log2Of(i32 value) noexcept
{
    return ceillog2(value) - (isPowerOfTwo(value) ? 0 : 1);
}

// ============================================================================
// 角度逼近
// ============================================================================

/**
 * @brief 两个角度之间的最短有向差，对应原版 `Mth.degreesDifference`
 *
 * 结果落在 `[-180, 180)`。用 `wrapDegrees` 做规范化，因此跨越 0/360 边界时不会有
 * 「绕远路」的问题。
 */
[[nodiscard]] inline f32 degreesDifference(f32 first, f32 second) noexcept
{
    return wrapDegrees(second - first);
}

/**
 * @brief 让角度朝目标逼近一步，对应原版 `Mth.approach`
 *
 * 步长取绝对值。已经越过目标时 clamp 会把结果钉在目标上，不会来回振荡。
 */
[[nodiscard]] inline f32 approach(f32 from, f32 to, f32 step) noexcept
{
    const f32 magnitude = std::abs(step);
    if (from < to) {
        return clamp(from + magnitude, from, to);
    }
    return clamp(from - magnitude, to, from);
}

/**
 * @brief 按最短方向逼近目标角度，对应原版 `Mth.approachDegrees`
 *
 * 先求最短有向差，再沿该方向逼近——直接把角度差喂给 `approach` 会在跨越 0/360
 * 时朝反方向绕一大圈。
 */
[[nodiscard]] inline f32 approachDegrees(f32 from, f32 to, f32 step) noexcept
{
    const f32 difference = degreesDifference(from, to);
    return approach(from, from + difference, step);
}

/**
 * @brief 角度差超出阈值时向目标靠拢，对应原版 `Mth.rotateIfNecessary`
 *
 * 只在偏差超过 `maxDelta` 时修正，否则返回原目标值。
 */
[[nodiscard]] inline f32 rotateIfNecessary(f32 from, f32 to, f32 maxDelta) noexcept
{
    const f32 difference = degreesDifference(from, to);
    const f32 clamped = clamp(difference, -maxDelta, maxDelta);
    return to - clamped;
}

// ============================================================================
// 曲线工具
// ============================================================================

/**
 * @brief smoothstep 的导数，对应原版 `Mth.smoothstepDerivative`
 *
 * `30 * t² * (t-1)²`。注意原版用 `t - 1.0` 而非 `1.0 - t`：两者平方后相等，
 * 但 `t - 1.0` 在 t 略小于 1 时的舍入方向与 `1.0 - t` 不同。
 */
[[nodiscard]] inline constexpr f64 smoothstepDerivative(f64 t) noexcept
{
    return 30.0 * t * t * (t - 1.0) * (t - 1.0);
}

/**
 * @brief 三角波，对应原版 `Mth.triangleWave`
 *
 * 先把 `x` 对周期取余（Java 的 `%`，向零截断），再折成 `[-1, 1]` 的三角形状。
 *
 * 注意负输入的余数在 Java 中同样为负，`|r - p/2|` 因此会给出超出 `[-1, 1]` 的值——
 * 原版就是这样，没有做额外夹取，调用方需自行保证 `x >= 0`。
 */
[[nodiscard]] inline f32 triangleWave(f32 x, f32 period) noexcept
{
    const f32 quarter = period * 0.25f;
    return (std::abs(std::fmod(x, period) - period * 0.5f) - quarter) / quarter;
}

/**
 * @brief 五次平滑曲线，对应原版 `Mth.smoothstep(double)`
 *
 * 与上面三参数的 `smoothstep(edge0, edge1, x)`（Hermite 插值）**不是同一个东西**：
 * 这里假设输入已归一化到 `[0, 1]`，直接对值做五次整形
 * `t³(t(6t-15)+10)`。两者名字相同但签名不同，因此加 `Mth` 前缀区分。
 *
 * 用的系数是五次版本（6/15/10）而不是三次的（2/3），这是噪声插值里最关键的一条。
 */
[[nodiscard]] inline constexpr f64 mthSmoothstep(f64 t) noexcept
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

} // namespace mc::math
