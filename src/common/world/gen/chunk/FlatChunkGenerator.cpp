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
 * The above copyright notice shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "FlatChunkGenerator.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeGenerationSettings.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include "common/world/gen/feature/FeatureIds.hpp"
#include "common/world/gen/feature/FeatureSorter.hpp"
#include "perfetto/TraceEvents.hpp"

namespace mc {

FlatChunkGenerator::FlatChunkGenerator(u64 seed, FlatLevelGeneratorSettings settings)
    : BaseChunkGenerator(seed, DimensionSettings::flat())
    , m_flatSettings(std::move(settings))
    , m_biomeSource(std::make_unique<world::biome::source::FixedBiomeSource>(seed, m_flatSettings.biomeId()))
{
    // 设置默认生物群系，让 BaseChunkGenerator::generateBiomes 使用
    m_defaultBiome = m_flatSettings.biomeId();
}

// ============================================================================
// 区块生成接口
// ============================================================================

void FlatChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_UNUSED(region);
    // TODO: 实现平坦世界结构生成（根据 structureOverrides）
    chunk.setChunkStatus(ChunkStatuses::STRUCTURE_STARTS);
}

void FlatChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
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

    // 如果需要放置装饰特性或湖泊，使用完整的特性放置流水线
    if (hasDecoration || hasLakes) {
        // 初始化特征注册表（线程安全，仅初始化一次）
        static std::once_flag s_featureRegistryInitFlag;
        std::call_once(s_featureRegistryInitFlag, []() { FeatureRegistry::instance().initialize(); });

        // 懒初始化 FeatureSorter
        // 平坦世界使用 FixedBiomeSource，possibleBiomes() 只有一个生物群系
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

        // 收集当前区块的生物群系（平坦世界只有一个生物群系）
        const BiomeId flatBiomeId = m_flatSettings.biomeId();

        // 种子初始化
        math::Random worldgenRandom;
        const u64 decorSeed = worldgenRandom.setDecorationSeed(m_seed, startX, startZ);

        // 按装饰阶段放置特征
        const i32 featureSteps = static_cast<i32>(m_featuresPerStep.size());
        const i32 totalSteps = std::max(static_cast<i32>(DecorationStage::Count), featureSteps);

        for (i32 stepIndex = 0; stepIndex < totalSteps; ++stepIndex) {
            const DecorationStage stage = DecorationStages::fromIndex(static_cast<u8>(stepIndex));
            const i32 stageOrdinal = stepIndex;

            // 根据 decoration 和 addLakes 标志过滤阶段
            if (!hasDecoration) {
                // decoration 为 false 时，不放置任何生物群系特性
                // 参考 MC 1.21.11: FlatLevelGeneratorSettings.adjustGenerationSettings()
                // 当 decoration=false 时，flag=false，整个生物群系特性复制循环被跳过
                // 熔岩湖由循环后的 addLakes 专用逻辑放置，不经过此循环
                continue;
            } else {
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
            }

            // 放置该阶段的生物群系特征
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
                for (u32 fid : featureIds) {
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
        if (hasLakes) {
            const auto& lakeFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::Lakes);
            for (i32 i = 0; i < static_cast<i32>(lakeFeatures.size()); ++i) {
                ConfiguredFeatureBase* feature = lakeFeatures[static_cast<size_t>(i)];
                if (feature == nullptr) {
                    continue;
                }
                // 仅放置熔岩湖（跳过水湖，水湖由生物群系特性提供）
                // 参考 MC: createLakesList 只包含 LAKE_LAVA_UNDERGROUND 和 LAKE_LAVA_SURFACE
                if (feature->featureId() == LakeFeatureIds::LavaLake) {
                    worldgenRandom.setFeatureSeed(decorSeed, i, static_cast<i32>(DecorationStage::Lakes));
                    feature->place(region, chunk, *this, worldgenRandom, chunkOrigin);
                }
            }
        }
    }

    // 填充非运动阻挡层（如水层）—— 在所有特性放置之后
    // 参考 MC 1.21.11: FlatLevelGeneratorSettings.adjustGenerationSettings() 中的 FILL_LAYER 逻辑
    _placeFillLayers(region, chunkOrigin);

    chunk.setChunkStatus(ChunkStatuses::FEATURES);
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
