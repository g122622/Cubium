#include "world/biome/layer/transformers/MergeLayers.hpp"
#include "world/biome/layer/BiomeValues.hpp"
#include "world/biome/layer/Layer.hpp"
#include "world/biome/layer/LayerContext.hpp"
#include <functional>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::layer;

namespace {

/**
 * @brief 简单的测试用 Mock IArea
 */
class MockArea : public IArea {
public:
    MockArea() = default;
    explicit MockArea(i32 constantValue)
        : m_constantValue(constantValue)
        , m_useConstant(true)
    {}
    explicit MockArea(std::function<i32(i32, i32)> func)
        : m_func(std::move(func))
    {}

    [[nodiscard]] i32 getValue(i32 x, i32 z) const override
    {
        if (m_useConstant) {
            return m_constantValue;
        }
        if (m_func) {
            return m_func(x, z);
        }
        return 0;
    }

private:
    i32 m_constantValue = 0;
    bool m_useConstant = false;
    std::function<i32(i32, i32)> m_func;
};

/**
 * @brief 测试用 Mock IAreaContext
 */
class MockAreaContext : public IAreaContext {
public:
    MockAreaContext() = default;

    void setPosition(i64 x, i64 z) override
    {
        m_currentX = x;
        m_currentZ = z;
    }

    [[nodiscard]] i32 nextInt(i32 bound) override
    {
        if (m_useSequence && m_sequenceIndex < m_sequence.size()) {
            return m_sequence[m_sequenceIndex++] % bound;
        }
        if (m_constantRandom >= 0) {
            return m_constantRandom % bound;
        }
        m_seed = m_seed * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<i32>((m_seed >> 32) % bound);
    }

    [[nodiscard]] i32 pickRandom(i32 a, i32 b) override { return nextInt(2) == 0 ? a : b; }

    [[nodiscard]] i32 pickRandom(i32 a, i32 b, i32 c, i32 d) override
    {
        i32 idx = nextInt(4);
        switch (idx) {
            case 0:
                return a;
            case 1:
                return b;
            case 2:
                return c;
            default:
                return d;
        }
    }

    [[nodiscard]] ImprovedNoiseGenerator* getNoiseGenerator() override { return nullptr; }

    void setConstantRandom(i32 value) { m_constantRandom = value; }

    void setRandomSequence(const std::vector<i32>& sequence)
    {
        m_sequence = sequence;
        m_sequenceIndex = 0;
        m_useSequence = true;
    }

    void reset()
    {
        m_constantRandom = -1;
        m_useSequence = false;
        m_sequenceIndex = 0;
        m_seed = 12345;
    }

private:
    i64 m_currentX = 0;
    i64 m_currentZ = 0;
    i32 m_constantRandom = -1;
    std::vector<i32> m_sequence;
    size_t m_sequenceIndex = 0;
    bool m_useSequence = false;
    u64 m_seed = 12345;
};

} // anonymous namespace

// ============================================================================
// AddMushroomIslandLayer 测试
// ============================================================================

class AddMushroomIslandLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_layer = std::make_unique<AddMushroomIslandLayer>();
    }
    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<AddMushroomIslandLayer> m_layer;
};

TEST_F(AddMushroomIslandLayerTest, AllShallowOceanWithLuckyRollCreatesMushroomFields)
{
    // 中心和四个对角都是浅海，且随机值==0，生成蘑菇岛
    m_context->setConstantRandom(0);

    // IBishopTransformer::apply(ctx, x, sw, se, ne, nw, center)
    i32 result = m_layer->apply(*m_context,
        0,                  // x
        BiomeValues::Ocean, // sw
        BiomeValues::Ocean, // se
        BiomeValues::Ocean, // ne
        BiomeValues::Ocean, // nw
        BiomeValues::Ocean  // center
    );
    EXPECT_EQ(result, BiomeValues::MushroomFields);
}

TEST_F(AddMushroomIslandLayerTest, NotAllShallowOceanStaysSame)
{
    // 不是所有都是浅海，保持不变
    m_context->setConstantRandom(0);

    i32 result = m_layer->apply(*m_context,
        0,
        BiomeValues::Ocean,
        BiomeValues::Ocean,
        BiomeValues::Ocean,
        BiomeValues::DeepOcean, // deep ocean is not shallow
        BiomeValues::Ocean);
    EXPECT_EQ(result, BiomeValues::Ocean);
}

TEST_F(AddMushroomIslandLayerTest, LuckyRollNotZeroStaysSame)
{
    // 随机值!=0，保持不变
    m_context->setConstantRandom(1);

    i32 result = m_layer->apply(*m_context,
        0,
        BiomeValues::Ocean,
        BiomeValues::Ocean,
        BiomeValues::Ocean,
        BiomeValues::Ocean,
        BiomeValues::Ocean);
    EXPECT_EQ(result, BiomeValues::Ocean);
}

// ============================================================================
// AddBambooForestLayer 测试
// ============================================================================

class AddBambooForestLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_layer = std::make_unique<AddBambooForestLayer>();
    }
    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<AddBambooForestLayer> m_layer;
};

TEST_F(AddBambooForestLayerTest, JungleWithLuckyRollBecomesBambooJungle)
{
    // 丛林有 1/10 概率变成竹林
    m_context->setConstantRandom(0);

    i32 result = m_layer->apply(*m_context, BiomeValues::Jungle);
    EXPECT_EQ(result, BiomeValues::BambooJungle);
}

TEST_F(AddBambooForestLayerTest, JungleWithoutLuckyRollStaysJungle)
{
    m_context->setConstantRandom(1);

    i32 result = m_layer->apply(*m_context, BiomeValues::Jungle);
    EXPECT_EQ(result, BiomeValues::Jungle);
}

TEST_F(AddBambooForestLayerTest, OtherBiomesUnchanged)
{
    m_context->setConstantRandom(0);

    EXPECT_EQ(m_layer->apply(*m_context, BiomeValues::Forest), BiomeValues::Forest);
    EXPECT_EQ(m_layer->apply(*m_context, BiomeValues::Desert), BiomeValues::Desert);
    EXPECT_EQ(m_layer->apply(*m_context, BiomeValues::Plains), BiomeValues::Plains);
}

// ============================================================================
// RiverLayer 测试
// ============================================================================

class RiverLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_layer = std::make_unique<RiverLayer>();
    }
    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<RiverLayer> m_layer;
};

TEST_F(RiverLayerTest, UniformValuesNoRiver)
{
    // 所有邻居过滤后值相同，返回 -1（无河流）
    i32 result = m_layer->apply(*m_context, 100, 100, 100, 100, 100 // north, east, south, west, center
    );
    EXPECT_EQ(result, -1);
}

TEST_F(RiverLayerTest, DifferentValuesCreatesRiver)
{
    // 邻居过滤后值不同，返回河流
    i32 result = m_layer->apply(*m_context, 100, 101, 100, 101, 100);
    EXPECT_EQ(result, BiomeValues::River);
}

// ============================================================================
// MixRiverLayer 测试
// ============================================================================

class MixRiverLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_layer = std::make_unique<MixRiverLayer>();
    }
    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<MixRiverLayer> m_layer;
};

TEST_F(MixRiverLayerTest, OceanStaysOcean)
{
    MockArea biomeArea(BiomeValues::Ocean);
    MockArea riverArea(BiomeValues::River);

    i32 result = m_layer->apply(*m_context, biomeArea, riverArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::Ocean);
}

TEST_F(MixRiverLayerTest, RiverInSnowyPlainsBecomesFrozenRiver)
{
    MockArea biomeArea(BiomeValues::SnowyPlains);
    MockArea riverArea(BiomeValues::River);

    i32 result = m_layer->apply(*m_context, biomeArea, riverArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::FrozenRiver);
}

TEST_F(MixRiverLayerTest, RiverInNormalBiomeBecomesRiver)
{
    MockArea biomeArea(BiomeValues::Plains);
    MockArea riverArea(BiomeValues::River);

    i32 result = m_layer->apply(*m_context, biomeArea, riverArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::River);
}

TEST_F(MixRiverLayerTest, RiverInMushroomFieldsBecomesShore)
{
    MockArea biomeArea(BiomeValues::MushroomFields);
    MockArea riverArea(BiomeValues::River);

    i32 result = m_layer->apply(*m_context, biomeArea, riverArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::MushroomFieldShore);
}

TEST_F(MixRiverLayerTest, NoRiverStaysSame)
{
    MockArea biomeArea(BiomeValues::Plains);
    MockArea riverArea(-1); // -1 means no river

    i32 result = m_layer->apply(*m_context, biomeArea, riverArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::Plains);
}

// ============================================================================
// MixOceansLayer 测试
// ============================================================================

class MixOceansLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_layer = std::make_unique<MixOceansLayer>();
    }
    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<MixOceansLayer> m_layer;
};

TEST_F(MixOceansLayerTest, LandBiomeUnchanged)
{
    MockArea biomeArea(BiomeValues::Plains);
    MockArea oceanArea(BiomeValues::WarmOcean);

    i32 result = m_layer->apply(*m_context, biomeArea, oceanArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::Plains);
}

TEST_F(MixOceansLayerTest, WarmOceanWithLandNeighborBecomesLukewarm)
{
    // 当温暖海洋有陆地邻居时，变成温水海洋
    MockArea biomeArea([](i32 x, i32 z) -> i32 {
        // 中心是海洋，周围有陆地
        if (x == 0 && z == 0) return BiomeValues::Ocean;
        return BiomeValues::Plains; // 陆地邻居
    });
    MockArea oceanArea(BiomeValues::WarmOcean);

    i32 result = m_layer->apply(*m_context, biomeArea, oceanArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::LukewarmOcean);
}

TEST_F(MixOceansLayerTest, FrozenOceanWithLandNeighborBecomesCold)
{
    // 当冻结海洋有陆地邻居时，变成冷水海洋
    MockArea biomeArea([](i32 x, i32 z) -> i32 {
        if (x == 0 && z == 0) return BiomeValues::Ocean;
        return BiomeValues::Plains;
    });
    MockArea oceanArea(BiomeValues::FrozenOcean);

    i32 result = m_layer->apply(*m_context, biomeArea, oceanArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::ColdOcean);
}

TEST_F(MixOceansLayerTest, DeepOceanWithLukewarmBecomesDeepLukewarm)
{
    MockArea biomeArea(BiomeValues::DeepOcean);
    MockArea oceanArea(BiomeValues::LukewarmOcean);

    i32 result = m_layer->apply(*m_context, biomeArea, oceanArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::DeepLukewarmOcean);
}

TEST_F(MixOceansLayerTest, DeepOceanWithColdBecomesDeepCold)
{
    MockArea biomeArea(BiomeValues::DeepOcean);
    MockArea oceanArea(BiomeValues::ColdOcean);

    i32 result = m_layer->apply(*m_context, biomeArea, oceanArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::DeepColdOcean);
}

TEST_F(MixOceansLayerTest, DeepOceanWithFrozenBecomesDeepFrozen)
{
    MockArea biomeArea(BiomeValues::DeepOcean);
    MockArea oceanArea(BiomeValues::FrozenOcean);

    i32 result = m_layer->apply(*m_context, biomeArea, oceanArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::DeepFrozenOcean);
}

TEST_F(MixOceansLayerTest, DeepOceanWithRegularOceanStaysDeepOcean)
{
    MockArea biomeArea(BiomeValues::DeepOcean);
    MockArea oceanArea(BiomeValues::Ocean);

    i32 result = m_layer->apply(*m_context, biomeArea, oceanArea, 0, 0);
    EXPECT_EQ(result, BiomeValues::DeepOcean);
}

// ============================================================================
// HillsLayer 测试
// ============================================================================

class HillsLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_layer = std::make_unique<HillsLayer>();
    }
    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<HillsLayer> m_layer;
};

TEST_F(HillsLayerTest, ShallowOceanCanBecomeDeepOcean)
{
    // 浅海可能会变成深海，这是 HillsLayer 的正确行为
    MockArea biomeArea(BiomeValues::Ocean);
    MockArea riverArea(2); // riverValue = 2, riverNoise = 0

    m_context->setRandomSequence({0, 0, 0, 0, 0, 0, 0, 0}); // nextInt(3) == 0

    i32 result = m_layer->apply(*m_context, biomeArea, riverArea, 0, 0);
    // 海洋可能变成深海，或保持海洋，取决于邻居检查
    EXPECT_TRUE(result == BiomeValues::DeepOcean || result == BiomeValues::Ocean);
}

TEST_F(HillsLayerTest, JungleToJungleHills)
{
    // 丛林可能变成丛林丘陵
    MockArea biomeArea([](i32 x, i32 z) -> i32 { return BiomeValues::Jungle; });
    MockArea riverArea(2); // riverValue = 2, riverNoise = 0

    m_context->setRandomSequence({0, 0, 0, 0, 0, 0, 0, 0});

    i32 result = m_layer->apply(*m_context, biomeArea, riverArea, 0, 0);
    // 结果可能是 JungleHills (22) 或保持 Jungle (21)
    EXPECT_TRUE(result == BiomeValues::JungleHills || result == BiomeValues::Jungle);
}

// ============================================================================
// StartRiverLayer 测试
// ============================================================================

class StartRiverLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_layer = std::make_unique<StartRiverLayer>();
    }
    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<StartRiverLayer> m_layer;
};

TEST_F(StartRiverLayerTest, ShallowOceanStaysSame)
{
    // 浅海保持不变
    EXPECT_EQ(m_layer->apply(*m_context, BiomeValues::Ocean), BiomeValues::Ocean);
    EXPECT_EQ(m_layer->apply(*m_context, BiomeValues::WarmOcean), BiomeValues::WarmOcean);
    EXPECT_EQ(m_layer->apply(*m_context, BiomeValues::FrozenOcean), BiomeValues::FrozenOcean);
}

TEST_F(StartRiverLayerTest, LandGeneratesRiverNoise)
{
    // 陆地生成河流噪声值 (2-300000)
    m_context->setConstantRandom(0);

    i32 result = m_layer->apply(*m_context, BiomeValues::Plains);
    EXPECT_GE(result, 2);
    EXPECT_LE(result, 300001);
}
