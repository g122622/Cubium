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

#include "SpreadAlgorithm.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace mc {
namespace command {
namespace support {

// ============================================================================
// SpreadPosition 方法
// ============================================================================

f64 SpreadPosition::dist(const SpreadPosition& other) const
{
    f64 dx = x - other.x;
    f64 dz = z - other.z;
    return std::sqrt(dx * dx + dz * dz);
}

f64 SpreadPosition::getLength() const
{
    return std::sqrt(x * x + z * z);
}

void SpreadPosition::normalize()
{
    f64 len = getLength();
    if (len > 0.0) {
        x /= len;
        z /= len;
    }
}

void SpreadPosition::moveAway(const SpreadPosition& direction)
{
    x -= direction.x;
    z -= direction.z;
}

bool SpreadPosition::clamp(f64 minX, f64 minZ, f64 maxX, f64 maxZ)
{
    bool clamped = false;
    if (x < minX) {
        x = minX;
        clamped = true;
    } else if (x > maxX) {
        x = maxX;
        clamped = true;
    }
    if (z < minZ) {
        z = minZ;
        clamped = true;
    } else if (z > maxZ) {
        z = maxZ;
        clamped = true;
    }
    return clamped;
}

i32 SpreadPosition::getSpawnY(IWorld& world, i32 maxHeight) const
{
    i32 blockX = static_cast<i32>(std::floor(x));
    i32 blockZ = static_cast<i32>(std::floor(z));

    // 使用三变量滚动法
    // 从 maxHeight+1 开始向下搜索，找到"当前格非空气、上方两格都是空气"的站立位置
    // flag: 上方第二格是否为空气
    // flag1: 上方第一格是否为空气
    // flag2: 当前格是否为空气
    i32 y = maxHeight + 1;
    const BlockState* state = world.getBlockState(blockX, y, blockZ);
    bool flag = (state == nullptr) || state->isAir(); // maxHeight+1 处是否为空气

    --y;
    state = world.getBlockState(blockX, y, blockZ);
    bool flag1 = (state == nullptr) || state->isAir(); // maxHeight 处是否为空气

    while (y > world.getMinBuildHeight()) {
        --y;
        state = world.getBlockState(blockX, y, blockZ);
        bool flag2 = (state == nullptr) || state->isAir(); // 当前格是否为空气

        // 当前格非空气（固体）、上方两格都是空气 -> 找到站立位置
        if (!flag2 && flag1 && flag) {
            return y + 1;
        }

        flag = flag1;
        flag1 = flag2;
    }

    // 如果找不到合适的位置，返回 maxHeight + 1
    return maxHeight + 1;
}

bool SpreadPosition::isSafe(IWorld& world, i32 maxHeight) const
{
    i32 blockX = static_cast<i32>(std::floor(x));
    i32 blockZ = static_cast<i32>(std::floor(z));
    i32 spawnY = getSpawnY(world, maxHeight);

    // 脚下方块为空（世界边界外），不安全
    const BlockState* belowState = world.getBlockState(blockX, spawnY - 1, blockZ);
    if (belowState == nullptr) {
        return false;
    }

    // 检查是否是液体
    if (belowState->isLiquid()) {
        return false;
    }

    // 检查是否是火焰方块（包含 fire 和 soul_fire）
    if (BlockTags::FIRE().contains(*belowState)) {
        return false;
    }

    return spawnY < maxHeight;
}

void SpreadPosition::randomize(math::Random& rng, f64 minX, f64 minZ, f64 maxX, f64 maxZ)
{
    x = rng.nextDouble(minX, maxX);
    z = rng.nextDouble(minZ, maxZ);
}

// ============================================================================
// 分散算法函数
// ============================================================================

std::vector<SpreadPosition> createInitialPositions(math::Random& rng, i32 count, f64 minX, f64 minZ, f64 maxX, f64 maxZ)
{
    std::vector<SpreadPosition> positions(static_cast<size_t>(count));
    for (auto& pos : positions) {
        pos.randomize(rng, minX, minZ, maxX, maxZ);
    }
    return positions;
}

bool spreadPositions(f64 spreadDistance,
    IWorld& world,
    math::Random& rng,
    f64 minX,
    f64 minZ,
    f64 maxX,
    f64 maxZ,
    i32 maxHeight,
    std::vector<SpreadPosition>& positions,
    f64& outMinDist)
{
    // 使用 float 最大值作为哨兵值
    constexpr f64 sentinelDist = static_cast<f64>(std::numeric_limits<f32>::max());
    bool moved = true;
    f64 minDist = sentinelDist;

    i32 iteration = 0;
    for (; iteration < SPREAD_MAX_ITERATIONS && moved; ++iteration) {
        moved = false;
        minDist = sentinelDist;

        for (size_t j = 0; j < positions.size(); ++j) {
            i32 closeCount = 0;
            SpreadPosition delta;

            for (size_t l = 0; l < positions.size(); ++l) {
                if (j == l) {
                    continue;
                }

                f64 d = positions[j].dist(positions[l]);
                minDist = std::min(minDist, d);

                if (d < spreadDistance) {
                    ++closeCount;
                    delta.x += (positions[l].x - positions[j].x);
                    delta.z += (positions[l].z - positions[j].z);
                }
            }

            if (closeCount > 0) {
                delta.x /= static_cast<f64>(closeCount);
                delta.z /= static_cast<f64>(closeCount);

                f64 len = delta.getLength();
                if (len > 0.0) {
                    delta.normalize();
                    positions[j].moveAway(delta);
                } else {
                    positions[j].randomize(rng, minX, minZ, maxX, maxZ);
                }

                moved = true;
            }

            if (positions[j].clamp(minX, minZ, maxX, maxZ)) {
                moved = true;
            }
        }

        // 如果所有位置都已满足距离要求，检查安全性
        if (!moved) {
            for (auto& pos : positions) {
                if (!pos.isSafe(world, maxHeight)) {
                    pos.randomize(rng, minX, minZ, maxX, maxZ);
                    moved = true;
                }
            }
        }
    }

    // 只有一个分散位置时，minDist 保持哨兵值，设为 0
    if (minDist == sentinelDist) {
        minDist = 0.0;
    }

    outMinDist = minDist;

    // 如果超过最大迭代次数，分散失败
    return iteration < SPREAD_MAX_ITERATIONS;
}

} // namespace support
} // namespace command
} // namespace mc
