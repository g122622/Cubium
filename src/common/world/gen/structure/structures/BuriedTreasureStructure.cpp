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

#include "BuriedTreasureStructure.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// BuriedTreasurePiece 实现
BuriedTreasurePiece::BuriedTreasurePiece(i32 x, i32 y, i32 z)
    : StructurePiece(StructurePieceTypes::BURIED_TREASURE, x, y, z, x + 2, y + 2, z + 2) // 3x3x3 区域
{}

bool BuriedTreasurePiece::_isInBounds(i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds) const
{
    return chunkBounds.contains(x, y, z);
}

void BuriedTreasurePiece::generate(IWorldWriter& world,
    math::Random& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* goldState = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
    const BlockState* sandState = VanillaBlocks::getState(VanillaBlocks::SAND);
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);

    // 放置宝藏箱子（中心位置）
    i32 centerX = minX() + 1;
    i32 centerY = minY() + 1;
    i32 centerZ = minZ() + 1;

    if (_isInBounds(centerX, centerY, centerZ, chunkBounds)) {
        // 放置金块作为宝藏占位符（箱子方块尚未实现）
        if (goldState) {
            world.setBlockState(centerX, centerY, centerZ, goldState);
        }
    }

    // 在周围放置沙子/石头作为保护
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) continue; // 跳过中心

            i32 x = centerX + dx;
            i32 y = centerY - 1;
            i32 z = centerZ + dz;

            if (_isInBounds(x, y, z, chunkBounds)) {
                world.setBlockState(x, y, z, stoneState ? stoneState : sandState);
            }
        }
    }
}

const std::string BuriedTreasureStructure::m_name = "buried_treasure";

const biome::BiomeTag* BuriedTreasureStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_BURIED_TREASURE();
}

bool BuriedTreasureStructure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // 使用单独的 salt=10387320 计算种子，然后检查概率
    // 埋藏宝藏的概率检查：nextFloat() < 0.01 (1% 概率)
    return rng.nextFloat() < 0.01f;
}

std::unique_ptr<StructureStart> BuriedTreasureStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 在区块中心附近找一个合适的位置
    // MC 原版 BuriedTreasureStructure.generatePieces() 使用区块中心偏移 9 格
    i32 baseX = (chunkX << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);
    i32 baseZ = (chunkZ << world::CHUNK_SHIFT) + rng.nextInt(world::CHUNK_WIDTH);

    // 使用 OceanFloorWG 高度图获取海底（最高固体方块）的 Y 坐标
    // MC 原版通过 onTopOfChunkCenter() 回调使用 OCEAN_FLOOR_WG 高度图确定生成高度
    i32 surfaceY = generator.getHeight(baseX, baseZ, HeightmapType::OceanFloorWG);

    // 宝藏应该埋在沙子下面 3-6 格
    // MC 原版在 BuriedTreasurePiece.postProcess() 中从海底向下搜索合适位置
    i32 treasureY = surfaceY - rng.nextInt(3, 6);
    if (treasureY < world::MIN_BUILD_HEIGHT) {
        treasureY = world::MIN_BUILD_HEIGHT;
    }

    // 创建并添加片段（方块写入延迟到 FEATURES 阶段由 placeInChunk() 执行）
    auto piece = std::make_unique<BuriedTreasurePiece>(baseX, treasureY, baseZ);
    start->addPiece(std::move(piece));
    start->recalculateStructureSize();

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
