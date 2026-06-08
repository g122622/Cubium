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

#include "NoiseChunkGenerator.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../WorldConstants.hpp"
#include "../../biome/BiomeGenerationSettings.hpp"
#include "../../biome/BiomeRegistry.hpp"
#include "../../biome/source/MultiNoiseBiomeSource.hpp"
#include "../aquifer/Aquifer.hpp"
#include "../carver/CarverConfiguration.hpp"
#include "../carver/CarvingContext.hpp"
#include "../carver/CarvingMask.hpp"
#include "../carver/WorldCarver.hpp"
#include "../density/Beardifier.hpp"
#include "../density/NoiseRouterData.hpp"
#include "../feature/ConfiguredFeature.hpp"
#include "../feature/ore/OreFeature.hpp"
#include "../jigsaw/JigsawPiece.hpp"
#include "../placement/PlacementRegistry.hpp"
#include "../spawn/WorldGenSpawner.hpp"
#include "../structure/Structure.hpp"
#include "../structure/StructureManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <unordered_set>

namespace mc {

// ============================================================================
// NoiseChunkGenerator 实现
// ============================================================================

NoiseChunkGenerator::NoiseChunkGenerator(
    u64 seed, DimensionSettings settings, std::unique_ptr<world::biome::BiomeSource> biomeSource)
    : BaseChunkGenerator(seed, std::move(settings))
    , m_biomeSource(std::move(biomeSource))
{
    // 确保生物群系注册表已初始化（默认构造路径会初始化，注入路径也需要）
    BiomeRegistry::instance().initialize();

    MC_ASSERT_RELEASE(m_biomeSource != nullptr);

    _initGenerationRegistries();

    // MC 1.21: 初始化密度函数管线
    _initDensityFunctionPipeline();
}

NoiseChunkGenerator::~NoiseChunkGenerator() = default;

// ============================================================================
// 初始化
// ============================================================================

void NoiseChunkGenerator::_initGenerationRegistries()
{
    MC_TRACE_EVENT("server.initialization", "NoiseChunkGenerator::initGenerationRegistries");

    // 初始化结构管理器
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(m_seed));

    // 初始化放置器注册表
    PlacementRegistry::instance().initialize();
}

void NoiseChunkGenerator::_initDensityFunctionPipeline()
{
    MC_TRACE_EVENT("server.initialization", "NoiseChunkGenerator::initDensityFunctionPipeline");

    // 创建 RandomState，统一持有 NoiseRouter、SurfaceSystem、随机工厂等
    m_randomState = world::gen::RandomState::create(m_settings, static_cast<u64>(m_seed));

    // 设置 cell 大小参数（根据维度类型）
    switch (m_settings.dimensionKind) {
        case DimensionKind::End:
        case DimensionKind::Nether:
            m_cellWidth = 8;
            m_cellHeight = 4;
            break;
        case DimensionKind::Overworld:
        default:
            m_cellWidth = 4;
            m_cellHeight = 8;
            break;
    }
}

// ============================================================================
// 结构生成
// ============================================================================

void NoiseChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "GenerateStructureStarts", "x", chunk.x(), "z", chunk.z());

    if (!m_structureManager) {
        chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 遍历所有已注册的结构，检查是否应该在此区块生成
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL + m_seed);

    for (const auto* structure : world::gen::structure::StructureRegistry::getAll()) {
        if (!structure) continue;

        // 检查是否应该在此位置生成结构
        if (m_structureManager->shouldGenerateStructureStart(*structure, chunkX, chunkZ)) {
            // 生成结构起点
            auto start = structure->generate(region, *this, rng, chunkX, chunkZ);

            if (start) {
                // 将结构起点存储到区块中
                chunk.addStructureStart(structure->name(), std::move(start));
            }
        }
    }

    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
}

void NoiseChunkGenerator::generateStructureReferences(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "GenerateStructureReferences", "x", chunk.x(), "z", chunk.z());

    // 结构引用阶段：计算结构之间的引用关系
    // 这主要用于结构之间的连接（如要塞、村庄道路等）

    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_REFERENCES);
}

// ============================================================================
// 生物群系生成
// ============================================================================

void NoiseChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "GenerateBiomes", "x", chunk.x(), "z", chunk.z());
    (void)region;

    MC_ASSERT_RELEASE(m_randomState != nullptr);

    BiomeContainer& biomes = chunk.getBiomes();
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 使用 NoiseChunk 的缓存气候采样器填充生物群系
    // NoiseChunk.cachedClimateSampler() 使用经过 mapAll 包装的密度函数，
    // 在插值上下文中采样时利用缓存和插值优化。
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;
    const i32 startBlockY = m_settings.noise.minY;
    const i32 cellCountY = math::floorDiv(m_settings.noise.height, m_cellHeight);
    auto& noiseChunk = chunk.getOrCreateNoiseChunk([&]() {
        auto nc = std::make_unique<world::gen::density::NoiseChunk>(
            m_randomState->createRouterCopy(), m_cellWidth, m_cellHeight, cellCountY, startX, startBlockY, startZ);
        return nc;
    });

    // 获取缓存气候采样器
    auto sampler = noiseChunk.cachedClimateSampler();

    // 获取 BiomeSource 的参数列表用于生物群系查找
    auto* multiNoiseSource = dynamic_cast<world::biome::source::MultiNoiseBiomeSource*>(m_biomeSource.get());
    if (multiNoiseSource != nullptr) {
        const auto& parameters = multiNoiseSource->parameters();
        constexpr i32 HORIZ_SIZE = 4;
        constexpr i32 VERT_SIZE = 4;
        constexpr i32 SECTION_COUNT = world::CHUNK_SECTIONS;

        for (i32 section = 0; section < SECTION_COUNT; ++section) {
            for (i32 y = 0; y < VERT_SIZE; ++y) {
                for (i32 z = 0; z < HORIZ_SIZE; ++z) {
                    for (i32 x = 0; x < HORIZ_SIZE; ++x) {
                        const i32 quartX = (chunkX * HORIZ_SIZE) + x;
                        const i32 quartY = (section * VERT_SIZE) + y + math::floorDiv(world::MIN_BUILD_HEIGHT, 4);
                        const i32 quartZ = (chunkZ * HORIZ_SIZE) + z;

                        const auto target = sampler.sample(quartX, quartY, quartZ);
                        const BiomeId biome = parameters.findValue(target);
                        biomes.setBiome(section, x, y, z, biome);
                    }
                }
            }
        }
    } else {
        // 非 MultiNoiseBiomeSource（如 EndBiomeSource），使用传统路径
        m_biomeSource->fillBiomeContainer(biomes, chunkX, chunkZ);
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::BIOMES);
}

// ============================================================================
// 噪声地形生成
// ============================================================================

void NoiseChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "GenerateNoise", "x", chunk.x(), "z", chunk.z());

    MC_ASSERT_RELEASE(m_randomState != nullptr);

    _generateNoiseWithDensityFunction(region, chunk);
}

// ============================================================================
// 地表生成
// ============================================================================

void NoiseChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "BuildSurface", "x", chunk.x(), "z", chunk.z());

    MC_ASSERT_RELEASE(m_randomState != nullptr);

    const auto getBiomeAt = [&region](i32 x, i32 y, i32 z) -> BiomeId { return region.getBiome(x, y, z); };
    // SurfaceRules.Context 直接持有 NoiseChunk 引用，
    // 通过 NoiseChunk.samplePreliminarySurfaceLevel() 查询预备表面高度
    // NoiseChunk 在 generateNoise 阶段已创建，此处直接获取
    auto* noiseChunkPtr = chunk.noiseChunk();
    if (noiseChunkPtr != nullptr) {
        m_randomState->surfaceSystem().buildSurface(chunk, getBiomeAt, *noiseChunkPtr);
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

// ============================================================================
// 雕刻和特性
// ============================================================================

void NoiseChunkGenerator::applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "ApplyCarvers", "x", chunk.x(), "z", chunk.z());
    const ChunkCoord targetChunkX = chunk.x();
    const ChunkCoord targetChunkZ = chunk.z();

    // MC 1.21.11: 单一雕刻阶段（无 LIQUID_CARVERS），含水层系统决定填充内容
    CarvingMask& carvingMask = chunk.carvingMask();

    // 从 ChunkPrimer 缓存的 NoiseChunk 获取 Aquifer
    world::gen::aquifer::Aquifer* aquifer = nullptr;
    if (chunk.hasNoiseChunk()) {
        aquifer = chunk.noiseChunk()->aquifer();
    }
    CarvingContext context(m_settings.noise.minY, m_settings.noise.height, aquifer);

    // MC 1.21.11: 按生物群系选择雕刻器
    // 遍历 [-8, +8] 范围内的起始区块坐标
    // 对于每个起始区块，采样其中心生物群系的雕刻器列表
    // 参考: NoiseBasedChunkGenerator.applyCarvers
    math::Random worldgenRandom;

    for (i32 dx = -8; dx <= 8; ++dx) {
        for (i32 dz = -8; dz <= 8; ++dz) {
            const ChunkCoord originChunkX = targetChunkX + dx;
            const ChunkCoord originChunkZ = targetChunkZ + dz;

            // 采样起始区块中心位置的四分位生物群系
            const i32 originBlockX = (originChunkX << 4) + 8;
            const i32 originBlockZ = (originChunkZ << 4) + 8;
            const BiomeId biomeId = m_biomeSource->getNoiseBiome(originBlockX >> 2, 64 >> 2, originBlockZ >> 2);
            const Biome& biome = m_biomeSource->getBiomeDefinition(biomeId);
            const BiomeGenerationSettings& biomeSettings = biome.generationSettings();

            // 遍历该生物群系的所有雕刻器
            const auto& carvers = biomeSettings.getCarvers();
            for (size_t carverIndex = 0; carverIndex < carvers.size(); ++carverIndex) {
                const auto& configuredCarver = carvers[carverIndex];
                if (!configuredCarver) {
                    continue;
                }

                // MC 1.21.11: 每个雕刻器使用 carverIndex 偏移的种子
                worldgenRandom.setLargeFeatureSeed(m_seed + static_cast<u64>(carverIndex), originChunkX, originChunkZ);

                if (configuredCarver->shouldCarve(worldgenRandom, originChunkX, originChunkZ)) {
                    configuredCarver->carve(chunk,
                        context,
                        *m_biomeSource,
                        targetChunkX,
                        targetChunkZ,
                        originChunkX,
                        originChunkZ,
                        carvingMask,
                        worldgenRandom);
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatuses::CARVERS);
}

void NoiseChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "PlaceFeatures", "x", chunk.x(), "z", chunk.z());

    // MC 1.21.11: 在 FEATURES 阶段开始前，从已有方块数据初始化 FINAL_HEIGHTMAPS
    // CARVERS 阶段切换到 FINAL_HEIGHTMAPS 后，需要从 NOISE + SURFACE 阶段的方块重新计算
    chunk.primeHeightmaps(HeightmapFlag::POST_FEATURES);

    // 初始化特征注册表（线程安全，仅初始化一次）
    static std::once_flag s_featureRegistryInitFlag;
    std::call_once(s_featureRegistryInitFlag, []() { FeatureRegistry::instance().initialize(); });

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // === 阶段 1: 放置结构片段 ===
    // 结构起点在 STRUCTURE_STARTS 阶段已经计算并存储到 chunk 中
    // 现在需要将结构片段放置到世界中
    if (m_structureManager && chunk.hasStructureStarts()) {
        MC_TRACE_EVENT("world.chunk_gen", "PlaceFeatures_Structures", "x", chunkX, "z", chunkZ);
        for (const auto& [structureName, start] : chunk.structureStarts()) {
            if (!start || !start->isValid()) {
                continue;
            }

            const world::gen::structure::Structure* structure =
                world::gen::structure::StructureRegistry::get(structureName);
            if (!structure) {
                continue;
            }

            // 放置结构片段到当前区块
            structure->placeInChunk(region, chunk, *start, chunkX, chunkZ);
        }
    }

    // === 阶段 2: 放置生物群系特征 ===
    // MC 1.21.11: 收集 3x3 区块邻域内所有 section biomes（BiomeGenerationSettings.getFeatures）
    // 这确保生物群系边界的特征也能被放置
    std::unordered_set<BiomeId> sectionBiomes;
    for (ChunkCoord dz = -1; dz <= 1; ++dz) {
        for (ChunkCoord dx = -1; dx <= 1; ++dx) {
            const IChunk* neighborChunk = region.getIChunk(chunkX + dx, chunkZ + dz);
            if (!neighborChunk) {
                // 当前区块不可用通过 region 时，回退到直接采样
                if (dx == 0 && dz == 0) {
                    for (i32 y = 0; y < world::CHUNK_SECTIONS; ++y) {
                        const i32 sectionY = y * world::CHUNK_SECTION_HEIGHT + world::MIN_BUILD_HEIGHT + 8;
                        for (i32 sz = 0; sz < 4; ++sz) {
                            for (i32 sx = 0; sx < 4; ++sx) {
                                sectionBiomes.insert(chunk.getBiomeAtBlock(sx * 4 + 2, sectionY, sz * 4 + 2));
                            }
                        }
                    }
                }
                continue;
            }

            // MC 1.21: 从每个邻域区块的 section biomes 中采样
            // section grid: 4x4x4 = 每个 section 占 4 个方块
            // 采样点: section 中心 (sx*4 + 2, sectionY*16 + minY + 8, sz*4 + 2)
            for (i32 y = 0; y < world::CHUNK_SECTIONS; ++y) {
                const i32 sectionY = y * world::CHUNK_SECTION_HEIGHT + world::MIN_BUILD_HEIGHT + 8;
                for (i32 sz = 0; sz < 4; ++sz) {
                    for (i32 sx = 0; sx < 4; ++sx) {
                        sectionBiomes.insert(neighborChunk->getBiomeAtBlock(sx * 4 + 2, sectionY, sz * 4 + 2));
                    }
                }
            }
        }
    }
    // 排序以确保确定性遍历顺序（MC 1.21: FeatureSorter 维护排序后的特征列表）
    std::vector<BiomeId> sortedBiomes(sectionBiomes.begin(), sectionBiomes.end());
    std::sort(sortedBiomes.begin(), sortedBiomes.end());

    // MC 1.21: 使用 setDecorationSeed / setFeatureSeed 计算确定性种子
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    math::Random worldgenRandom;
    const u64 decorSeed = worldgenRandom.setDecorationSeed(m_seed, startX, startZ);

    // 按装饰阶段顺序放置特征
    for (DecorationStage stage : DecorationStages::getAll()) {
        const i32 stageOrdinal = static_cast<i32>(stage);

        // MC 1.21: 对每个 stage，收集所有生物群系的特征并去重
        // 使用 map 保持 featureIndex → feature 的有序映射
        std::map<u32, ConfiguredFeatureBase*> stageFeatures;
        FeatureRegistry& registry = FeatureRegistry::instance();
        const auto& allFeatures = registry.getFeatures(stage);

        for (BiomeId biomeId : sortedBiomes) {
            const Biome& biome = m_biomeSource->getBiomeDefinition(biomeId);
            const BiomeGenerationSettings& biomeSettings = biome.generationSettings();
            const auto& featureIds = biomeSettings.getFeatures(stage);
            for (u32 fid : featureIds) {
                if (fid < allFeatures.size() && allFeatures[fid] != nullptr) {
                    // 去重：同一 feature ID 只添加一次
                    stageFeatures.emplace(fid, allFeatures[fid]);
                }
            }
        }

        // MC 1.21: 按排序后的 feature ID 顺序放置
        i32 featureIndex = 0;
        for (const auto& [fid, feature] : stageFeatures) {
            worldgenRandom.setFeatureSeed(decorSeed, featureIndex, stageOrdinal);

            const BlockPos chunkOrigin(startX, 0, startZ);
            feature->place(region, chunk, *this, worldgenRandom, chunkOrigin);
            ++featureIndex;
        }
    }

    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId NoiseChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    return m_biomeSource->getNoiseBiome(math::floorDiv(x, 4), math::floorDiv(y, 4), math::floorDiv(z, 4));
}

BiomeId NoiseChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    return m_biomeSource->getNoiseBiome(noiseX, noiseY, noiseZ);
}

i32 NoiseChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    MC_ASSERT_RELEASE(m_randomState != nullptr);

    // iterateNoiseColumn — 创建单列 NoiseChunk 采样高度
    // 与直接逐方块采样相比，cell 插值方式与实际区块生成管线完全一致
    const NoiseSettings& noise = m_settings.noise;
    const i32 minY = noise.minY;
    const i32 cellHeight = m_cellHeight;
    const i32 cellWidth = m_cellWidth;
    const i32 cellCountY = math::floorDiv(noise.height, cellHeight);

    // 对齐坐标到 cell 网格
    const i32 cellX = math::floorDiv(x, cellWidth);
    const i32 cellZ = math::floorDiv(z, cellWidth);
    const i32 alignedX = cellX * cellWidth;
    const i32 alignedZ = cellZ * cellWidth;
    const f64 deltaX = static_cast<f64>(x - alignedX) / static_cast<f64>(cellWidth);
    const f64 deltaZ = static_cast<f64>(z - alignedZ) / static_cast<f64>(cellWidth);

    // 创建单列 NoiseChunk（cellCountXZ=1）
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(
        m_randomState->createRouterCopy(), cellWidth, cellHeight, cellCountY, alignedX, minY, alignedZ);

    // 设置 DisabledAquiferFiller（高度查询不需要实际含水层计算，
    // 但需要正确判断海平面以下的流体方块）
    {
        std::vector<std::unique_ptr<world::gen::density::BlockStateFiller>> fillers;
        fillers.push_back(
            std::make_unique<world::gen::density::DisabledAquiferFiller>(m_settings.defaultFluid, m_settings.seaLevel));
        noiseChunk->setBlockStateRule(std::make_unique<world::gen::density::MaterialRuleList>(std::move(fillers)));
    }

    noiseChunk->initializeForFirstCellX();
    noiseChunk->advanceCellX(0);

    auto matchesHeightmap = [type](const BlockState* state) -> bool {
        if (!state || state->isAir()) {
            return false;
        }
        const Block& block = state->owner();
        switch (type) {
            case HeightmapType::WorldSurface:
            case HeightmapType::WorldSurfaceWG:
                return true;
            case HeightmapType::OceanFloor:
            case HeightmapType::OceanFloorWG:
                return block.isSolid(*state);
            case HeightmapType::MotionBlocking:
                return block.isSolid(*state) || state->isLiquid();
            case HeightmapType::MotionBlockingNoLeaves:
                return (block.isSolid(*state) || state->isLiquid()) && (&block.material() != &Material::LEAVES) &&
                    (&block.material() != &Material::PLANT);
            case HeightmapType::LightBlocking:
                return block.isSolid(*state) && state->getOpacity() > 0;
            default:
                return true;
        }
    };

    for (i32 cellY = cellCountY - 1; cellY >= 0; --cellY) {
        noiseChunk->selectCellXYZ(0, cellY, 0);

        for (i32 inCellY = cellHeight - 1; inCellY >= 0; --inCellY) {
            const i32 blockY = (math::floorDiv(minY, cellHeight) + cellY) * cellHeight + inCellY;
            const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellHeight);
            noiseChunk->updateForY(yLerp);
            noiseChunk->updateForX(deltaX);

            const f64 density = noiseChunk->updateForZ(deltaZ);
            noiseChunk->setBlockPos(x, blockY, z);

            // 使用 BlockStateFiller 链确定方块状态
            const BlockState* blockState = noiseChunk->getInterpolatedState(density);
            if (blockState == nullptr && density > 0.0) {
                blockState = m_settings.defaultBlock;
            }

            if (matchesHeightmap(blockState)) {
                return blockY + 1;
            }
        }
    }

    return minY;
}

i32 NoiseChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    MC_TRACE_EVENT("world.chunk_gen", "NoiseChunkGenerator::spawnInitialMobs", "x", chunk.x(), "z", chunk.z());

    // 使用 WorldGenSpawner 放置被动动物
    if (!m_worldGenSpawner || !m_worldGenSpawner->isEnabled()) {
        spdlog::warn("[NoiseChunkGenerator] WorldGenSpawner is not enabled. Skipping initial mob spawning.");
        return 0;
    }

    // 获取区块中心位置的生物群系
    const BiomeId biomeId = chunk.getBiomeAtBlock(8, 64, 8);
    const Biome& biome = m_biomeSource->getBiomeDefinition(biomeId);

    // 使用种子创建随机数生成器
    // 参考 MC: setDecorationSeed
    math::Random rng;
    rng.setSeed(static_cast<u64>(chunk.x()) * 341873128712ULL + static_cast<u64>(chunk.z()) * 132897987541ULL + m_seed);

    return m_worldGenSpawner->spawnInitialMobs(region, biome, chunk.x(), chunk.z(), *this, rng, outEntities);
}

void NoiseChunkGenerator::_generateNoiseWithDensityFunction(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "GenerateNoise_DF", "x", chunk.x(), "z", chunk.z());
    (void)region;

    if (!m_randomState) {
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    // MC 1.21: 通过 ChunkPrimer 缓存 NoiseChunk，确保 biomes/noise/surface/carvers 阶段共享
    const i32 startBlockY = m_settings.noise.minY;
    const i32 cellCountY = math::floorDiv(m_settings.noise.height, m_cellHeight);
    auto& noiseChunk = chunk.getOrCreateNoiseChunk([&]() {
        // MC 1.21: NoiseChunk 拥有自己的路由器副本，mapAll() 会将 Marker 替换为区块特定实现
        auto nc = std::make_unique<world::gen::density::NoiseChunk>(
            m_randomState->createRouterCopy(), m_cellWidth, m_cellHeight, cellCountY, startX, startBlockY, startZ);

        // MC 1.21: 创建含水层采样器
        {
            // 使用 RandomState 中的 aquiferRandom（与 MC 1.21 RandomState.aquiferRandom() 对应）
            auto positionalRandom = std::make_unique<math::PositionalRandomFactory>(
                m_randomState->aquiferRandom().seedLo(), m_randomState->aquiferRandom().seedHi());

            // 根据维度选择 FluidPicker
            world::gen::aquifer::FluidPicker fluidPicker;
            switch (m_settings.dimensionKind) {
                case DimensionKind::Nether:
                    fluidPicker = world::gen::aquifer::createNetherFluidPicker();
                    break;
                case DimensionKind::End:
                    fluidPicker = world::gen::aquifer::createEndFluidPicker();
                    break;
                case DimensionKind::Overworld:
                default:
                    fluidPicker =
                        world::gen::aquifer::createOverworldFluidPicker(m_settings.seaLevel, m_settings.defaultFluid);
                    break;
            }

            if (m_settings.noise.aquifersEnabled) {
                auto aquifer = world::gen::aquifer::Aquifer::createNoiseBased(*nc,
                    chunkX,
                    chunkZ,
                    nc->router(),
                    *positionalRandom,
                    m_settings.noise.minY,
                    m_settings.noise.height,
                    std::move(fluidPicker));

                // MC 1.21: 构建 BlockStateFiller 链
                // AquiferFiller: 传入密度值，aquifer 确定流体/空气
                auto* aquiferPtr = aquifer.get();
                nc->setAquifer(std::move(aquifer));

                std::vector<std::unique_ptr<world::gen::density::BlockStateFiller>> fillers;
                fillers.push_back(std::make_unique<world::gen::density::AquiferFiller>(*aquiferPtr));
                nc->setBlockStateRule(std::make_unique<world::gen::density::MaterialRuleList>(std::move(fillers)));
            } else {
                nc->setAquifer(world::gen::aquifer::Aquifer::createDisabled(std::move(fluidPicker)));

                // 禁用含水层时: density > 0 → nullptr(density > 0 → defaultBlock outside), density <= 0 → check fluid
                std::vector<std::unique_ptr<world::gen::density::BlockStateFiller>> fillers;
                fillers.push_back(std::make_unique<world::gen::density::DisabledAquiferFiller>(
                    m_settings.defaultFluid, m_settings.seaLevel));
                nc->setBlockStateRule(std::make_unique<world::gen::density::MaterialRuleList>(std::move(fillers)));
            }
        }
        return nc;
    });

    // MC 1.21: 构建 Beardifier 用于结构地形平滑
    const auto beardifier = _buildBeardifier(chunk);

    const auto& cellConfig = noiseChunk.cellConfig();
    noiseChunk.initializeForFirstCellX();

    for (i32 cellX = 0; cellX < cellConfig.cellCountXZ; ++cellX) {
        noiseChunk.advanceCellX(cellX);

        for (i32 cellZ = 0; cellZ < cellConfig.cellCountXZ; ++cellZ) {
            for (i32 cellY = cellConfig.cellCountY - 1; cellY >= 0; --cellY) {
                noiseChunk.selectCellXYZ(cellX, cellY, cellZ);

                for (i32 inCellY = cellConfig.cellHeight - 1; inCellY >= 0; --inCellY) {
                    const i32 blockY = (noiseChunk.firstCellY() + cellY) * cellConfig.cellHeight + inCellY;
                    const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellConfig.cellHeight);
                    noiseChunk.updateForY(yLerp);

                    for (i32 inCellX = 0; inCellX < cellConfig.cellWidth; ++inCellX) {
                        const i32 localX = cellX * cellConfig.cellWidth + inCellX;
                        const i32 blockX = startX + localX;
                        const f64 xLerp = static_cast<f64>(inCellX) / static_cast<f64>(cellConfig.cellWidth);
                        noiseChunk.updateForX(xLerp);

                        for (i32 inCellZ = 0; inCellZ < cellConfig.cellWidth; ++inCellZ) {
                            const i32 localZ = cellZ * cellConfig.cellWidth + inCellZ;
                            const i32 blockZ = startZ + localZ;
                            const f64 zLerp = static_cast<f64>(inCellZ) / static_cast<f64>(cellConfig.cellWidth);

                            noiseChunk.setBlockPos(blockX, blockY, blockZ);
                            noiseChunk.setInCellPos(inCellX, inCellY, inCellZ);

                            const f64 rawDensity = noiseChunk.updateForZ(zLerp);
                            const f64 density = rawDensity + beardifier.compute(blockX, blockY, blockZ);

                            // MC 1.21: 通过 BlockStateFiller 链确定方块状态
                            // AquiferFiller 在 density <= 0 时返回流体/空气，density > 0 返回 nullptr
                            const BlockState* blockState = noiseChunk.getInterpolatedState(density);
                            if (blockState == nullptr && density > 0.0) {
                                blockState = m_settings.defaultBlock;
                            }

                            if (blockState != nullptr) {
                                chunk.setBlockState(localX, blockY, localZ, blockState);
                                chunk.updateHeightmap(
                                    HeightmapType::WorldSurfaceWG, localX, blockY, localZ, blockState);
                                if (blockState->isSolid()) {
                                    chunk.updateHeightmap(
                                        HeightmapType::OceanFloorWG, localX, blockY, localZ, blockState);
                                }
                            }
                        }
                    }
                }
            }
        }

        noiseChunk.swapSlices();
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

// ============================================================================
// Beardifier
// ============================================================================

world::gen::density::Beardifier NoiseChunkGenerator::_buildBeardifier(ChunkPrimer& chunk) const
{
    MC_TRACE_EVENT("world.chunk_gen", "BuildBeardifier", "x", chunk.x(), "z", chunk.z());

    std::vector<world::gen::density::Beardifier::Rigid> pieces;
    std::vector<world::gen::jigsaw::JigsawJunction> junctions;

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    for (const auto& [structureName, start] : chunk.structureStarts()) {
        if (!start || !start->isValid()) {
            continue;
        }

        // 获取结构的地形适配类型
        const auto* structure = world::gen::structure::StructureRegistry::get(structureName);
        const auto terrainAdaptation =
            (structure != nullptr) ? structure->terrainAdaptation() : TerrainAdaptation::None;

        for (const auto& piece : start->pieces()) {
            if (!piece) {
                continue;
            }

            // 检查片段是否在区块附近（Beardifier.BEARD_KERNEL_RADIUS = 12 格范围）
            const auto& box = piece->getBoundingBox();
            if (box.maxX() < startX - 12 || box.minX() > startX + world::CHUNK_WIDTH - 1 + 12 ||
                box.maxZ() < startZ - 12 || box.minZ() > startZ + world::CHUNK_WIDTH - 1 + 12) {
                continue;
            }

            // MC 1.21: 只有 TerrainAdaptation != None 的结构才影响地形
            if (terrainAdaptation != TerrainAdaptation::None) {
                pieces.push_back(
                    world::gen::density::Beardifier::Rigid{box, terrainAdaptation, piece->getGroundLevelDelta()});
            }

            // 收集 JigsawJunction（仅 Jigsaw 片段）
            if (piece->isJigsawPiece()) {
                for (const auto& junction : piece->getJunctions()) {
                    const i32 jx = junction.getSourceX();
                    const i32 jz = junction.getSourceZ();
                    if (jx > startX - 12 && jx < startX + world::CHUNK_WIDTH - 1 + 12 && jz > startZ - 12 &&
                        jz < startZ + world::CHUNK_WIDTH - 1 + 12) {
                        junctions.push_back(junction);
                    }
                }
            }
        }
    }

    return world::gen::density::Beardifier(std::move(pieces), std::move(junctions));
}

} // namespace mc
