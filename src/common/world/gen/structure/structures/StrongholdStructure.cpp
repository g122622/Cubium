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

#include "StrongholdStructure.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../jigsaw/JigsawManager.hpp"
#include "../../jigsaw/JigsawPattern.hpp"
#include "../StructureBoundingBox.hpp"
#include <cmath>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string StrongholdStructure::m_name = "stronghold";

StrongholdStructure::StrongholdStructure()
    : Structure(StructureType::Stronghold)
{
    initializeBiomes();
}

StrongholdStructure::StrongholdStructure(const Config& config)
    : Structure(StructureType::Stronghold)
    , m_config(config)
{
    initializeBiomes();
}

void StrongholdStructure::initializeBiomes()
{
    // 要塞可以在大多数主世界生物群系生成
    m_validBiomes = {Plains,
        SunflowerPlains,
        Forest,
        FlowerForest,
        BirchForest,
        BirchForestHills,
        DarkForest,
        DarkForestHills,
        Taiga,
        TaigaHills,
        TaigaMountains,
        GiantTreeTaiga,
        GiantTreeTaigaHills,
        GiantSpruceTaiga,
        GiantSpruceTaigaHills,
        Mountains,
        WoodedMountains,
        GravellyMountains,
        MountainEdge,
        Jungle,
        JungleHills,
        JungleEdge,
        ModifiedJungle,
        ModifiedJungleEdge,
        Desert,
        DesertHills,
        DesertLakes,
        Badlands,
        BadlandsPlateau,
        WoodedBadlandsPlateau,
        Savanna,
        SavannaPlateau,
        ShatteredSavanna,
        Swamp,
        SwampHills,
        SnowyPlains,
        SnowyMountains,
        SnowyTaiga,
        SnowyTaigaHills,
        SnowyTaigaMountains,
        SnowyPlains,
        SnowyBeach};
}

bool StrongholdStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // 要塞位置由种子决定，不能随机生成
    // 需要检查当前位置是否是预计算的要塞位置
    return true;
}

std::unique_ptr<StructureStart> StrongholdStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算起始位置
    i32 startX = chunkX * 16 + 8;
    i32 startZ = chunkZ * 16 + 8;

    // 要塞生成在地下
    i32 startY = m_config.minY + rng.nextInt(m_config.maxY - m_config.minY);

    BlockPos startPos(startX, startY, startZ);

    // 使用预定义的模板池
    ResourceLocation startPoolLocation("minecraft", "stronghold/start");
    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(startPoolLocation);

    if (startPool && !startPool->isEmpty()) {
        // 使用 Jigsaw 系统生成要塞
        jigsaw::JigsawManager::assembleAndPlace(world,
            patternRegistry,
            *startPool,
            8, // 要塞深度较大
            startPos,
            rng);
    } else {
        // 回退：生成简单的要塞入口
        generateFallbackEntrance(world, rng, startPos);
    }

    return start;
}

void StrongholdStructure::generateFallbackEntrance(
    IWorldWriter& world, math::Random& rng, const BlockPos& startPos) const
{
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* mossyStoneBricks = VanillaBlocks::getState(VanillaBlocks::MOSSY_STONE_BRICKS);
    const BlockState* crackedStoneBricks = VanillaBlocks::getState(VanillaBlocks::CRACKED_STONE_BRICKS);
    const BlockState* chiseledStoneBricks = VanillaBlocks::getState(VanillaBlocks::CHISELED_STONE_BRICKS);
    const BlockState* goldBlock = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    // 辅助lambda: 随机选择石砖类型
    auto randomBrick = [&]() -> const BlockState* {
        i32 r = rng.nextInt(100);
        if (r < 50) return stoneBricks;
        if (r < 75) return mossyStoneBricks;
        if (r < 95) return crackedStoneBricks;
        return chiseledStoneBricks;
    };

    i32 baseX = startPos.x;
    i32 baseY = startPos.y;
    i32 baseZ = startPos.z;

    // 生成入口楼梯井（5x5）
    for (i32 y = 0; y < 10; ++y) {
        for (i32 x = -2; x <= 2; ++x) {
            for (i32 z = -2; z <= 2; ++z) {
                // 边缘是石砖墙，中间是空气
                bool isEdge = std::abs(x) == 2 || std::abs(z) == 2;
                if (isEdge) {
                    world.setBlockState(baseX + x, baseY + y, baseZ + z, randomBrick(), 18);
                } else if (y == 0) {
                    // 底部
                    world.setBlockState(baseX + x, baseY + y, baseZ + z, stoneBricks, 18);
                }
                // 中间是空气（已挖空）
            }
        }
    }

    // 生成传送门房间（10x10）
    i32 portalRoomY = baseY + 10;
    i32 roomSize = 10;

    // 地板
    for (i32 x = -roomSize / 2; x <= roomSize / 2; ++x) {
        for (i32 z = -roomSize / 2; z <= roomSize / 2; ++z) {
            world.setBlockState(baseX + x, portalRoomY, baseZ + z, stoneBricks, 18);
        }
    }

    // 墙壁
    i32 roomHeight = 6;
    for (i32 y = 1; y <= roomHeight; ++y) {
        for (i32 x = -roomSize / 2; x <= roomSize / 2; ++x) {
            world.setBlockState(baseX + x, portalRoomY + y, baseZ - roomSize / 2, randomBrick(), 18);
            world.setBlockState(baseX + x, portalRoomY + y, baseZ + roomSize / 2, randomBrick(), 18);
        }
        for (i32 z = -roomSize / 2; z <= roomSize / 2; ++z) {
            world.setBlockState(baseX - roomSize / 2, portalRoomY + y, baseZ + z, randomBrick(), 18);
            world.setBlockState(baseX + roomSize / 2, portalRoomY + y, baseZ + z, randomBrick(), 18);
        }
    }

    // 天花板
    for (i32 x = -roomSize / 2; x <= roomSize / 2; ++x) {
        for (i32 z = -roomSize / 2; z <= roomSize / 2; ++z) {
            world.setBlockState(baseX + x, portalRoomY + roomHeight + 1, baseZ + z, stoneBricks, 18);
        }
    }

    // 末地传送门框架（中央）
    // 传送门是 3x3，四边有框架
    i32 portalY = portalRoomY + 1;
    for (i32 x = -1; x <= 1; ++x) {
        for (i32 z = -1; z <= 1; ++z) {
            // 框架位置（四边）
            bool isFrame = (std::abs(x) == 1 && z == 0) || (std::abs(z) == 1 && x == 0) || (x == 0 && z == 0);
            if (isFrame) {
                // 使用金块作为传送门框架占位符（末地传送门框架方块尚未实现）
                world.setBlockState(baseX + x, portalY, baseZ + z, goldBlock ? goldBlock : chiseledStoneBricks, 18);
            }
        }
    }
}

std::pair<i32, i32> StrongholdStructure::calculateStrongholdPos(i32 index, i64 worldSeed)
{
    // 要塞分布算法（参考 MC 1.16.5: StrongholdStructure.java）
    // 8 个环，每个环有不同数量的要塞
    // 环 0: 3 个要塞，距离 1408-2688
    // 环 1: 3 个要塞，距离 4480-5760
    // 环 2: 3 个要塞，距离 7552-8832
    // 环 3: 4 个要塞，距离 10624-11904
    // 环 4: 6 个要塞，距离 13696-14976
    // 环 5: 10 个要塞，距离 16768-18048
    // 环 6: 15 个要塞，距离 19840-21120
    // 环 7: 21 个要塞，距离 22912-24192
    // 总计: 3+3+3+4+6+10+15+21 = 65 个要塞 (MC 1.16.5)

    // MC 1.16.5: StrongholdStructure.java 第 47-48 行
    static const i32 ringCounts[] = {3, 3, 3, 4, 6, 10, 15, 21};
    static const i32 ringDistances[] = {1408, 4480, 7552, 10624, 13696, 16768, 19840, 22912};
    static const i32 ringSpreads[] = {1280, 1280, 1280, 1280, 1280, 1280, 1280, 1280};

    i32 ring = getRing(index);
    i32 ringIndex = index;
    for (i32 i = 0; i < ring; ++i) {
        ringIndex -= ringCounts[i];
    }

    i32 count = ringCounts[ring];
    i32 distance = ringDistances[ring];
    i32 spread = ringSpreads[ring];

    // 计算角度
    math::Random rng(worldSeed);
    [[maybe_unused]] const i32 skippedValue0 = rng.nextInt(); // 跳过一些值
    [[maybe_unused]] const i32 skippedValue1 = rng.nextInt();

    // 计算该要塞的角度
    f64 angleStep = 2.0 * mc::math::PI_DOUBLE / count;
    f64 angle = angleStep * ringIndex;

    // 添加随机偏移
    f64 randomOffset = (rng.nextDouble() - 0.5) * angleStep * 0.5;
    angle += randomOffset;

    // 计算距离
    i32 actualDistance = distance + rng.nextInt(spread);

    // 计算坐标
    i32 x = static_cast<i32>(std::cos(angle) * actualDistance);
    i32 z = static_cast<i32>(std::sin(angle) * actualDistance);

    return {x >> 4, z >> 4}; // 转换为区块坐标
}

i32 StrongholdStructure::getRing(i32 index)
{
    // MC 1.16.5: 3+3+3+4+6+10+15+21 = 65 个要塞
    static const i32 ringCounts[] = {3, 3, 3, 4, 6, 10, 15, 21};
    i32 cumulative = 0;
    for (i32 ring = 0; ring < 8; ++ring) {
        cumulative += ringCounts[ring];
        if (index < cumulative) {
            return ring;
        }
    }
    return 7;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
