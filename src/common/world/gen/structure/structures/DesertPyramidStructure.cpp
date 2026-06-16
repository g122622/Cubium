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

#include "DesertPyramidStructure.hpp"

#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

// ============================================================================
// DesertPyramidPiece
// ============================================================================

DesertPyramidPiece::DesertPyramidPiece(const BlockPos& pos)
    : StructurePiece(StructurePieceTypes::DESERT_PYRAMID, pos.x, pos.y - 9, pos.z, pos.x + 20, pos.y + 7, pos.z + 20)
    , m_startPos(pos)
{}

void DesertPyramidPiece::generate(
    IWorldWriter& world, math::Random& rng, i32 /*chunkX*/, i32 /*chunkZ*/, const StructureBoundingBox& chunkBounds)
{
    if (!getBoundingBox().intersects(chunkBounds)) {
        return;
    }
    _generatePyramid(world, rng, chunkBounds);
}

void DesertPyramidPiece::_generatePyramid(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds)
{
    const BlockState* sandstone = VanillaBlocks::getState(VanillaBlocks::SANDSTONE);
    const BlockState* cutSandstone = VanillaBlocks::getState(VanillaBlocks::CUT_SANDSTONE);
    const BlockState* chiseledSandstone = VanillaBlocks::getState(VanillaBlocks::CHISELED_SANDSTONE);
    const BlockState* tnt = VanillaBlocks::getState(VanillaBlocks::TNT);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    i32 baseX = m_startPos.x;
    i32 baseY = m_startPos.y;
    i32 baseZ = m_startPos.z;
    i32 size = 21;
    i32 halfSize = size / 2;

    // 生成基础平台
    for (i32 x = 0; x < size; ++x) {
        for (i32 z = 0; z < size; ++z) {
            i32 wx = baseX + x;
            i32 wy = baseY;
            i32 wz = baseZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, sandstone, 18);
            }
        }
    }

    // 生成四层递减的平台
    for (i32 layer = 1; layer <= 4; ++layer) {
        i32 layerSize = size - layer * 2;
        i32 offset = layer;

        for (i32 x = 0; x < layerSize; ++x) {
            for (i32 z = 0; z < layerSize; ++z) {
                if (x == 0 || x == layerSize - 1 || z == 0 || z == layerSize - 1) {
                    i32 wx = baseX + offset + x;
                    i32 wy = baseY + layer;
                    i32 wz = baseZ + offset + z;
                    if (bounds.contains(wx, wy, wz)) {
                        world.setBlockState(wx, wy, wz, sandstone, 18);
                    }
                }
            }
        }
    }

    // 生成四个角塔
    for (i32 tower = 0; tower < 4; ++tower) {
        i32 tx = (tower % 2 == 0) ? 0 : size - 3;
        i32 tz = (tower < 2) ? 0 : size - 3;

        for (i32 y = 1; y <= 6; ++y) {
            for (i32 x = 0; x < 3; ++x) {
                for (i32 z = 0; z < 3; ++z) {
                    if (x == 1 && z == 1 && y > 1 && y < 6) {
                        continue;
                    }
                    i32 wx = baseX + tx + x;
                    i32 wy = baseY + y;
                    i32 wz = baseZ + tz + z;
                    if (bounds.contains(wx, wy, wz)) {
                        world.setBlockState(wx, wy, wz, sandstone, 18);
                    }
                }
            }
        }

        // 塔顶装饰
        {
            i32 wx = baseX + tx + 1;
            i32 wy = baseY + 7;
            i32 wz = baseZ + tz + 1;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, chiseledSandstone, 18);
            }
        }
    }

    // 生成入口（南面中央）
    i32 entranceZ = size - 1;
    i32 entranceX = halfSize;

    for (i32 step = 0; step < 3; ++step) {
        for (i32 x = entranceX - 1; x <= entranceX + 1; ++x) {
            i32 wx = baseX + x;
            i32 wy = baseY + step;
            i32 wz = baseZ + entranceZ + step + 1;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, sandstone, 18);
            }
        }
    }

    // 生成地下宝藏室
    i32 chamberY = baseY - 7;
    i32 chamberX = baseX + halfSize - 3;
    i32 chamberZ = baseZ + halfSize - 3;

    // 宝藏室地板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 7; ++z) {
            i32 wx = chamberX + x;
            i32 wy = chamberY;
            i32 wz = chamberZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, sandstone, 18);
            }
        }
    }

    // 宝藏室墙壁
    for (i32 y = 1; y <= 4; ++y) {
        for (i32 x = 0; x < 7; ++x) {
            if (bounds.contains(chamberX + x, chamberY + y, chamberZ)) {
                world.setBlockState(chamberX + x, chamberY + y, chamberZ, sandstone, 18);
            }
            if (bounds.contains(chamberX + x, chamberY + y, chamberZ + 6)) {
                world.setBlockState(chamberX + x, chamberY + y, chamberZ + 6, sandstone, 18);
            }
        }
        for (i32 z = 0; z < 7; ++z) {
            if (bounds.contains(chamberX, chamberY + y, chamberZ + z)) {
                world.setBlockState(chamberX, chamberY + y, chamberZ + z, sandstone, 18);
            }
            if (bounds.contains(chamberX + 6, chamberY + y, chamberZ + z)) {
                world.setBlockState(chamberX + 6, chamberY + y, chamberZ + z, sandstone, 18);
            }
        }
    }

    // 宝藏室天花板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 7; ++z) {
            i32 wx = chamberX + x;
            i32 wy = chamberY + 5;
            i32 wz = chamberZ + z;
            if (bounds.contains(wx, wy, wz)) {
                world.setBlockState(wx, wy, wz, sandstone, 18);
            }
        }
    }

    // TNT 陷阱（四个角落）
    const BlockState* stonePressurePlate = VanillaBlocks::getState(VanillaBlocks::STONE_PRESSURE_PLATE);
    constexpr i32 trapPositions[4][2] = {{1, 1}, {5, 1}, {1, 5}, {5, 5}};

    for (i32 trap = 0; trap < 4; ++trap) {
        i32 trapX = chamberX + trapPositions[trap][0];
        i32 trapZ = chamberZ + trapPositions[trap][1];

        if (tnt) {
            if (bounds.contains(trapX, chamberY - 1, trapZ)) {
                world.setBlockState(trapX, chamberY - 1, trapZ, tnt, 18);
            }
            if (bounds.contains(trapX, chamberY - 2, trapZ)) {
                world.setBlockState(trapX, chamberY - 2, trapZ, tnt, 18);
            }
        }

        if (stonePressurePlate) {
            if (bounds.contains(trapX, chamberY + 1, trapZ)) {
                world.setBlockState(trapX, chamberY + 1, trapZ, stonePressurePlate, 18);
            }
        }
    }

    // 宝藏室中央（使用金块作为宝箱占位符，宝箱方块尚未实现）
    const BlockState* goldBlock = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
    if (rng.nextInt(100) < 80) {
        i32 chestX = chamberX + 3;
        i32 chestZ = chamberZ + 3;
        if (goldBlock && bounds.contains(chestX, chamberY + 1, chestZ)) {
            world.setBlockState(chestX, chamberY + 1, chestZ, goldBlock, 18);
        }
    }
}

// ============================================================================
// DesertPyramidStructure
// ============================================================================

const std::string DesertPyramidStructure::m_name = "desert_pyramid";

DesertPyramidStructure::DesertPyramidStructure()
    : Structure(ResourceLocation("minecraft", "desert_pyramid"))
{
    _initializeBiomes();
}

void DesertPyramidStructure::_initializeBiomes()
{
    m_validBiomes = {Desert, DesertHills, DesertLakes};
}

bool DesertPyramidStructure::canGenerate(
    IWorld& /*world*/, IChunkGenerator& /*generator*/, math::Random& /*rng*/, i32 /*chunkX*/, i32 /*chunkZ*/)
{
    // TODO: 实现完整的生物群系检查逻辑
    return true;
}

std::unique_ptr<StructureStart> DesertPyramidStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 沙漠神殿尺寸: 21x21 地面部分，深度 12
    // 计算起始位置（居中）
    i32 startX = chunkX * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);
    i32 startZ = chunkZ * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);

    // 获取地表高度
    i32 startY = generator.getHeight(startX, startZ, HeightmapType::WorldSurfaceWG);
    if (startY < 60) startY = 64;

    BlockPos startPos(startX, startY, startZ);

    // 创建片段（方块写入延迟到 FEATURES 阶段由 placeInChunk() 执行）
    auto piece = std::make_unique<DesertPyramidPiece>(startPos);
    start->addPiece(std::move(piece));
    start->recalculateStructureSize();

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
