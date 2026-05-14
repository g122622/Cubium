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

#define _USE_MATH_DEFINES
#include "OreFeature.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../placement/PlacementUtils.hpp"
#include "../Feature.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>

namespace mc {

namespace {

std::mutex g_oreFeaturesMutex;

} // namespace

// ============================================================================
// OreFeature 实现
// ============================================================================

bool OreFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    math::Random& random,
    const BlockPos& origin,
    const OreFeatureConfig& config)
{
    // 参考 MC OreFeature.place()
    // 生成椭圆形矿脉

    // 计算矿脉参数
    f32 angle = random.nextFloat() * static_cast<f32>(M_PI);
    f32 sizeFactor = static_cast<f32>(config.size) / 8.0f;
    i32 halfSize = static_cast<i32>(std::ceil((static_cast<f32>(config.size) / 16.0f * 2.0f + 1.0f) / 2.0f));

    // 计算矿脉两端位置
    f32 x1 = static_cast<f32>(origin.x) + std::sin(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);
    f32 x2 = static_cast<f32>(origin.x) - std::sin(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);
    f32 z1 = static_cast<f32>(origin.z) + std::cos(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);
    f32 z2 = static_cast<f32>(origin.z) - std::cos(static_cast<f32>(angle)) * static_cast<f32>(sizeFactor);

    f32 y1 = static_cast<f32>(origin.y + random.nextInt(-2, 2));
    f32 y2 = static_cast<f32>(origin.y + random.nextInt(-2, 2));

    // 计算边界框
    i32 minX = origin.x - static_cast<i32>(std::ceil(sizeFactor)) - halfSize;
    i32 minY = origin.y - 2 - halfSize;
    i32 minZ = origin.z - static_cast<i32>(std::ceil(sizeFactor)) - halfSize;
    i32 sizeX = 2 * (static_cast<i32>(std::ceil(sizeFactor)) + halfSize);
    i32 sizeY = 2 * (2 + halfSize);
    i32 sizeZ = sizeX;

    // 检查是否在有效范围内
    for (i32 checkX = minX; checkX <= minX + sizeX; ++checkX) {
        for (i32 checkZ = minZ; checkZ <= minZ + sizeZ; ++checkZ) {
            i32 topY = region.getTopBlockY(checkX, checkZ, HeightmapType::WorldSurfaceWG);
            if (minY <= topY) {
                i32 placedCount = 0;
                generateSphere(
                    chunk, random, config, x1, y1, z1, x2, y2, z2, minX, minY, minZ, sizeX, sizeY, sizeZ, placedCount);
                return placedCount > 0;
            }
        }
    }

    return false;
}

void OreFeature::generateSphere(ChunkPrimer& chunk,
    math::Random& random,
    const OreFeatureConfig& config,
    f32 x1,
    f32 y1,
    f32 z1,
    f32 x2,
    f32 y2,
    f32 z2,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 sizeX,
    i32 sizeY,
    i32 sizeZ,
    i32& placedCount)
{
    // 参考 MC OreFeature.func_207803_a()
    // 使用球形采样算法

    placedCount = 0;
    i32 totalSize = sizeX * sizeY * sizeZ;

    // 使用位数组跟踪已处理的方块
    std::vector<bool> processed(totalSize, false);

    // 计算每个"球心"的位置
    std::vector<f32> sphereCenters;
    sphereCenters.resize(static_cast<size_t>(config.size) * 4);

    for (i32 i = 0; i < config.size; ++i) {
        f32 progress = static_cast<f32>(i) / static_cast<f32>(config.size);

        f32 cx = x1 + (x2 - x1) * progress;
        f32 cy = y1 + (y2 - y1) * progress;
        f32 cz = z1 + (z2 - z1) * progress;

        f32 radiusFactor = static_cast<f32>(random.nextDouble() * static_cast<f64>(config.size) / 16.0);
        f32 radius = (std::sin(static_cast<f32>(M_PI) * progress) + 1.0f) * radiusFactor + 1.0f;

        sphereCenters[static_cast<size_t>(i) * 4 + 0] = cx;
        sphereCenters[static_cast<size_t>(i) * 4 + 1] = cy;
        sphereCenters[static_cast<size_t>(i) * 4 + 2] = cz;
        sphereCenters[static_cast<size_t>(i) * 4 + 3] = radius;
    }

    // 处理每个球心
    for (i32 i = 0; i < config.size - 1; ++i) {
        f32 radius1 = sphereCenters[static_cast<size_t>(i) * 4 + 3];

        if (radius1 <= 0.0) {
            continue;
        }

        for (i32 j = i + 1; j < config.size; ++j) {
            f32 radius2 = sphereCenters[static_cast<size_t>(j) * 4 + 3];

            if (radius2 <= 0.0) {
                continue;
            }

            // 计算两球心之间的距离
            f32 dx = sphereCenters[static_cast<size_t>(i) * 4 + 0] - sphereCenters[static_cast<size_t>(j) * 4 + 0];
            f32 dy = sphereCenters[static_cast<size_t>(i) * 4 + 1] - sphereCenters[static_cast<size_t>(j) * 4 + 1];
            f32 dz = sphereCenters[static_cast<size_t>(i) * 4 + 2] - sphereCenters[static_cast<size_t>(j) * 4 + 2];

            f32 distSq = dx * dx + dy * dy + dz * dz;
            f32 radiusSum = radius1 + radius2;

            // 如果两球重叠太多，移除较小的那个
            if (radiusSum * radiusSum > distSq) {
                if (radius1 > radius2) {
                    sphereCenters[static_cast<size_t>(j) * 4 + 3] = -1.0;
                } else {
                    sphereCenters[static_cast<size_t>(i) * 4 + 3] = -1.0;
                    break;
                }
            }
        }
    }

    // 获取目标方块状态
    const BlockState* oreState = config.state;
    if (!oreState) {
        return;
    }

    // 在每个球心周围放置矿石
    for (i32 i = 0; i < config.size; ++i) {
        f32 radius = sphereCenters[static_cast<size_t>(i) * 4 + 3];

        if (radius < 0.0) {
            continue;
        }

        f32 cx = sphereCenters[static_cast<size_t>(i) * 4 + 0];
        f32 cy = sphereCenters[static_cast<size_t>(i) * 4 + 1];
        f32 cz = sphereCenters[static_cast<size_t>(i) * 4 + 2];

        // 计算边界
        i32 localMinX = std::max(static_cast<i32>(std::floor(cx - radius)), minX);
        i32 localMinY = std::max(static_cast<i32>(std::floor(cy - radius)), minY);
        i32 localMinZ = std::max(static_cast<i32>(std::floor(cz - radius)), minZ);
        i32 localMaxX = std::max(static_cast<i32>(std::floor(cx + radius)), localMinX);
        i32 localMaxY = std::max(static_cast<i32>(std::floor(cy + radius)), localMinY);
        i32 localMaxZ = std::max(static_cast<i32>(std::floor(cz + radius)), localMinZ);

        // 遍历边界内的每个方块
        for (i32 bx = localMinX; bx <= localMaxX; ++bx) {
            f32 dx = (static_cast<f32>(bx) + 0.5f - cx) / radius;

            if (dx * dx >= 1.0f) {
                continue;
            }

            for (i32 by = localMinY; by <= localMaxY; ++by) {
                f32 dy = (static_cast<f32>(by) + 0.5f - cy) / radius;

                if (dx * dx + dy * dy >= 1.0f) {
                    continue;
                }

                for (i32 bz = localMinZ; bz <= localMaxZ; ++bz) {
                    f32 dz = (static_cast<f32>(bz) + 0.5f - cz) / radius;

                    if (dx * dx + dy * dy + dz * dz >= 1.0f) {
                        continue;
                    }

                    // 计算位数组索引
                    i32 index = (bx - minX) + (by - minY) * sizeX + (bz - minZ) * sizeX * sizeY;

                    if (index < 0 || index >= totalSize) {
                        continue;
                    }

                    if (processed[static_cast<size_t>(index)]) {
                        continue;
                    }

                    processed[static_cast<size_t>(index)] = true;

                    // 获取当前方块并检查是否可以替换
                    const BlockState* currentState = chunk.getBlockState(static_cast<BlockCoord>(bx & 15),
                        static_cast<BlockCoord>(by),
                        static_cast<BlockCoord>(bz & 15));

                    if (currentState && config.target->test(*currentState, random)) {
                        chunk.setBlockState(static_cast<BlockCoord>(bx & 15),
                            static_cast<BlockCoord>(by),
                            static_cast<BlockCoord>(bz & 15),
                            oreState);
                        ++placedCount;
                    }
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredOreFeature 实现
// ============================================================================

ConfiguredOreFeature::ConfiguredOreFeature(std::unique_ptr<OreFeatureConfig> featureConfig,
    std::unique_ptr<ConfiguredPlacement> placement,
    const char* featureName)
    : m_config(std::move(featureConfig))
    , m_placement(std::move(placement))
    , m_name(featureName)
{}

bool ConfiguredOreFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    // 忽略 pos 参数，矿石生成使用自己的放置逻辑
    (void)pos;
    (void)generator;

    if (!m_placement || !m_config) {
        return false;
    }

    // 获取放置位置
    BlockPos chunkPos(chunk.x() * 16, 0, chunk.z() * 16);
    auto positions = m_placement->getPositions(region, random, chunkPos);

    // 在每个位置生成矿石
    OreFeature feature;
    bool placed = false;
    for (const BlockPos& orePos : positions) {
        if (feature.place(region, chunk, random, orePos, *m_config)) {
            placed = true;
        }
    }
    return placed;
}

void ConfiguredOreFeature::generate(WorldGenRegion& region, ChunkPrimer& chunk, math::Random& random)
{
    if (!m_placement || !m_config) {
        return;
    }

    // 获取放置位置
    BlockPos chunkPos(chunk.x() * 16, 0, chunk.z() * 16);
    auto positions = m_placement->getPositions(region, random, chunkPos);

    // 在每个位置生成矿石
    OreFeature feature;
    for (const BlockPos& pos : positions) {
        feature.place(region, chunk, random, pos, *m_config);
    }
}

// ============================================================================
// OreFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredOreFeature>> OreFeatures::s_features;

void OreFeatures::initialize()
{
    std::lock_guard<std::mutex> lock(g_oreFeaturesMutex);
    s_features.clear();

    // 主世界矿石
    s_features.push_back(createCoalOre());
    s_features.push_back(createIronOre());
    s_features.push_back(createGoldOre());
    s_features.push_back(createRedstoneOre());
    s_features.push_back(createDiamondOre());
    s_features.push_back(createLapisOre());
    s_features.push_back(createEmeraldOre());
    s_features.push_back(createCopperOre());

    // 下界矿石
    s_features.push_back(createNetherQuartzOre());
    s_features.push_back(createNetherGoldOre());
    s_features.push_back(createAncientDebris());
}

const std::vector<std::unique_ptr<ConfiguredOreFeature>>& OreFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredOreFeature>> OreFeatures::getAllFeaturesAndClear()
{
    std::lock_guard<std::mutex> lock(g_oreFeaturesMutex);
    std::vector<std::unique_ptr<ConfiguredOreFeature>> result;
    result.swap(s_features);
    return result;
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createCoalOre()
{
    // 煤矿：Y 0-127，每区块20个，矿脉大小17
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::COAL_ORE), 17);

    // 创建链式放置：Count -> Square -> HeightRange
    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 128); // Y 0-127

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(20);

    // 构建链：Count -> Square -> HeightRange
    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    // 链接：count -> square -> height
    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(countConfigured), "coal_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createIronOre()
{
    // 铁矿：Y 0-63，每区块20个，矿脉大小9
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::IRON_ORE), 9);

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 64); // Y 0-63

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(20);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(countConfigured), "iron_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createGoldOre()
{
    // 金矿：Y 0-31，每区块2个，矿脉大小9
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::GOLD_ORE), 9);

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 32); // Y 0-31

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(2);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(countConfigured), "gold_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createRedstoneOre()
{
    // 红石：Y 0-15，每区块8个，矿脉大小8
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::REDSTONE_ORE), 8);

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 16); // Y 0-15

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(8);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(countConfigured), "redstone_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createDiamondOre()
{
    // 钻石：Y 0-15，每区块1个，矿脉大小8
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::DIAMOND_ORE), 8);

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 16); // Y 0-15

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(1);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(countConfigured), "diamond_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createLapisOre()
{
    // 青金石：Y 0-30，每区块1个，矿脉大小7
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::LAPIS_ORE), 7);

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 31); // Y 0-30

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(1);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(countConfigured), "lapis_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createEmeraldOre()
{
    // 绿宝石：Y 4-31，每区块1个，矿脉大小1（山地生物群系特有）
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::EMERALD_ORE), 1);

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(4, 0, 32); // Y 4-31

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(1);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    auto placement = PlacementUtils::appendBiomePlacement(std::move(countConfigured),
        {Biomes::Mountains, Biomes::WoodedMountains, Biomes::MountainEdge, Biomes::StoneShore});

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "emerald_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createCopperOre()
{
    // 铜矿：Y 0-96，每区块6个，矿脉大小10（1.17+新增）
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), VanillaBlocks::getState(VanillaBlocks::COPPER_ORE), 10);

    auto heightPlacement = std::make_unique<HeightRangePlacement>();
    auto heightConfig = std::make_unique<HeightRangePlacementConfig>(0, 0, 96); // Y 0-95

    auto squarePlacement = std::make_unique<SquarePlacement>();
    auto squareConfig = std::make_unique<EmptyPlacementConfig>();

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(6);

    auto heightConfigured = std::make_unique<ConfiguredPlacement>(std::move(heightPlacement), std::move(heightConfig));
    auto squareConfigured = std::make_unique<ConfiguredPlacement>(std::move(squarePlacement), std::move(squareConfig));
    auto countConfigured = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    squareConfigured->setNext(std::move(heightConfigured));
    countConfigured->setNext(std::move(squareConfigured));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(countConfigured), "copper_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createNetherQuartzOre()
{
    // 下界石英：Y 10-117，每区块16个，矿脉大小14
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::Netherrack), VanillaBlocks::getState(VanillaBlocks::NETHER_QUARTZ_ORE), 14);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(16);

    auto placement = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "nether_quartz_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createNetherGoldOre()
{
    // 下界金矿：Y 10-117，每区块10个，矿脉大小10
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::Netherrack), VanillaBlocks::getState(VanillaBlocks::NETHER_GOLD_ORE), 10);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(10);

    auto placement = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "nether_gold_ore");
}

std::unique_ptr<ConfiguredOreFeature> OreFeatures::createAncientDebris()
{
    // 远古残骸：Y 8-22，每区块2个，矿脉大小3
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::Netherrack), VanillaBlocks::getState(VanillaBlocks::ANCIENT_DEBRIS), 3);

    auto countPlacement = std::make_unique<CountPlacement>();
    auto countConfig = std::make_unique<CountPlacementConfig>(2);

    auto placement = std::make_unique<ConfiguredPlacement>(std::move(countPlacement), std::move(countConfig));

    return std::make_unique<ConfiguredOreFeature>(std::move(config), std::move(placement), "ancient_debris");
}

} // namespace mc
