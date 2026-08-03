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

#include "ReplaceBlobsFeature.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <utility>

namespace mc {
namespace world::gen::feature {

ConfiguredReplaceBlobsFeature::ConfiguredReplaceBlobsFeature(
    std::unique_ptr<ReplaceSphereConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

namespace {

/// MC ReplaceBlobsFeature.findTarget：从 startY 向下逐格查找第一个方块身份等于 targetBlock 的位置，
/// 循环条件 Y > minY + 1（与 MC 一致），找不到返回无效。
bool findTargetBlock(
    WorldGenRegion& region, const Block* targetBlock, BlockPos& outPos, i32 startX, i32 startY, i32 startZ)
{
    const i32 minY = region.getMinBuildHeight();
    for (i32 y = startY; y > minY + 1; --y) {
        const BlockState* state = region.getBlockState(startX, y, startZ);
        if (state != nullptr && targetBlock != nullptr && state->is(targetBlock)) {
            outPos = BlockPos(startX, y, startZ);
            return true;
        }
    }
    return false;
}

} // namespace

bool ConfiguredReplaceBlobsFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    if (!m_config || m_config->targetState == nullptr || m_config->replaceState == nullptr ||
        m_config->radius == nullptr) {
        return false;
    }

    const Block* targetBlock = &m_config->targetState->getBlock();
    // MC: origin Y 钳制到 [minY+1, maxY]。
    const i32 minY = region.getMinBuildHeight();
    const i32 maxY = region.getMaxBuildHeight();
    const i32 startY = std::clamp(origin.y, minY + 1, maxY);

    BlockPos center(0, 0, 0);
    if (!findTargetBlock(region, targetBlock, center, origin.x, startY, origin.z)) {
        return false;
    }

    // i/j/k 各采样一次半径，l = max(i,j,k)。
    const i32 i = m_config->radius->sample(random);
    const i32 j = m_config->radius->sample(random);
    const i32 k = m_config->radius->sample(random);
    const i32 l = std::max({i, j, k});

    bool any = false;
    // withinManhattan(center, i, j, k)：|dx|<=i && |dy|<=j && |dz|<=k，distManhattan<=l 才放置。
    for (i32 dx = -i; dx <= i; ++dx) {
        for (i32 dy = -j; dy <= j; ++dy) {
            for (i32 dz = -k; dz <= k; ++dz) {
                const i32 dist = std::abs(dx) + std::abs(dy) + std::abs(dz);
                if (dist > l) {
                    continue;
                }
                const BlockPos pos(center.x + dx, center.y + dy, center.z + dz);
                const BlockState* state = region.getBlockState(pos);
                if (state != nullptr && state->is(targetBlock)) {
                    region.setBlockState(pos, m_config->replaceState, 3);
                    any = true;
                }
            }
        }
    }

    return any;
}

} // namespace world::gen::feature
} // namespace mc
