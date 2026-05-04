#include "NoiseChunkGenerator.hpp"
#include "../spawn/WorldGenSpawner.hpp"
#include "../carver/UnderwaterCarver.hpp"
#include "../structure/StructureManager.hpp"
#include "../structure/Structure.hpp"
#include "../placement/PlacementRegistry.hpp"
#include "../jigsaw/JigsawPiece.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../biome/BiomeRegistry.hpp"
#include "../../biome/BiomeGenerationSettings.hpp"
#include "../../biome/layer/LayerUtil.hpp"
#include "../feature/ConfiguredFeature.hpp"
#include "../feature/ore/OreFeature.hpp"
#include "../surface/SurfaceBuilders.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 静态成员定义
// ============================================================================

std::array<f32, 13824> NoiseChunkGenerator::s_gaussianLUT;
bool NoiseChunkGenerator::s_gaussianLUTInitialized = false;

// ============================================================================
// 常量
// ============================================================================

constexpr f32 BIOME_WEIGHT_SCALE = 10.0f;

namespace {

[[nodiscard]] SurfaceBuilderConfig makeSurfaceConfigFromBiome(const Biome& biome, const BlockState* defaultBlock) {
    const BlockState* top = biome.surfaceBlock();
    const BlockState* under = biome.subSurfaceBlock();
    const BlockState* underWater = biome.underWaterBlock();

    if (top == nullptr) {
        top = defaultBlock;
    }
    if (under == nullptr) {
        under = defaultBlock;
    }
    if (underWater == nullptr) {
        underWater = under;
    }

    return SurfaceBuilderConfig(top, under, underWater);
}

[[nodiscard]] SurfaceBuilder* selectSpecialSurfaceBuilder(const Biome& biome, BiomeId biomeId) {
    static MountainSurfaceBuilder mountainBuilder;
    static GravellyMountainSurfaceBuilder gravellyMountainBuilder;
    static SwampSurfaceBuilder swampBuilder;
    static FrozenOceanSurfaceBuilder frozenOceanBuilder;
    static BadlandsSurfaceBuilder badlandsBuilder;
    static ErodedBadlandsSurfaceBuilder erodedBadlandsBuilder;
    static WoodedBadlandsSurfaceBuilder woodedBadlandsBuilder;
    static GiantTreeTaigaSurfaceBuilder giantTaigaBuilder;
    static ShatteredSavannaSurfaceBuilder shatteredSavannaBuilder;
    static NetherSurfaceBuilder netherBuilder;
    static NetherForestsSurfaceBuilder netherForestsBuilder;
    static SoulSandValleySurfaceBuilder soulSandValleyBuilder;
    static BasaltDeltasSurfaceBuilder basaltDeltasBuilder;
    static DefaultSurfaceBuilder defaultBuilder;

    switch (biome.category()) {
        case Biome::Category::ExtremeHills:
            return &mountainBuilder;
        case Biome::Category::Swamp:
            return &swampBuilder;
        case Biome::Category::Mesa:
            return &badlandsBuilder;
        default:
            break;
    }

    switch (biomeId) {
        case Biomes::Mountains:
        case Biomes::SnowyMountains:
        case Biomes::MountainEdge:
        case Biomes::WoodedMountains:
            return &mountainBuilder;

        case Biomes::GravellyMountains:
        case Biomes::ModifiedGravellyMountains:
            return &gravellyMountainBuilder;

        case Biomes::Swamp:
        case Biomes::SwampHills:
            return &swampBuilder;

        case Biomes::FrozenOcean:
        case Biomes::DeepFrozenOcean:
            return &frozenOceanBuilder;

        case Biomes::Badlands:
        case Biomes::BadlandsPlateau:
        case Biomes::ModifiedBadlandsPlateau:
            return &badlandsBuilder;

        case Biomes::ErodedBadlands:
            return &erodedBadlandsBuilder;

        case Biomes::WoodedBadlandsPlateau:
        case Biomes::ModifiedWoodedBadlandsPlateau:
            return &woodedBadlandsBuilder;

        case Biomes::GiantTreeTaiga:
        case Biomes::GiantTreeTaigaHills:
        case Biomes::GiantSpruceTaiga:
        case Biomes::GiantSpruceTaigaHills:
            return &giantTaigaBuilder;

        case Biomes::ShatteredSavanna:
        case Biomes::ShatteredSavannaPlateau:
            return &shatteredSavannaBuilder;

        // 下界生物群系
        case Biomes::NetherWastes:
            return &netherBuilder;

        case Biomes::CrimsonForest:
        case Biomes::WarpedForest:
            return &netherForestsBuilder;

        case Biomes::SoulSandValley:
            return &soulSandValleyBuilder;

        case Biomes::BasaltDeltas:
            return &basaltDeltasBuilder;

        default:
            return nullptr;
    }
}

} // namespace

// ============================================================================
// NoiseChunkGenerator 实现
// ============================================================================

NoiseChunkGenerator::NoiseChunkGenerator(u64 seed, DimensionSettings settings)
    : BaseChunkGenerator(seed, std::move(settings))
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
    , m_verticalNoiseGranularity(0)
    , m_horizontalNoiseGranularity(0)
{
    initNoiseGenerators();
    initBiomeWeights();
    initGaussianLUT();

    // 使用完整的生物群系列表
    BiomeRegistry::instance().initialize();
    m_biomeProvider = std::make_unique<LayerBiomeProvider>(seed);

    initCarvers();
    initGenerationRegistries();
}

NoiseChunkGenerator::NoiseChunkGenerator(u64 seed, DimensionSettings settings,
                                          std::unique_ptr<BiomeProvider> biomeProvider)
    : BaseChunkGenerator(seed, std::move(settings))
    , m_biomeProvider(std::move(biomeProvider))
    , m_noiseSizeX(0)
    , m_noiseSizeY(0)
    , m_noiseSizeZ(0)
    , m_verticalNoiseGranularity(0)
    , m_horizontalNoiseGranularity(0)
{
    initNoiseGenerators();
    initBiomeWeights();
    initGaussianLUT();

    // 确保生物群系注册表已初始化（默认构造路径会初始化，注入路径也需要）
    BiomeRegistry::instance().initialize();

    MC_PRECONDITION(m_biomeProvider != nullptr);
    if (m_biomeProvider == nullptr) {
        spdlog::warn("[NoiseChunkGenerator] Injected biome provider is null, fallback to LayerBiomeProvider");
        m_biomeProvider = std::make_unique<LayerBiomeProvider>(seed);
    }

    initCarvers();
    initGenerationRegistries();
}

NoiseChunkGenerator::~NoiseChunkGenerator() = default;

// ============================================================================
// 初始化
// ============================================================================

void NoiseChunkGenerator::initNoiseGenerators()
{
    const NoiseSettings& noise = m_settings.noise;

    // 计算噪声尺寸
    m_verticalNoiseGranularity = noise.sizeVertical * 4;
    m_horizontalNoiseGranularity = noise.sizeHorizontal * 4;
    m_noiseSizeX = 16 / m_horizontalNoiseGranularity;
    m_noiseSizeY = noise.height / m_verticalNoiseGranularity;
    m_noiseSizeZ = 16 / m_horizontalNoiseGranularity;

    // 创建噪声生成器（参考 MC）
    math::Random rng(m_seed);

    // 主密度噪声：16 倍频（-15 到 0）
    m_mainDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // 次密度噪声：16 倍频（-15 到 0）
    m_secondaryDensityNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);

    // 权重噪声：8 倍频（-7 到 0）
    m_weightNoise = std::make_unique<OctavesNoiseGenerator>(rng, -7, 0);

    // 地表深度噪声
    // 参考 MC：simplexSurfaceNoise=true 时使用 PerlinNoiseGenerator，
    // 否则使用 OctavesNoiseGenerator。
    if (noise.simplexSurfaceNoise) {
        m_surfaceDepthNoise = std::make_unique<PerlinNoiseGenerator>(rng, -3, 0);
    } else {
        m_surfaceDepthNoise = std::make_unique<OctavesNoiseGenerator>(rng, -3, 0);
    }

    // 跳过一些随机数（参考 MC）
    rng.skip(2620);

    // 随机密度偏移噪声
    // 参考 MC：基于同一个随机序列继续构建该噪声层。
    m_randomDensityOffsetNoise = std::make_unique<OctavesNoiseGenerator>(rng, -15, 0);
}

void NoiseChunkGenerator::initBiomeWeights()
{
    // 参考 MC 的 field_236081_j_ 查找表
    for (i32 dz = -2; dz <= 2; ++dz) {
        for (i32 dx = -2; dx <= 2; ++dx) {
            const i32 index = (dx + 2) + (dz + 2) * 5;
            const f32 distance = static_cast<f32>(dx * dx + dz * dz);
            m_biomeWeights[index] = static_cast<f32>(BIOME_WEIGHT_SCALE / std::sqrt(distance + 0.2));
        }
    }
}

void NoiseChunkGenerator::initGaussianLUT()
{
    if (s_gaussianLUTInitialized) {
        return;
    }

    // 参考 MC 1.16.5: NoiseChunkGenerator.field_222561_h
    // 24x24x24 高斯核，索引公式: x * 24 * 24 + y * 24 + z
    // 索引偏移: +12（因为坐标范围是 -12 到 +11）
    for (i32 x = 0; x < 24; ++x) {
        for (i32 y = 0; y < 24; ++y) {
            for (i32 z = 0; z < 24; ++z) {
                const i32 index = x * 24 * 24 + y * 24 + z;
                // MC 1.16.5: func_222554_b 计算高斯衰减
                // 参数是距离中心的偏移（-12 到 +11）
                s_gaussianLUT[index] = static_cast<f32>(calculateStructureDensityOffset(x - 12, y - 12, z - 12));
            }
        }
    }

    s_gaussianLUTInitialized = true;
}

f64 NoiseChunkGenerator::calculateStructureDensityOffset(i32 dx, i32 dy, i32 dz)
{
    // 参考 MC 1.16.5: NoiseChunkGenerator.func_222554_b
    // 高斯衰减函数，用于结构边界地形平滑
    //
    // 计算:
    // 1. 水平距离平方: d0 = dx*dx + dz*dz
    // 2. 垂直偏移: d1 = dy + 0.5
    // 3. 垂直距离平方: d2 = d1*d1
    // 4. 高斯衰减: d3 = exp(-(d2/16 + d0/16))
    // 5. 垂直梯度: d4 = -d1 * fastInvSqrt(d2/2 + d0/2) / 2
    // 6. 结果: d4 * d3

    const f64 d0 = static_cast<f64>(dx * dx) + static_cast<f64>(dz * dz);
    const f64 d1 = static_cast<f64>(dy) + 0.5;
    const f64 d2 = d1 * d1;

    // 高斯衰减
    const f64 d3 = std::exp(-(d2 / 16.0 + d0 / 16.0));

    // 快速逆平方根
    const f64 d4 = -d1 * math::fastInverseSqrt(static_cast<f32>((d2 / 2.0 + d0 / 2.0))) / 2.0;

    return d4 * d3;
}

void NoiseChunkGenerator::initCarvers()
{
    // 洞穴概率参考 MC: 1/7 ≈ 0.14285715
    m_caveCarver = std::make_unique<CaveCarver>(world::MAX_BUILD_HEIGHT);
    m_caveConfig = ProbabilityConfig(0.14285715f);

    // 峡谷概率更低
    m_canyonCarver = std::make_unique<CanyonCarver>(world::MAX_BUILD_HEIGHT);
    m_canyonConfig = ProbabilityConfig(0.02f);

    // 水下雕刻器与普通雕刻阶段共用概率配置
    m_underwaterCaveCarver = std::make_unique<world::gen::carver::UnderwaterCaveCarver>();
    m_underwaterCanyonCarver = std::make_unique<world::gen::carver::UnderwaterCanyonCarver>();
}

void NoiseChunkGenerator::initGenerationRegistries()
{
    // 初始化结构管理器
    world::gen::structure::StructureRegistry::initialize();
    m_structureManager = std::make_unique<world::gen::structure::StructureManager>(static_cast<i64>(m_seed));

    // 初始化放置器注册表
    PlacementRegistry::instance().initialize();
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
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL +
                     static_cast<u64>(chunkZ) * 132897987541ULL +
                     m_seed);

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

    BiomeContainer& biomes = chunk.getBiomes();
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    m_biomeProvider->fillBiomeContainer(biomes, chunkX, chunkZ);

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::BIOMES);
}

// ============================================================================
// 噪声地形生成
// ============================================================================

void NoiseChunkGenerator::generateNoise(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "GenerateNoise", "x", chunk.x(), "z", chunk.z());
    (void)region;

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;
    BiomeWindowCache biomeWindowCache;

    // === 收集结构数据用于地形平滑 ===
    // 参考 MC 1.16.5: NoiseChunkGenerator.func_230352_b_
    std::vector<const world::gen::structure::StructurePiece*> structurePieces;
    std::vector<world::gen::jigsaw::JigsawJunction> junctions;
    collectStructureData(chunk, structurePieces, junctions);

    // === 噪声缓存初始化 ===
    std::vector<std::vector<f32>> noiseCache[2];
    {
        MC_TRACE_EVENT("world.chunk_gen", "GenerateNoise_InitCache");
        noiseCache[0].resize(m_noiseSizeZ + 1, std::vector<f32>(m_noiseSizeY + 1));
        noiseCache[1].resize(m_noiseSizeZ + 1, std::vector<f32>(m_noiseSizeY + 1));

        // 初始化第一列噪声数据
        for (i32 noiseZ = 0; noiseZ <= m_noiseSizeZ; ++noiseZ) {
            const i32 globalNoiseZ = chunkZ * m_noiseSizeZ + noiseZ;
            fillNoiseColumn(noiseCache[0][noiseZ], chunkX * m_noiseSizeX, globalNoiseZ, biomeWindowCache);
        }
    }

    // === 噪声填充与方块放置 ===
    // 注意：地形密度计算已在 fillNoiseColumn() 中完成，包含 biome depth/scale 的影响
    // 参考 MC 1.16.5 NoiseChunkGenerator
    {
        MC_TRACE_EVENT("world.chunk_gen", "GenerateNoise_FillBlocks");
        // 遍历每个噪声单元
        for (i32 noiseX = 0; noiseX < m_noiseSizeX; ++noiseX) {
            // 预计算下一列 X 的噪声数据
            for (i32 noiseZ = 0; noiseZ <= m_noiseSizeZ; ++noiseZ) {
                const i32 globalNoiseX = chunkX * m_noiseSizeX + noiseX + 1;
                const i32 globalNoiseZ = chunkZ * m_noiseSizeZ + noiseZ;
                fillNoiseColumn(noiseCache[1][noiseZ], globalNoiseX, globalNoiseZ, biomeWindowCache);
            }

            // 处理当前噪声单元
            for (i32 noiseZ = 0; noiseZ < m_noiseSizeZ; ++noiseZ) {
                // 获取三线性插值所需的 8 个角点
                for (i32 noiseY = m_noiseSizeY - 1; noiseY >= 0; --noiseY) {
                    // 从当前列和下一列获取密度值
                    const f32 d0 = noiseCache[0][noiseZ][noiseY];           // (x0, z0, y0)
                    const f32 d1 = noiseCache[0][noiseZ + 1][noiseY];       // (x0, z1, y0)
                    const f32 d2 = noiseCache[1][noiseZ][noiseY];           // (x1, z0, y0)
                    const f32 d3 = noiseCache[1][noiseZ + 1][noiseY];       // (x1, z1, y0)
                    const f32 d4 = noiseCache[0][noiseZ][noiseY + 1];       // (x0, z0, y1)
                    const f32 d5 = noiseCache[0][noiseZ + 1][noiseY + 1];   // (x0, z1, y1)
                    const f32 d6 = noiseCache[1][noiseZ][noiseY + 1];       // (x1, z0, y1)
                    const f32 d7 = noiseCache[1][noiseZ + 1][noiseY + 1];   // (x1, z1, y1)

                    // Y 轴细分
                    for (i32 localY = m_verticalNoiseGranularity - 1; localY >= 0; --localY) {
                        const i32 worldY = noiseY * m_verticalNoiseGranularity + localY;
                        const f32 yLerp = static_cast<f32>(localY) / static_cast<f32>(m_verticalNoiseGranularity);

                        // Y 轴插值
                        const f32 y0 = math::lerp(d0, d4, yLerp); // (x0, z0)
                        const f32 y1 = math::lerp(d1, d5, yLerp); // (x0, z1)
                        const f32 y2 = math::lerp(d2, d6, yLerp); // (x1, z0)
                        const f32 y3 = math::lerp(d3, d7, yLerp); // (x1, z1)

                        // X 轴细分
                        for (i32 localX = 0; localX < m_horizontalNoiseGranularity; ++localX) {
                            const i32 worldX = startX + noiseX * m_horizontalNoiseGranularity + localX;
                            const f32 xLerp = static_cast<f32>(localX) / static_cast<f32>(m_horizontalNoiseGranularity);

                            // X 轴插值
                            const f32 x0 = math::lerp(y0, y2, xLerp); // (z0)
                            const f32 x1 = math::lerp(y1, y3, xLerp); // (z1)

                            // Z 轴细分
                            for (i32 localZ = 0; localZ < m_horizontalNoiseGranularity; ++localZ) {
                                const i32 worldZ = startZ + noiseZ * m_horizontalNoiseGranularity + localZ;
                                const f32 zLerp = static_cast<f32>(localZ) / static_cast<f32>(m_horizontalNoiseGranularity);

                                // Z 轴插值 - 最终密度值
                                // 密度值已包含 biome depth/scale 影响（在 fillNoiseColumn 中计算）
                                f64 density = static_cast<f64>(math::lerp(x0, x1, zLerp));
                                const i32 localBlockX = worldX & 15;
                                const i32 localBlockZ = worldZ & 15;

                                // === 结构地形平滑 ===
                                // 参考 MC 1.16.5: NoiseChunkGenerator.func_230352_b_
                                // 计算结构片段对密度的影响
                                if (!structurePieces.empty() || !junctions.empty()) {
                                    // 将密度归一化到 [-1, 1] 范围
                                    density = std::clamp(density / 200.0, -1.0, 1.0);
                                    // 应用二次变换
                                    density = density / 2.0 - density * density * density / 24.0;

                                    // 应用结构片段密度偏移（权重 0.8）
                                    for (const auto* piece : structurePieces) {
                                        if (!piece) continue;

                                        const auto& box = piece->getBoundingBox();
                                        // 计算到结构边界的距离
                                        const i32 dx = std::max(0, std::max(box.minX() - worldX, worldX - box.maxX()));
                                        const i32 dy = worldY - (box.minY() + piece->getGroundLevelDelta());
                                        const i32 dz = std::max(0, std::max(box.minZ() - worldZ, worldZ - box.maxZ()));

                                        // 使用高斯查找表计算偏移
                                        density += static_cast<f64>(calculateStructureDensityOffset(dx, dy, dz)) * 0.8;
                                    }

                                    // 应用 JigsawJunction 密度偏移（权重 0.4）
                                    for (const auto& junction : junctions) {
                                        const i32 dx = worldX - junction.getSourceX();
                                        const i32 dy = worldY - junction.getSourceGroundY();
                                        const i32 dz = worldZ - junction.getSourceZ();
                                        density += static_cast<f64>(calculateStructureDensityOffset(dx, dy, dz)) * 0.4;
                                    }
                                }

                                // 确定方块
                                const BlockState* blockState = getBlockForDensity(static_cast<f32>(density), worldY);

                                if (blockState) {
                                    chunk.setBlockState(localBlockX, worldY, localBlockZ, blockState);

                                    // 更新高度图
                                    chunk.updateHeightmap(HeightmapType::WorldSurfaceWG, localBlockX, worldY, localBlockZ, blockState);
                                    if (blockState->isSolid()) {
                                        chunk.updateHeightmap(HeightmapType::OceanFloorWG, localBlockX, worldY, localBlockZ, blockState);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 交换缓存（swap 比 copy 更高效）
            std::swap(noiseCache[0], noiseCache[1]);
        }
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::NOISE);
}

void NoiseChunkGenerator::fillNoiseColumn(std::vector<f32>& column, i32 noiseX, i32 noiseZ, BiomeWindowCache& biomeWindowCache) const
{
    MC_TRACE_EVENT("world.chunk_gen", "FillNoiseColumn", "x", noiseX, "z", noiseZ);
    column.resize(m_noiseSizeY + 1);

    const NoiseSettings& noise = m_settings.noise;

    // === 阶段 1: 计算生物群系权重 ===
    f32 totalScale = 0.0f;   // f - 累加比例
    f32 totalDepth = 0.0f;   // f1 - 累加深度
    f32 totalWeight = 0.0f;  // f2 - 累加权重

    {
        // 参考 MC：地形密度计算使用 sea level 作为 biome 噪声采样 Y
        const i32 biomeNoiseY = m_settings.seaLevel;

        // 使用 5x5 批量采样 + 滑窗复用，减少重复调用 getNoiseBiome()
        if (biomeWindowCache.valid && noiseX == biomeWindowCache.centerNoiseX && noiseZ == biomeWindowCache.centerNoiseZ + 1) {
            // 沿 Z 正方向滑窗：将第 1~4 行上移到第 0~3 行
            for (i32 row = 0; row < 4; ++row) {
                for (i32 col = 0; col < 5; ++col) {
                    biomeWindowCache.window[static_cast<size_t>(row * 5 + col)] =
                        biomeWindowCache.window[static_cast<size_t>((row + 1) * 5 + col)];
                }
            }

            // 仅采样新进入窗口的最后一行（dz = +2）
            std::array<BiomeId, 5> newRow{};
            m_biomeProvider->getNoiseBiomesBatch(noiseX - 2, biomeNoiseY, noiseZ + 2, 5, 1, newRow.data());
            for (i32 col = 0; col < 5; ++col) {
                biomeWindowCache.window[static_cast<size_t>(20 + col)] = newRow[static_cast<size_t>(col)];
            }

            biomeWindowCache.centerNoiseX = noiseX;
            biomeWindowCache.centerNoiseZ = noiseZ;
        } else {
            // 首次或不连续访问：完整采样 5x5
            m_biomeProvider->getNoiseBiomesBatch(noiseX - 2, biomeNoiseY, noiseZ - 2, 5, 5, biomeWindowCache.window.data());
            biomeWindowCache.valid = true;
            biomeWindowCache.centerNoiseX = noiseX;
            biomeWindowCache.centerNoiseZ = noiseZ;
        }

        // 获取中心生物群系的深度（用于权重调整）
        const BiomeId centerBiome = biomeWindowCache.window[12];
        const Biome& centerDef = m_biomeProvider->getBiomeDefinition(centerBiome);
        const f32 centerDepth = centerDef.depth();

        for (i32 dz = -2; dz <= 2; ++dz) {
            for (i32 dx = -2; dx <= 2; ++dx) {
                const i32 kernelIndex = (dx + 2) + (dz + 2) * 5;
                const BiomeId biome = biomeWindowCache.window[static_cast<size_t>(kernelIndex)];
                const Biome& def = m_biomeProvider->getBiomeDefinition(biome);

                const f32 depth = def.depth();   // f4
                const f32 scale = def.scale();   // f5

                f32 weightedDepth = depth;  // f6
                f32 weightedScale = scale;  // f7

                // 参考 MC：放大化世界对正深度生物群系进行额外拉伸
                if (noise.isAmplified && depth > 0.0f) {
                    weightedDepth = 1.0f + depth * 2.0f;
                    weightedScale = 1.0f + scale * 4.0f;
                }

                // 参考 MC：权重因子
                const f32 depthFactor = (depth > centerDepth) ? 0.5f : 1.0f;  // f8

                // 参考 MC：计算权重
                const f32 baseWeight = m_biomeWeights[static_cast<size_t>(kernelIndex)];
                const f32 weightFactor = depthFactor * baseWeight / (weightedDepth + 2.0f);  // f9

                // 参考 MC：累加
                totalScale += weightedScale * weightFactor;  // f += f7 * f9
                totalDepth += weightedDepth * weightFactor;  // f1 += f6 * f9
                totalWeight += weightFactor;                  // f2 += f9
            }
        }
    }

    // 参考 MC：计算平均深度和比例
    const f32 avgDepth = totalDepth / totalWeight;          // f10 = f1 / f2
    const f32 avgScale = totalScale / totalWeight;          // f11 = f / f2

    // 转换为地形参数
    const f32 depthOffset = (avgDepth * 0.5f - 0.125f) * 0.265625f;
    const f32 heightFactor = 96.0f / (avgScale * 0.9f + 0.1f);

    // === 噪声参数 ===
    const f32 xzScale = 684.412f * noise.scaling.xzScale;
    const f32 yScale = 684.412f * noise.scaling.yScale;
    const f32 xzFactor = xzScale / noise.scaling.xzFactor;
    const f32 yFactor = yScale / noise.scaling.yFactor;

    // === 随机密度偏移 ===
    const f32 randomDensityOffset = noise.randomDensityOffset ? calculateRandomDensityOffset(noiseX, noiseZ) : 0.0f;

    const f32 densityFactor = noise.densityFactor;
    const f32 densityOffset = noise.densityOffset;

    // === 阶段 2: 填充噪声列 ===
    // 参考 MC 1.16.5 NoiseChunkGenerator.fillNoiseColumn()
    for (i32 y = 0; y <= m_noiseSizeY; ++y) {
        // 计算 3D 噪声密度
        f32 density = calculateNoiseDensity(noiseX, y, noiseZ, xzScale, yScale, xzFactor, yFactor);

        // 高度归一化 + 随机密度偏移
        // 参考 MC: double d8 = 1.0D - (double)i1 * 2.0D / (double)this.noiseSizeY + d4;
        // 其中 d4 是 randomDensityOffset
        const f32 normalizedY = 1.0f - static_cast<f32>(y) * 2.0f / static_cast<f32>(m_noiseSizeY) + randomDensityOffset;

        // 应用密度因子和偏移
        // 参考 MC: double d9 = d8 * d5 + d6; (d5=densityFactor, d6=densityOffset)
        f32 value = normalizedY * densityFactor + densityOffset;

        // 应用地形偏移 (biome depth/scale 影响)
        // 参考 MC: double d10 = (d9 + d0) * d1; (d0=depthOffset, d1=heightFactor)
        f32 terrainMod = (value + depthOffset) * heightFactor;

        // 参考 MC: if (d10 > 0.0D) d7 = d7 + d10 * 4.0D; else d7 = d7 + d10;
        if (terrainMod > 0.0f) {
            density += terrainMod * 4.0f;
        } else {
            density += terrainMod;
        }

        // 顶部滑动
        // 参考 MC: MathHelper.clampedLerp(target, density, slide)
        // 仅 clamp 插值因子到 [0, 1]，不裁剪 density 本身，避免整列密度被硬截断。
        if (noise.topSlide.size > 0) {
            const f32 slide = static_cast<f32>(m_noiseSizeY - y - noise.topSlide.offset) / static_cast<f32>(noise.topSlide.size);
            density = math::lerp(
                static_cast<f32>(noise.topSlide.target),
                density,
                std::clamp(slide, 0.0f, 1.0f)
            );
        }

        // 底部滑动
        if (noise.bottomSlide.size > 0) {
            const f32 slide = static_cast<f32>(y - noise.bottomSlide.offset) / static_cast<f32>(noise.bottomSlide.size);
            density = math::lerp(
                static_cast<f32>(noise.bottomSlide.target),
                density,
                std::clamp(slide, 0.0f, 1.0f)
            );
        }

        column[y] = density;
    }
}

f32 NoiseChunkGenerator::calculateNoiseDensity(i32 noiseX, i32 noiseY, i32 noiseZ,
                                                f32 xzScale, f32 yScale,
                                                f32 xzFactor, f32 yFactor) const
{
    // 参考 MC func_222552_a
    f32 density = 0.0f;
    f32 secondaryDensity = 0.0f;
    f32 weight = 0.0f;

    f32 octaveScale = 1.0f;

    for (i32 octave = 0; octave < 16; ++octave) {
        // 保持精度
        const f32 px = OctavesNoiseGenerator::maintainPrecision(static_cast<f32>(noiseX) * xzScale * octaveScale);
        const f32 py = OctavesNoiseGenerator::maintainPrecision(static_cast<f32>(noiseY) * yScale * octaveScale);
        const f32 pz = OctavesNoiseGenerator::maintainPrecision(static_cast<f32>(noiseZ) * xzScale * octaveScale);

        const f32 yFreq = yScale * octaveScale;

        // 主密度噪声
        if (m_mainDensityNoise) {
            const ImprovedNoiseGenerator* gen = m_mainDensityNoise->getOctave(octave);
            if (gen) {
                density += gen->noise(px, py, pz, yFreq, static_cast<f32>(noiseY) * yFreq) / octaveScale;
            }
        }

        // 次密度噪声
        if (m_secondaryDensityNoise) {
            const ImprovedNoiseGenerator* gen = m_secondaryDensityNoise->getOctave(octave);
            if (gen) {
                secondaryDensity += gen->noise(px, py, pz, yFreq, static_cast<f32>(noiseY) * yFreq) / octaveScale;
            }
        }

        // 权重噪声（前 8 个倍频）
        if (octave < 8 && m_weightNoise) {
            const ImprovedNoiseGenerator* gen = m_weightNoise->getOctave(octave);
            if (gen) {
                const f32 wpx = OctavesNoiseGenerator::maintainPrecision(static_cast<f32>(noiseX) * xzFactor * octaveScale);
                const f32 wpy = OctavesNoiseGenerator::maintainPrecision(static_cast<f32>(noiseY) * yFactor * octaveScale);
                const f32 wpz = OctavesNoiseGenerator::maintainPrecision(static_cast<f32>(noiseZ) * xzFactor * octaveScale);

                weight += gen->noise(wpx, wpy, wpz, yFactor * octaveScale, static_cast<f32>(noiseY) * yFactor * octaveScale) / octaveScale;
            }
        }

        octaveScale *= 0.5f;
    }

    // 混合密度
    const f32 blend = std::clamp((weight / 10.0f + 1.0f) / 2.0f, 0.0f, 1.0f);
    return math::lerp(density / 512.0f, secondaryDensity / 512.0f, blend);
}

f32 NoiseChunkGenerator::calculateRandomDensityOffset(i32 noiseX, i32 noiseZ) const
{
    if (!m_randomDensityOffsetNoise) {
        return 0.0f;
    }

    // 参考 MC func_236095_c_
    const f32 noise = m_randomDensityOffsetNoise->getValue(
        static_cast<f32>(noiseX) * 200.0f,
        10.0f,
        static_cast<f32>(noiseZ) * 200.0f,
        1.0f,
        0.0f,
        true
    );

    f32 offset;
    if (noise < 0.0f) {
        offset = -noise * 0.3f;
    } else {
        offset = noise;
    }

    const f32 result = offset * 24.575625f - 2.0f;

    if (result < 0.0f) {
        return result * 0.009486607142857142f;
    } else {
        return std::min(result, 1.0f) * 0.006640625f;
    }
}

const BlockState* NoiseChunkGenerator::getBlockForDensity(f32 density, i32 y) const
{
    if (density > 0.0f) {
        return m_settings.defaultBlock;
    } else if (y < m_settings.seaLevel) {
        return m_settings.defaultFluid;
    } else {
        return nullptr;  // 空气
    }
}

f32 NoiseChunkGenerator::sampleSurfaceDepthNoise(i32 worldX, i32 worldZ, i32 localX) const
{
    if (!m_surfaceDepthNoise) {
        return 0.0f;
    }

    const f32 sampleX = static_cast<f32>(worldX) * 0.0625f;
    const f32 sampleZ = static_cast<f32>(worldZ) * 0.0625f;

    // 参考 MC：Perlin 路径会额外乘 0.55，并忽略额外参数。
    if (const auto* perlin = dynamic_cast<const PerlinNoiseGenerator*>(m_surfaceDepthNoise.get())) {
        return perlin->noiseAt(sampleX, sampleZ, true) * 0.55f;
    }

    // 参考 MC：Octaves 路径使用 4 参数 noiseAt。
    if (const auto* octaves = dynamic_cast<const OctavesNoiseGenerator*>(m_surfaceDepthNoise.get())) {
        return octaves->noiseAt(sampleX, sampleZ, 0.0625f, static_cast<f32>(localX) * 0.0625f);
    }

    return m_surfaceDepthNoise->noise2D(sampleX, sampleZ);
}

// ============================================================================
// 地表生成
// ============================================================================

void NoiseChunkGenerator::buildSurface(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "BuildSurface", "x", chunk.x(), "z", chunk.z());
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 设置随机种子（参考 MC: setBaseChunkSeed）
    // 种子 = chunkX * 341873128712 + chunkZ * 132897987541 + worldSeed
    math::Random surfaceRng(static_cast<u64>(chunkX) * 341873128712ULL +
                            static_cast<u64>(chunkZ) * 132897987541ULL +
                            m_seed);

    // === 阶段 1: 遍历列生成地表 ===
    {
        MC_TRACE_EVENT("world.chunk_gen", "BuildSurface_Columns", "phase", "columns");
        // 遍历每个 XZ 列
        for (i32 localX = 0; localX < 16; ++localX) {
            for (i32 localZ = 0; localZ < 16; ++localZ) {
                const i32 worldX = startX + localX;
                const i32 worldZ = startZ + localZ;

                // 参考 MC：从高度图高度再上移 1 格开始扫描。
                const i32 startHeight = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ) + 1;

                // 获取生物群系
                const BiomeId biomeId = region.getBiome(worldX, startHeight, worldZ);

                // 计算地表噪声
                const f32 surfaceNoise = sampleSurfaceDepthNoise(worldX, worldZ, localX) * 15.0f;

                // 生成地表
                buildSurfaceForColumn(chunk, surfaceRng, localX, localZ, startHeight, surfaceNoise, biomeId);
            }
        }
    }

    // === 阶段 2: 生成基岩 ===
    {
        MC_TRACE_EVENT("world.chunk_gen", "BuildSurface_Bedrock", "phase", "bedrock");
        applyBedrock(chunk, surfaceRng);
    }

    // 标记阶段完成
    chunk.setChunkStatus(ChunkStatuses::SURFACE);
}

void NoiseChunkGenerator::buildSurfaceForColumn(
    ChunkPrimer& chunk,
    math::Random& random,
    i32 x,
    i32 z,
    i32 startHeight,
    f32 surfaceNoise,
    BiomeId biome)
{
    if (m_settings.defaultBlock == nullptr || m_settings.defaultFluid == nullptr) {
        return;
    }

    const Biome& biomeDef = m_biomeProvider->getBiomeDefinition(biome);

    if (SurfaceBuilder* specialBuilder = selectSpecialSurfaceBuilder(biomeDef, biome);
        specialBuilder != nullptr) {
        const SurfaceBuilderConfig config = makeSurfaceConfigFromBiome(biomeDef, m_settings.defaultBlock);
        specialBuilder->buildSurface(
            random,
            chunk,
            biomeDef,
            x,
            z,
            startHeight,
            static_cast<f64>(surfaceNoise),  // f32 -> f64
            m_settings.defaultBlock,
            m_settings.defaultFluid,
            m_settings.seaLevel,
            m_seed,  // worldSeed
            config);
        return;
    }

    const i32 seaLevel = m_settings.seaLevel;

    const BlockState* airState = VanillaBlocks::AIR ? &VanillaBlocks::AIR->defaultState() : nullptr;
    const BlockState* iceState = VanillaBlocks::ICE ? &VanillaBlocks::ICE->defaultState() : nullptr;

    const BlockState* topState = biomeDef.surfaceBlock();
    const BlockState* middleState = biomeDef.subSurfaceBlock();
    const BlockState* underWaterState = biomeDef.underWaterBlock();

    // 参考 MC DefaultSurfaceBuilder：每列深度含随机抖动。
    const i32 surfaceDepth = static_cast<i32>(surfaceNoise / 3.0f + 3.0f + random.nextDouble() * 0.25f);

    i32 currentDepth = -1;
    const i32 clampedStartHeight = std::min(startHeight, m_settings.noise.height - 1);

    for (i32 y = clampedStartHeight; y >= 0; --y) {
        const BlockState* state = chunk.getBlockState(x, y, z);

        if (state == nullptr || state->isAir()) {
            currentDepth = -1;
            continue;
        }

        if (state->blockId() != m_settings.defaultBlock->blockId()) {
            continue;
        }

        if (currentDepth == -1) {
            if (surfaceDepth <= 0) {
                topState = airState;
                middleState = m_settings.defaultBlock;
            } else if (y >= seaLevel - 4 && y <= seaLevel + 1) {
                topState = biomeDef.surfaceBlock();
                middleState = biomeDef.subSurfaceBlock();
            }

            if (y < seaLevel && (topState == nullptr || topState->isAir())) {
                if (biomeDef.temperature() < 0.15f && iceState != nullptr) {
                    topState = iceState;
                } else {
                    topState = m_settings.defaultFluid;
                }
            }

            currentDepth = surfaceDepth;
            if (y >= seaLevel - 1) {
                if (topState != nullptr) {
                    chunk.setBlockState(x, y, z, topState);
                }
            } else if (y < seaLevel - 7 - surfaceDepth) {
                topState = airState;
                middleState = m_settings.defaultBlock;
                if (underWaterState != nullptr) {
                    chunk.setBlockState(x, y, z, underWaterState);
                } else if (middleState != nullptr) {
                    chunk.setBlockState(x, y, z, middleState);
                }
            } else if (middleState != nullptr) {
                chunk.setBlockState(x, y, z, middleState);
            }
            continue;
        }

        if (currentDepth > 0) {
            --currentDepth;
            if (middleState != nullptr) {
                chunk.setBlockState(x, y, z, middleState);
            }

            const bool isSandLayer =
                middleState != nullptr &&
                ((VanillaBlocks::SAND != nullptr && middleState->is(VanillaBlocks::SAND)) ||
                 (VanillaBlocks::RED_SAND != nullptr && middleState->is(VanillaBlocks::RED_SAND)));

            if (currentDepth == 0 && isSandLayer && surfaceDepth > 1) {
                currentDepth = random.nextInt(4) + std::max(0, y - 63);

                if (VanillaBlocks::RED_SAND != nullptr && middleState->is(VanillaBlocks::RED_SAND)) {
                    if (VanillaBlocks::RED_SANDSTONE != nullptr) {
                        middleState = &VanillaBlocks::RED_SANDSTONE->defaultState();
                    }
                } else if (VanillaBlocks::SANDSTONE != nullptr) {
                    middleState = &VanillaBlocks::SANDSTONE->defaultState();
                }
            }
        }
    }
}

void NoiseChunkGenerator::applyBedrock(ChunkPrimer& chunk, math::Random& random) const
{
    const BlockState* bedrockState = VanillaBlocks::BEDROCK ? &VanillaBlocks::BEDROCK->defaultState() : nullptr;
    if (bedrockState == nullptr) {
        return;
    }

    const i32 noiseHeight = m_settings.noise.height;
    const i32 floorAnchor = m_settings.bedrockFloor;
    const i32 roofAnchor = m_settings.bedrockRoof;

    const bool hasFloor = floorAnchor + 4 >= 0 && floorAnchor < noiseHeight;
    const bool hasRoof = roofAnchor + 4 >= 0 && roofAnchor < noiseHeight;
    if (!hasFloor && !hasRoof) {
        return;
    }

    for (i32 localX = 0; localX < 16; ++localX) {
        for (i32 localZ = 0; localZ < 16; ++localZ) {
            if (hasFloor) {
                for (i32 offset = 0; offset < 5; ++offset) {
                    if (offset <= random.nextInt(5)) {
                        const i32 y = floorAnchor + offset;
                        if (y >= 0 && y < noiseHeight) {
                            chunk.setBlockState(localX, y, localZ, bedrockState);
                        }
                    }
                }
            }

            if (hasRoof) {
                for (i32 offset = 0; offset < 5; ++offset) {
                    if (offset <= random.nextInt(5)) {
                        const i32 y = roofAnchor - offset;
                        if (y >= 0 && y < noiseHeight) {
                            chunk.setBlockState(localX, y, localZ, bedrockState);
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// 雕刻和特性
// ============================================================================

void NoiseChunkGenerator::applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& chunk, bool isLiquid)
{
    MC_TRACE_EVENT("world.chunk_gen", "ApplyCarvers", "x", chunk.x(), "z", chunk.z());
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();

    // 创建雕刻掩码
    CarvingMask carvingMask(chunkX, chunkZ);

    if (!isLiquid) {
        // 空气雕刻阶段：洞穴和峡谷
        // 应用洞穴雕刻器
        if (m_caveCarver) {
            m_caveCarver->carve(chunk, *m_biomeProvider, m_settings.seaLevel, chunkX, chunkZ, carvingMask, m_caveConfig);
        }

        // 应用峡谷雕刻器
        if (m_canyonCarver) {
            m_canyonCarver->carve(chunk, *m_biomeProvider, m_settings.seaLevel, chunkX, chunkZ, carvingMask, m_canyonConfig);
        }

        chunk.setChunkStatus(ChunkStatuses::CARVERS);
    } else {
        // 液体雕刻阶段：水下洞穴和峡谷
        // 应用水下洞穴雕刻器（使用与普通洞穴相同的概率）
        if (m_underwaterCaveCarver) {
            m_underwaterCaveCarver->carve(chunk, *m_biomeProvider, m_settings.seaLevel, chunkX, chunkZ, carvingMask, m_caveConfig);
        }

        // 应用水下峡谷雕刻器（使用与普通峡谷相同的概率）
        if (m_underwaterCanyonCarver) {
            m_underwaterCanyonCarver->carve(chunk, *m_biomeProvider, m_settings.seaLevel, chunkX, chunkZ, carvingMask, m_canyonConfig);
        }

        chunk.setChunkStatus(ChunkStatuses::LIQUID_CARVERS);
    }
}

void NoiseChunkGenerator::placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk)
{
    MC_TRACE_EVENT("world.chunk_gen", "PlaceFeatures", "x", chunk.x(), "z", chunk.z());
    // 初始化特征注册表（线程安全，仅初始化一次）
    static std::once_flag s_featureRegistryInitFlag;
    std::call_once(s_featureRegistryInitFlag, []() {
        FeatureRegistry::instance().initialize();
    });

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

            const world::gen::structure::Structure* structure = world::gen::structure::StructureRegistry::get(structureName);
            if (!structure) {
                continue;
            }

            // 放置结构片段到当前区块
            structure->placeInChunk(region, chunk, *start, chunkX, chunkZ);
        }
    }

    // === 阶段 2: 放置生物群系特征 ===
    // 获取区块中心位置的主要生物群系
    const BiomeId biomeId = chunk.getBiomeAtBlock(8, 64, 8);
    const Biome& biome = m_biomeProvider->getBiomeDefinition(biomeId);
    const BiomeGenerationSettings& biomeSettings = biome.generationSettings();

    // 按装饰阶段顺序放置特征
    for (DecorationStage stage : DecorationStages::getAll()) {
        BiomeFeaturePlacer::placeFeaturesForStage(
            region, chunk, *this, biomeSettings, stage, m_seed);
    }

    chunk.setChunkStatus(ChunkStatuses::FEATURES);
}

// ============================================================================
// 生物群系
// ============================================================================

BiomeId NoiseChunkGenerator::getBiome(i32 x, i32 y, i32 z) const
{
    return m_biomeProvider->getBiome(x, y, z);
}

BiomeId NoiseChunkGenerator::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    return m_biomeProvider->getNoiseBiome(noiseX, noiseY, noiseZ);
}

i32 NoiseChunkGenerator::getHeight(i32 x, i32 z, HeightmapType type) const
{
    const NoiseSettings& noise = m_settings.noise;
    const i32 hGranularity = m_horizontalNoiseGranularity;
    const i32 vGranularity = m_verticalNoiseGranularity;

    if (hGranularity <= 0 || vGranularity <= 0 || m_noiseSizeY <= 0) {
        return m_settings.seaLevel + 1;
    }

    // 向下取整除法（支持负坐标）
    const i32 noiseX = math::floorDiv(x, hGranularity);
    const i32 noiseZ = math::floorDiv(z, hGranularity);
    const i32 localX = x - noiseX * hGranularity;
    const i32 localZ = z - noiseZ * hGranularity;

    const f32 xLerp = static_cast<f32>(localX) / static_cast<f32>(hGranularity);
    const f32 zLerp = static_cast<f32>(localZ) / static_cast<f32>(hGranularity);

    std::vector<f32> column00;
    std::vector<f32> column01;
    std::vector<f32> column10;
    std::vector<f32> column11;
    BiomeWindowCache biomeWindowCache;

    fillNoiseColumn(column00, noiseX, noiseZ, biomeWindowCache);
    fillNoiseColumn(column01, noiseX, noiseZ + 1, biomeWindowCache);
    fillNoiseColumn(column10, noiseX + 1, noiseZ, biomeWindowCache);
    fillNoiseColumn(column11, noiseX + 1, noiseZ + 1, biomeWindowCache);

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
                return (block.isSolid(*state) || state->isLiquid()) &&
                       (&block.material() != &Material::LEAVES) &&
                       (&block.material() != &Material::PLANT);

            case HeightmapType::LightBlocking:
                return block.isSolid(*state) && state->getOpacity() > 0;

            default:
                return true;
        }
    };

    for (i32 worldY = noise.height - 1; worldY >= 0; --worldY) {
        const i32 noiseY = worldY / vGranularity;
        const i32 localY = worldY % vGranularity;

        if (noiseY < 0 || noiseY >= m_noiseSizeY) {
            continue;
        }

        const f32 yLerp = static_cast<f32>(localY) / static_cast<f32>(vGranularity);

        const f32 y00 = math::lerp(column00[noiseY], column00[noiseY + 1], yLerp);
        const f32 y01 = math::lerp(column01[noiseY], column01[noiseY + 1], yLerp);
        const f32 y10 = math::lerp(column10[noiseY], column10[noiseY + 1], yLerp);
        const f32 y11 = math::lerp(column11[noiseY], column11[noiseY + 1], yLerp);

        const f32 x0 = math::lerp(y00, y10, xLerp);
        const f32 x1 = math::lerp(y01, y11, xLerp);
        const f32 density = math::lerp(x0, x1, zLerp);

        const BlockState* blockState = getBlockForDensity(density, worldY);
        if (matchesHeightmap(blockState)) {
            return worldY + 1;
        }
    }

    return 0;
}

i32 NoiseChunkGenerator::spawnInitialMobs(WorldGenRegion& region, ChunkPrimer& chunk,
                                          std::vector<SpawnedEntityData>& outEntities)
{
    MC_TRACE_EVENT("world.chunk_gen", "NoiseChunkGenerator::spawnInitialMobs", "x", chunk.x(), "z", chunk.z());

    // 使用 WorldGenSpawner 放置被动动物
    if (!m_worldGenSpawner || !m_worldGenSpawner->isEnabled()) {
        spdlog::warn("[NoiseChunkGenerator] WorldGenSpawner is not enabled. Skipping initial mob spawning.");
        return 0;
    }

    // 获取区块中心位置的生物群系
    const BiomeId biomeId = chunk.getBiomeAtBlock(8, 64, 8);
    const Biome& biome = m_biomeProvider->getBiomeDefinition(biomeId);

    // 使用种子创建随机数生成器
    // 参考 MC: setDecorationSeed
    math::Random rng;
    rng.setSeed(static_cast<u64>(chunk.x()) * 341873128712ULL +
                static_cast<u64>(chunk.z()) * 132897987541ULL +
                m_seed);

    return m_worldGenSpawner->spawnInitialMobs(region, biome, chunk.x(), chunk.z(), *this, rng, outEntities);
}

// ============================================================================
// JigsawJunction 地形平滑
// ============================================================================

void NoiseChunkGenerator::collectStructureData(
    ChunkPrimer& chunk,
    std::vector<const world::gen::structure::StructurePiece*>& outPieces,
    std::vector<world::gen::jigsaw::JigsawJunction>& outJunctions) const
{
    MC_TRACE_EVENT("world.chunk_gen", "CollectStructureData", "x", chunk.x(), "z", chunk.z());

    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    // 参考 MC 1.16.5: NoiseChunkGenerator.func_230352_b_
    // 收集 12 格范围内的结构片段和 JigsawJunction

    // 遍历区块中的所有结构起点
    for (const auto& [structureName, start] : chunk.structureStarts()) {
        if (!start || !start->isValid()) {
            continue;
        }

        // 遍历结构中的所有片段
        for (const auto& piece : start->pieces()) {
            if (!piece) {
                continue;
            }

            // 检查片段是否在区块附近（12 格范围）
            const auto& box = piece->getBoundingBox();
            if (box.maxX() < startX - 12 || box.minX() > startX + 15 + 12 ||
                box.maxZ() < startZ - 12 || box.minZ() > startZ + 15 + 12) {
                continue;  // 超出范围
            }

            // 添加结构片段
            outPieces.push_back(piece.get());

            // 如果是 Jigsaw 结构片段，收集其 Junctions
            if (piece->isJigsawPiece()) {
                for (const auto& junction : piece->getJunctions()) {
                    // 检查 Junction 是否在区块附近（12 格范围）
                    const i32 jx = junction.getSourceX();
                    const i32 jz = junction.getSourceZ();
                    if (jx > startX - 12 && jx < startX + 15 + 12 &&
                        jz > startZ - 12 && jz < startZ + 15 + 12) {
                        outJunctions.push_back(junction);
                    }
                }
            }
        }
    }
}

} // namespace mc
