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

#include "OceanMonumentStructure.hpp"
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

const std::string OceanMonumentStructure::m_name = "ocean_monument";

OceanMonumentStructure::OceanMonumentStructure()
    : Structure(StructureType::Monument)
{
    initializeBiomes();
}

void OceanMonumentStructure::initializeBiomes()
{
    m_validBiomes = {DeepOcean, DeepWarmOcean, DeepLukewarmOcean, DeepColdOcean, DeepFrozenOcean};
}

bool OceanMonumentStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // MC 1.16.5: OceanMonumentStructure.func_230363_a_
    // 需要检查两个条件:
    // 1. 中心 16x16 范围内所有生物群系必须支持海洋纪念碑
    // 2. 29x29 范围内所有生物群系必须是海洋或河流类别

    i32 centerX = chunkX * 16 + 8;
    i32 centerZ = chunkZ * 16 + 8;

    // 检查中心生物群系是否在深海
    BiomeId centerBiome = generator.getBiome(centerX, 64, centerZ);
    if (!isValidBiome(centerBiome)) {
        return false;
    }

    // 检查 29x29 区块范围 (约 464x464 方块范围)
    // MC 1.16.5 使用 29 区块半径检查所有生物群系都是海洋或河流
    constexpr i32 checkRadius = 29;
    constexpr i32 checkStep = 16; // 每 16 格检查一次

    for (i32 dx = -checkRadius; dx <= checkRadius; dx += checkStep) {
        for (i32 dz = -checkRadius; dz <= checkRadius; dz += checkStep) {
            i32 x = centerX + dx;
            i32 z = centerZ + dz;
            BiomeId biome = generator.getBiome(x, 64, z);

            // 检查是否是海洋或河流类生物群系
            // MC 1.16.5: Biome.Category.OCEAN 或 Biome.Category.RIVER
            if (!isOceanOrRiverBiome(biome)) {
                return false;
            }
        }
    }

    return true;
}

bool OceanMonumentStructure::isOceanOrRiverBiome(BiomeId biomeId) const
{
    // MC 1.16.5: 检查生物群系是否属于海洋或河流类别
    // 包括所有海洋变种和河流变种
    using namespace mc::Biomes;

    switch (biomeId) {
        // 海洋类
        case Ocean:
        case WarmOcean:
        case LukewarmOcean:
        case ColdOcean:
        case FrozenOcean:
        case DeepOcean:
        case DeepWarmOcean:
        case DeepLukewarmOcean:
        case DeepColdOcean:
        case DeepFrozenOcean:
        // 河流类
        case River:
        case FrozenRiver:
            return true;
        default:
            return false;
    }
}

std::unique_ptr<StructureStart> OceanMonumentStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 海洋纪念碑尺寸: 58x58 基底，高度 23
    // 计算起始位置（对齐区块边界便于生成）
    i32 startX = chunkX * 16;
    i32 startZ = chunkZ * 16;

    // 海洋纪念碑生成在水下，通常 Y=39 左右
    i32 startY = 39;

    BlockPos startPos(startX, startY, startZ);

    // 生成海洋纪念碑
    generateMonument(world, rng, startPos);

    return start;
}

void OceanMonumentStructure::generateMonument(IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const
{
    const BlockState* prismarine = VanillaBlocks::getState(VanillaBlocks::PRISMARINE);
    const BlockState* prismarineBricks = VanillaBlocks::getState(VanillaBlocks::PRISMARINE_BRICKS);
    const BlockState* darkPrismarine = VanillaBlocks::getState(VanillaBlocks::DARK_PRISMARINE);
    const BlockState* seaLantern = VanillaBlocks::getState(VanillaBlocks::SEA_LANTERN);
    const BlockState* goldBlock = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
    const BlockState* sponge = VanillaBlocks::getState(VanillaBlocks::SPONGE);
    const BlockState* wetSponge = VanillaBlocks::getState(VanillaBlocks::WET_SPONGE);
    const BlockState* water = VanillaBlocks::getState(VanillaBlocks::WATER);

    // 基础参数
    i32 baseX = startPos.x;
    i32 baseY = startPos.y;
    i32 baseZ = startPos.z;
    i32 width = 58;  // X轴宽度
    i32 height = 23; // 高度
    i32 depth = 58;  // Z轴深度

    // 辅助lambda: 随机选择海晶石类型
    auto randomPrismarine = [&]() -> const BlockState* {
        i32 r = rng.nextInt(100);
        if (r < 40) return prismarine;
        if (r < 70) return prismarineBricks;
        return darkPrismarine;
    };

    // ========================================================================
    // 生成基底平台
    // ========================================================================
    for (i32 x = 0; x < width; ++x) {
        for (i32 z = 0; z < depth; ++z) {
            world.setBlockState(baseX + x, baseY, baseZ + z, prismarine, 18);
        }
    }

    // ========================================================================
    // 生成外墙
    // ========================================================================
    i32 wallHeight = 10;
    for (i32 y = 1; y <= wallHeight; ++y) {
        for (i32 x = 0; x < width; ++x) {
            world.setBlockState(baseX + x, baseY + y, baseZ, randomPrismarine(), 18);
            world.setBlockState(baseX + x, baseY + y, baseZ + depth - 1, randomPrismarine(), 18);
        }
        for (i32 z = 0; z < depth; ++z) {
            world.setBlockState(baseX, baseY + y, baseZ + z, randomPrismarine(), 18);
            world.setBlockState(baseX + width - 1, baseY + y, baseZ + z, randomPrismarine(), 18);
        }
    }

    // ========================================================================
    // 生成角塔（四个角）
    // ========================================================================
    i32 towerHeight = height - wallHeight;
    for (i32 corner = 0; corner < 4; ++corner) {
        i32 tx = (corner % 2 == 0) ? 0 : width - 1;
        i32 tz = (corner < 2) ? 0 : depth - 1;

        // 塔基（4x4）
        for (i32 y = 1; y <= wallHeight; ++y) {
            for (i32 dx = -1; dx <= 2; ++dx) {
                for (i32 dz = -1; dz <= 2; ++dz) {
                    i32 px = tx + dx;
                    i32 pz = tz + dz;
                    if (px >= 0 && px < width && pz >= 0 && pz < depth) {
                        world.setBlockState(baseX + px, baseY + y, baseZ + pz, prismarineBricks, 18);
                    }
                }
            }
        }

        // 塔顶（向上延伸）
        for (i32 y = wallHeight + 1; y < height; ++y) {
            for (i32 dx = 0; dx <= 1; ++dx) {
                for (i32 dz = 0; dz <= 1; ++dz) {
                    i32 px = tx + dx;
                    i32 pz = tz + dz;
                    if (corner % 2 == 1) px = tx - 1 + dx;
                    if (corner >= 2) pz = tz - 1 + dz;
                    world.setBlockState(baseX + px, baseY + y, baseZ + pz, prismarineBricks, 18);
                }
            }
        }

        // 塔尖海晶灯
        i32 tipY = height;
        i32 tipX = (corner % 2 == 0) ? tx : tx - 1;
        i32 tipZ = (corner < 2) ? tz : tz - 1;
        world.setBlockState(baseX + tipX, baseY + tipY, baseZ + tipZ, seaLantern, 18);
    }

    // ========================================================================
    // 生成中央主体建筑
    // ========================================================================
    i32 centerX = width / 2;
    i32 centerZ = depth / 2;
    i32 mainWidth = 20;
    i32 mainDepth = 20;
    i32 mainX = centerX - mainWidth / 2;
    i32 mainZ = centerZ - mainDepth / 2;

    // 主体墙壁
    for (i32 y = 1; y <= wallHeight + 5; ++y) {
        for (i32 x = mainX; x < mainX + mainWidth; ++x) {
            world.setBlockState(baseX + x, baseY + y, baseZ + mainZ, prismarineBricks, 18);
            world.setBlockState(baseX + x, baseY + y, baseZ + mainZ + mainDepth - 1, prismarineBricks, 18);
        }
        for (i32 z = mainZ; z < mainZ + mainDepth; ++z) {
            world.setBlockState(baseX + mainX, baseY + y, baseZ + z, prismarineBricks, 18);
            world.setBlockState(baseX + mainX + mainWidth - 1, baseY + y, baseZ + z, prismarineBricks, 18);
        }
    }

    // 主体内部中空（生成房间）
    generateRoom(world,
        prismarine,
        seaLantern,
        baseX + mainX + 1,
        baseY + 1,
        baseZ + mainZ + 1,
        mainWidth - 2,
        wallHeight + 3,
        mainDepth - 2);

    // 中央宝箱房间（海绵室）
    i32 spongeRoomY = baseY + wallHeight + 6;
    for (i32 x = mainX + 2; x < mainX + mainWidth - 2; ++x) {
        for (i32 z = mainZ + 2; z < mainZ + mainDepth - 2; ++z) {
            world.setBlockState(baseX + x, spongeRoomY, baseZ + z, prismarineBricks, 18);
        }
    }

    // 放置海绵和湿海绵
    for (i32 x = mainX + 4; x < mainX + mainWidth - 4; ++x) {
        for (i32 z = mainZ + 4; z < mainZ + mainDepth - 4; ++z) {
            // 随机放置海绵
            if (rng.nextInt(100) < 60) {
                world.setBlockState(
                    baseX + x, spongeRoomY + 1, baseZ + z, rng.nextInt(100) < 50 ? sponge : wetSponge, 18);
            }
        }
    }

    // 金块（中心）
    world.setBlockState(baseX + centerX, spongeRoomY + 1, baseZ + centerZ, goldBlock, 18);

    // ========================================================================
    // 生成左右翼楼
    // ========================================================================
    i32 wingWidth = 8;
    i32 wingDepth = 25;
    i32 wingHeight = wallHeight;

    // 左翼
    generateWing(world,
        prismarine,
        darkPrismarine,
        seaLantern,
        baseX + 5,
        baseY + 1,
        baseZ + 10,
        wingWidth,
        wingHeight,
        wingDepth,
        true);

    // 右翼
    generateWing(world,
        prismarine,
        darkPrismarine,
        seaLantern,
        baseX + width - 5 - wingWidth,
        baseY + 1,
        baseZ + 10,
        wingWidth,
        wingHeight,
        wingDepth,
        false);

    // ========================================================================
    // 生成顶部结构
    // ========================================================================
    i32 topY = baseY + wallHeight + 6;

    // 顶部平台
    for (i32 x = mainX - 2; x < mainX + mainWidth + 2; ++x) {
        for (i32 z = mainZ - 2; z < mainZ + mainDepth + 2; ++z) {
            world.setBlockState(baseX + x, topY, baseZ + z, darkPrismarine, 18);
        }
    }

    // 顶部中央结构
    for (i32 y = topY + 1; y < topY + 5; ++y) {
        for (i32 x = mainX + 2; x < mainX + mainWidth - 2; ++x) {
            world.setBlockState(baseX + x, y, baseZ + mainZ + 2, prismarineBricks, 18);
            world.setBlockState(baseX + x, y, baseZ + mainZ + mainDepth - 3, prismarineBricks, 18);
        }
        for (i32 z = mainZ + 2; z < mainZ + mainDepth - 2; ++z) {
            world.setBlockState(baseX + mainX + 2, y, baseZ + z, prismarineBricks, 18);
            world.setBlockState(baseX + mainX + mainWidth - 3, y, baseZ + z, prismarineBricks, 18);
        }
    }

    // 顶部海晶灯装饰
    world.setBlockState(baseX + centerX, topY + 5, baseZ + centerZ, seaLantern, 18);
    world.setBlockState(baseX + centerX - 2, topY + 3, baseZ + centerZ, seaLantern, 18);
    world.setBlockState(baseX + centerX + 2, topY + 3, baseZ + centerZ, seaLantern, 18);
    world.setBlockState(baseX + centerX, topY + 3, baseZ + centerZ - 2, seaLantern, 18);
    world.setBlockState(baseX + centerX, topY + 3, baseZ + centerZ + 2, seaLantern, 18);

    // ========================================================================
    // 添加海晶灯装饰
    // ========================================================================
    // 墙上海晶灯
    for (i32 i = 0; i < 8; ++i) {
        i32 lx = rng.nextInt(width - 4) + 2;
        i32 lz = rng.nextInt(depth - 4) + 2;
        i32 ly = rng.nextInt(wallHeight - 2) + 2;

        // 选择最近的墙
        if (lx < width / 2) {
            world.setBlockState(baseX + 1, baseY + ly, baseZ + lz, seaLantern, 18);
        } else {
            world.setBlockState(baseX + width - 2, baseY + ly, baseZ + lz, seaLantern, 18);
        }
    }
}

void OceanMonumentStructure::generateWing(IWorldWriter& world,
    const BlockState* prismarine,
    const BlockState* darkPrismarine,
    const BlockState* seaLantern,
    i32 baseX,
    i32 baseY,
    i32 baseZ,
    i32 width,
    i32 height,
    i32 depth,
    bool isLeft) const
{
    // 翼楼墙壁
    for (i32 y = 0; y < height; ++y) {
        for (i32 x = 0; x < width; ++x) {
            world.setBlockState(baseX + x, baseY + y, baseZ, prismarine, 18);
            world.setBlockState(baseX + x, baseY + y, baseZ + depth - 1, prismarine, 18);
        }
        for (i32 z = 0; z < depth; ++z) {
            world.setBlockState(baseX, baseY + y, baseZ + z, prismarine, 18);
            world.setBlockState(baseX + width - 1, baseY + y, baseZ + z, prismarine, 18);
        }
    }

    // 翼楼地板
    for (i32 x = 1; x < width - 1; ++x) {
        for (i32 z = 1; z < depth - 1; ++z) {
            world.setBlockState(baseX + x, baseY, baseZ + z, darkPrismarine, 18);
        }
    }

    // 翼楼内部（水填充）
    // 内部已经是水，不需要额外填充

    // 海晶灯装饰
    i32 midZ = depth / 2;
    world.setBlockState(baseX + width / 2, baseY + height / 2, baseZ + midZ, seaLantern, 18);
    world.setBlockState(baseX + width / 2, baseY + height / 2, baseZ + midZ + 5, seaLantern, 18);
    world.setBlockState(baseX + width / 2, baseY + height / 2, baseZ + midZ - 5, seaLantern, 18);
}

void OceanMonumentStructure::generateRoom(IWorldWriter& world,
    const BlockState* prismarine,
    const BlockState* seaLantern,
    i32 baseX,
    i32 baseY,
    i32 baseZ,
    i32 width,
    i32 height,
    i32 depth) const
{
    // 房间内部清空（已经是水环境）
    // 这里我们只添加一些装饰性结构

    // 中央柱子
    i32 centerX = width / 2;
    i32 centerZ = depth / 2;
    for (i32 y = 0; y < height; ++y) {
        world.setBlockState(baseX + centerX, baseY + y, baseZ + centerZ, prismarine, 18);
    }

    // 角落柱子
    for (i32 corner = 0; corner < 4; ++corner) {
        i32 cx = (corner % 2 == 0) ? 1 : width - 2;
        i32 cz = (corner < 2) ? 1 : depth - 2;
        for (i32 y = 0; y < height; ++y) {
            world.setBlockState(baseX + cx, baseY + y, baseZ + cz, prismarine, 18);
        }
    }

    // 海晶灯装饰
    world.setBlockState(baseX + centerX, baseY + height - 1, baseZ + centerZ, seaLantern, 18);
    world.setBlockState(baseX + centerX, baseY + 1, baseZ + centerZ, seaLantern, 18);
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
