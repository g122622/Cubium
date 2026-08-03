/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/world/gen/FeaturePlacer.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world::gen {

std::unique_ptr<WorldGenRegion> FeaturePlacer::createRegion(ChunkCoord centerChunkX,
    ChunkCoord centerChunkZ,
    std::vector<IChunk*> chunks,
    i32 chunkRadius,
    DimensionId dimensionId)
{
    // 使用无步骤验证的构造函数，按需放置时不限制写入窗口
    // （m_generatingStep 为 nullptr，setBlockState 的访问窗口检查被跳过）
    auto region =
        std::make_unique<WorldGenRegion>(centerChunkX, centerChunkZ, chunkRadius, std::move(chunks), dimensionId);

    return region;
}

void FeaturePlacer::populateWorldState(
    WorldGenRegion& region, u64 seed, u64 currentTick, i64 dayTime, bool hardcore, Difficulty difficulty)
{
    region.setSeed(seed);
    region.setCurrentTick(currentTick);
    region.setDayTime(dayTime);
    region.setHardcore(hardcore);
    region.setDifficulty(difficulty);
}

} // namespace world::gen
} // namespace mc
