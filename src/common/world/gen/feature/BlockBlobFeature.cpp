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

#include "BlockBlobFeature.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc::world::gen::feature {

ConfiguredBlockBlobFeature::ConfiguredBlockBlobFeature(
    std::unique_ptr<BlockStateConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBlockBlobFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& random,
    const BlockPos& origin) const
{
    if (!m_config || m_config->state == nullptr) {
        return false;
    }

    // 从 origin 向下找第一个"下方非空且为泥土或石头"的格子作为放置点
    BlockPos blockpos = origin;
    while (blockpos.y > region.getMinBuildHeight() + 3) {
        const BlockPos below = blockpos.down();
        const BlockState* belowState = region.getBlockState(below);
        const bool empty = (belowState == nullptr) || belowState->isAir();
        if (!empty) {
            const bool dirtOrStone = (belowState != nullptr) &&
                (BlockTags::DIRT().contains(*belowState) || BlockTags::STONE().contains(*belowState));
            if (dirtOrStone) {
                break;
            }
        }
        blockpos = blockpos.down();
    }

    if (blockpos.y <= region.getMinBuildHeight() + 3) {
        return false;
    }

    // 3 个小岩球
    for (i32 l = 0; l < 3; ++l) {
        const i32 i = random.nextInt(2);
        const i32 j = random.nextInt(2);
        const i32 k = random.nextInt(2);
        const f32 f = static_cast<f32>(i + j + k) * 0.333f + 0.5f;
        const f32 fSq = f * f;

        // betweenClosed(offset(-i,-j,-k), offset(i,j,k))：遍历 AABB，distSqr <= f*f 放方块
        const BlockPos minCorner = BlockPos(blockpos.x - i, blockpos.y - j, blockpos.z - k);
        const BlockPos maxCorner = BlockPos(blockpos.x + i, blockpos.y + j, blockpos.z + k);
        for (i32 px = minCorner.x; px <= maxCorner.x; ++px) {
            for (i32 py = minCorner.y; py <= maxCorner.y; ++py) {
                for (i32 pz = minCorner.z; pz <= maxCorner.z; ++pz) {
                    const i32 dx = px - blockpos.x;
                    const i32 dy = py - blockpos.y;
                    const i32 dz = pz - blockpos.z;
                    const f32 distSqr = static_cast<f32>(dx * dx + dy * dy + dz * dz);
                    if (distSqr <= fSq) {
                        region.setBlockState(BlockPos(px, py, pz), m_config->state);
                    }
                }
            }
        }

        // 中心随机偏移：offset(-1+nextInt(2), -nextInt(2), -1+nextInt(2))
        blockpos = BlockPos(blockpos.x + (-1 + random.nextInt(2)),
            blockpos.y - random.nextInt(2),
            blockpos.z + (-1 + random.nextInt(2)));
    }

    return true;
}

} // namespace mc::world::gen::feature
