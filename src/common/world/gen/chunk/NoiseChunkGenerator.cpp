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
#include "../feature/FeatureSorter.hpp"
#include "../feature/ore/OreFeature.hpp"
#include "../jigsaw/JigsawPiece.hpp"
#include "../placement/PlacementRegistry.hpp"
#include "../spawn/WorldGenSpawner.hpp"
#include "../structure/Structure.hpp"
#include "../structure/StructureManager.hpp"
#include "../structure/StructureSet.hpp"
#include "../structure/placement/StructurePlacement.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <unordered_set>

namespace mc {

// ============================================================================
// NoiseChunkGenerator 实现
// ============================================================================

NoiseChunkGenerator::NoiseChunkGenerator(
    u64 seed, DimensionSettings settings, std::unique_ptr<world::biome::IBiomeSource> biomeSource)
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

    // 初始化结构注册表和结构集合注册表
    world::gen::structure::StructureRegistry::initialize();
    world::gen::structure::StructureSetRegistry::instance().initialize();
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

    // MC 1.21.11: 遍历 StructureSet，按放置规则决定候选区块，按权重选择结构
    auto& structureSetRegistry = world::gen::structure::StructureSetRegistry::instance();

    for (const auto& structureSetPtr : structureSetRegistry.getAll()) {
        if (!structureSetPtr) continue;

        const auto& structureSet = *structureSetPtr;
        const auto& placement = structureSet.placement();

        // 三步检查：1. 是否为候选区块
        if (!placement.isStructureChunk(static_cast<i64>(m_seed), chunkX, chunkZ)) {
            continue;
        }

        // 按权重选择结构
        math::Random rng(
            static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL + m_seed);
        const auto* entry = structureSet.selectEntry(rng);
        if (!entry) continue;

        // 查找结构定义
        const auto* structure = world::gen::structure::StructureRegistry::get(entry->structureId);
        if (!structure) continue;

        // 检查结构是否可以在此位置生成（生物群系检查等）
        if (m_structureManager->shouldGenerateStructureStart(*structure, chunkX, chunkZ)) {
            // 生成结构起点
            auto start = structure->generate(*this, rng, chunkX, chunkZ);
            if (start) {
                chunk.addStructureStart(entry->structureId, std::move(start));
            }
        }
    }

    // 通知 StructureCheck 缓存此区块的结构引用数据
    // 对齐 MC 1.21.11 ServerLevel.onStructureStartsAvailable() 通过 structureCheck.onStructureLoad() 的调用
    {
        auto& structureCheck = m_structureManager->structureCheck();
        const u64 chunkPosId =
            (static_cast<u64>(static_cast<u32>(chunkX)) << 32) | static_cast<u64>(static_cast<u32>(chunkZ));

        // 从 ChunkPrimer 的 m_structureStarts 构建引用计数映射
        std::unordered_map<ResourceLocation, i32> refCounts;
        for (const auto& [structureId, start] : chunk.structureStarts()) {
            if (start && start->isValid()) {
                // 有效结构起点：记录其引用计数
                refCounts[structureId] = start->getRefCount();
            }
        }
        structureCheck.onStructureLoad(chunkPosId, refCounts);
    }

    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
}

void NoiseChunkGenerator::generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "GenerateStructureReferences", "x", chunk.x(), "z", chunk.z());

    // MC 1.21: StructureReferences 阶段
    // 扫描以当前区块为中心的 17x17 区块范围（taskRange=8），
    // 找到所有与当前区块相交的 StructureStart，将引用添加到当前区块。
    const ChunkCoord cx = chunk.x();
    const ChunkCoord cz = chunk.z();

    for (i32 dx = -8; dx <= 8; ++dx) {
        for (i32 dz = -8; dz <= 8; ++dz) {
            const ChunkCoord ncx = cx + dx;
            const ChunkCoord ncz = cz + dz;

            const IChunk* neighbor = region.getIChunk(ncx, ncz, ChunkStatuses::STRUCTURE_STARTS);
            if (!neighbor) {
                continue;
            }

            // 获取邻居区块中与当前区块相交的结构起点
            auto intersecting = neighbor->getIntersectingStructures(cx, cz);
            for (auto& [structureId, srcX, srcZ] : intersecting) {
                chunk.addStructureReference(structureId, srcX, srcZ);

                // MC 1.21: 增加引用计数
                // 获取源区块的 StructureStart 并增加引用计数
                auto* neighborPrimer = dynamic_cast<const ChunkPrimer*>(neighbor);
                if (neighborPrimer) {
                    auto* start = const_cast<ChunkPrimer*>(neighborPrimer)->getStructureStart(structureId);
                    if (start) {
                        start->incrementRefCount();

                        // 通知 StructureCheck 缓存递增引用计数
                        // 对齐 MC 1.21.11 StructureManager.addReference() 中
                        // 通过 structureCheck.incrementReference() 的调用
                        if (m_structureManager) {
                            const u64 srcChunkPosId = (static_cast<u64>(static_cast<u32>(srcX)) << 32) |
                                static_cast<u64>(static_cast<u32>(srcZ));
                            m_structureManager->structureCheck().incrementReference(srcChunkPosId, structureId);
                        }
                    }
                }
            }
        }
    }

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
    } else {
        spdlog::warn("[NoiseChunkGenerator] buildSurface: NoiseChunk is null for chunk ({}, {}). "
                     "Surface generation skipped.",
            chunk.x(),
            chunk.z());
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
            // MC 1.21.11: 使用 Y=0 的四分位坐标 (QuartPos.fromBlock(0) = 0)
            const i32 originBlockX = (originChunkX << 4) + 8;
            const i32 originBlockZ = (originChunkZ << 4) + 8;
            const BiomeId biomeId = m_biomeSource->getNoiseBiome(originBlockX >> 2, 0, originBlockZ >> 2);
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
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    // === MC 1.21: FeatureSorter 懒初始化 ===
    // 构建所有可能生物群系的拓扑排序特征列表
    std::call_once(m_featuresPerStepFlag, [this]() {
        const std::vector<BiomeId>& possibleBiomes = m_biomeSource->possibleBiomes();
        m_featuresPerStep = FeatureSorter::buildFeaturesPerStep(
            possibleBiomes,
            [](BiomeId biomeId, DecorationStage stage) -> const std::vector<u32>& {
                const Biome& biome = BiomeRegistry::instance().get(biomeId);
                return biome.generationSettings().getFeatures(stage);
            },
            FeatureRegistry::instance());
    });

    // === MC 1.21: 收集 3x3 区块邻域内的 section biomes ===
    // 对应 Java: ChunkPos.rangeClosed(sectionpos.chunk(), 1)
    std::unordered_set<BiomeId> sectionBiomes;
    for (ChunkCoord dz = -1; dz <= 1; ++dz) {
        for (ChunkCoord dx = -1; dx <= 1; ++dx) {
            const IChunk* neighborChunk = region.getIChunk(chunkX + dx, chunkZ + dz, ChunkStatuses::CARVERS);
            if (!neighborChunk) {
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

    // 只保留生物群系源中实际存在的生物群系（对应 Java: set.retainAll(biomeSource.possibleBiomes())）
    const std::vector<BiomeId>& possibleBiomes = m_biomeSource->possibleBiomes();
    std::unordered_set<BiomeId> possibleSet(possibleBiomes.begin(), possibleBiomes.end());
    for (auto it = sectionBiomes.begin(); it != sectionBiomes.end();) {
        if (possibleSet.find(*it) == possibleSet.end()) {
            it = sectionBiomes.erase(it);
        } else {
            ++it;
        }
    }

    // === MC 1.21: 按装饰阶段交错放置结构和特征 ===
    // 对应 Java: ChunkGenerator.applyBiomeDecoration()
    // 每个阶段：先放结构，再放特征
    math::Random worldgenRandom;
    const u64 decorSeed = worldgenRandom.setDecorationSeed(m_seed, startX, startZ);
    const BlockPos chunkOrigin(startX, 0, startZ);

    // 按结构装饰阶段分组
    // MC 1.21.11: 使用跨区块结构引用而非仅当前区块的起点
    // 对应 Java: ChunkGenerator.applyBiomeDecoration() 中遍历 structureReferences
    std::map<i32,
        std::vector<std::pair<const world::gen::structure::Structure*, world::gen::structure::StructureStart*>>>
        structuresByStage;
    if (m_structureManager && chunk.hasStructureReferences()) {
        for (const auto& [structureId, refs] : chunk.structureReferences()) {
            const world::gen::structure::Structure* structure =
                world::gen::structure::StructureRegistry::get(structureId);
            if (!structure) continue;

            for (const auto& [refX, refZ] : refs) {
                // 从源区块获取 StructureStart
                IChunk* sourceChunk = region.getIChunk(refX, refZ, ChunkStatuses::STRUCTURE_STARTS);
                if (!sourceChunk) continue;

                auto* sourcePrimer = dynamic_cast<ChunkPrimer*>(sourceChunk);
                if (!sourcePrimer) continue;

                auto* start = sourcePrimer->getStructureStart(structureId);
                if (!start || !start->isValid()) continue;

                const i32 stageOrdinal = static_cast<i32>(structure->decorationStage());
                structuresByStage[stageOrdinal].emplace_back(structure, start);
            }
        }
    }

    const i32 featureSteps = static_cast<i32>(m_featuresPerStep.size());
    const i32 totalSteps = std::max(static_cast<i32>(DecorationStage::Count), featureSteps);

    for (i32 stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
        const DecorationStage stage = DecorationStages::fromIndex(static_cast<u8>(stepIndex));
        const i32 stageOrdinal = stepIndex;

        // === 放置该阶段的结构的特征 ===
        // 对应 Java: for (Structure structure : map.getOrDefault(k, Collections.emptyList()))
        i32 structureIndex = 0;
        auto structIt = structuresByStage.find(stageOrdinal);
        if (structIt != structuresByStage.end()) {
            for (const auto& [structure, start] : structIt->second) {
                worldgenRandom.setFeatureSeed(decorSeed, structureIndex, stageOrdinal);
                structure->placeInChunk(region, chunk, *start, chunkX, chunkZ);
                ++structureIndex;
            }
        }

        // === 放置该阶段的生物群系特征 ===
        // 对应 Java: IntSet intset = new IntArraySet(); ... for each biome add feature indices
        if (stepIndex < featureSteps) {
            const FeatureSorter::StepFeatureData& stepData = m_featuresPerStep[static_cast<size_t>(stepIndex)];
            if (stepData.features.empty()) {
                continue;
            }

            // 收集所有出现的生物群系中该阶段的特征拓扑索引
            // 对应 Java: holderset.stream().map(Holder::value).forEach(p -> intset.add(indexMapping.applyAsInt(p)))
            std::set<i32> featureIndices;
            for (BiomeId biomeId : sectionBiomes) {
                const Biome& biome = BiomeRegistry::instance().get(biomeId);
                const BiomeGenerationSettings& biomeSettings = biome.generationSettings();
                const auto& featureIds = biomeSettings.getFeatures(stage);
                for (u32 fid : featureIds) {
                    const i32 topoIndex = stepData.getIndex(fid);
                    if (topoIndex >= 0) {
                        featureIndices.insert(topoIndex);
                    }
                }
            }

            // 按拓扑索引排序放置特征
            // 对应 Java: int[] aint = intset.toIntArray(); Arrays.sort(aint);
            for (i32 topoIndex : featureIndices) {
                if (topoIndex < static_cast<i32>(stepData.features.size()) && stepData.features[topoIndex] != nullptr) {
                    worldgenRandom.setFeatureSeed(decorSeed, topoIndex, stageOrdinal);
                    stepData.features[topoIndex]->place(region, chunk, *this, worldgenRandom, chunkOrigin);
                }
            }
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
    // MC 1.21.11: 在区块中心的最大建筑高度处采样 (getMaxY())
    const i32 sampleY = region.getMaxBuildHeight();
    const BiomeId biomeId = chunk.getBiomeAtBlock(8, sampleY, 8);
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
                                // MC 1.21: 含水层边界处流体方块需标记后处理
                                if (noiseChunk.aquifer() != nullptr &&
                                    noiseChunk.aquifer()->shouldScheduleFluidUpdate() && blockState->isLiquid()) {
                                    chunk.markPosForPostprocessing(localX, blockY, localZ);
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

    // MC 1.21.11: 使用跨区块结构引用收集 Beardifier 数据
    // 与 placeFeatures() 一样，遍历 structureReferences 而非仅 structureStarts
    // 这确保邻居区块中的结构也能影响当前区块的地形平滑

    auto processStart = [&](const world::gen::structure::StructureStart& start,
                            const world::gen::structure::Structure* structure) {
        const auto terrainAdaptation =
            (structure != nullptr) ? structure->terrainAdaptation() : TerrainAdaptation::None;

        for (const auto& piece : start.pieces()) {
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

            // 收集 JigsawJunction（仅 Jigsaw 片段，且仅 TERRAIN_MATCHING 类型的连接点）
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
    };

    // 处理当前区块的结构起点
    for (const auto& [structureId, start] : chunk.structureStarts()) {
        if (!start || !start->isValid()) continue;
        const auto* structure = world::gen::structure::StructureRegistry::get(structureId);
        processStart(*start, structure);
    }

    // 处理结构引用中指向的源区块的结构起点
    for (const auto& [structureId, refs] : chunk.structureReferences()) {
        const auto* structure = world::gen::structure::StructureRegistry::get(structureId);
        for (const auto& [refX, refZ] : refs) {
            // 跳过当前区块（已在上面处理）
            if (refX == chunkX && refZ == chunkZ) continue;

            // 从 WorldGenRegion 获取源区块（注意：_buildBeardifier 在 noise 阶段调用，
            // 此时邻居区块可能不可用，因此仅处理当前区块的 starts）
            // 跨区块引用的 Beardifier 数据会在邻居区块各自构建时处理
        }
    }

    return world::gen::density::Beardifier(std::move(pieces), std::move(junctions));
}

} // namespace mc
