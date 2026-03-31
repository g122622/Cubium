#include "NetherChunkGenerator.hpp"
#include "../spawn/WorldGenSpawner.hpp"
#include "../structure/StructureManager.hpp"
#include "../structure/Structure.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../biome/BiomeRegistry.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 常量
// ============================================================================

// 下界高度范围：0-127
constexpr i32 NETHER_HEIGHT = 128;
constexpr i32 NETHER_MIN_Y = 0;

// 噪声参数（参考 MC 1.16.5 NetherChunkGenerator）
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
    , m_random(seed)
{
    initSettings();
    initNoiseGenerators();

    // 确保生物群系注册表已初始化
    BiomeRegistry::instance().initialize();

    // 创建下界生物群系提供者
    m_biomeProvider = std::make_unique<biome::nether::NetherBiomeProvider>(seed);

    // 初始化结构管理器（下界堡垒等）
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(seed));
}

NetherChunkGenerator::NetherChunkGenerator(u64 seed, DimensionSettings settings)
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

    // 创建下界生物群系提供者
    m_biomeProvider = std::make_unique<biome::nether::NetherBiomeProvider>(seed);

    // 初始化结构管理器
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(seed));
}

// ============================================================================
// 初始化
// ============================================================================

void NetherChunkGenerator::initSettings() {
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

void NetherChunkGenerator::initNoiseGenerators() {
    const NoiseSettings& noise = m_settings.noise;

    // 计算噪声尺寸
    // 下界使用较小的噪声尺寸以获得更开阔的地形
    constexpr i32 verticalGranularity = 8;
    constexpr i32 horizontalGranularity = 4;
    m_noiseSizeX = 16 / horizontalGranularity;
    m_noiseSizeY = NETHER_HEIGHT / verticalGranularity;
    m_noiseSizeZ = 16 / horizontalGranularity;

    // 创建噪声生成器（参考 MC 1.16.5）
    math::Random rng(m_seed);

    // 主密度噪声：16 倍频
    m_mainDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // 次密度噪声：16 倍频
    m_secondaryDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // Simplex 噪声（用于下界地形变化）
    m_simplexNoise = std::make_unique<SimplexNoiseGenerator>(rng);
}

// ============================================================================
// 生成阶段
// ============================================================================

void NetherChunkGenerator::generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) {
    MC_TRACE_EVENT("world.gen.nether", "GenerateStructureStarts");
    // 下界结构：堡垒、废弃传送门等
    // 目前使用基类实现
    BaseChunkGenerator::generateStructureStarts(region, chunk);
}

void NetherChunkGenerator::generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk) {
    MC_TRACE_EVENT("world.gen.nether", "GenerateStructureReferences");
    BaseChunkGenerator::generateStructureReferences(region, chunk);
}

void NetherChunkGenerator::generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk) {
    MC_TRACE_EVENT("world.gen.nether", "GenerateBiomes");

    // 使用 NetherBiomeProvider 填充生物群系
    if (m_biomeProvider) {
        m_biomeProvider->fillBiomeContainer(chunk.getBiomes(), chunk.x(), chunk.z());
    }
}

void NetherChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) {
    MC_TRACE_EVENT("world.gen.nether", "GenerateNoise");

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 获取默认方块（下界岩）
    const BlockState* netherrack = &VanillaBlocks::NETHERRACK->getDefaultState();
    const BlockState* lava = &VanillaBlocks::LAVA->getDefaultState();
    const BlockState* air = &VanillaBlocks::AIR->getDefaultState();

    // 计算噪声列
    const i32 noiseSizeX = m_noiseSizeX + 1;
    const i32 noiseSizeZ = m_noiseSizeZ + 1;

    // 存储噪声列
    std::vector<std::vector<f32>> noiseColumns(noiseSizeX);
    for (auto& col : noiseColumns) {
        col.resize(m_noiseSizeY + 1);
    }

    // 计算每个噪声列
    for (i32 nx = 0; nx < noiseSizeX; ++nx) {
        const i32 noiseX = chunkX * m_noiseSizeX + nx;
        for (i32 nz = 0; nz < noiseSizeZ; ++nz) {
            const i32 noiseZ = chunkZ * m_noiseSizeZ + nz;
            fillNoiseColumn(noiseColumns[nx], noiseX, noiseZ);
        }
    }

    // 填充方块
    for (i32 sectionY = 0; sectionY < 8; ++sectionY) { // 下界只有 8 个区块段 (0-127)
        if (!chunk.hasSection(sectionY)) {
            chunk.createSection(sectionY);
        }

        ChunkSection* section = chunk.getSection(sectionY);
        const i32 worldY = sectionY * 16;

        for (i32 lx = 0; lx < 16; ++lx) {
            for (i32 lz = 0; lz < 16; ++lz) {
                // 计算噪声索引
                const i32 nx = lx / 4;
                const i32 nz = lz / 4;

                for (i32 ly = 0; ly < 16; ++ly) {
                    const i32 globalY = worldY + ly;
                    const i32 ny = globalY / 8;

                    // 双线性插值噪声值
                    const f32 density = noiseColumns[nx][ny];

                    // 判断方块类型
                    const BlockState* block = getBlockForDensity(density, globalY);
                    if (block != nullptr) {
                        section->setBlock(lx, ly, lz, block);
                    }
                }
            }
        }
    }
}

void NetherChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) {
    MC_TRACE_EVENT("world.gen.nether", "BuildSurface");

    // 下界地表生成
    // 主要处理基岩层
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            generateBedrock(chunk, x, z);
        }
    }

    // 填充熔岩海
    const BlockState* lava = &VanillaBlocks::LAVA->getDefaultState();
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 y = 0; y <= m_lavaLevel; ++y) {
                const BlockState* current = chunk.getBlock(x, y, z);
                if (current == nullptr || current == &VanillaBlocks::AIR->getDefaultState()) {
                    chunk.setBlock(x, y, z, lava);
                }
            }
        }
    }
}

void NetherChunkGenerator::applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid) {
    MC_TRACE_EVENT("world.gen.nether", "ApplyCarvers");
    // 下界洞穴雕刻
    // 暂时使用基类实现
    BaseChunkGenerator::applyCarvers(region, chunk, isLiquid);
}

void NetherChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) {
    MC_TRACE_EVENT("world.gen.nether", "PlaceFeatures");
    // 下界特性：萤石、岩浆块、灵魂沙等
    // 暂时使用基类实现
    BaseChunkGenerator::placeFeatures(region, chunk);
}

i32 NetherChunkGenerator::spawnInitialMobs(WorldGenRegion& region, ChunkPrimer& chunk,
                                            std::vector<SpawnedEntityData>& outEntities) {
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

BiomeId NetherChunkGenerator::getBiome(i32 x, i32 y, i32 z) const {
    if (m_biomeProvider) {
        return m_biomeProvider->getBiome(x, y, z);
    }
    return m_defaultBiome;
}

BiomeId NetherChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const {
    if (m_biomeProvider) {
        return m_biomeProvider->getNoiseBiome(noiseX, noiseY, noiseZ);
    }
    return m_defaultBiome;
}

i32 NetherChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const {
    MC_UNUSED(x);
    MC_UNUSED(z);
    MC_UNUSED(type);
    // 下界地形复杂，返回中间高度
    return 64;
}

// ============================================================================
// 核心生成方法
// ============================================================================

void NetherChunkGenerator::fillNoiseColumn(std::vector<f32>& column, i32 noiseX, i32 noiseZ) {
    column.resize(m_noiseSizeY + 1);

    // 计算基础密度
    const f32 baseDensity = calculateNoiseDensity(noiseX, 0, noiseZ);

    for (i32 y = 0; y <= m_noiseSizeY; ++y) {
        const i32 worldY = y * 8; // 8 格一个噪声采样点

        // 计算密度
        // 下界地形特征：中间空旷，边缘实心
        f32 density = calculateNoiseDensity(noiseX, y, noiseZ);

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

f32 NetherChunkGenerator::calculateNoiseDensity(i32 noiseX, i32 noiseY, i32 noiseZ) const {
    // 缩放因子
    constexpr f32 SCALE_X = 0.0625f;  // 1/16
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

const BlockState* NetherChunkGenerator::getBlockForDensity(f32 density, i32 y) const {
    // 密度 > 0 表示实心方块
    if (density > 0.0f) {
        return &VanillaBlocks::NETHERRACK->getDefaultState();
    }
    // 空气
    return nullptr;
}

void NetherChunkGenerator::generateBedrock(ChunkPrimer& chunk, i32 x, i32 z) {
    const BlockState* bedrock = &VanillaBlocks::BEDROCK->getDefaultState();

    // 底部基岩层（Y = 0 到几层）
    // 基岩概率随高度递减
    m_random.setSeed(chunk.x() * 341873128712LL + chunk.z() * 132897987541LL + x * 12345LL + z * 67890LL);

    // 底部基岩
    for (i32 y = 0; y <= m_bedrockFloor + 4; ++y) {
        // 基岩概率：底部 100%，向上递减
        const f32 probability = static_cast<f32>(m_bedrockFloor + 5 - y) / 5.0f;
        if (m_random.nextFloat() < probability) {
            chunk.setBlock(x, y, z, bedrock);
        }
    }

    // 顶部基岩（Y = ceiling 到 ceiling - 几层）
    for (i32 y = m_bedrockCeiling - 4; y <= m_bedrockCeiling; ++y) {
        // 基岩概率：顶部 100%，向下递减
        const f32 probability = static_cast<f32>(y - (m_bedrockCeiling - 5)) / 5.0f;
        if (m_random.nextFloat() < probability) {
            chunk.setBlock(x, y, z, bedrock);
        }
    }
}

} // namespace mc
