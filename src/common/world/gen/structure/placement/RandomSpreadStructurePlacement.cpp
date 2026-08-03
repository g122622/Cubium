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
 * copies of substantial portions of the Software.
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

#include "RandomSpreadStructurePlacement.hpp"

#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/gen/structure/placement/StructurePlacement.hpp"
#include <memory>
#include <optional>
#include <utility>

namespace mc::world::gen::structure::placement {

// ============================================================================
// RandomSpreadStructurePlacement
// ============================================================================

RandomSpreadStructurePlacement::RandomSpreadStructurePlacement(i32 spacing,
    i32 separation,
    i64 salt,
    RandomSpreadType spreadType,
    FrequencyReductionMethod frequencyReduction,
    f32 frequency,
    math::Vector3i locateOffset,
    std::optional<ExclusionZone> exclusionZone)
    : m_spacing(spacing)
    , m_separation(separation)
    , m_spreadType(spreadType)
{
    MC_ASSERT_RELEASE(spacing > separation);

    m_locateOffset = locateOffset;
    m_frequencyReduction = frequencyReduction;
    m_frequency = frequency;
    m_salt = salt;
    m_exclusionZone = std::move(exclusionZone);
}

world::chunk::ChunkPos RandomSpreadStructurePlacement::getPotentialStructureChunk(
    i64 worldSeed, i32 chunkX, i32 chunkZ) const
{
    // 计算网格坐标（向负无穷方向取整）
    i32 gridX = math::floorDiv(chunkX, m_spacing);
    i32 gridZ = math::floorDiv(chunkZ, m_spacing);

    // 使用网格坐标和种子初始化随机数生成器
    math::Random rng;
    rng.setLargeFeatureWithSalt(worldSeed, gridX, gridZ, m_salt);

    // 偏移范围
    i32 range = m_spacing - m_separation;

    i32 offsetX;
    i32 offsetZ;

    if (m_spreadType == RandomSpreadType::Linear) {
        // 均匀随机分布
        offsetX = rng.nextInt(range);
        offsetZ = rng.nextInt(range);
    } else {
        // 三角分布：两次随机取平均，产生更集中的分布
        offsetX = (rng.nextInt(range) + rng.nextInt(range)) / 2;
        offsetZ = (rng.nextInt(range) + rng.nextInt(range)) / 2;
    }

    return world::chunk::ChunkPos(gridX * m_spacing + offsetX, gridZ * m_spacing + offsetZ);
}

bool RandomSpreadStructurePlacement::isStructureChunk(i64 worldSeed, i32 chunkX, i32 chunkZ) const
{
    // 步骤1: 计算候选区块
    auto candidate = getPotentialStructureChunk(worldSeed, chunkX, chunkZ);

    // 步骤2: 候选区块必须等于当前区块
    if (candidate.x != chunkX || candidate.z != chunkZ) {
        return false;
    }

    // 步骤3: 频率缩减检查
    if (!applyAdditionalChunkRestrictions(chunkX, chunkZ, worldSeed)) {
        return false;
    }

    // 步骤4: 排斥区检查
    if (!applyInteractionsWithOtherStructures(chunkX, chunkZ, worldSeed)) {
        return false;
    }

    return true;
}

std::unique_ptr<StructurePlacement> RandomSpreadStructurePlacement::clone() const
{
    return std::make_unique<RandomSpreadStructurePlacement>(m_spacing,
        m_separation,
        m_salt,
        m_spreadType,
        m_frequencyReduction,
        m_frequency,
        m_locateOffset,
        m_exclusionZone);
}

} // namespace mc::world::gen::structure::placement
