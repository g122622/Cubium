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
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ChorusPlantFeature.hpp"

#include "common/world/block/blocks/end/ChorusFlowerBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// ChorusPlantFeature
// ============================================================================

bool ChorusPlantFeature::place(WorldGenRegion& world, math::Random& random, const BlockPos& pos)
{
    // 检查起始位置是否为空气且下方为末地石
    const BlockState* stateAtPos = world.getBlockState(pos);
    if (stateAtPos == nullptr || !stateAtPos->isAir()) {
        return false;
    }

    const BlockState* stateBelow = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (stateBelow == nullptr || !stateBelow->is(VanillaBlocks::END_STONE)) {
        return false;
    }

    // 生成紫颂树，最大水平扩展距离为8
    blocks::ChorusFlowerBlock::generatePlant(world, pos, random, 8);
    return true;
}

// ============================================================================
// ConfiguredChorusPlantFeature
// ============================================================================

ConfiguredChorusPlantFeature::ConfiguredChorusPlantFeature(const char* featureName)
    : m_name(featureName)
{}

bool ConfiguredChorusPlantFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return ChorusPlantFeature::place(region, random, pos);
}

} // namespace mc
