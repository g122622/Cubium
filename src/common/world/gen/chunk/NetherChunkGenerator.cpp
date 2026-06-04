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

#include "NetherChunkGenerator.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../biome/BiomeGenerationSettings.hpp"
#include "../../biome/BiomeRegistry.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../carver/WorldCarver.hpp"
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

// 下界高度范围：0-127
// TODO: NETHER_HEIGHT 常量在 NetherCaveCarver.cpp 中也有定义，应该统一提取到公共头文件中
constexpr i32 NETHER_HEIGHT = 128;
constexpr i32 NETHER_MIN_Y = 0;

// 噪声参数
constexpr f32 NOISE_SCALE_X = 1.0f;
constexpr f32 NOISE_SCALE_Y = 2.0f;
constexpr f32 NOISE_SCALE_Z = 1.0f;
constexpr f32 DENSITY_FACTOR = 1000.0f;
constexpr f32 DENSITY_OFFSET = -0.5f;

// ============================================================================
// NetherChunkGenerator 实现
// ============================================================================

NetherChunkGenerator::NetherChunkGenerator(u64 seed)
    : BaseChunkGenerator(seed, DimensionSettings::nether())
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
{
    _initSettings();
    _initNoiseGenerators();

    // 确保生物群系注册表已初始化
    BiomeRegistry::instance().initialize();

    // 创建下界生物群系源（使用 MultiNoiseBiomeSource）
    m_biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(seed);

    // 初始化结构管理器（下界堡垒等）
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(seed));
}

NetherChunkGenerator::NetherChunkGenerator(u64 seed, DimensionSettings settings)
    : BaseChunkGenerator(seed, std::move(settings))
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
{
    _initSettings();
    _initNoiseGenerators();

    // 确保生物群系注册表已初始化
    BiomeRegistry::instance().initialize();

    // 创建下界生物群系源（使用 MultiNoiseBiomeSource）
    m_biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(seed);

    // 初始化结构管理器
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(seed));
}

// ============================================================================
// 初始化
// ============================================================================

void NetherChunkGenerator::_initSettings()
{
    // 下界特有设置
    m_lavaLevel = m_settings.seaLevel; // 使用 seaLevel 作为熔岩高度
    m_bedrockCeiling = m_settings.bedrockRoof;
    m_bedrockFloor = m_settings.bedrockFloor;

    // 确保基岩层在合理范围内
    if (m_bedrockCeiling < 0 || m_bedrockCeiling >= NETHER_HEIGHT) {
        m_bedrockCeiling = 127;
    }
    if (m_bedrockFloor < 0 || m_bedrockFloor >= NETHER_HEIGHT) {
        m_bedrockFloor = 0;
    }
}

void NetherChunkGenerator::_initNoiseGenerators()
{
    const NoiseSettings& noise = m_settings.noise;

    // 计算噪声尺寸
    // 下界使用较小的噪声尺寸以获得更开阔的地形
    constexpr i32 verticalGranularity = 8;
    constexpr i32 horizontalGranularity = 4;
    m_noiseSizeX = 16 / horizontalGranularity;
    m_noiseSizeY = NETHER_HEIGHT / verticalGranularity;
    m_noiseSizeZ = 16 / horizontalGranularity;

    // 创建噪声生成器
    math::Random rng(m_seed);

    // 主密度噪声：16 倍频
    m_mainDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // 次密度噪声：16 倍频
    m_secondaryDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // Simplex 噪声（用于下界地形变化）
    m_simplexNoise = std::make_unique<SimplexNoiseGenerator>(rng);

    // 初始化下界洞穴雕刻器
    // 下界洞穴概率较高，约 1/5
    m_caveCarver = std::make_unique<NetherCaveCarver>();
    m_caveConfig = ProbabilityConfig(0.2f);
}

// ============================================================================
// 生成阶段
// ============================================================================

void NetherChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.nether", "GenerateStructureStarts");
    // 下界结构：堡垒、废弃传送门等
    // 目前使用基类实现
    BaseChunkGenerator::generateStructureStarts(region, chunk);
}

void NetherChunkGenerator::generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.nether", "GenerateStructureReferences");
    BaseChunkGenerator::generateStructureReferences(region, chunk);
}

void NetherChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.nether", "GenerateBiomes");

    // 使用 BiomeSource 填充生物群系
    if (m_biomeSource) {
        m_biomeSource->fillBiomeContainer(chunk.getBiomes(), chunk.x(), chunk.z());
    }

    chunk.setChunkStatus(ChunkStatuses::BIOMES);
}

void NetherChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.nether", "GenerateNoise");
    MC_UNUSED(region);

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    const i32 horizontalNoiseGranularity = 16 / m_noiseSizeX;
    const i32 verticalNoiseGranularity = NETHER_HEIGHT / m_noiseSizeY;

    if (horizontalNoiseGranularity <= 0 || verticalNoiseGranularity <= 0) {
        return;
    }

    // 与主世界噪声生成一致：使用两列缓存并进行三线性插值
    std::vector<std::vector<f32>> noiseCache[2];
    noiseCache[0].resize(m_noiseSizeZ + 1, std::vector<f32>(m_noiseSizeY + 1));
    noiseCache[1].resize(m_noiseSizeZ + 1, std::vector<f32>(m_noiseSizeY + 1));

    for (i32 noiseZ = 0; noiseZ <= m_noiseSizeZ; ++noiseZ) {
        const i32 globalNoiseZ = chunkZ * m_noiseSizeZ + noiseZ;
        _fillNoiseColumn(noiseCache[0][noiseZ], chunkX * m_noiseSizeX, globalNoiseZ);
    }

    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    for (i32 noiseX = 0; noiseX < m_noiseSizeX; ++noiseX) {
        for (i32 noiseZ = 0; noiseZ <= m_noiseSizeZ; ++noiseZ) {
            const i32 globalNoiseX = chunkX * m_noiseSizeX + noiseX + 1;
            const i32 globalNoiseZ = chunkZ * m_noiseSizeZ + noiseZ;
            _fillNoiseColumn(noiseCache[1][noiseZ], globalNoiseX, globalNoiseZ);
        }

        for (i32 noiseZ = 0; noiseZ < m_noiseSizeZ; ++noiseZ) {
            for (i32 noiseY = m_noiseSizeY - 1; noiseY >= 0; --noiseY) {
                const f32 d0 = noiseCache[0][noiseZ][noiseY];
                const f32 d1 = noiseCache[0][noiseZ + 1][noiseY];
                const f32 d2 = noiseCache[1][noiseZ][noiseY];
                const f32 d3 = noiseCache[1][noiseZ + 1][noiseY];
                const f32 d4 = noiseCache[0][noiseZ][noiseY + 1];
                const f32 d5 = noiseCache[0][noiseZ + 1][noiseY + 1];
                const f32 d6 = noiseCache[1][noiseZ][noiseY + 1];
                const f32 d7 = noiseCache[1][noiseZ + 1][noiseY + 1];

                for (i32 localY = verticalNoiseGranularity - 1; localY >= 0; --localY) {
                    const i32 worldY = noiseY * verticalNoiseGranularity + localY;
                    const f32 yLerp = static_cast<f32>(localY) / static_cast<f32>(verticalNoiseGranularity);

                    const f32 y0 = math::lerp(d0, d4, yLerp);
                    const f32 y1 = math::lerp(d1, d5, yLerp);
                    const f32 y2 = math::lerp(d2, d6, yLerp);
                    const f32 y3 = math::lerp(d3, d7, yLerp);

                    for (i32 localX = 0; localX < horizontalNoiseGranularity; ++localX) {
                        const i32 worldX = startX + noiseX * horizontalNoiseGranularity + localX;
                        const f32 xLerp = static_cast<f32>(localX) / static_cast<f32>(horizontalNoiseGranularity);

                        const f32 x0 = math::lerp(y0, y2, xLerp);
                        const f32 x1 = math::lerp(y1, y3, xLerp);

                        for (i32 localZ = 0; localZ < horizontalNoiseGranularity; ++localZ) {
                            const i32 worldZ = startZ + noiseZ * horizontalNoiseGranularity + localZ;
                            const f32 zLerp = static_cast<f32>(localZ) / static_cast<f32>(horizontalNoiseGranularity);
                            const f32 density = math::lerp(x0, x1, zLerp);

                            const BlockState* blockState = _getBlockForDensity(density, worldY);
                            if (blockState == nullptr) {
                                continue;
                            }

                            const i32 localBlockX = worldX & 15;
                            const i32 localBlockZ = worldZ & 15;
                            chunk.setBlockState(localBlockX, worldY, localBlockZ, blockState);

                            chunk.updateHeightmap(
                                HeightmapType::WorldSurfaceWG, localBlockX, worldY, localBlockZ, blockState);
                            if (blockState->isSolid()) {
                                chunk.updateHeightmap(
                                    HeightmapType::OceanFloorWG, localBlockX, worldY, localBlockZ, blockState);
                            }
                        }
                    }
                }
            }
        }

        std::swap(noiseCache[0], noiseCache[1]);
    }

    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

void NetherChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.nether", "BuildSurface");
    MC_UNUSED(region);

    // 下界地表生成
    // 主要处理基岩层
    math::Random bedrockRng(
        static_cast<u64>(chunk.x()) * 341873128712ULL + static_cast<u64>(chunk.z()) * 132897987541ULL + m_seed);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            _generateBedrock(chunk, x, z, bedrockRng);
        }
    }

    // 填充熔岩海
    const BlockState* lava = &VanillaBlocks::LAVA->getDefaultState();
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 y = 0; y <= m_lavaLevel; ++y) {
                const BlockState* current = chunk.getBlockState(x, y, z);
                if (current == nullptr || current == &VanillaBlocks::AIR->getDefaultState()) {
                    chunk.setBlockState(x, y, z, lava);
                }
            }
        }
    }

    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

void NetherChunkGenerator::applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid)
{
    MC_TRACE_EVENT("world.gen.nether", "ApplyCarvers");
    MC_UNUSED(region);

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 创建雕刻掩码
    CarvingMask carvingMask(chunkX, chunkZ);

    // 下界只使用洞穴雕刻器（不使用峡谷和水下雕刻器）
    if (!isLiquid && m_caveCarver) {
        m_caveCarver->carve(chunk, *m_biomeSource, m_lavaLevel, chunkX, chunkZ, carvingMask, m_caveConfig);
    }

    chunk.setChunkStatus(isLiquid ? ChunkStatuses::LIQUID_CARVERS : ChunkStatuses::CARVERS);
}

void NetherChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.gen.nether", "PlaceFeatures");

    // 线程安全初始化特征注册表（包含下界特征）
    static std::once_flag s_featureRegistryInitFlag;
    std::call_once(s_featureRegistryInitFlag, []() { FeatureRegistry::instance().initialize(); });

    const BiomeId biomeId = chunk.getBiomeAtBlock(8, 64, 8);
    const Biome& biome = m_biomeSource->getBiomeDefinition(biomeId);
    const BiomeGenerationSettings& settings = biome.generationSettings();

    // 使用统一的阶段管线放置下界生物群系配置的特征
    for (DecorationStage stage : DecorationStages::getAll()) {
        BiomeFeaturePlacer::placeFeaturesForStage(region, chunk, *this, settings, stage, m_seed);
    }

    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

i32 NetherChunkGenerator::spawnInitialMobs(
    WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities)
{
    // 下界生物生成
    // 暂时不生成初始生物
    MC_UNUSED(region);
    MC_UNUSED(chunk);
    MC_UNUSED(outEntities);
    return 0;
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId NetherChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    if (m_biomeSource) {
        return m_biomeSource->getNoiseBiome(x >> 2, y >> 2, z >> 2);
    }
    return m_defaultBiome;
}

BiomeId NetherChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    if (m_biomeSource) {
        return m_biomeSource->getNoiseBiome(noiseX, noiseY, noiseZ);
    }
    return m_defaultBiome;
}

i32 NetherChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    const i32 horizontalNoiseGranularity = 16 / m_noiseSizeX;
    const i32 verticalNoiseGranularity = NETHER_HEIGHT / m_noiseSizeY;

    if (horizontalNoiseGranularity <= 0 || verticalNoiseGranularity <= 0 || m_noiseSizeY <= 0) {
        return m_lavaLevel + 1;
    }

    // 向下取整除法（支持负坐标）
    const i32 noiseX = math::floorDiv(x, horizontalNoiseGranularity);
    const i32 noiseZ = math::floorDiv(z, horizontalNoiseGranularity);
    const i32 localX = x - noiseX * horizontalNoiseGranularity;
    const i32 localZ = z - noiseZ * horizontalNoiseGranularity;

    const f32 xLerp = static_cast<f32>(localX) / static_cast<f32>(horizontalNoiseGranularity);
    const f32 zLerp = static_cast<f32>(localZ) / static_cast<f32>(horizontalNoiseGranularity);

    std::vector<f32> column00;
    std::vector<f32> column01;
    std::vector<f32> column10;
    std::vector<f32> column11;

    _fillNoiseColumn(column00, noiseX, noiseZ);
    _fillNoiseColumn(column01, noiseX, noiseZ + 1);
    _fillNoiseColumn(column10, noiseX + 1, noiseZ);
    _fillNoiseColumn(column11, noiseX + 1, noiseZ + 1);

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

    for (i32 worldY = NETHER_HEIGHT - 1; worldY >= 0; --worldY) {
        const i32 noiseY = worldY / verticalNoiseGranularity;
        const i32 localY = worldY % verticalNoiseGranularity;

        if (noiseY < 0 || noiseY >= m_noiseSizeY) {
            continue;
        }

        const f32 yLerp = static_cast<f32>(localY) / static_cast<f32>(verticalNoiseGranularity);

        const f32 y00 = math::lerp(column00[noiseY], column00[noiseY + 1], yLerp);
        const f32 y01 = math::lerp(column01[noiseY], column01[noiseY + 1], yLerp);
        const f32 y10 = math::lerp(column10[noiseY], column10[noiseY + 1], yLerp);
        const f32 y11 = math::lerp(column11[noiseY], column11[noiseY + 1], yLerp);

        const f32 x0 = math::lerp(y00, y10, xLerp);
        const f32 x1 = math::lerp(y01, y11, xLerp);
        const f32 density = math::lerp(x0, x1, zLerp);

        const BlockState* blockState = _getBlockForDensity(density, worldY);
        if (matchesHeightmap(blockState)) {
            return worldY + 1;
        }
    }

    return 0;
}

// ============================================================================
// 核心生成方法
// ============================================================================

void NetherChunkGenerator::_fillNoiseColumn(std::vector<f32>& column, i32 noiseX, i32 noiseZ) const
{
    column.resize(m_noiseSizeY + 1);

    for (i32 y = 0; y <= m_noiseSizeY; ++y) {
        const i32 worldY = y * 8; // 8 格一个噪声采样点

        // 计算密度
        // 下界地形特征：中间空旷，边缘实心
        f32 density = _calculateNoiseDensity(noiseX, y, noiseZ);

        // 应用高度衰减
        // 靠近顶部和底部时密度增加
        const f32 heightFactor = static_cast<f32>(worldY) / static_cast<f32>(NETHER_HEIGHT);

        // 使用 S 曲线在中间创造空腔
        // 顶部和底部更实心，中间更空旷
        const f32 centerDist = std::abs(heightFactor - 0.5f) * 2.0f;
        const f32 heightDensity = centerDist * 0.5f - 0.3f;

        density += heightDensity;

        column[y] = density;
    }
}

f32 NetherChunkGenerator::_calculateNoiseDensity(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    // 缩放因子
    constexpr f32 SCALE_X = 0.0625f; // 1/16
    constexpr f32 SCALE_Y = 0.0625f;
    constexpr f32 SCALE_Z = 0.0625f;

    const f32 nx = static_cast<f32>(noiseX) * SCALE_X;
    const f32 ny = static_cast<f32>(noiseY) * SCALE_Y;
    const f32 nz = static_cast<f32>(noiseZ) * SCALE_Z;

    // 主噪声
    f32 density = m_mainDensityNoise->noise(nx, ny, nz);

    // 次噪声（增加细节）
    density += m_secondaryDensityNoise->noise(nx * 2.0f, ny * 2.0f, nz * 2.0f) * 0.5f;

    return density;
}

const BlockState* NetherChunkGenerator::_getBlockForDensity(f32 density, i32 y) const
{
    // 密度 > 0 表示实心方块
    if (density > 0.0f) {
        return &VanillaBlocks::NETHERRACK->getDefaultState();
    }
    // 低于熔岩海高度使用默认流体
    if (m_settings.defaultFluid != nullptr && y < m_lavaLevel) {
        return m_settings.defaultFluid;
    }
    // 空气
    return nullptr;
}

void NetherChunkGenerator::_generateBedrock(ChunkPrimer& chunk, i32 x, i32 z, math::Random& random) const
{
    const BlockState* bedrock = &VanillaBlocks::BEDROCK->getDefaultState();

    // 底部/顶部各 5 层，按 nextInt(5) 决定每层是否放置
    if (m_bedrockFloor + 4 >= NETHER_MIN_Y && m_bedrockFloor < NETHER_HEIGHT) {
        for (i32 offset = 0; offset < 5; ++offset) {
            if (offset <= random.nextInt(5)) {
                const i32 y = m_bedrockFloor + offset;
                if (y >= NETHER_MIN_Y && y < NETHER_HEIGHT) {
                    chunk.setBlockState(x, y, z, bedrock);
                }
            }
        }
    }

    if (m_bedrockCeiling + 4 >= NETHER_MIN_Y && m_bedrockCeiling < NETHER_HEIGHT) {
        for (i32 offset = 0; offset < 5; ++offset) {
            if (offset <= random.nextInt(5)) {
                const i32 y = m_bedrockCeiling - offset;
                if (y >= NETHER_MIN_Y && y < NETHER_HEIGHT) {
                    chunk.setBlockState(x, y, z, bedrock);
                }
            }
        }
    }
}

} // namespace mc
