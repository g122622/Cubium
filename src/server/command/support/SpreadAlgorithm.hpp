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
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"

#include <vector>

namespace mc {
namespace command {
namespace support {

// ============================================================================
// SpreadPosition - 2D 位置辅助结构
// ============================================================================

/// 表示一个 2D (x, z) 位置，用于 /spreadplayers 迭代分散算法中的位置计算。
struct SpreadPosition {
    f64 x = 0.0;
    f64 z = 0.0;

    /// 计算与另一个位置的距离
    [[nodiscard]] f64 dist(const SpreadPosition& other) const;

    /// 获取向量长度
    [[nodiscard]] f64 getLength() const;

    /// 归一化向量
    void normalize();

    /// 沿指定方向的反方向移动（远离）
    void moveAway(const SpreadPosition& direction);

    /// 将位置钳制到指定矩形范围内，返回是否发生了钳制
    bool clamp(f64 minX, f64 minZ, f64 maxX, f64 maxZ);

    /// 计算此位置的生成 Y 坐标（从上往下搜索第一个安全的站立位置）
    /// 使用三变量滚动法，对齐 MC Java 版 SpreadPlayersCommand.Position.getSpawnY
    [[nodiscard]] i32 getSpawnY(IWorld& world, i32 maxHeight) const;

    /// 检查此位置是否安全（脚下不是液体、不是火焰）
    [[nodiscard]] bool isSafe(IWorld& world, i32 maxHeight) const;

    /// 在指定范围内随机初始化位置
    void randomize(math::Random& rng, f64 minX, f64 minZ, f64 maxX, f64 maxZ);
};

// ============================================================================
// 分散算法常量
// ============================================================================

/// 迭代分散算法最大迭代次数
constexpr i32 SPREAD_MAX_ITERATIONS = 10000;

// ============================================================================
// 分散算法函数
// ============================================================================

/// 创建初始随机位置
std::vector<SpreadPosition> createInitialPositions(
    math::Random& rng, i32 count, f64 minX, f64 minZ, f64 maxX, f64 maxZ);

/// 迭代分散算法：将位置推开到满足最小距离要求
/// TODO: 当前实现与 MC Java 版在以下方面存在差异：
///   - minDist 初始值使用了 double max 而非 MC 的 Float.MAX_VALUE 作为哨兵值
///   - 分散失败时的错误消息应包含中心坐标和实际达到的最小距离
bool spreadPositions(f64 spreadDistance,
    IWorld& world,
    math::Random& rng,
    f64 minX,
    f64 minZ,
    f64 maxX,
    f64 maxZ,
    i32 maxHeight,
    std::vector<SpreadPosition>& positions);

} // namespace support
} // namespace command
} // namespace mc
