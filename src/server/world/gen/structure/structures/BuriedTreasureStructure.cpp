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
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// BuriedTreasurePiece 实现
BuriedTreasurePiece::BuriedTreasurePiece(i32 x, i32 y, i32 z)
    // 原版 `new BoundingBox(p_227366_)` 是**单点**盒子；生成阶段向下搜索到合适地层后
    // 才把包围盒改成落点那一格。这里若铺成 3x3x3，起点包围盒会凭空大一圈，
    // 整体尺寸、碰撞判定与差分输出都会偏离。
    : StructurePiece(StructurePieceTypes::BURIED_TREASURE, x, y, z, x, y, z)
{}

bool BuriedTreasurePiece::_isInBounds(i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds) const
{
    return chunkBounds.contains(x, y, z);
}

void BuriedTreasurePiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* goldState = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
    const BlockState* sandState = VanillaBlocks::getState(VanillaBlocks::SAND);
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);

    // 包围盒是单点盒，宝藏就落在这一格上（原版落点同样直接取包围盒的 min 角）。
    i32 centerX = minX();
    i32 centerY = minY();
    i32 centerZ = minZ();

    if (_isInBounds(centerX, centerY, centerZ, chunkBounds)) {
        // 放置金块作为宝藏占位符（箱子方块尚未实现）
        if (goldState) {
            world.setBlockState(centerX, centerY, centerZ, goldState);
        }
    }

    // 在周围放置沙子/石头作为保护
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) {
                continue; // 跳过中心
            }

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

std::unique_ptr<StructureStart> BuriedTreasureStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(rng);

    auto start = makeStructureStart(chunkX, chunkZ);

    // 生物群系校验落在候选生成点（区块中心、海底高度）上；不匹配即返回空起点。
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, HeightmapType::OceanFloorWG))) {
        return start;
    }

    // 生成点固定为「区块中心列 + (9, 90, 9)」，Y 的 90 只是占位值：片段的包围盒是一个
    // 单点，真正的落点在 FEATURES 阶段沿该列向下搜索时确定。这里不能抽样任何随机数以
    // 挑选 X/Z 或深度——原版 generatePieces 全程不消耗随机数。
    const BlockPos position(chunkX * world::CHUNK_WIDTH + 9, 90, chunkZ * world::CHUNK_WIDTH + 9);

    start->addPiece(std::make_unique<BuriedTreasurePiece>(position.x, position.y, position.z));
    start->recalculateStructureSize();

    MC_UNUSED(generator);
    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
