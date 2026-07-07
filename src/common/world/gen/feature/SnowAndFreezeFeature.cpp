/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software to
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

#include "SnowAndFreezeFeature.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/blocks/ice/SnowBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// ConfiguredSnowAndFreezeFeature
// ============================================================================

ConfiguredSnowAndFreezeFeature::ConfiguredSnowAndFreezeFeature(const char* featureName)
    : m_name(featureName)
{}

bool ConfiguredSnowAndFreezeFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)generator;
    (void)random;

    const i32 chunkX = pos.x;
    const i32 chunkZ = pos.z;
    const i32 seaLevel = world::SEA_LEVEL;

    // 遍历区块内所有 16x16 列
    for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
        for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
            i32 worldX = chunkX + localX;
            i32 worldZ = chunkZ + localZ;

            // 获取 MOTION_BLOCKING 高度图最高方块 Y
            i32 topY = region.getTopBlockY(worldX, worldZ, HeightmapType::MotionBlocking);

            // MOTION_BLOCKING 返回的是最高方块 Y+1（空气层），
            // 但 getTopBlockY 已经减 1 了，所以 topY 是实际方块 Y
            if (topY < world::MIN_BUILD_HEIGHT) {
                continue;
            }

            // 结冰位置：topY（水面）
            // 降雪位置：topY + 1（水面上方的空气）
            i32 freezeY = topY;
            i32 snowY = topY + 1;

            // 获取生物群系
            BiomeId biomeId = region.getBiome(worldX, topY, worldZ);
            const world::biome::Biome& biome = world::biome::BiomeRegistry::instance().get(biomeId);

            // === 结冰检查 ===
            // checkNeighbors=false：生成阶段所有暴露水面都冻结（与 LakeFeature 一致）
            if (biome.shouldFreeze(region, worldX, freezeY, worldZ, seaLevel, false)) {
                const BlockState* iceState = VanillaBlocks::getState(VanillaBlocks::ICE);
                if (iceState) {
                    region.setBlockState(worldX, freezeY, worldZ, iceState, 2);
                }
            }

            // === 降雪检查 ===
            if (biome.shouldSnow(region, worldX, snowY, worldZ, seaLevel)) {
                // 放置雪层
                const BlockState* snowState = &VanillaBlocks::SNOW->defaultState();
                if (snowState) {
                    region.setBlockState(worldX, snowY, worldZ, snowState, 2);

                    // 更新下方方块的 SNOWY 属性（草方块、菌丝等）
                    if (freezeY >= world::MIN_BUILD_HEIGHT) {
                        const BlockState* belowBlock = region.getBlockState(worldX, freezeY, worldZ);
                        if (belowBlock && belowBlock->hasProperty(BlockStateProperties::SNOWY())) {
                            const BlockState* snowyState = &belowBlock->with(BlockStateProperties::SNOWY(), true);
                            if (snowyState) {
                                region.setBlockState(worldX, freezeY, worldZ, snowyState, 2);
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

} // namespace mc
