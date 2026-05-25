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
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../StructureBoundingBox.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string DesertPyramidStructure::m_name = "desert_pyramid";

DesertPyramidStructure::DesertPyramidStructure()
    : Structure(StructureType::Temple)
{
    initializeBiomes();
}

void DesertPyramidStructure::initializeBiomes()
{
    m_validBiomes = {Desert, DesertHills, DesertLakes};
}

bool DesertPyramidStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // 检查生物群系是否合适
    return true;
}

std::unique_ptr<StructureStart> DesertPyramidStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 沙漠神殿尺寸: 21x21 地面部分，深度 12
    // 计算起始位置（居中）
    i32 startX = chunkX * 16 + rng.nextInt(16);
    i32 startZ = chunkZ * 16 + rng.nextInt(16);

    // 获取地表高度
    i32 startY = generator.getHeight(startX, startZ, HeightmapType::WorldSurfaceWG);
    if (startY < 60) startY = 64;

    BlockPos startPos(startX, startY, startZ);

    // 生成沙漠神殿
    generatePyramid(world, rng, startPos);

    return start;
}

void DesertPyramidStructure::generatePyramid(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const
{
    const BlockState* sandstone = VanillaBlocks::getState(VanillaBlocks::SANDSTONE);
    const BlockState* cutSandstone = VanillaBlocks::getState(VanillaBlocks::CUT_SANDSTONE);
    const BlockState* chiseledSandstone = VanillaBlocks::getState(VanillaBlocks::CHISELED_SANDSTONE);
    const BlockState* tnt = VanillaBlocks::getState(VanillaBlocks::TNT);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    // 基础参数
    i32 baseX = startPos.x;
    i32 baseY = startPos.y;
    i32 baseZ = startPos.z;
    i32 size = 21; // 神殿宽度/深度
    i32 halfSize = size / 2;

    // 生成基础平台
    for (i32 x = 0; x < size; ++x) {
        for (i32 z = 0; z < size; ++z) {
            world.setBlockState(baseX + x, baseY, baseZ + z, sandstone, 18);
        }
    }

    // 生成四层递减的平台
    for (i32 layer = 1; layer <= 4; ++layer) {
        i32 layerSize = size - layer * 2;
        i32 offset = layer;

        for (i32 x = 0; x < layerSize; ++x) {
            for (i32 z = 0; z < layerSize; ++z) {
                // 只生成边缘
                if (x == 0 || x == layerSize - 1 || z == 0 || z == layerSize - 1) {
                    world.setBlockState(baseX + offset + x, baseY + layer, baseZ + offset + z, sandstone, 18);
                }
            }
        }
    }

    // 生成四个角塔
    for (i32 tower = 0; tower < 4; ++tower) {
        i32 tx = (tower % 2 == 0) ? 0 : size - 3;
        i32 tz = (tower < 2) ? 0 : size - 3;

        // 塔高 6 格
        for (i32 y = 1; y <= 6; ++y) {
            for (i32 x = 0; x < 3; ++x) {
                for (i32 z = 0; z < 3; ++z) {
                    if (x == 1 && z == 1 && y > 1 && y < 6) {
                        // 内部空心
                        continue;
                    }
                    world.setBlockState(baseX + tx + x, baseY + y, baseZ + tz + z, sandstone, 18);
                }
            }
        }

        // 塔顶装饰
        world.setBlockState(baseX + tx + 1, baseY + 7, baseZ + tz + 1, chiseledSandstone, 18);
    }

    // 生成入口（南面中央）
    i32 entranceZ = size - 1;
    i32 entranceX = halfSize;

    // 入口台阶
    for (i32 step = 0; step < 3; ++step) {
        world.setBlockState(baseX + entranceX - 1, baseY + step, baseZ + entranceZ + step + 1, sandstone, 18);
        world.setBlockState(baseX + entranceX, baseY + step, baseZ + entranceZ + step + 1, sandstone, 18);
        world.setBlockState(baseX + entranceX + 1, baseY + step, baseZ + entranceZ + step + 1, sandstone, 18);
    }

    // 生成地下宝藏室
    i32 chamberY = baseY - 7;
    i32 chamberX = baseX + halfSize - 3;
    i32 chamberZ = baseZ + halfSize - 3;

    // 宝藏室地板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 7; ++z) {
            world.setBlockState(chamberX + x, chamberY, chamberZ + z, sandstone, 18);
        }
    }

    // 宝藏室墙壁
    for (i32 y = 1; y <= 4; ++y) {
        for (i32 x = 0; x < 7; ++x) {
            world.setBlockState(chamberX + x, chamberY + y, chamberZ, sandstone, 18);
            world.setBlockState(chamberX + x, chamberY + y, chamberZ + 6, sandstone, 18);
        }
        for (i32 z = 0; z < 7; ++z) {
            world.setBlockState(chamberX, chamberY + y, chamberZ + z, sandstone, 18);
            world.setBlockState(chamberX + 6, chamberY + y, chamberZ + z, sandstone, 18);
        }
    }

    // 宝藏室天花板
    for (i32 x = 0; x < 7; ++x) {
        for (i32 z = 0; z < 7; ++z) {
            world.setBlockState(chamberX + x, chamberY + 5, chamberZ + z, sandstone, 18);
        }
    }

    // TNT 陷阱（四个角落）
    const BlockState* stonePressurePlate = VanillaBlocks::getState(VanillaBlocks::STONE_PRESSURE_PLATE);

    // 四个陷阱位置
    constexpr i32 trapPositions[4][2] = {{1, 1}, {5, 1}, {1, 5}, {5, 5}};

    for (i32 trap = 0; trap < 4; ++trap) {
        i32 trapX = chamberX + trapPositions[trap][0];
        i32 trapZ = chamberZ + trapPositions[trap][1];

        // TNT 在地板下
        if (tnt) {
            world.setBlockState(trapX, chamberY - 1, trapZ, tnt, 18);
            world.setBlockState(trapX, chamberY - 2, trapZ, tnt, 18);
        }

        // 压力板在地板上
        if (stonePressurePlate) {
            world.setBlockState(trapX, chamberY + 1, trapZ, stonePressurePlate, 18);
        }
    }

    // 宝藏室中央（使用金块作为宝箱占位符，宝箱方块尚未实现）
    const BlockState* goldBlock = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
    if (rng.nextInt(100) < 80) { // 80% 概率生成宝藏
        i32 chestX = chamberX + 3;
        i32 chestZ = chamberZ + 3;
        if (goldBlock) {
            world.setBlockState(chestX, chamberY + 1, chestZ, goldBlock, 18);
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
