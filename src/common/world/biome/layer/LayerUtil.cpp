#include "LayerUtil.hpp"
#include "../BiomeRegistry.hpp"
#include "LayerCacheConfig.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// LayerStack 实现
// ============================================================================

LayerStack::LayerStack(std::unique_ptr<IArea> area)
    : m_area(std::move(area))
{}

BiomeId LayerStack::sample(i32 x, i32 z) const
{
    MC_TRACE_EVENT("world.biome", "LayerStack_Sample", "x", x, "z", z);
    if (!m_area) {
        return Biomes::Plains;
    }

    i32 value = m_area->getValue(x, z);

    // 确保值是有效的生物群系 ID
    if (value >= 0 && value < static_cast<i32>(Biomes::Count)) {
        return static_cast<BiomeId>(value);
    }

    // 可能是稀有变体或其他有效 ID
    if (value >= 129 && value <= 169) {
        return static_cast<BiomeId>(value);
    }

    // 河流值 (7) 是有效的
    if (value == layer::BiomeValues::River) {
        return Biomes::River;
    }
    if (value == layer::BiomeValues::FrozenRiver) {
        return Biomes::FrozenRiver;
    }

    // 河流噪声值（>= 2 且不是生物群系 ID）或 -1（无河流）
    // 返回 Plains 作为默认
    return Biomes::Plains;
}

std::vector<BiomeId> LayerStack::sampleArea(i32 startX, i32 startZ, i32 width, i32 height) const
{
    MC_TRACE_EVENT(
        "world.biome", "LayerStack_SampleArea", "startX", startX, "startZ", startZ, "width", width, "height", height);
    std::vector<BiomeId> result;
    result.reserve(static_cast<size_t>(width) * height);

    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            result.push_back(sample(startX + x, startZ + z));
        }
    }

    return result;
}

void LayerStack::sampleBatch(i32 startX, i32 startZ, i32 width, i32 height, BiomeId* output) const
{
    MC_TRACE_EVENT(
        "world.biome", "LayerStack_SampleBatch", "startX", startX, "startZ", startZ, "width", width, "height", height);

    if (!m_area || output == nullptr || width <= 0 || height <= 0) {
        return;
    }

    // 使用批量采样接口
    const size_t totalSize = static_cast<size_t>(width) * height;
    std::vector<i32> values(totalSize);
    m_area->getValuesBatch(startX, startZ, width, height, values.data());

    // 转换为 BiomeId
    for (size_t i = 0; i < totalSize; ++i) {
        i32 value = values[i];

        // 确保值是有效的生物群系 ID
        if (value >= 0 && value < static_cast<i32>(Biomes::Count)) {
            output[i] = static_cast<BiomeId>(value);
        } else if (value >= 129 && value <= 169) {
            // 可能是稀有变体或其他有效 ID
            output[i] = static_cast<BiomeId>(value);
        } else if (value == layer::BiomeValues::River) {
            output[i] = Biomes::River;
        } else if (value == layer::BiomeValues::FrozenRiver) {
            output[i] = Biomes::FrozenRiver;
        } else {
            // 默认返回 Plains
            output[i] = Biomes::Plains;
        }
    }
}

// ============================================================================
// LayerUtil 实现 - 完整的 MC 1.16.5 层链（包含河流分支）
// ============================================================================

namespace LayerUtil {

namespace {

/**
 * @brief 构建气候层链的基础部分
 *
 * 构建从 IslandLayer 到 DeepOceanLayer 的公共层链。
 * 这部分被生物群系分支和河流分支共同使用。
 *
 * @return 气候层工厂
 */
std::unique_ptr<IAreaFactory> buildClimateLayers(
    u64 seed, const std::function<std::shared_ptr<LayerContext>(u64)>& createContext)
{
    MC_TRACE_EVENT("world.biome", "BuildClimateLayers");

    static layer::IslandLayer islandLayer;
    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    static layer::AddIslandLayer addIslandLayer;
    static layer::RemoveTooMuchOceanLayer removeTooMuchOceanLayer;
    static layer::AddSnowLayer addSnowLayer;
    static layer::CoolWarmEdgeLayer coolWarmEdgeLayer;
    static layer::HeatIceEdgeLayer heatIceEdgeLayer;
    static layer::SpecialEdgeLayer specialEdgeLayer;
    static layer::AddMushroomIslandLayer addMushroomIslandLayer;
    static layer::DeepOceanLayer deepOceanLayer;

    auto context = createContext(1L);
    auto factory = islandLayer.apply(*context);

    context = createContext(2000L);
    factory = fuzzyZoom.apply(*context, std::move(factory));

    context = createContext(1L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(2001L);
    factory = normalZoom.apply(*context, std::move(factory));

    context = createContext(2L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(50L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(70L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(2L);
    factory = removeTooMuchOceanLayer.apply(*context, std::move(factory));

    context = createContext(2L);
    factory = addSnowLayer.apply(*context, std::move(factory));

    context = createContext(3L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(2L);
    factory = coolWarmEdgeLayer.apply(*context, std::move(factory));

    context = createContext(2L);
    factory = heatIceEdgeLayer.apply(*context, std::move(factory));

    context = createContext(3L);
    factory = specialEdgeLayer.apply(*context, std::move(factory));

    context = createContext(2002L);
    factory = normalZoom.apply(*context, std::move(factory));

    context = createContext(2003L);
    factory = normalZoom.apply(*context, std::move(factory));

    context = createContext(4L);
    factory = addIslandLayer.apply(*context, std::move(factory));

    context = createContext(5L);
    factory = addMushroomIslandLayer.apply(*context, std::move(factory));

    context = createContext(4L);
    factory = deepOceanLayer.apply(*context, std::move(factory));

    return factory;
}

} // anonymous namespace

std::unique_ptr<IAreaFactory> repeatZoom(u64 seed,
    layer::ZoomLayer& zoom,
    std::unique_ptr<IAreaFactory> input,
    i32 count,
    std::function<std::shared_ptr<LayerContext>(u64)> contextFactory)
{
    MC_TRACE_EVENT("world.biome", "RepeatZoom", "count", count);
    std::unique_ptr<IAreaFactory> result = std::move(input);

    for (i32 i = 0; i < count; ++i) {
        auto context = contextFactory(seed + static_cast<u64>(i));
        result = zoom.apply(*context, std::move(result));
    }

    return result;
}

std::unique_ptr<IAreaFactory> buildOverworldLayers(
    u64 seed, bool legacyBiomeInit, bool largeBiomes, i32 biomeSize, i32 riverSize)
{
    MC_TRACE_EVENT("world.biome", "BuildOverworldLayers", "biomeSize", biomeSize, "riverSize", riverSize);

    // 实现 largeBiomes 参数支持
    // 参考 MC 1.16.5: biomeSize = largeBiomes ? 6 : 4, riverSize = largeBiomes ? 6 : 4
    if (largeBiomes) {
        biomeSize = 6;
        riverSize = 6;
    }

    // 创建上下文工厂 - 使用 shared_ptr 保持生命周期
    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(LayerCacheConfig::DEFAULT_CACHE_SIZE, seed, modifier);
    };

    // 静态层实例（避免重复构造）
    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    static layer::StartRiverLayer startRiverLayer;
    static layer::BiomeLayer biomeLayer(layer::BiomeLayer::Config{});
    static layer::AddBambooForestLayer addBambooForestLayer;
    static layer::BiomeEdgeLayer biomeEdgeLayer;
    static layer::HillsLayer hillsLayer;
    static layer::RiverLayer riverLayer;
    static layer::SmoothLayer smoothLayer;
    static layer::RareBiomeLayer rareBiomeLayer;
    static layer::ShoreLayer shoreLayer;
    static layer::MixRiverLayer mixRiverLayer;
    static layer::MixOceansLayer mixOceansLayer;
    static layer::OceanLayer oceanLayer;
    static layer::AddIslandLayer addIslandLayer;

    // ========================================================================
    // 第一阶段：气候层（只计算一次，通过 SharedFactory 复用）
    // 参考 MC 1.16.5 LayerUtil.func_237216_a_
    // ========================================================================
    auto climateFactory = std::make_shared<SharedFactory>(buildClimateLayers(seed, createContext));

    // ========================================================================
    // 第二阶段：海洋温度分支（独立）
    // ========================================================================
    auto context = createContext(2L);
    auto oceanFactory = oceanLayer.apply(*context);
    oceanFactory = repeatZoom(2001L, normalZoom, std::move(oceanFactory), 6, createContext);

    // ========================================================================
    // 第三阶段：河流分支（从气候层派生）
    // 注意：MC Java 的河流分支架构：
    //   climateFactory → StartRiverLayer → Zoom(2次) → [分支给 HillsLayer]
    //                                            → [继续 Zoom → RiverLayer → MixRiverLayer]
    // ========================================================================

    // 河流基础层（从气候层派生）
    context = createContext(100L);
    auto riverBaseFactory = startRiverLayer.apply(*context, climateFactory->share());
    riverBaseFactory = repeatZoom(1000L, normalZoom, std::move(riverBaseFactory), 2, createContext);

    // 分支1：给 HillsLayer 使用（缩放 2 次后的河流噪声）
    auto riverFactoryForHills = repeatZoom(1000L, normalZoom, climateFactory->share(), 0, createContext);
    context = createContext(100L);
    riverFactoryForHills = startRiverLayer.apply(*context, std::move(riverFactoryForHills));
    riverFactoryForHills = repeatZoom(1000L, normalZoom, std::move(riverFactoryForHills), 2, createContext);

    // 分支2：给 MixRiverLayer 使用（继续缩放到 riverSize 次）
    context = createContext(100L);
    auto riverFactoryForMix = startRiverLayer.apply(*context, climateFactory->share());
    riverFactoryForMix = repeatZoom(1000L, normalZoom, std::move(riverFactoryForMix), 2, createContext);
    riverFactoryForMix = repeatZoom(1000L, normalZoom, std::move(riverFactoryForMix), riverSize, createContext);
    context = createContext(1L);
    riverFactoryForMix = riverLayer.apply(*context, std::move(riverFactoryForMix));
    context = createContext(1000L);
    riverFactoryForMix = smoothLayer.apply(*context, std::move(riverFactoryForMix));

    // ========================================================================
    // 第四阶段：生物群系分支（从气候层派生）
    // ========================================================================
    context = createContext(200L);
    auto biomeFactory = biomeLayer.apply(*context, climateFactory->share());

    // AddBambooForestLayer
    context = createContext(1001L);
    biomeFactory = addBambooForestLayer.apply(*context, std::move(biomeFactory));

    // 生物群系缩放 2 次
    biomeFactory = repeatZoom(1000L, normalZoom, std::move(biomeFactory), 2, createContext);

    // BiomeEdgeLayer
    context = createContext(1000L);
    biomeFactory = biomeEdgeLayer.apply(*context, std::move(biomeFactory));

    // ========================================================================
    // 第五阶段：山丘层（合并生物群系和河流噪声）
    // ========================================================================
    context = createContext(1000L);
    biomeFactory = hillsLayer.apply(*context, std::move(biomeFactory), std::move(riverFactoryForHills));

    // ========================================================================
    // 第六阶段：稀有生物群系和海岸
    // ========================================================================
    context = createContext(1001L);
    biomeFactory = rareBiomeLayer.apply(*context, std::move(biomeFactory));

    for (i32 i = 0; i < biomeSize; ++i) {
        context = createContext(static_cast<u64>(1000 + i));
        biomeFactory = normalZoom.apply(*context, std::move(biomeFactory));

        if (i == 0) {
            context = createContext(3L);
            biomeFactory = addIslandLayer.apply(*context, std::move(biomeFactory));
        }

        // 修复：ShoreLayer 应该在 i == 1 或 biomeSize == 1 时执行
        // 参考 MC 1.16.5: if (i == 1 || biomeSize == 1)
        if (i == 1 || biomeSize == 1) {
            context = createContext(1000L);
            biomeFactory = shoreLayer.apply(*context, std::move(biomeFactory));
        }
    }

    context = createContext(1000L);
    biomeFactory = smoothLayer.apply(*context, std::move(biomeFactory));

    // ========================================================================
    // 第七阶段：合并河流
    // ========================================================================
    context = createContext(100L);
    auto finalFactory = mixRiverLayer.apply(*context, std::move(biomeFactory), std::move(riverFactoryForMix));

    // ========================================================================
    // 第八阶段：合并海洋温度
    // ========================================================================
    context = createContext(100L);
    finalFactory = mixOceansLayer.apply(*context, std::move(finalFactory), std::move(oceanFactory));

    return finalFactory;
}

std::unique_ptr<LayerStack> createOverworldLayers(u64 seed, bool isLargeBiomes)
{
    MC_TRACE_EVENT(
        "world.biome", "CreateOverworldLayers", "seed", static_cast<i64>(seed), "isLargeBiomes", isLargeBiomes);
    // 初始化生物群系注册表
    BiomeRegistry::instance().initialize();

    // 构建层链
    auto factory = buildOverworldLayers(seed, false, isLargeBiomes);

    // 创建最终区域
    auto area = factory->create();

    return std::make_unique<LayerStack>(std::move(area));
}

std::unique_ptr<LayerStack> createNetherLayers(u64 seed)
{
    // 下界使用简化的层堆叠
    BiomeRegistry::instance().initialize();
    auto context = std::make_shared<LayerContext>(LayerCacheConfig::DEFAULT_CACHE_SIZE, seed, 1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*context);
    auto area = factory->create();
    return std::make_unique<LayerStack>(std::move(area));
}

std::unique_ptr<LayerStack> createEndLayers(u64 seed)
{
    // 末地使用简化的层堆叠
    BiomeRegistry::instance().initialize();
    auto context = std::make_shared<LayerContext>(LayerCacheConfig::DEFAULT_CACHE_SIZE, seed, 1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*context);
    auto area = factory->create();
    return std::make_unique<LayerStack>(std::move(area));
}

} // namespace LayerUtil

// ============================================================================
// LayerBiomeProvider 实现
// ============================================================================

LayerBiomeProvider::LayerBiomeProvider(u64 seed, bool isLargeBiomes)
    : BiomeProvider(seed)
    , m_layerStack(LayerUtil::createOverworldLayers(seed, isLargeBiomes))
{}

BiomeId LayerBiomeProvider::getBiome(i32 x, i32 y, i32 z) const
{
    MC_TRACE_EVENT("world.biome", "LayerBiomeProvider_GetBiome", "x", x, "z", z);
    (void)y; // Layer 系统不使用 Y 坐标
    return m_layerStack->sample(x, z);
}

BiomeId LayerBiomeProvider::getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const
{
    MC_TRACE_EVENT("world.biome", "LayerBiomeProvider_GetNoiseBiome", "noiseX", noiseX, "noiseZ", noiseZ);
    (void)noiseY; // Layer 系统不使用 Y 坐标
    // 对齐 MC 1.16.5 OverworldBiomeProvider：噪声坐标直接传入层采样。
    return m_layerStack->sample(noiseX, noiseZ);
}

f32 LayerBiomeProvider::getDepth(i32 x, i32 z) const
{
    MC_TRACE_EVENT("world.biome", "LayerBiomeProvider_GetDepth", "x", x, "z", z);
    const BiomeId biomeId = m_layerStack->sample(x, z);
    const Biome& biome = BiomeRegistry::instance().get(biomeId);
    return biome.depth();
}

f32 LayerBiomeProvider::getScale(i32 x, i32 z) const
{
    MC_TRACE_EVENT("world.biome", "LayerBiomeProvider_GetScale", "x", x, "z", z);
    const BiomeId biomeId = m_layerStack->sample(x, z);
    const Biome& biome = BiomeRegistry::instance().get(biomeId);
    return biome.scale();
}

const Biome& LayerBiomeProvider::getBiomeDefinition(BiomeId id) const
{
    return BiomeRegistry::instance().get(id);
}

void LayerBiomeProvider::fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ)
{
    MC_TRACE_EVENT("world.biome", "LayerBiomeProvider_FillBiomeContainer", "chunkX", chunkX, "chunkZ", chunkZ);
    const i32 startNoiseX = chunkX << 2;
    const i32 startNoiseZ = chunkZ << 2;

    // 使用批量采样优化
    // BiomeContainer 是 4x4x4 网格，水平坐标使用噪声坐标（每格对应 4 方块）
    std::array<BiomeId, 16> horizontalBiomes;
    getNoiseBiomesBatch(startNoiseX, 0, startNoiseZ, 4, 4, horizontalBiomes.data());

    // 填充到容器（高度方向复用水平分布）
    for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
        for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                const i32 idx = bz * BiomeContainer::BIOME_WIDTH + bx;
                container.setBiome(bx, by, bz, horizontalBiomes[idx]);
            }
        }
    }
}

void LayerBiomeProvider::getBiomesBatch(
    i32 startX, i32 startY, i32 startZ, i32 width, i32 height, BiomeId* output) const
{
    MC_TRACE_EVENT("world.biome",
        "LayerBiomeProvider_GetBiomesBatch",
        "startX",
        startX,
        "startZ",
        startZ,
        "width",
        width,
        "height",
        height);
    (void)startY; // Layer 系统不使用 Y 坐标

    m_layerStack->sampleBatch(startX, startZ, width, height, output);
}

void LayerBiomeProvider::getNoiseBiomesBatch(
    i32 startNoiseX, i32 startNoiseY, i32 startNoiseZ, i32 width, i32 height, BiomeId* output) const
{
    MC_TRACE_EVENT("world.biome",
        "LayerBiomeProvider_GetNoiseBiomesBatch",
        "startNoiseX",
        startNoiseX,
        "startNoiseZ",
        startNoiseZ,
        "width",
        width,
        "height",
        height);
    (void)startNoiseY; // Layer 系统不使用 Y 坐标

    if (output == nullptr || width <= 0 || height <= 0) {
        return;
    }

    // 对齐 getNoiseBiome：噪声坐标直接采样层系统。
    size_t idx = 0;
    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            output[idx++] = m_layerStack->sample(startNoiseX + x, startNoiseZ + z);
        }
    }
}

} // namespace mc
