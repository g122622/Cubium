#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/ore/OreFeature.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// RuleTest 测试
// ============================================================================

class RuleTestTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(RuleTestTest, StoneRuleTest)
{
    auto rule = createOreTarget(OreTargetType::NaturalStone);
    math::Random random(12345);

    // 石头应该匹配
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stone, nullptr);
    EXPECT_TRUE(rule->test(*stone, random));

    // 空气不应该匹配
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    ASSERT_NE(air, nullptr);
    EXPECT_FALSE(rule->test(*air, random));
}

TEST_F(RuleTestTest, BlockIdRuleTest)
{
    auto rule = createOreTarget(OreTargetType::Netherrack);
    math::Random random(12345);

    // 下界岩应该匹配
    const BlockState* netherrack = &VanillaBlocks::NETHERRACK->defaultState();
    if (netherrack) {
        EXPECT_TRUE(rule->test(*netherrack, random));
    }

    // 石头不应该匹配
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(rule->test(*stone, random));
}

// ============================================================================
// OreFeatureConfig 测试
// ============================================================================

class OreFeatureConfigTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(OreFeatureConfigTest, CreateConfig)
{
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), &VanillaBlocks::COAL_ORE->defaultState(), 17);

    EXPECT_TRUE(config->state->is(VanillaBlocks::COAL_ORE));
    EXPECT_EQ(config->size, 17);
    EXPECT_NE(config->target, nullptr);
}

TEST_F(OreFeatureConfigTest, NaturalStoneTarget)
{
    auto target = OreFeatureConfig::naturalStone();
    EXPECT_NE(target, nullptr);
}

// ============================================================================
// Placement 测试
// ============================================================================

class PlacementTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(PlacementTest, CountPlacement)
{
    CountPlacement placement;
    CountPlacementConfig config(5); // 5次尝试
    ChunkPrimer chunk(0, 0);
    math::Random random(12345);

    // 创建一个简单的 WorldGenRegion 需要太多依赖，暂时跳过位置测试
    // 这里只测试配置
    EXPECT_EQ(config.count, 5);
}

TEST_F(PlacementTest, HeightRangePlacementConfig)
{
    HeightRangePlacementConfig config(world::MIN_BUILD_HEIGHT, 128, world::MAX_BUILD_HEIGHT);

    math::Random random(12345);
    for (int i = 0; i < 100; ++i) {
        i32 y = config.getRandomY(random);
        EXPECT_GE(y, world::MIN_BUILD_HEIGHT);
        EXPECT_LT(y, world::MAX_BUILD_HEIGHT);
    }
}

TEST_F(PlacementTest, HeightRangePlacementConfigUniform)
{
    auto config = HeightRangePlacementConfig::uniform(10, 50);

    math::Random random(12345);
    for (int i = 0; i < 100; ++i) {
        i32 y = config.getRandomY(random);
        EXPECT_GE(y, 10);
        EXPECT_LT(y, 50);
    }
}

TEST_F(PlacementTest, BiomePlacementConfig)
{
    BiomePlacementConfig config({1, 5, 10});

    EXPECT_TRUE(config.isAllowed(1));
    EXPECT_TRUE(config.isAllowed(5));
    EXPECT_TRUE(config.isAllowed(10));
    EXPECT_FALSE(config.isAllowed(2));
    EXPECT_FALSE(config.isAllowed(100));
}

TEST_F(PlacementTest, ChancePlacementConfig)
{
    ChancePlacementConfig config(0.5f); // 50% 概率

    math::Random random(12345);
    int success = 0;
    for (int i = 0; i < 1000; ++i) {
        if (random.nextFloat() < config.chance) {
            success++;
        }
    }

    // 应该接近 500 次（允许一定误差）
    EXPECT_GT(success, 400);
    EXPECT_LT(success, 600);
}

// ============================================================================
// SimpleBlockStateProvider 测试
// ============================================================================

class BlockStateProviderTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(BlockStateProviderTest, SimpleProvider)
{
    SimpleBlockStateProvider provider(&VanillaBlocks::STONE->defaultState());
    math::Random random(12345);

    const BlockState* state = provider.getState(random, 0, 0, 0);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::STONE));
}

TEST_F(BlockStateProviderTest, DifferentBlocks)
{
    SimpleBlockStateProvider grassProvider(&VanillaBlocks::GRASS_BLOCK->defaultState());
    SimpleBlockStateProvider dirtProvider(&VanillaBlocks::DIRT->defaultState());
    math::Random random(12345);

    const BlockState* grass = grassProvider.getState(random, 0, 0, 0);
    const BlockState* dirt = dirtProvider.getState(random, 0, 0, 0);

    ASSERT_NE(grass, nullptr);
    ASSERT_NE(dirt, nullptr);
    EXPECT_TRUE(grass->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(dirt->is(VanillaBlocks::DIRT));
}

// ============================================================================
// OreFeature 测试
// ============================================================================

class OreFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        random = std::make_unique<math::Random>(12345);
    }

    void fillWithStone()
    {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        ASSERT_NE(stone, nullptr);

        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    chunk->setBlockState(x, y, z, stone);
                }
            }
        }
    }

    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<math::Random> random;
};

TEST_F(OreFeatureTest, CreateOreFeature)
{
    EXPECT_STREQ(OreFeature::name(), "ore");
}

TEST_F(OreFeatureTest, PlaceSmallOre)
{
    fillWithStone();

    auto config = std::make_unique<OreFeatureConfig>(createOreTarget(OreTargetType::NaturalStone),
        &VanillaBlocks::COAL_ORE->defaultState(),
        8); // 小矿脉

    // 注意：WorldGenRegion 需要完整实现才能测试 place 方法
    // 这里只验证配置正确创建
    EXPECT_TRUE(config->state->is(VanillaBlocks::COAL_ORE));
    EXPECT_EQ(config->size, 8);
}

// ============================================================================
// OreFeatures 注册表测试
// ============================================================================

class OreFeaturesTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(OreFeaturesTest, InitializeFeatures)
{
    // 初始化不应该崩溃
    EXPECT_NO_THROW(OreFeatures::initialize());

    const auto& features = OreFeatures::getAllFeatures();
    EXPECT_GT(features.size(), 0u);
}

TEST_F(OreFeaturesTest, CreateCoalOre)
{
    auto feature = OreFeatures::createCoalOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::COAL_ORE));
    EXPECT_EQ(config.size, 17);
}

TEST_F(OreFeaturesTest, CreateIronOre)
{
    auto feature = OreFeatures::createIronOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::IRON_ORE));
    EXPECT_EQ(config.size, 9);
}

TEST_F(OreFeaturesTest, CreateGoldOre)
{
    auto feature = OreFeatures::createGoldOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::GOLD_ORE));
    EXPECT_EQ(config.size, 9);
}

TEST_F(OreFeaturesTest, CreateDiamondOre)
{
    auto feature = OreFeatures::createDiamondOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::DIAMOND_ORE));
    EXPECT_EQ(config.size, 8);
}

TEST_F(OreFeaturesTest, CreateRedstoneOre)
{
    auto feature = OreFeatures::createRedstoneOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::REDSTONE_ORE));
    EXPECT_EQ(config.size, 8);
}

TEST_F(OreFeaturesTest, CreateLapisOre)
{
    auto feature = OreFeatures::createLapisOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::LAPIS_ORE));
    EXPECT_EQ(config.size, 7);
}

TEST_F(OreFeaturesTest, CreateEmeraldOre)
{
    auto feature = OreFeatures::createEmeraldOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::EMERALD_ORE));
    EXPECT_EQ(config.size, 1); // 绿宝石是单个方块
}

TEST_F(OreFeaturesTest, CreateCopperOre)
{
    auto feature = OreFeatures::createCopperOre();
    ASSERT_NE(feature, nullptr);

    const auto& config = feature->getConfig();
    EXPECT_TRUE(config.state->is(VanillaBlocks::COPPER_ORE));
    EXPECT_EQ(config.size, 10);
}

// ============================================================================
// 矿石分布参数测试
// ============================================================================

TEST_F(OreFeaturesTest, OreDistributionParameters)
{
    // 验证矿石分布参数符合 MC 1.16.5 规范

    // 煤矿：Y 0-127，每区块20个，矿脉大小17
    auto coal = OreFeatures::createCoalOre();
    EXPECT_EQ(coal->getConfig().size, 17);

    // 铁矿：Y 0-63，每区块20个，矿脉大小9
    auto iron = OreFeatures::createIronOre();
    EXPECT_EQ(iron->getConfig().size, 9);

    // 金矿：Y 0-31，每区块2个，矿脉大小9
    auto gold = OreFeatures::createGoldOre();
    EXPECT_EQ(gold->getConfig().size, 9);

    // 红石：Y 0-15，每区块8个，矿脉大小8
    auto redstone = OreFeatures::createRedstoneOre();
    EXPECT_EQ(redstone->getConfig().size, 8);

    // 钻石：Y 0-15，每区块1个，矿脉大小8
    auto diamond = OreFeatures::createDiamondOre();
    EXPECT_EQ(diamond->getConfig().size, 8);

    // 青金石：深度平均分布，每区块1个，矿脉大小7
    auto lapis = OreFeatures::createLapisOre();
    EXPECT_EQ(lapis->getConfig().size, 7);

    // 绿宝石：Y 4-31，每区块1个，矿脉大小1（山地生物群系特有）
    auto emerald = OreFeatures::createEmeraldOre();
    EXPECT_EQ(emerald->getConfig().size, 1);
}
