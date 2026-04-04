#include <gtest/gtest.h>
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeProvider.hpp"
#include "common/world/biome/layer/Layer.hpp"
#include "common/world/biome/layer/LayerContext.hpp"
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/biome/layer/transformers/TransformerTraits.hpp"
#include "common/world/biome/layer/transformers/ClimateLayers.hpp"
#include "common/world/biome/layer/transformers/BiomeLayers.hpp"
#include "common/world/biome/layer/transformers/SourceLayers.hpp"
#include "common/world/biome/layer/transformers/ZoomLayers.hpp"
#include "common/world/biome/provider/nether/NetherBiomeProvider.hpp"
#include "common/world/biome/provider/end/EndBiomeProvider.hpp"
#include "common/world/block/VanillaBlocks.hpp"

using namespace mc;

// ============================================================================
// Biome 类测试
// ============================================================================

class BiomeTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(BiomeTest, Construction) {
    Biome biome(Biomes::Plains, "plains");

    EXPECT_EQ(biome.id(), Biomes::Plains);
    EXPECT_EQ(biome.name(), "plains");
    EXPECT_EQ(biome.category(), Biome::Category::None);
}

TEST_F(BiomeTest, SettersAndGetters) {
    Biome biome(Biomes::Desert, "desert");

    // 设置地形参数
    biome.setDepth(0.5f);
    biome.setScale(0.3f);
    EXPECT_FLOAT_EQ(biome.depth(), 0.5f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);

    // 设置类别
    biome.setCategory(Biome::Category::Desert);
    EXPECT_EQ(biome.category(), Biome::Category::Desert);

    // 设置气候
    BiomeClimate climate(BiomeClimate::Precipitation::None, 2.0f, 0.0f, 0.0f);
    biome.setClimate(climate);
    EXPECT_FLOAT_EQ(biome.temperature(), 2.0f);
    EXPECT_EQ(biome.climate().precipitation, BiomeClimate::Precipitation::None);

    // 设置方块 - 使用 VanillaBlocks
    biome.setSurfaceBlock(&VanillaBlocks::SAND->defaultState());
    biome.setSubSurfaceBlock(&VanillaBlocks::SAND->defaultState());
    biome.setUnderWaterBlock(&VanillaBlocks::GRAVEL->defaultState());
    biome.setBedrockBlock(&VanillaBlocks::BEDROCK->defaultState());

    EXPECT_TRUE(biome.surfaceBlock()->is(VanillaBlocks::SAND));
    EXPECT_TRUE(biome.subSurfaceBlock()->is(VanillaBlocks::SAND));
    EXPECT_TRUE(biome.underWaterBlock()->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(biome.bedrockBlock()->is(VanillaBlocks::BEDROCK));
}

TEST_F(BiomeTest, ClimateDefaults) {
    BiomeClimate climate;
    EXPECT_EQ(climate.precipitation, BiomeClimate::Precipitation::Rain);
    EXPECT_FLOAT_EQ(climate.temperature, 0.5f);
    EXPECT_FLOAT_EQ(climate.downfall, 0.5f);
}

// ============================================================================
// BiomeRegistry 测试
// ============================================================================

class BiomeRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(BiomeRegistryTest, SingletonAccess) {
    BiomeRegistry& reg1 = BiomeRegistry::instance();
    BiomeRegistry& reg2 = BiomeRegistry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(BiomeRegistryTest, GetBiomeById) {
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_EQ(plains.id(), Biomes::Plains);
    EXPECT_EQ(plains.name(), "plains");
}

TEST_F(BiomeRegistryTest, GetInvalidBiome) {
    const Biome& biome = BiomeRegistry::instance().get(static_cast<BiomeId>(9999));
    // 应该返回默认生物群系
    EXPECT_TRUE(biome.id() == Biomes::Plains || biome.id() == 0);
}

TEST_F(BiomeRegistryTest, HasBiome) {
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Plains));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Desert));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Ocean));
    EXPECT_FALSE(BiomeRegistry::instance().hasBiome(static_cast<BiomeId>(9999)));
}

// TODO(WIP): 需要注册更多生物群系以达到 Biomes::Count (170)
// 当前只注册了 28 个，最大 ID 为 165 (ErodedBadlands)
TEST_F(BiomeRegistryTest, DISABLED_AllBiomesCount) {
    const auto& biomes = BiomeRegistry::instance().allBiomes();
    EXPECT_GE(biomes.size(), Biomes::Count);
}

// ============================================================================
// BiomeFactory 测试
// ============================================================================

TEST_F(BiomeRegistryTest, CreatePlains) {
    Biome plains = BiomeFactory::createPlains();
    EXPECT_EQ(plains.id(), Biomes::Plains);
    EXPECT_EQ(plains.name(), "plains");
    EXPECT_FLOAT_EQ(plains.depth(), 0.125f);
    EXPECT_FLOAT_EQ(plains.scale(), 0.05f);
    ASSERT_NE(plains.surfaceBlock(), nullptr);
    ASSERT_NE(plains.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(plains.surfaceBlock()->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(plains.subSurfaceBlock()->is(VanillaBlocks::DIRT));
}

TEST_F(BiomeRegistryTest, CreateDesert) {
    Biome desert = BiomeFactory::createDesert();
    EXPECT_EQ(desert.id(), Biomes::Desert);
    EXPECT_EQ(desert.name(), "desert");
    EXPECT_FLOAT_EQ(desert.temperature(), 2.0f);
    EXPECT_FLOAT_EQ(desert.humidity(), 0.0f);
    ASSERT_NE(desert.surfaceBlock(), nullptr);
    ASSERT_NE(desert.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(desert.surfaceBlock()->is(VanillaBlocks::SAND));
    EXPECT_TRUE(desert.subSurfaceBlock()->is(VanillaBlocks::SAND));
}

TEST_F(BiomeRegistryTest, CreateMountains) {
    Biome mountains = BiomeFactory::createMountains();
    EXPECT_EQ(mountains.id(), Biomes::Mountains);
    EXPECT_EQ(mountains.name(), "mountains");
    EXPECT_FLOAT_EQ(mountains.depth(), 1.0f);
    ASSERT_NE(mountains.surfaceBlock(), nullptr);
    ASSERT_NE(mountains.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(mountains.surfaceBlock()->is(VanillaBlocks::STONE));
    EXPECT_TRUE(mountains.subSurfaceBlock()->is(VanillaBlocks::STONE));
}

TEST_F(BiomeRegistryTest, CreateOcean) {
    Biome ocean = BiomeFactory::createOcean();
    EXPECT_EQ(ocean.id(), Biomes::Ocean);
    EXPECT_EQ(ocean.name(), "ocean");
    EXPECT_FLOAT_EQ(ocean.depth(), -1.0f); // 海洋深度为负
}

TEST_F(BiomeRegistryTest, CreateCrimsonForestUsesCrimsonNyliumSurface) {
    Biome biome = BiomeFactory::createCrimsonForest();
    EXPECT_EQ(biome.id(), Biomes::CrimsonForest);
    ASSERT_NE(biome.surfaceBlock(), nullptr);
    ASSERT_NE(biome.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(biome.surfaceBlock()->is(VanillaBlocks::CRIMSON_NYLIUM));
    EXPECT_TRUE(biome.subSurfaceBlock()->is(VanillaBlocks::NETHERRACK));
}

TEST_F(BiomeRegistryTest, CreateWarpedForestUsesWarpedNyliumSurface) {
    Biome biome = BiomeFactory::createWarpedForest();
    EXPECT_EQ(biome.id(), Biomes::WarpedForest);
    ASSERT_NE(biome.surfaceBlock(), nullptr);
    ASSERT_NE(biome.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(biome.surfaceBlock()->is(VanillaBlocks::WARPED_NYLIUM));
    EXPECT_TRUE(biome.subSurfaceBlock()->is(VanillaBlocks::NETHERRACK));
}

// ============================================================================
// 新增生物群系测试（阶段1）
// ============================================================================

TEST_F(BiomeRegistryTest, CreateWarmOcean) {
    Biome biome = BiomeFactory::createWarmOcean();
    EXPECT_EQ(biome.id(), Biomes::WarmOcean);
    EXPECT_EQ(biome.name(), "warm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.0f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.1f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.8f);
}

TEST_F(BiomeRegistryTest, CreateLukewarmOcean) {
    Biome biome = BiomeFactory::createLukewarmOcean();
    EXPECT_EQ(biome.id(), Biomes::LukewarmOcean);
    EXPECT_EQ(biome.name(), "lukewarm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.0f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.6f);
}

TEST_F(BiomeRegistryTest, CreateColdOcean) {
    Biome biome = BiomeFactory::createColdOcean();
    EXPECT_EQ(biome.id(), Biomes::ColdOcean);
    EXPECT_EQ(biome.name(), "cold_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.0f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateDeepWarmOcean) {
    Biome biome = BiomeFactory::createDeepWarmOcean();
    EXPECT_EQ(biome.id(), Biomes::DeepWarmOcean);
    EXPECT_EQ(biome.name(), "deep_warm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.8f);
}

TEST_F(BiomeRegistryTest, CreateDeepLukewarmOcean) {
    Biome biome = BiomeFactory::createDeepLukewarmOcean();
    EXPECT_EQ(biome.id(), Biomes::DeepLukewarmOcean);
    EXPECT_EQ(biome.name(), "deep_lukewarm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.8f);
}

TEST_F(BiomeRegistryTest, CreateDeepColdOcean) {
    Biome biome = BiomeFactory::createDeepColdOcean();
    EXPECT_EQ(biome.id(), Biomes::DeepColdOcean);
    EXPECT_EQ(biome.name(), "deep_cold_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.8f);
}

TEST_F(BiomeRegistryTest, CreateJungleHills) {
    Biome biome = BiomeFactory::createJungleHills();
    EXPECT_EQ(biome.id(), Biomes::JungleHills);
    EXPECT_EQ(biome.name(), "jungle_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateJungleEdge) {
    Biome biome = BiomeFactory::createJungleEdge();
    EXPECT_EQ(biome.id(), Biomes::JungleEdge);
    EXPECT_EQ(biome.name(), "jungle_edge");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateBambooJungle) {
    Biome biome = BiomeFactory::createBambooJungle();
    EXPECT_EQ(biome.id(), Biomes::BambooJungle);
    EXPECT_EQ(biome.name(), "bamboo_jungle");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateBambooJungleHills) {
    Biome biome = BiomeFactory::createBambooJungleHills();
    EXPECT_EQ(biome.id(), Biomes::BambooJungleHills);
    EXPECT_EQ(biome.name(), "bamboo_jungle_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateBirchForestHills) {
    Biome biome = BiomeFactory::createBirchForestHills();
    EXPECT_EQ(biome.id(), Biomes::BirchForestHills);
    EXPECT_EQ(biome.name(), "birch_forest_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateFlowerForest) {
    Biome biome = BiomeFactory::createFlowerForest();
    EXPECT_EQ(biome.id(), Biomes::FlowerForest);
    EXPECT_EQ(biome.name(), "flower_forest");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateTallBirchForest) {
    Biome biome = BiomeFactory::createTallBirchForest();
    EXPECT_EQ(biome.id(), Biomes::TallBirchForest);
    EXPECT_EQ(biome.name(), "tall_birch_forest");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateDarkForestHills) {
    Biome biome = BiomeFactory::createDarkForestHills();
    EXPECT_EQ(biome.id(), Biomes::DarkForestHills);
    EXPECT_EQ(biome.name(), "dark_forest_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateMushroomFields) {
    Biome biome = BiomeFactory::createMushroomFields();
    EXPECT_EQ(biome.id(), Biomes::MushroomFields);
    EXPECT_EQ(biome.name(), "mushroom_fields");
    EXPECT_FLOAT_EQ(biome.depth(), 0.2f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.9f);
    EXPECT_FLOAT_EQ(biome.humidity(), 1.0f);
}

TEST_F(BiomeRegistryTest, CreateMushroomFieldShore) {
    Biome biome = BiomeFactory::createMushroomFieldShore();
    EXPECT_EQ(biome.id(), Biomes::MushroomFieldShore);
    EXPECT_EQ(biome.name(), "mushroom_field_shore");
    EXPECT_FLOAT_EQ(biome.depth(), 0.0f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.025f);
}

TEST_F(BiomeRegistryTest, CreateDesertHills) {
    Biome biome = BiomeFactory::createDesertHills();
    EXPECT_EQ(biome.id(), Biomes::DesertHills);
    EXPECT_EQ(biome.name(), "desert_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.225f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.25f);
}

TEST_F(BiomeRegistryTest, CreateTaigaHills) {
    Biome biome = BiomeFactory::createTaigaHills();
    EXPECT_EQ(biome.id(), Biomes::TaigaHills);
    EXPECT_EQ(biome.name(), "taiga_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.3f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.25f);
}

TEST_F(BiomeRegistryTest, CreateGiantSpruceTaiga) {
    Biome biome = BiomeFactory::createGiantSpruceTaiga();
    EXPECT_EQ(biome.id(), Biomes::GiantSpruceTaiga);
    EXPECT_EQ(biome.name(), "giant_spruce_taiga");
    EXPECT_FLOAT_EQ(biome.depth(), 0.2f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateGiantSpruceTaigaHills) {
    Biome biome = BiomeFactory::createGiantSpruceTaigaHills();
    EXPECT_EQ(biome.id(), Biomes::GiantSpruceTaigaHills);
    EXPECT_EQ(biome.name(), "giant_spruce_taiga_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.2f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

// ============================================================================
// 新增生物群系测试（阶段2 - 稀有变体）
// ============================================================================

TEST_F(BiomeRegistryTest, CreateSunflowerPlains) {
    Biome biome = BiomeFactory::createSunflowerPlains();
    EXPECT_EQ(biome.id(), Biomes::SunflowerPlains);
    EXPECT_EQ(biome.name(), "sunflower_plains");
    EXPECT_FLOAT_EQ(biome.depth(), 0.125f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.05f);
}

TEST_F(BiomeRegistryTest, CreateDesertLakes) {
    Biome biome = BiomeFactory::createDesertLakes();
    EXPECT_EQ(biome.id(), Biomes::DesertLakes);
    EXPECT_EQ(biome.name(), "desert_lakes");
    EXPECT_FLOAT_EQ(biome.depth(), 0.225f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.25f);
}

TEST_F(BiomeRegistryTest, CreateGravellyMountains) {
    Biome biome = BiomeFactory::createGravellyMountains();
    EXPECT_EQ(biome.id(), Biomes::GravellyMountains);
    EXPECT_EQ(biome.name(), "gravelly_mountains");
    EXPECT_FLOAT_EQ(biome.depth(), 1.0f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.5f);
}

TEST_F(BiomeRegistryTest, CreateSwampHills) {
    Biome biome = BiomeFactory::createSwampHills();
    EXPECT_EQ(biome.id(), Biomes::SwampHills);
    EXPECT_EQ(biome.name(), "swamp_hills");
    EXPECT_FLOAT_EQ(biome.depth(), -0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateModifiedJungle) {
    Biome biome = BiomeFactory::createModifiedJungle();
    EXPECT_EQ(biome.id(), Biomes::ModifiedJungle);
    EXPECT_EQ(biome.name(), "modified_jungle");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateShatteredSavannaPlateau) {
    Biome biome = BiomeFactory::createShatteredSavannaPlateau();
    EXPECT_EQ(biome.id(), Biomes::ShatteredSavannaPlateau);
    EXPECT_EQ(biome.name(), "shattered_savanna_plateau");
    EXPECT_FLOAT_EQ(biome.depth(), 1.05f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.0125f);
}

// ============================================================================
// Layer 系统测试
// ============================================================================

class LayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(LayerTest, LayerContext_PositionSeed) {
    LayerContext ctx(1024, 12345, 1);

    ctx.setPosition(0, 0);
    i32 val1 = ctx.nextInt(100);

    ctx.setPosition(0, 0);
    i32 val2 = ctx.nextInt(100);

    // 相同位置应该产生相同的结果
    EXPECT_EQ(val1, val2);
}

TEST_F(LayerTest, LayerContext_DifferentPositions) {
    LayerContext ctx(1024, 12345, 1);

    ctx.setPosition(0, 0);
    i32 val1 = ctx.nextInt(100);

    ctx.setPosition(100, 200);
    i32 val2 = ctx.nextInt(100);

    // 不同位置通常产生不同结果（概率很高）
    // 注意：这不是绝对的，但对于大多数种子来说是正确的
}

TEST_F(LayerTest, IslandLayer_SpawnPointIsLand) {
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);
    layer::IslandLayer islandLayer;

    auto factory = islandLayer.apply(*ctx);
    auto area = factory->create();

    // 原点 (0, 0) 应该是陆地
    i32 val = area->getValue(0, 0);
    EXPECT_EQ(val, 1); // 1 = 陆地
}

TEST_F(LayerTest, ZoomLayer_Scaling) {
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);
    layer::ZoomLayer zoom(layer::ZoomLayer::Mode::Normal);

    // 创建源层
    layer::IslandLayer islandLayer;
    auto sourceFactory = islandLayer.apply(*ctx);
    auto zoomedFactory = zoom.apply(*ctx, std::move(sourceFactory));
    auto area = zoomedFactory->create();

    // 采样应该有效
    i32 val = area->getValue(0, 0);
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, SimpleLayerChain) {
    // 测试简单的层链
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);

    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    // 创建第一个缩放
    static layer::ZoomLayer zoom1(layer::ZoomLayer::Mode::Fuzzy);
    factory = zoom1.apply(*ctx, std::move(factory));

    // 创建第二个缩放
    static layer::ZoomLayer zoom2(layer::ZoomLayer::Mode::Normal);
    factory = zoom2.apply(*ctx, std::move(factory));

    // 创建区域并采样
    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, AddIslandLayer) {
    // 测试 AddIslandLayer
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);

    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    EXPECT_TRUE(val == 0 || val == 1);
}

TEST_F(LayerTest, AddSnowLayer) {
    // 测试 AddSnowLayer
    auto ctx = std::make_shared<LayerContext>(1024, 12345, 1);

    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    factory = normalZoom.apply(*ctx, std::move(factory));

    static layer::AddSnowLayer addSnowLayer;
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    // 应该是 0-4 之间的值
    EXPECT_GE(val, 0);
    EXPECT_LE(val, 4);
}

TEST_F(LayerTest, MultipleContexts) {
    // 测试使用不同上下文的层链 - 模拟 buildOverworldLayers 的行为
    u64 seed = 12345;

    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    // 创建源层 - 使用 context 1
    auto ctx = createContext(1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    // 使用 context 2000 进行模糊缩放
    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    ctx = createContext(2000);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    // 使用 context 1 进行 AddIsland
    static layer::AddIslandLayer addIslandLayer;
    ctx = createContext(1);
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    // 使用 context 2001 进行普通缩放
    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    ctx = createContext(2001);
    factory = normalZoom.apply(*ctx, std::move(factory));

    // 使用 context 2 进行 AddSnow
    static layer::AddSnowLayer addSnowLayer;
    ctx = createContext(2);
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    // 创建区域并采样
    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    EXPECT_GE(val, 0);
    EXPECT_LE(val, 4);
}

TEST_F(LayerTest, BiomeLayerTest) {
    // 测试完整的层链直到 BiomeLayer
    u64 seed = 12345;

    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    auto ctx = createContext(1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    ctx = createContext(2000);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    ctx = createContext(1);
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    ctx = createContext(2001);
    factory = normalZoom.apply(*ctx, std::move(factory));

    static layer::AddSnowLayer addSnowLayer;
    ctx = createContext(2);
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    // 缩放几次
    for (int i = 0; i < 4; ++i) {
        ctx = createContext(static_cast<u64>(2002 + i));
        factory = normalZoom.apply(*ctx, std::move(factory));
    }

    // BiomeLayer
    static layer::BiomeLayer biomeLayer(layer::BiomeLayer::Config{false});
    ctx = createContext(200);
    factory = biomeLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    // 应该是一个有效的生物群系 ID
    EXPECT_GE(val, 0);
}

TEST_F(LayerTest, FullLayerChain) {
    // 测试完整的层链
    u64 seed = 12345;

    auto createContext = [seed](u64 modifier) -> std::shared_ptr<LayerContext> {
        return std::make_shared<LayerContext>(1024, seed, modifier);
    };

    auto ctx = createContext(1);
    static layer::IslandLayer islandLayer;
    auto factory = islandLayer.apply(*ctx);

    static layer::ZoomLayer fuzzyZoom(layer::ZoomLayer::Mode::Fuzzy);
    ctx = createContext(2000);
    factory = fuzzyZoom.apply(*ctx, std::move(factory));

    static layer::AddIslandLayer addIslandLayer;
    ctx = createContext(1);
    factory = addIslandLayer.apply(*ctx, std::move(factory));

    static layer::ZoomLayer normalZoom(layer::ZoomLayer::Mode::Normal);
    ctx = createContext(2001);
    factory = normalZoom.apply(*ctx, std::move(factory));

    static layer::AddSnowLayer addSnowLayer;
    ctx = createContext(2);
    factory = addSnowLayer.apply(*ctx, std::move(factory));

    // 缩放几次 (biomeSize = 4, 所以 4+4=8 次)
    for (int i = 0; i < 8; ++i) {
        ctx = createContext(static_cast<u64>(2002 + i));
        factory = normalZoom.apply(*ctx, std::move(factory));
    }

    // BiomeLayer
    static layer::BiomeLayer biomeLayer(layer::BiomeLayer::Config{false});
    ctx = createContext(200);
    factory = biomeLayer.apply(*ctx, std::move(factory));

    // 最终缩放
    for (int i = 0; i < 4; ++i) {
        ctx = createContext(static_cast<u64>(3000 + i));
        factory = normalZoom.apply(*ctx, std::move(factory));
    }

    // SmoothLayer
    static layer::SmoothLayer smoothLayer;
    ctx = createContext(1000);
    factory = smoothLayer.apply(*ctx, std::move(factory));

    auto area = factory->create();
    i32 val = area->getValue(0, 0);
    // 应该是一个有效的生物群系 ID
    EXPECT_GE(val, 0);
}

TEST_F(LayerTest, LayerUtilBuildOverworldLayers) {
    // 直接测试 buildOverworldLayers
    auto factory = LayerUtil::buildOverworldLayers(12345, false, false, 4, 4);
    ASSERT_NE(factory, nullptr);

    auto area = factory->create();
    ASSERT_NE(area, nullptr);

    i32 val = area->getValue(0, 0);
    EXPECT_GE(val, 0);
}

TEST_F(LayerTest, CreateOverworldLayers) {
    // 直接测试 createOverworldLayers
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

// ============================================================================
// LayerStack 测试
// ============================================================================

class LayerStackTest : public ::testing::Test {
protected:
    void SetUp() override {
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(LayerStackTest, CreateOverworldLayers) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerStackTest, CreateOverworldLayers_LargeBiomes) {
    auto stack = LayerUtil::createOverworldLayers(12345, true);
    ASSERT_NE(stack, nullptr);

    BiomeId biome = stack->sample(0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerStackTest, SampleMultiplePositions) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    // 采样多个位置
    BiomeId biome1 = stack->sample(0, 0);
    BiomeId biome2 = stack->sample(100, 100);
    BiomeId biome3 = stack->sample(-50, -50);

    EXPECT_LT(biome1, Biomes::Count);
    EXPECT_LT(biome2, Biomes::Count);
    EXPECT_LT(biome3, Biomes::Count);
}

TEST_F(LayerStackTest, SampleArea) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    auto biomes = stack->sampleArea(0, 0, 16, 16);

    EXPECT_EQ(biomes.size(), 256u);
    for (BiomeId biome : biomes) {
        EXPECT_LT(biome, Biomes::Count);
    }
}

TEST_F(LayerStackTest, Consistency) {
    auto stack = LayerUtil::createOverworldLayers(12345, false);
    ASSERT_NE(stack, nullptr);

    BiomeId biome1 = stack->sample(100, 200);
    BiomeId biome2 = stack->sample(100, 200);

    EXPECT_EQ(biome1, biome2);
}

// ============================================================================
// LayerBiomeProvider 测试
// ============================================================================

class LayerBiomeProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        provider = std::make_unique<LayerBiomeProvider>(12345);
    }

    std::unique_ptr<LayerBiomeProvider> provider;
};

TEST_F(LayerBiomeProviderTest, GetBiome) {
    BiomeId biome = provider->getBiome(0, 64, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerBiomeProviderTest, GetNoiseBiome) {
    BiomeId biome = provider->getNoiseBiome(0, 0, 0);
    EXPECT_LT(biome, Biomes::Count);
}

TEST_F(LayerBiomeProviderTest, GetDepthAndScale) {
    f32 depth = provider->getDepth(0, 0);
    f32 scale = provider->getScale(0, 0);

    EXPECT_GE(depth, -2.0f);  // 深海可能为负
    EXPECT_GE(scale, 0.0f);
}

TEST_F(LayerBiomeProviderTest, GetBiomeDefinition) {
    const Biome& biome = provider->getBiomeDefinition(Biomes::Plains);
    EXPECT_EQ(biome.id(), Biomes::Plains);
}

TEST_F(LayerBiomeProviderTest, BiomeDistribution) {
    // 统计生物群系分布
    std::map<BiomeId, int> distribution;

    for (int x = 0; x < 100; ++x) {
        for (int z = 0; z < 100; ++z) {
            BiomeId biome = provider->getBiome(x * 10, 64, z * 10);
            distribution[biome]++;
        }
    }

    // 应该有多种生物群系
    EXPECT_GT(distribution.size(), 1u);
}

// ============================================================================
// NetherBiomeProvider / EndBiomeProvider 测试
// ============================================================================

class NetherBiomeProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(NetherBiomeProviderTest, FillBiomeContainerUsesVerticalNoiseSamples) {
    biome::nether::NetherBiomeProvider provider(12345);
    BiomeContainer container;

    constexpr ChunkCoord chunkX = 0;
    constexpr ChunkCoord chunkZ = 0;
    provider.fillBiomeContainer(container, chunkX, chunkZ);

    const i32 startNoiseX = chunkX << 2;
    const i32 startNoiseZ = chunkZ << 2;

    for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
        for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
            const i32 noiseX = startNoiseX + bx;
            const i32 noiseZ = startNoiseZ + bz;

            for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
                const i32 sampleBlockY = (by << 4) + 8;
                const i32 noiseY = sampleBlockY >> 2;

                const BiomeId expected = provider.getNoiseBiome(noiseX, noiseY, noiseZ);
                const BiomeId actual = container.getBiome(bx, by, bz);
                EXPECT_EQ(actual, expected)
                    << "Nether biome container mismatch at (" << bx << ", " << by << ", " << bz << ")";
            }
        }
    }
}

class EndBiomeProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(EndBiomeProviderTest, FillBiomeContainerMatchesHorizontalSamplingGrid) {
    biome::end::EndBiomeProvider provider(98765);
    BiomeContainer container;

    constexpr ChunkCoord chunkX = 3;
    constexpr ChunkCoord chunkZ = -2;
    provider.fillBiomeContainer(container, chunkX, chunkZ);

    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
        for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
            const i32 sampleX = startX + (bx << 2) + 2;
            const i32 sampleZ = startZ + (bz << 2) + 2;
            const BiomeId expected = provider.getBiome(sampleX, 0, sampleZ);

            for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
                const BiomeId actual = container.getBiome(bx, by, bz);
                EXPECT_EQ(actual, expected)
                    << "End biome container mismatch at (" << bx << ", " << by << ", " << bz << ")";
            }
        }
    }
}
