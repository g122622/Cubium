/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "StructurePlacement.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <functional>
#include <utility>

namespace mc::world::gen::structure::placement {

// ============================================================================
// StructurePlacement
// ============================================================================

BlockPos StructurePlacement::getLocatePos(const world::chunk::ChunkPos& chunkPos) const
{
    // 区块原点（左上角）加上 8（区块中心偏移）再加上 locateOffset
    return BlockPos(chunkPos.x * 16 + 8 + m_locateOffset.x, m_locateOffset.y, chunkPos.z * 16 + 8 + m_locateOffset.z);
}

bool StructurePlacement::applyAdditionalChunkRestrictions(i32 chunkX, i32 chunkZ, i64 worldSeed) const
{
    // 频率为 1.0 时不做任何缩减
    if (m_frequency >= 1.0f) {
        return true;
    }

    math::Random rng;

    switch (m_frequencyReduction) {
        case FrequencyReductionMethod::Default: {
            // 使用 setLargeFeatureWithSalt 种子后 nextFloat() < frequency
            rng.setLargeFeatureWithSalt(worldSeed, chunkX, chunkZ, m_salt);
            return rng.nextFloat() < m_frequency;
        }
        case FrequencyReductionMethod::LegacyType1: {
            // 掠夺者前哨站风格：基于方块坐标计算种子
            i64 blockX = static_cast<i64>(chunkX) * 16;
            i64 blockZ = static_cast<i64>(chunkZ) * 16;
            i64 seed = (blockX >> 4) ^ (blockZ << 4) ^ worldSeed;
            rng.setSeed(static_cast<u64>(seed));
            i32 bound = static_cast<i32>(1.0 / m_frequency);
            return bound > 0 && rng.nextInt(bound) == 0;
        }
        case FrequencyReductionMethod::LegacyType2: {
            // 埋藏宝藏风格：使用固定盐值 10387320
            rng.setLargeFeatureWithSalt(worldSeed, chunkX, chunkZ, 10387320);
            return rng.nextFloat() < m_frequency;
        }
        case FrequencyReductionMethod::LegacyType3: {
            // 废弃矿井风格：setLargeFeatureSeed + nextDouble
            rng.setLargeFeatureSeed(worldSeed, chunkX, chunkZ);
            return rng.nextDouble() < m_frequency;
        }
    }

    return true;
}

bool StructurePlacement::applyInteractionsWithOtherStructures(i32 chunkX, i32 chunkZ, i64 worldSeed) const
{
    // 没有排斥区配置时直接通过
    if (!m_exclusionZone.has_value()) {
        return true;
    }

    // 没有设置检查回调时直接通过
    if (!m_exclusionZoneChecker) {
        return true;
    }

    const auto& zone = m_exclusionZone.value();

    // 在搜索半径内检查另一个 StructureSet 是否有结构
    for (i32 dx = -zone.chunkCount; dx <= zone.chunkCount; ++dx) {
        for (i32 dz = -zone.chunkCount; dz <= zone.chunkCount; ++dz) {
            if (dx == 0 && dz == 0) {
                continue;
            }
            if (m_exclusionZoneChecker(zone.otherSetId, chunkX + dx, chunkZ + dz, worldSeed)) {
                return false;
            }
        }
    }

    return true;
}

void StructurePlacement::setExclusionZoneChecker(std::function<bool(const ResourceLocation&, i32, i32, i64)> callback)
{
    m_exclusionZoneChecker = std::move(callback);
}

} // namespace mc::world::gen::structure::placement
