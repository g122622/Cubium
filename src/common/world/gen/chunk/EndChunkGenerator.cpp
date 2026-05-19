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

#include "EndChunkGenerator.hpp"
#include "../../../core/Constants.hpp"
#include "../../../util/math/MathConstants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../biome/BiomeGenerationSettings.hpp"
#include "../../biome/BiomeRegistry.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../feature/ConfiguredFeature.hpp"
#include "../spawn/WorldGenSpawner.hpp"
#include "../structure/Structure.hpp"
#include "../structure/StructureManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

// 末地高度范围
constexpr i32 END_HEIGHT = world::MAX_BUILD_HEIGHT;
constexpr i32 END_MIN_Y = world::MIN_BUILD_HEIGHT;

// 主岛参数（参考 MC 1.16.5）
constexpr i32 MAIN_ISLAND_RADIUS = 96;
constexpr i32 ISLAND_HEIGHT_BASE = 64;
constexpr f32 VOID_HEIGHT = -64.0f; // 虚空高度

// 黑曜石柱参数
constexpr i32 PILLAR_COUNT = 10;
constexpr i32 MIN_PILLAR_HEIGHT = 76;
constexpr i32 MAX_PILLAR_HEIGHT = 103;
constexpr i32 MIN_PILLAR_RADIUS = 2;
constexpr i32 MAX_PILLAR_RADIUS = 4;

// ============================================================================
// EndChunkGenerator 实现
// ============================================================================

EndChunkGenerator::EndChunkGenerator(u64 seed)
    : BaseChunkGenerator(seed, DimensionSettings::end())
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
    , m_random(seed)
{
    initSettings();
    initNoiseGenerators();

    // 确保生物群系注册表已初始化
    BiomeRegistry::instance().initialize();

    // 创建末地生物群系提供者
    m_biomeProvider = std::make_unique<biome::end::EndBiomeProvider>(seed);

    // 初始化结构管理器（末地城等）
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(seed));
}

EndChunkGenerator::EndChunkGenerator(u64 seed, DimensionSettings settings)
    : BaseChunkGenerator(seed, std::move(settings))
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
    , m_random(seed)
{
    initSettings();
    initNoiseGenerators();

    // 确保生物群系注册表已初始化
    BiomeRegistry::instance().initialize();

    // 创建末地生物群系提供者
    m_biomeProvider = std::make_unique<biome::end::EndBiomeProvider>(seed);

    // 初始化结构管理器
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(seed));
}

// ============================================================================
// 初始化
// ============================================================================

void EndChunkGenerator::initSettings()
{
    // 末地特有设置
    m_mainIslandRadius = MAIN_ISLAND_RADIUS;
    m_endIslandHeight = ISLAND_HEIGHT_BASE;
}

void EndChunkGenerator::initNoiseGenerators()
{
    // 计算噪声尺寸
    constexpr i32 verticalGranularity = 8;
    constexpr i32 horizontalGranularity = 4;
    m_noiseSizeX = 16 / horizontalGranularity;
    m_noiseSizeY = END_HEIGHT / verticalGranularity;
    m_noiseSizeZ = 16 / horizontalGranularity;

    // 创建噪声生成器（参考 MC 1.16.5）
    math::Random rng(m_seed);

    // 岛屿噪声（Simplex）
    m_islandNoise = std::make_unique<SimplexNoiseGenerator>(rng);

    // 密度噪声（用于外岛）
    m_densityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -7, 0);
}

// ============================================================================
// 生成阶段
// ============================================================================

void EndChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.end", "GenerateStructureStarts");
    // 末地结构：末地城、末地船等
    // 目前使用基类实现
    BaseChunkGenerator::generateStructureStarts(region, chunk);
}

void EndChunkGenerator::generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.end", "GenerateStructureReferences");
    BaseChunkGenerator::generateStructureReferences(region, chunk);
}

void EndChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.end", "GenerateBiomes");

    // 使用 EndBiomeProvider 填充生物群系
    if (m_biomeProvider) {
        m_biomeProvider->fillBiomeContainer(chunk.getBiomes(), chunk.x(), chunk.z());
    }

    chunk.setChunkStatus(ChunkStatuses::BIOMES);
}

void EndChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.end", "GenerateNoise");

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 获取默认方块（末地石）
    const BlockState* endStone = &VanillaBlocks::END_STONE->getDefaultState();
    const BlockState* air = &VanillaBlocks::AIR->getDefaultState();

    // 判断区块是否在主岛范围内
    const bool mainIsland = isChunkInMainIsland(chunkX, chunkZ);

    if (mainIsland) {
        // 生成主岛
        generateMainIsland(chunk);
    } else {
        // 生成外岛
        generateOuterIslands(chunk);
    }

    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

void EndChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.end", "BuildSurface");

    // 主岛生成黑曜石柱
    if (isChunkInMainIsland(chunk.x(), chunk.z())) {
        generateObsidianPillars(chunk);
    }

    // 末地无地表生成（无草地、泥土等）
    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

void EndChunkGenerator::applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid)
{
    MC_TRACE_EVENT("world.gen.end", "ApplyCarvers");
    // 末地无洞穴雕刻
    MC_UNUSED(region);

    chunk.setChunkStatus(isLiquid ? ChunkStatuses::LIQUID_CARVERS : ChunkStatuses::CARVERS);
}

void EndChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.end", "PlaceFeatures");

    // 线程安全初始化特征注册表
    static std::once_flag s_featureRegistryInitFlag;
    std::call_once(s_featureRegistryInitFlag, []() { FeatureRegistry::instance().initialize(); });

    const BiomeId biomeId = chunk.getBiomeAtBlock(8, 64, 8);
    const Biome& biome = m_biomeProvider->getBiomeDefinition(biomeId);
    const BiomeGenerationSettings& settings = biome.generationSettings();

    for (DecorationStage stage : DecorationStages::getAll()) {
        BiomeFeaturePlacer::placeFeaturesForStage(region, chunk, *this, settings, stage, m_seed);
    }

    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

i32 EndChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    // 末地生物生成（末影人等）
    // 暂时不生成初始生物
    MC_UNUSED(region);
    MC_UNUSED(chunk);
    MC_UNUSED(outEntities);
    return 0;
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId EndChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    if (m_biomeProvider) {
        return m_biomeProvider->getBiome(x, y, z);
    }
    return m_defaultBiome;
}

BiomeId EndChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    if (m_biomeProvider) {
        return m_biomeProvider->getNoiseBiome(noiseX, noiseY, noiseZ);
    }
    return m_defaultBiome;
}

i32 EndChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    auto matchesHeightmap = [type](bool isSolid) -> bool {
        if (!isSolid) {
            return false;
        }

        switch (type) {
            case HeightmapType::WorldSurface:
            case HeightmapType::WorldSurfaceWG:
            case HeightmapType::OceanFloor:
            case HeightmapType::OceanFloorWG:
            case HeightmapType::MotionBlocking:
            case HeightmapType::MotionBlockingNoLeaves:
            case HeightmapType::LightBlocking:
                return true;

            default:
                return true;
        }
    };

    const auto isSolidAt = [this, x, z](i32 y) -> bool {
        if (y < END_MIN_Y || y >= END_HEIGHT) {
            return false;
        }

        // 主岛地形（与 generateMainIsland 保持一致）
        if (isInMainIsland(x, z)) {
            const f32 radiusF = static_cast<f32>(m_mainIslandRadius);
            const i64 distSq = static_cast<i64>(x) * x + static_cast<i64>(z) * z;
            const f32 normalizedDist = static_cast<f32>(std::sqrt(static_cast<f64>(distSq))) / radiusF;

            i32 height = 0;
            if (normalizedDist < 0.3f) {
                height = 45 + static_cast<i32>((1.0f - normalizedDist / 0.3f) * 20.0f);
            } else if (normalizedDist < 0.7f) {
                height = 50 + static_cast<i32>((normalizedDist - 0.3f) / 0.4f * 20.0f);
            } else if (normalizedDist < 1.0f) {
                height = 70 + static_cast<i32>((normalizedDist - 0.7f) / 0.3f * 25.0f);
            }

            if (y >= 40 && y <= height) {
                const f32 noise = m_islandNoise->noise2D(static_cast<f32>(x) * 0.1f, static_cast<f32>(z) * 0.1f);
                if (y < height - 5 || noise > -0.3f) {
                    return true;
                }
            }

            // 黑曜石柱（与 generateObsidianPillars 保持一致）
            constexpr f32 PILLAR_RADIUS = 43.0f;
            const f64 pillarDistSq = static_cast<f64>(x) * x + static_cast<f64>(z) * z;
            const f32 distance = static_cast<f32>(std::sqrt(pillarDistSq));
            if (std::abs(distance - PILLAR_RADIUS) < 5.0f) {
                const f32 angle = std::atan2(static_cast<f32>(z), static_cast<f32>(x));
                const i32 pillarIndex =
                    static_cast<i32>((angle + static_cast<f32>(mc::math::PI_DOUBLE)) /
                        (2.0f * static_cast<f32>(mc::math::PI_DOUBLE)) * static_cast<f32>(PILLAR_COUNT)) %
                    PILLAR_COUNT;
                const i32 pillarHeight =
                    MIN_PILLAR_HEIGHT + (pillarIndex * (MAX_PILLAR_HEIGHT - MIN_PILLAR_HEIGHT)) / PILLAR_COUNT;
                const i32 pillarRadius = MIN_PILLAR_RADIUS + (pillarIndex % 3);
                const f32 distFromPillarCenter = std::abs(distance - PILLAR_RADIUS);
                if (distFromPillarCenter < static_cast<f32>(pillarRadius) && y <= pillarHeight) {
                    return true;
                }
            }

            return false;
        }

        // 外岛地形（与 generateOuterIslands 保持一致）
        const f32 height = calculateIslandHeight(x, z);
        if (height <= 0.0f) {
            return false;
        }

        const f32 thicknessNoise = m_islandNoise->noise2D(static_cast<f32>(x) * 0.05f, static_cast<f32>(z) * 0.05f);
        const i32 thickness = static_cast<i32>(5.0f + thicknessNoise * 10.0f);
        const i32 topY = static_cast<i32>(height);
        const i32 bottomY = std::max(40, topY - thickness);
        return y >= bottomY && y <= topY;
    };

    for (i32 y = END_HEIGHT - 1; y >= END_MIN_Y; --y) {
        if (matchesHeightmap(isSolidAt(y))) {
            return y + 1;
        }
    }

    return 0;
}

// ============================================================================
// 末地特有方法
// ============================================================================

bool EndChunkGenerator::isInMainIsland(i32 x, i32 z) const
{
    const i64 distSq = static_cast<i64>(x) * x + static_cast<i64>(z) * z;
    return distSq <= static_cast<i64>(m_mainIslandRadius) * m_mainIslandRadius;
}

bool EndChunkGenerator::isChunkInMainIsland(ChunkCoord chunkX, ChunkCoord chunkZ) const
{
    // 检查区块中心是否在主岛范围内
    const i32 centerX = chunkX * 16 + 8;
    const i32 centerZ = chunkZ * 16 + 8;
    return isInMainIsland(centerX, centerZ);
}

f32 EndChunkGenerator::calculateIslandHeight(i32 x, i32 z) const
{
    // 主岛：固定高度
    if (isInMainIsland(x, z)) {
        return static_cast<f32>(m_endIslandHeight);
    }

    // 外岛：使用噪声确定高度
    // 外岛距离主岛至少 1000 方块
    const i64 distSq = static_cast<i64>(x) * x + static_cast<i64>(z) * z;
    if (distSq < 1000000LL) { // 1000^2
        return 0.0f;          // 主岛和外岛之间的虚空
    }

    // 外岛高度噪声
    constexpr f32 SCALE = 0.015625f; // 1/64
    const f32 noise = m_islandNoise->noise2D(static_cast<f32>(x) * SCALE, static_cast<f32>(z) * SCALE);

    // 只有噪声值大于阈值才有岛屿
    if (noise < m_islandNoiseThreshold) {
        return 0.0f;
    }

    return static_cast<f32>(m_endIslandHeight);
}

f32 EndChunkGenerator::calculateNoiseDensity(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    // 缩放因子
    constexpr f32 SCALE = 0.0625f; // 1/16

    const f32 nx = static_cast<f32>(noiseX) * SCALE;
    const f32 ny = static_cast<f32>(noiseY) * SCALE;
    const f32 nz = static_cast<f32>(noiseZ) * SCALE;

    // 密度噪声
    return m_densityNoise->noise(nx, ny, nz);
}

const BlockState* EndChunkGenerator::getBlockForDensity(f32 density, i32 y) const
{
    // 密度 > 0 表示实心方块
    if (density > 0.0f) {
        return &VanillaBlocks::END_STONE->getDefaultState();
    }
    // 空气
    return nullptr;
}

void EndChunkGenerator::generateMainIsland(ChunkPrimer& chunk)
{
    const BlockState* endStone = &VanillaBlocks::END_STONE->getDefaultState();

    // 主岛生成算法
    // 参考 MC 1.16.5: 主岛是一个凹陷的圆形岛屿
    // 中间是虚空（用于末地龙战斗）

    const i32 chunkX = chunk.x();
    const i32 chunkZ = chunk.z();
    const f32 radiusF = static_cast<f32>(m_mainIslandRadius);
    const i64 radiusSq = static_cast<i64>(m_mainIslandRadius) * m_mainIslandRadius;

    for (i32 lx = 0; lx < 16; ++lx) {
        for (i32 lz = 0; lz < 16; ++lz) {
            const i32 worldX = chunkX * 16 + lx;
            const i32 worldZ = chunkZ * 16 + lz;

            // 计算到中心的距离（使用平方距离避免开方）
            const i64 distSq = static_cast<i64>(worldX) * worldX + static_cast<i64>(worldZ) * worldZ;
            const f32 normalizedDist = static_cast<f32>(std::sqrt(static_cast<f64>(distSq))) / radiusF;

            // 主岛地形
            // 边缘较高，中间较低（战斗平台）
            for (i32 sectionY = 0; sectionY < 16; ++sectionY) {
                if (!chunk.hasSection(sectionY)) {
                    chunk.createSection(sectionY);
                }

                ChunkSection* section = chunk.getSection(sectionY);
                const i32 worldY = sectionY * 16;

                for (i32 ly = 0; ly < 16; ++ly) {
                    const i32 globalY = worldY + ly;

                    // 计算高度
                    i32 height = 0;
                    if (normalizedDist < 0.3f) {
                        // 内圈：战斗平台，较低
                        height = 45 + static_cast<i32>((1.0f - normalizedDist / 0.3f) * 20);
                    } else if (normalizedDist < 0.7f) {
                        // 中圈：凹陷区域
                        height = 50 + static_cast<i32>((normalizedDist - 0.3f) / 0.4f * 20);
                    } else if (normalizedDist < 1.0f) {
                        // 外圈：边缘区域，较高
                        height = 70 + static_cast<i32>((normalizedDist - 0.7f) / 0.3f * 25);
                    }

                    // 填充末地石
                    if (globalY <= height && globalY >= 40) {
                        // 添加一些噪声变化
                        const f32 noise =
                            m_islandNoise->noise2D(static_cast<f32>(worldX) * 0.1f, static_cast<f32>(worldZ) * 0.1f);

                        if (globalY < height - 5 || noise > -0.3f) {
                            section->setBlockState(lx, ly, lz, endStone);
                        }
                    }
                }
            }
        }
    }
}

void EndChunkGenerator::generateOuterIslands(ChunkPrimer& chunk)
{
    const BlockState* endStone = &VanillaBlocks::END_STONE->getDefaultState();

    const i32 chunkX = chunk.x();
    const i32 chunkZ = chunk.z();

    // 外岛生成
    for (i32 lx = 0; lx < 16; ++lx) {
        for (i32 lz = 0; lz < 16; ++lz) {
            const i32 worldX = chunkX * 16 + lx;
            const i32 worldZ = chunkZ * 16 + lz;

            // 计算岛屿高度
            const f32 height = calculateIslandHeight(worldX, worldZ);
            if (height <= 0.0f) {
                continue; // 虚空
            }

            // 计算岛屿厚度
            const f32 thicknessNoise =
                m_islandNoise->noise2D(static_cast<f32>(worldX) * 0.05f, static_cast<f32>(worldZ) * 0.05f);
            const i32 thickness = static_cast<i32>(5.0f + thicknessNoise * 10.0f);

            // 填充区块段
            const i32 topY = static_cast<i32>(height);
            const i32 bottomY = std::max(40, topY - thickness);

            for (i32 sectionY = 0; sectionY < 16; ++sectionY) {
                if (!chunk.hasSection(sectionY)) {
                    chunk.createSection(sectionY);
                }

                ChunkSection* section = chunk.getSection(sectionY);
                const i32 worldY = sectionY * 16;

                for (i32 ly = 0; ly < 16; ++ly) {
                    const i32 globalY = worldY + ly;

                    if (globalY >= bottomY && globalY <= topY) {
                        section->setBlockState(lx, ly, lz, endStone);
                    }
                }
            }
        }
    }
}

void EndChunkGenerator::generateObsidianPillars(ChunkPrimer& chunk)
{
    // 黑曜石柱在主岛中心
    // 参考 MC 1.16.5: 10 根黑曜石柱，围成一圈

    const i32 chunkX = chunk.x();
    const i32 chunkZ = chunk.z();

    // 检查区块是否包含黑曜石柱
    // 黑曜石柱位于距离中心约 43 方块的圆上
    constexpr f32 PILLAR_RADIUS = 43.0f;

    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->getDefaultState();
    const BlockState* bedrock = &VanillaBlocks::BEDROCK->getDefaultState();

    m_random.setSeed(static_cast<u64>(chunkX) * 341873128712LL + static_cast<u64>(chunkZ) * 132897987541LL);

    for (i32 lx = 0; lx < 16; ++lx) {
        for (i32 lz = 0; lz < 16; ++lz) {
            const i32 worldX = chunkX * 16 + lx;
            const i32 worldZ = chunkZ * 16 + lz;

            // 检查是否在黑曜石柱圆周上（使用平方距离）
            const f64 distSq = static_cast<f64>(worldX) * worldX + static_cast<f64>(worldZ) * worldZ;
            const f32 distance = static_cast<f32>(std::sqrt(distSq));

            if (std::abs(distance - PILLAR_RADIUS) < 5.0f) {
                // 计算角度，确定是哪根柱子
                const f32 angle = std::atan2(static_cast<f32>(worldZ), static_cast<f32>(worldX));
                const i32 pillarIndex = static_cast<i32>((angle + static_cast<f32>(mc::math::PI_DOUBLE)) /
                                            (2.0f * static_cast<f32>(mc::math::PI_DOUBLE)) * PILLAR_COUNT) %
                    PILLAR_COUNT;

                // 柱子高度和半径
                const i32 pillarHeight =
                    MIN_PILLAR_HEIGHT + (pillarIndex * (MAX_PILLAR_HEIGHT - MIN_PILLAR_HEIGHT)) / PILLAR_COUNT;
                const i32 pillarRadius = MIN_PILLAR_RADIUS + (pillarIndex % 3);

                // 检查是否在柱子半径内
                const f32 distFromPillarCenter = std::abs(distance - PILLAR_RADIUS);
                if (distFromPillarCenter < static_cast<f32>(pillarRadius)) {
                    // 填充黑曜石柱
                    for (i32 sectionY = 0; sectionY < 16; ++sectionY) {
                        if (!chunk.hasSection(sectionY)) {
                            continue;
                        }

                        ChunkSection* section = chunk.getSection(sectionY);
                        const i32 worldY = sectionY * 16;

                        for (i32 ly = 0; ly < 16; ++ly) {
                            const i32 globalY = worldY + ly;

                            if (globalY <= pillarHeight) {
                                // 顶部是基岩（用于水晶）
                                if (globalY == pillarHeight) {
                                    section->setBlockState(lx, ly, lz, bedrock);
                                } else {
                                    section->setBlockState(lx, ly, lz, obsidian);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

} // namespace mc
