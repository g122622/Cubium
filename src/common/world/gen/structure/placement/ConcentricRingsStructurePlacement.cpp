/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or/or sell
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

#include "ConcentricRingsStructurePlacement.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/structure/placement/StructurePlacement.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace mc::world::gen::structure::placement {

// ============================================================================
// ConcentricRingsStructurePlacement
// ============================================================================

ConcentricRingsStructurePlacement::ConcentricRingsStructurePlacement(
    i32 distance, i32 spread, i32 count, std::vector<BiomeId> preferredBiomes, i64 salt, math::Vector3i locateOffset)
    : m_distance(distance)
    , m_spread(spread)
    , m_count(count)
    , m_preferredBiomes(std::move(preferredBiomes))
{
    m_locateOffset = locateOffset;
    m_frequencyReduction = FrequencyReductionMethod::Default;
    m_frequency = 1.0f;
    m_salt = salt;
    // 同心环放置不需要排斥区
    m_exclusionZone = std::nullopt;
}

std::vector<world::chunk::ChunkPos> ConcentricRingsStructurePlacement::generateRingPositions(i64 worldSeed) const
{
    std::vector<world::chunk::ChunkPos> positions;
    positions.reserve(static_cast<size_t>(m_count));

    math::Random rng;
    rng.setSeed(static_cast<u64>(worldSeed));

    // 同心环参数
    // MC 1.21+ 的环分布：
    // 每个环的基数: 3, 6, 10, 15, 21, 28, 36, 9 (共 128)
    // 距离乘数: 环索引从 0 开始
    i32 remainingCount = m_count;
    i32 currentRing = 0;
    f64 distanceMultiplier = 1.0; // 距离乘数，随环数递增

    while (remainingCount > 0) {
        // 当前环的要塞数量
        // MC 1.21 的环数量规律：第 n 环的数量 = (n+2)(n+3)/2 - (n+1)(n+2)/2 = n+2
        // 但实际 MC 使用固定分配
        i32 ringCount;
        if (currentRing == 0) {
            ringCount = 3;
        } else if (currentRing == 1) {
            ringCount = 6;
        } else if (currentRing == 2) {
            ringCount = 10;
        } else if (currentRing == 3) {
            ringCount = 15;
        } else if (currentRing == 4) {
            ringCount = 21;
        } else if (currentRing == 5) {
            ringCount = 28;
        } else if (currentRing == 6) {
            ringCount = 36;
        } else {
            // 第 7+ 环把剩余的全部分配
            ringCount = remainingCount;
        }

        if (ringCount > remainingCount) {
            ringCount = remainingCount;
        }

        // 起始角度随机
        f64 angle = rng.nextDouble() * 2.0 * math::PI_DOUBLE;

        for (i32 i = 0; i < ringCount; ++i) {
            // 计算环的半径
            // radius = baseDistance + baseDistance * distanceMultiplier * 6 + (random - 0.5) * baseDistance * 2.5
            f64 radius = static_cast<f64>(m_distance) + static_cast<f64>(m_distance) * distanceMultiplier * 6.0 +
                (rng.nextDouble() - 0.5) * static_cast<f64>(m_distance) * 2.5;

            // 确保半径不为负
            if (radius < 1.0) {
                radius = 1.0;
            }

            // 计算区块坐标
            f64 chunkX = std::round(std::cos(angle) * radius);
            f64 chunkZ = std::round(std::sin(angle) * radius);

            positions.emplace_back(static_cast<i32>(chunkX), static_cast<i32>(chunkZ));

            // 每个要塞之间均匀分布角度
            angle += 2.0 * math::PI_DOUBLE / static_cast<f64>(ringCount);
        }

        remainingCount -= ringCount;
        ++currentRing;
        distanceMultiplier += 1.0;
    }

    return positions;
}

const std::vector<world::chunk::ChunkPos>& ConcentricRingsStructurePlacement::getRingPositions(i64 worldSeed) const
{
    // 检查缓存是否有效
    if (m_cachedPositions.has_value() && m_cachedSeed == worldSeed) {
        return m_cachedPositions.value();
    }

    // 加锁生成
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 双重检查（可能另一个线程已经生成了）
    if (m_cachedPositions.has_value() && m_cachedSeed == worldSeed) {
        return m_cachedPositions.value();
    }

    m_cachedPositions = generateRingPositions(worldSeed);
    m_cachedSeed = worldSeed;

    return m_cachedPositions.value();
}

bool ConcentricRingsStructurePlacement::isStructureChunk(i64 worldSeed, i32 chunkX, i32 chunkZ) const
{
    // 同心环放置不需要频率缩减和排斥区检查
    // 直接在环形位置列表中查找
    const auto& positions = getRingPositions(worldSeed);

    for (const auto& pos : positions) {
        if (pos.x == chunkX && pos.z == chunkZ) {
            return true;
        }
    }

    return false;
}

std::unique_ptr<StructurePlacement> ConcentricRingsStructurePlacement::clone() const
{
    return std::make_unique<ConcentricRingsStructurePlacement>(
        m_distance, m_spread, m_count, m_preferredBiomes, m_salt, m_locateOffset);
}

} // namespace mc::world::gen::structure::placement
