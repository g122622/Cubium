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

#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"

#include <cmath>
#include <limits>

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
    // 从 maxHeight + 1 开始向下搜索，找到第一个"上方两格都是空气、脚下不是空气"的位置
    i32 blockX = static_cast<i32>(std::floor(x));
    i32 blockZ = static_cast<i32>(std::floor(z));

    // 从 maxHeight + 1 开始搜索
    i32 y = maxHeight + 1;
    const BlockState* state = world.getBlockState(blockX, y, blockZ);
    bool above = (state != nullptr) && !state->isAir();
    bool current = false;

    // 逐格向下搜索
    while (y > world::MIN_BUILD_HEIGHT) {
        --y;
        state = world.getBlockState(blockX, y, blockZ);
        current = (state != nullptr) && !state->isAir();

        // 找到脚下是固体、上方两格是空气的位置
        if (!current && above) {
            // 检查再上一格是否也是空气
            if (y + 2 <= maxHeight + 1) {
                const BlockState* aboveState = world.getBlockState(blockX, y + 2, blockZ);
                bool aboveTwo = (aboveState == nullptr) || aboveState->isAir();
                if (aboveTwo) {
                    return y + 1;
                }
            }
            // 如果无法检查上方两格，仍然返回当前位置
            return y + 1;
        }
        above = current;
    }

    // 如果找不到合适的位置，返回 maxHeight + 1
    return maxHeight + 1;
}

bool SpreadPosition::isSafe(IWorld& world, i32 maxHeight) const
{
    i32 blockX = static_cast<i32>(std::floor(x));
    i32 blockZ = static_cast<i32>(std::floor(z));
    i32 spawnY = getSpawnY(world, maxHeight);

    // 脚下方块
    const BlockState* belowState = world.getBlockState(blockX, spawnY - 1, blockZ);
    if (belowState == nullptr) {
        return false;
    }

    // 检查是否是液体
    if (belowState->isLiquid()) {
        return false;
    }

    // 检查是否是火焰方块
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
    std::vector<SpreadPosition>& positions)
{
    bool moved = true;
    f64 minDist = std::numeric_limits<f64>::max();

    i32 iteration = 0;
    for (; iteration < SPREAD_MAX_ITERATIONS && moved; ++iteration) {
        moved = false;
        minDist = std::numeric_limits<f64>::max();

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

    if (minDist == std::numeric_limits<f64>::max()) {
        minDist = 0.0;
    }

    // 如果超过最大迭代次数，分散失败
    if (iteration >= SPREAD_MAX_ITERATIONS) {
        return false;
    }

    return true;
}

} // namespace support
} // namespace command
} // namespace mc
