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
 * The above copyright notice shall this permission notice shall be included in all
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

#include "FlatChunkGenerator.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/ConfiguredFeatureRegistry.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/FeatureSorter.hpp"
#include "common/world/gen/placement/PlacedFeatureRegistry.hpp"
#include "common/world/gen/placement/PlacementRegistry.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include "common/world/gen/structure/placement/StructurePlacement.hpp"
#include "perfetto/TraceEvents.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>

namespace mc {

FlatChunkGenerator::FlatChunkGenerator(u64 seed, FlatLevelGeneratorSettings settings)
    : BaseChunkGenerator(seed, DimensionSettings::flat())
    , m_flatSettings(std::move(settings))
    , m_biomeSource(std::make_unique<world::biome::source::FixedBiomeSource>(seed, m_flatSettings.biomeId()))
{
    // 设置默认生物群系，让 BaseChunkGenerator::generateBiomes 使用
    m_defaultBiome = m_flatSettings.biomeId();

    // 初始化结构与放置器注册表
    _initGenerationRegistries();
}

FlatChunkGenerator::~FlatChunkGenerator() = default;

void FlatChunkGenerator::clearStructureCache()
{
    if (m_structureManager) {
        m_structureManager->clearCache();
    }
}

// ============================================================================
// 结构生成注册表初始化
// ============================================================================

void FlatChunkGenerator::_initGenerationRegistries()
{
    std::call_once(m_generationRegistriesFlag, [this]() {
        MC_TRACE_EVENT("server.initialization", "FlatChunkGenerator::initGenerationRegistries");

        // 初始化结构注册表和结构集合注册表
        world::gen::structure::StructureRegistry::initialize();
        world::gen::structure::StructureSetRegistry::instance().initialize();
        m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(m_seed));

        // 初始化放置器注册表
        PlacementRegistry::instance().initialize();
    });
}

// ============================================================================
// 结构生成
// ============================================================================

void FlatChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "FlatGenerateStructureStarts", "x", chunk.x(), "z", chunk.z());

    if (!m_structureManager) {
        chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
        return;
    }

    // MC 1.21.11: FlatChunkGenerator 根据 structureOverrides 过滤结构集
    // 如果 structureOverrides 为空，则不生成任何结构
    // 如果指定了 structureOverrides，则只生成指定的结构集（受生物群系兼容性过滤）
    const auto& overrides = m_flatSettings.structureOverrides();
    if (overrides.empty()) {
        chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
        return;
    }

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 构建 structureOverrides 的快速查找集合
    std::unordered_set<ResourceLocation> overrideSet(overrides.begin(), overrides.end());

    // MC 1.21.11: 遍历 StructureSet，按放置规则决定候选区块，按权重选择结构
    // 参考: ChunkGenerator.createStructures() + FlatLevelSource.createState()
    auto& structureSetRegistry = world::gen::structure::StructureSetRegistry::instance();

    for (const auto& structureSetPtr : structureSetRegistry.getAll()) {
        if (!structureSetPtr) continue;

        const auto& structureSet = *structureSetPtr;

        // MC 1.21.11: 如果指定了 structureOverrides，只处理覆盖列表中的结构集
        if (overrideSet.find(structureSet.id()) == overrideSet.end()) {
            continue;
        }

        // MC 1.21.11: 检查结构集是否与平坦世界的生物群系兼容
        // 参考: ChunkGeneratorStructureState.createForFlat() 中的 hasBiomesForStructureSet 过滤
        if (!_hasBiomesForStructureSet(structureSet)) {
            continue;
        }

        const auto& placement = structureSet.placement();

        // MC 1.21.11: 检查是否已有同 StructureSet 中的有效 StructureStart
        bool hasExistingStart = false;
        for (const auto& entry : structureSet.entries()) {
            auto* existingStart = chunk.getStructureStart(entry.structureId);
            if (existingStart && existingStart->isValid()) {
                hasExistingStart = true;
                break;
            }
        }
        if (hasExistingStart) continue;

        // 三步检查：1. 是否为候选区块
        if (!placement.isStructureChunk(static_cast<i64>(m_seed), chunkX, chunkZ)) {
            continue;
        }

        // 按权重选择结构
        math::JavaLegacyRandom legacyRng;
        legacyRng.setLargeFeatureSeed(static_cast<i64>(m_seed), chunkX, chunkZ);
        math::Random rng(legacyRng.nextLong());
        const auto* entry = structureSet.selectEntry(rng);
        if (!entry) continue;

        // 查找结构定义
        const auto* structure = world::gen::structure::StructureRegistry::get(entry->structureId);
        if (!structure) continue;

        // 生成结构起点（生物群系兼容性已由 _hasBiomesForStructureSet 预过滤保证，
        // FlatChunkGenerator 使用 FixedBiomeSource，所有位置生物群系相同，无需逐区块检查）
        auto start = structure->generate(*this, rng, chunkX, chunkZ);
        if (start) {
            chunk.addStructureStart(entry->structureId, std::move(start));
        }
    }

    // 通知 StructureCheck 缓存此区块的结构引用数据
    {
        auto& structureCheck = m_structureManager->structureCheck();
        const u64 chunkPosId =
            (static_cast<u64>(static_cast<u32>(chunkX)) << 32) | static_cast<u64>(static_cast<u32>(chunkZ));

        std::unordered_map<ResourceLocation, i32> refCounts;
        for (const auto& [structureId, start] : chunk.structureStarts()) {
            if (start && start->isValid()) {
                refCounts[structureId] = start->getRefCount();
            }
        }
        structureCheck.onStructureLoad(chunkPosId, refCounts);
    }

    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
}

void FlatChunkGenerator::generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "FlatGenerateStructureReferences", "x", chunk.x(), "z", chunk.z());

    // MC 1.21: StructureReferences 阶段
    // 扫描以当前区块为中心的 17x17 区块范围（taskRange=8），
    // 找到所有与当前区块相交的 StructureStart，将引用添加到当前区块。
    // 此逻辑与 NoiseChunkGenerator 完全一致，因为结构引用阶段
    // 不依赖于区块生成器类型，只依赖于已生成的结构起点。
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
                auto* neighborPrimer = dynamic_cast<const ChunkPrimer*>(neighbor);
                if (neighborPrimer) {
                    auto* start = const_cast<ChunkPrimer*>(neighborPrimer)->getStructureStart(structureId);
                    if (start) {
                        start->incrementRefCount();

                        // 通知 StructureCheck 缓存递增引用计数
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
// 区块生成接口
// ============================================================================

void FlatChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_UNUSED(region);
    // 逐层填充方块，null 条目跳过（由特性系统放置）
    const auto& layers = m_flatSettings.layers();
    const i32 minY = getMinY();
    const i32 genDepth = getGenDepth();
    const i32 fillHeight = std::min(genDepth, static_cast<i32>(layers.size()));

    for (i32 i = 0; i < fillHeight; ++i) {
        const BlockState* blockState = layers[static_cast<size_t>(i)];
        if (blockState == nullptr) {
            // null 条目跳过，由 FILL_LAYER 特性放置（如水层）
            continue;
        }

        const i32 worldY = minY + i;
        for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
            for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
                chunk.setBlockState(localX, worldY, localZ, blockState);
                chunk.updateHeightmap(HeightmapType::WorldSurfaceWG, localX, worldY, localZ, blockState);
                chunk.updateHeightmap(HeightmapType::OceanFloorWG, localX, worldY, localZ, blockState);
            }
        }
    }

    MC_UNUSED(region);
    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

void FlatChunkGenerator::buildSurface(WorldGenRegion& /*region*/, ChunkPrimer& chunk)
{
    // 平坦世界不需要地表生成，由 SurfaceRules 处理但平坦世界不需要
    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

void FlatChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "FlatPlaceFeatures", "x", chunk.x(), "z", chunk.z());

    // MC 1.21.11: 在 FEATURES 阶段开始前，从已有方块数据初始化 FINAL_HEIGHTMAPS
    chunk.primeHeightmaps(HeightmapFlag::POST_FEATURES);

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;
    const BlockPos chunkOrigin(startX, 0, startZ);

    const bool hasDecoration = m_flatSettings.hasDecoration();
    const bool hasLakes = m_flatSettings.hasLakes();

    // === MC 1.21.11: 按装饰阶段交错放置结构和特征 ===
    // 参考: NoiseChunkGenerator::placeFeatures() 和 MC ChunkGenerator.applyBiomeDecoration()
    math::Random worldgenRandom;
    const u64 decorSeed = worldgenRandom.setDecorationSeed(m_seed, startX, startZ);

    // 按结构装饰阶段分组
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

    // 如果需要放置装饰特性或湖泊，使用完整的特性放置流水线
    if (hasDecoration || hasLakes || !structuresByStage.empty()) {
        // 懒初始化 FeatureSorter（数据驱动：placed_feature 已由 PlacedFeatureLoader 注册）
        std::call_once(m_featuresPerStepFlag, [this]() {
            const std::vector<BiomeId>& possibleBiomes = m_biomeSource->possibleBiomes();
            m_featuresPerStep = FeatureSorter::buildFeaturesPerStep(
                possibleBiomes,
                [](BiomeId biomeId, DecorationStage stage) -> const std::vector<ResourceLocation>& {
                    const Biome& biome = BiomeRegistry::instance().get(biomeId);
                    return biome.generationSettings().getFeatures(stage);
                },
                PlacedFeatureRegistry::instance());
        });

        // 收集当前区块的生物群系（平坦世界只有一个生物群系）
        const BiomeId flatBiomeId = m_flatSettings.biomeId();

        const i32 featureSteps = static_cast<i32>(m_featuresPerStep.size());
        const i32 totalSteps = std::max(static_cast<i32>(DecorationStage::Count), featureSteps);

        for (i32 stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
            const DecorationStage stage = DecorationStages::fromIndex(static_cast<u8>(stepIndex));
            const i32 stageOrdinal = stepIndex;

            // === 放置该阶段的结构的特征 ===
            i32 structureIndex = 0;
            auto structIt = structuresByStage.find(stageOrdinal);
            if (structIt != structuresByStage.end()) {
                for (const auto& [structure, start] : structIt->second) {
                    worldgenRandom.setFeatureSeed(decorSeed, structureIndex, stageOrdinal);
                    structure->placeInChunk(region, chunk, *start, chunkX, chunkZ, this);
                    ++structureIndex;
                }
            }

            // === 放置该阶段的生物群系特征 ===
            if (!hasDecoration) {
                // decoration 为 false 时，不放置任何生物群系特性
                // 参考 MC 1.21.11: FlatLevelGeneratorSettings.adjustGenerationSettings()
                // 当 decoration=false 时，flag=false，整个生物群系特性复制循环被跳过
                // 熔岩湖由循环后的 addLakes 专用逻辑放置，不经过此循环
                // 但如果 addLakes=true 且当前阶段是 Lakes，需要跳过以避免与后续专用逻辑重复
                if (hasLakes && stage == DecorationStage::Lakes) {
                    continue;
                }
                // 非 Lakes 阶段且 decoration=false，直接跳过生物群系特性
                continue;
            }

            // decoration 为 true 时，排除 UndergroundStructures 和 SurfaceStructures 阶段
            // 参考 MC 1.21.11: FlatLevelGeneratorSettings.adjustGenerationSettings()
            if (stage == DecorationStage::UndergroundStructures || stage == DecorationStage::SurfaceStructures) {
                continue;
            }
            // 如果 addLakes=true，跳过生物群系原生的 Lakes 阶段特性
            // 避免与循环后的 addLakes 专用熔岩湖放置重复
            // 参考 MC: !this.addLakes || i != GenerationStep.Decoration.LAKES.ordinal()
            if (hasLakes && stage == DecorationStage::Lakes) {
                continue;
            }

            if (stepIndex < featureSteps) {
                const FeatureSorter::StepFeatureData& stepData = m_featuresPerStep[static_cast<size_t>(stepIndex)];
                if (stepData.features.empty()) {
                    continue;
                }

                // 收集该生物群系在该阶段的特征拓扑索引
                const Biome& biome = BiomeRegistry::instance().get(flatBiomeId);
                const BiomeGenerationSettings& biomeSettings = biome.generationSettings();
                const auto& featureIds = biomeSettings.getFeatures(stage);

                std::set<i32> featureIndices;
                for (const ResourceLocation& fid : featureIds) {
                    const i32 topoIndex = stepData.getIndex(fid);
                    if (topoIndex >= 0) {
                        featureIndices.insert(topoIndex);
                    }
                }

                // 按拓扑索引排序放置特征
                for (i32 topoIndex : featureIndices) {
                    if (topoIndex < static_cast<i32>(stepData.features.size()) &&
                        stepData.features[topoIndex] != nullptr) {
                        worldgenRandom.setFeatureSeed(decorSeed, topoIndex, stageOrdinal);
                        stepData.features[topoIndex]->place(region, chunk, *this, worldgenRandom, chunkOrigin);
                    }
                }
            }
        }

        // addLakes 为 true 时，放置平坦世界专用的熔岩湖特征
        // 参考 MC 1.21.11: FlatLevelGeneratorSettings.createLakesList()
        // MC 原版只放置 LAKE_LAVA_UNDERGROUND 和 LAKE_LAVA_SURFACE，不放置水湖
        // 水湖由生物群系自身的 Lakes 阶段特性提供（decoration=true 时才生效）
        // 数据驱动：直接从 ConfiguredFeatureRegistry 按 id 取配置化熔岩湖并放置
        if (hasLakes) {
            const ConfiguredFeatureBase* lavaLake =
                ConfiguredFeatureRegistry::instance().get(ResourceLocation("minecraft", "lake_lava"));
            if (lavaLake != nullptr) {
                worldgenRandom.setFeatureSeed(decorSeed, 0, static_cast<i32>(DecorationStage::Lakes));
                lavaLake->place(region, chunk, *this, worldgenRandom, chunkOrigin);
            }
        }
    }

    // 填充非运动阻挡层（如水层）—— 在所有特性放置之后
    // 参考 MC 1.21.11: FlatLevelGeneratorSettings.adjustGenerationSettings() 中的 FILL_LAYER 逻辑
    _placeFillLayers(region, chunkOrigin);

    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

bool FlatChunkGenerator::_hasBiomesForStructureSet(const world::gen::structure::StructureSet& structureSet) const
{
    // MC 1.21.11: 参考 ChunkGeneratorStructureState.hasBiomesForStructureSet()
    // 检查结构集中的任何结构是否与平坦世界的生物群系兼容
    // FixedBiomeSource 只有一个生物群系（m_flatSettings.biomeId()）
    const BiomeId flatBiomeId = m_flatSettings.biomeId();

    for (const auto& entry : structureSet.entries()) {
        const auto* structure = world::gen::structure::StructureRegistry::get(entry.structureId);
        if (!structure) continue;

        // 使用 isValidBiome() 检查兼容性（内部优先使用 biomeTag，回退到空列表检查）
        if (structure->isValidBiome(flatBiomeId)) {
            return true;
        }
    }

    return false;
}

void FlatChunkGenerator::_placeFillLayers(WorldGenRegion& region, const BlockPos& chunkOrigin)
{
    const auto& fillEntries = m_flatSettings.fillLayerEntries();
    if (fillEntries.empty()) {
        return;
    }

    const i32 minY = getMinY();

    // 参考 MC 1.21.11: FillLayerFeature.place()
    // 在指定高度的 16x16 区域内，将所有空气方块替换为指定方块状态
    for (const auto& entry : fillEntries) {
        const i32 targetY = minY + entry.height;

        // 高度范围检查
        if (targetY < minY || targetY >= minY + getGenDepth()) {
            continue;
        }

        for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
            for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
                const i32 worldX = chunkOrigin.x + localX;
                const i32 worldZ = chunkOrigin.z + localZ;

                const BlockState* currentState = region.getBlockState(worldX, targetY, worldZ);
                // 仅替换空气方块，保留已有方块（如湖泊雕刻的空气和流体）
                if (currentState && currentState->isAir()) {
                    region.setBlockState(worldX, targetY, worldZ, entry.blockState);
                }
            }
        }
    }
}

i32 FlatChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    MC_UNUSED(region);
    MC_UNUSED(chunk);
    MC_UNUSED(outEntities);
    return 0;
}

// ============================================================================
// 生物群系和高度查询
// ============================================================================

BiomeId FlatChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    MC_UNUSED(x);
    MC_UNUSED(y);
    MC_UNUSED(z);
    return m_flatSettings.biomeId();
}

BiomeId FlatChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    MC_UNUSED(noiseX);
    MC_UNUSED(noiseY);
    MC_UNUSED(noiseZ);
    return m_flatSettings.biomeId();
}

i32 FlatChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    MC_UNUSED(x);
    MC_UNUSED(z);

    // 从上到下扫描层列表，找到第一个不透明方块
    const auto& layers = m_flatSettings.layers();
    const i32 minY = getMinY();

    for (i32 i = static_cast<i32>(layers.size()) - 1; i >= 0; --i) {
        const BlockState* state = layers[static_cast<size_t>(i)];
        if (state != nullptr) {
            bool isOpaque = false;
            switch (type) {
                case HeightmapType::WorldSurface:
                case HeightmapType::WorldSurfaceWG:
                    isOpaque = true;
                    break;
                case HeightmapType::OceanFloor:
                case HeightmapType::OceanFloorWG:
                    isOpaque = state->owner().isSolid(*state);
                    break;
                case HeightmapType::MotionBlocking:
                case HeightmapType::MotionBlockingNoLeaves:
                    isOpaque = state->owner().isSolid(*state) || state->isLiquid();
                    break;
                case HeightmapType::LightBlocking:
                    isOpaque = state->owner().isSolid(*state) && state->getOpacity() > 0;
                    break;
                default:
                    break;
            }
            if (isOpaque) {
                return minY + i + 1;
            }
        }
    }

    return minY;
}

i32 FlatChunkGenerator::getGroundHeight() const
{
    return getHeight(0, 0, HeightmapType::WorldSurfaceWG);
}

NoiseColumn FlatChunkGenerator::getBaseColumn(i32 x, i32 z) const
{
    MC_UNUSED(x);
    MC_UNUSED(z);

    // 返回展开层列表，null 替换为空气
    const auto& layers = m_flatSettings.layers();
    const i32 minY = getMinY();
    const i32 genDepth = getGenDepth();

    NoiseColumn column(minY, genDepth);
    const i32 fillCount = std::min(static_cast<i32>(layers.size()), genDepth);

    for (i32 i = 0; i < fillCount; ++i) {
        const BlockState* state = layers[static_cast<size_t>(i)];
        // null → 空气
        column.setBlock(minY + i, state != nullptr ? state : VanillaBlocks::getState(VanillaBlocks::AIR));
    }
    // 剩余位置保持 nullptr（空气）

    return column;
}

} // namespace mc
