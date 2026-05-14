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

#include "world/biome/layer/BiomeValues.hpp"
#include "world/biome/layer/Layer.hpp"
#include "world/biome/layer/LayerContext.hpp"
#include "world/biome/layer/transformers/BiomeLayers.hpp"
#include "world/biome/layer/transformers/MergeLayers.hpp"
#include <functional>
#include <unordered_map>
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
// BiomeLayer 测试
// ============================================================================

class BiomeLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        BiomeLayer::Config config;
        config.legacyDesertInit = false;
        m_biomeLayer = std::make_unique<BiomeLayer>(config);
    }

    void TearDown() override
    {
        m_context.reset();
        m_biomeLayer.reset();
    }

    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<BiomeLayer> m_biomeLayer;
};

TEST_F(BiomeLayerTest, OceanValuePreserved)
{
    // 海洋值应该保持不变
    EXPECT_EQ(m_biomeLayer->apply(*m_context, BiomeValues::Ocean), BiomeValues::Ocean);
    EXPECT_EQ(m_biomeLayer->apply(*m_context, BiomeValues::DeepOcean), BiomeValues::DeepOcean);
    EXPECT_EQ(m_biomeLayer->apply(*m_context, BiomeValues::WarmOcean), BiomeValues::WarmOcean);
    EXPECT_EQ(m_biomeLayer->apply(*m_context, BiomeValues::FrozenOcean), BiomeValues::FrozenOcean);
}

TEST_F(BiomeLayerTest, MushroomFieldsPreserved)
{
    // 蘑菇岛应该保持不变
    EXPECT_EQ(m_biomeLayer->apply(*m_context, BiomeValues::MushroomFields), BiomeValues::MushroomFields);
}

TEST_F(BiomeLayerTest, WarmClimateProducesCorrectBiomes)
{
    // Warm 气候 (1) 应该产生: Desert, Savanna, Plains
    m_context->setRandomSequence({0, 1, 2, 3, 4, 5});

    i32 result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Warm);
    EXPECT_EQ(result, BiomeValues::Desert); // index 0

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Warm);
    EXPECT_EQ(result, BiomeValues::Desert); // index 1

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Warm);
    EXPECT_EQ(result, BiomeValues::Desert); // index 2

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Warm);
    EXPECT_EQ(result, BiomeValues::Savanna); // index 3

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Warm);
    EXPECT_EQ(result, BiomeValues::Savanna); // index 4

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Warm);
    EXPECT_EQ(result, BiomeValues::Plains); // index 5
}

TEST_F(BiomeLayerTest, WarmClimateWithSpecialBitsProducesBadlands)
{
    // Warm 气候带特殊位应该产生恶地变体
    // 特殊位在 bits 8-11 (mask 0xF00)

    // 设置 nextInt(3) 返回 0，应该返回 WoodedBadlandsPlateau (38)
    m_context->setRandomSequence({0});
    i32 valueWithSpecial = BiomeValues::Climate::Warm | 0x100; // 设置特殊位
    i32 result = m_biomeLayer->apply(*m_context, valueWithSpecial);
    EXPECT_EQ(result, BiomeValues::WoodedBadlandsPlateau);

    // 设置 nextInt(3) 返回非0，应该返回 BadlandsPlateau (39)
    m_context->setRandomSequence({1});
    result = m_biomeLayer->apply(*m_context, valueWithSpecial);
    EXPECT_EQ(result, BiomeValues::BadlandsPlateau);
}

TEST_F(BiomeLayerTest, MediumClimateProducesCorrectBiomes)
{
    // Medium 气候 (2) 应该产生: Forest, DarkForest, Mountains, Plains, BirchForest, Swamp
    m_context->setRandomSequence({0, 10, 20, 30, 40, 50});

    i32 result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Medium);
    EXPECT_EQ(result, BiomeValues::Forest); // 0-9

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Medium);
    EXPECT_EQ(result, BiomeValues::DarkForest); // 10-19

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Medium);
    EXPECT_EQ(result, BiomeValues::Mountains); // 20-29

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Medium);
    EXPECT_EQ(result, BiomeValues::Plains); // 30-39

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Medium);
    EXPECT_EQ(result, BiomeValues::BirchForest); // 40-49

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Medium);
    EXPECT_EQ(result, BiomeValues::Swamp); // 50-59
}

TEST_F(BiomeLayerTest, MediumClimateWithSpecialBitsProducesJungle)
{
    // Medium 气候带特殊位应该产生丛林 (21)
    i32 valueWithSpecial = BiomeValues::Climate::Medium | 0x100;
    i32 result = m_biomeLayer->apply(*m_context, valueWithSpecial);
    EXPECT_EQ(result, BiomeValues::Jungle);
}

TEST_F(BiomeLayerTest, CoolClimateProducesCorrectBiomes)
{
    // Cool 气候 (3) 应该产生: Forest, GiantTreeTaigaHills, Mountains, Plains, BirchForest, Swamp
    m_context->setRandomSequence({0, 1, 2, 3, 4, 5});

    i32 result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Cool);
    EXPECT_EQ(result, BiomeValues::Forest); // index 0

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Cool);
    EXPECT_EQ(result, BiomeValues::GiantTreeTaigaHills); // index 1

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Cool);
    EXPECT_EQ(result, BiomeValues::Mountains); // index 2

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Cool);
    EXPECT_EQ(result, BiomeValues::Plains); // index 3

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Cool);
    EXPECT_EQ(result, BiomeValues::BirchForest); // index 4

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Cool);
    EXPECT_EQ(result, BiomeValues::Swamp); // index 5
}

TEST_F(BiomeLayerTest, CoolClimateWithSpecialBitsProducesGiantTreeTaiga)
{
    // Cool 气候带特殊位应该产生 GiantTreeTaiga (32)
    i32 valueWithSpecial = BiomeValues::Climate::Cool | 0x100;
    i32 result = m_biomeLayer->apply(*m_context, valueWithSpecial);
    EXPECT_EQ(result, BiomeValues::GiantTreeTaiga);
}

TEST_F(BiomeLayerTest, IcyClimateProducesCorrectBiomes)
{
    // Icy 气候 (4) 应该产生: SnowyPlains x3, WoodedMountains x1
    m_context->setRandomSequence({0, 1, 2, 3});

    i32 result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Icy);
    EXPECT_EQ(result, BiomeValues::SnowyPlains);

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Icy);
    EXPECT_EQ(result, BiomeValues::SnowyPlains);

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Icy);
    EXPECT_EQ(result, BiomeValues::SnowyPlains);

    result = m_biomeLayer->apply(*m_context, BiomeValues::Climate::Icy);
    EXPECT_EQ(result, BiomeValues::WoodedMountains);
}

TEST_F(BiomeLayerTest, UnknownValueReturnsMushroomFields)
{
    // 未知值应该返回蘑菇岛
    EXPECT_EQ(m_biomeLayer->apply(*m_context, 999), BiomeValues::MushroomFields);
    EXPECT_EQ(m_biomeLayer->apply(*m_context, -1), BiomeValues::MushroomFields);
}

// ============================================================================
// RareBiomeLayer 测试
// ============================================================================

class RareBiomeLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_rareBiomeLayer = std::make_unique<RareBiomeLayer>();
    }

    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<RareBiomeLayer> m_rareBiomeLayer;
};

TEST_F(RareBiomeLayerTest, PlainsToSunflowerPlains)
{
    // 平原有 1/57 概率变成向日葵平原
    m_context->setConstantRandom(0); // nextInt(57) == 0

    i32 result = m_rareBiomeLayer->apply(*m_context, BiomeValues::Plains);
    EXPECT_EQ(result, BiomeValues::SunflowerPlains);
}

TEST_F(RareBiomeLayerTest, PlainsStaysPlainsWhenNotLucky)
{
    // 平原不满足 1/57 概率时保持不变
    m_context->setConstantRandom(1); // nextInt(57) == 1

    i32 result = m_rareBiomeLayer->apply(*m_context, BiomeValues::Plains);
    EXPECT_EQ(result, BiomeValues::Plains);
}

TEST_F(RareBiomeLayerTest, OtherBiomesUnchanged)
{
    // 其他生物群系应该保持不变
    m_context->setConstantRandom(0);

    EXPECT_EQ(m_rareBiomeLayer->apply(*m_context, BiomeValues::Desert), BiomeValues::Desert);
    EXPECT_EQ(m_rareBiomeLayer->apply(*m_context, BiomeValues::Forest), BiomeValues::Forest);
    EXPECT_EQ(m_rareBiomeLayer->apply(*m_context, BiomeValues::Jungle), BiomeValues::Jungle);
}

// ============================================================================
// SmoothLayer 测试
// ============================================================================

class SmoothLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_smoothLayer = std::make_unique<SmoothLayer>();
    }

    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<SmoothLayer> m_smoothLayer;
};

TEST_F(SmoothLayerTest, AllEqualReturnsEither)
{
    // 所有邻居相等，随机选择
    m_context->setConstantRandom(0);

    i32 result = m_smoothLayer->apply(*m_context,
        BiomeValues::Desert, // north
        BiomeValues::Desert, // east
        BiomeValues::Desert, // south
        BiomeValues::Desert, // west
        BiomeValues::Desert  // center
    );
    EXPECT_EQ(result, BiomeValues::Desert);
}

TEST_F(SmoothLayerTest, EastWestEqualReturnsEast)
{
    // 东西相等但南北不等，返回东
    m_context->setConstantRandom(0);

    i32 result = m_smoothLayer->apply(*m_context,
        BiomeValues::Desert, // north
        BiomeValues::Forest, // east
        BiomeValues::Plains, // south
        BiomeValues::Forest, // west
        BiomeValues::Desert  // center
    );
    EXPECT_EQ(result, BiomeValues::Forest); // east
}

TEST_F(SmoothLayerTest, NorthSouthEqualReturnsNorth)
{
    // 南北相等但东西不等，返回北
    m_context->setConstantRandom(0);

    i32 result = m_smoothLayer->apply(*m_context,
        BiomeValues::Forest, // north
        BiomeValues::Desert, // east
        BiomeValues::Forest, // south
        BiomeValues::Plains, // west
        BiomeValues::Desert  // center
    );
    EXPECT_EQ(result, BiomeValues::Forest); // north
}

TEST_F(SmoothLayerTest, NoneEqualReturnsCenter)
{
    // 都不相等，返回中心
    m_context->setConstantRandom(0);

    i32 result = m_smoothLayer->apply(*m_context,
        BiomeValues::Desert, // north
        BiomeValues::Forest, // east
        BiomeValues::Plains, // south
        BiomeValues::Jungle, // west
        BiomeValues::Swamp   // center
    );
    EXPECT_EQ(result, BiomeValues::Swamp); // center
}

// ============================================================================
// ShoreLayer 测试
// ============================================================================

class ShoreLayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_context = std::make_unique<MockAreaContext>();
        m_shoreLayer = std::make_unique<ShoreLayer>();
    }

    std::unique_ptr<MockAreaContext> m_context;
    std::unique_ptr<ShoreLayer> m_shoreLayer;
};

TEST_F(ShoreLayerTest, MushroomFieldsAdjacentToShallowOceanBecomesShore)
{
    // 蘑菇岛相邻浅海变成蘑菇岛海岸
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Ocean,         // north - shallow ocean
        BiomeValues::Desert,        // east
        BiomeValues::Desert,        // south
        BiomeValues::Desert,        // west
        BiomeValues::MushroomFields // center
    );
    EXPECT_EQ(result, BiomeValues::MushroomFieldShore);
}

TEST_F(ShoreLayerTest, MushroomFieldsNotAdjacentToOceanStaysSame)
{
    // 蘑菇岛不相邻海洋保持不变
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Desert,        // north
        BiomeValues::Desert,        // east
        BiomeValues::Desert,        // south
        BiomeValues::Desert,        // west
        BiomeValues::MushroomFields // center
    );
    EXPECT_EQ(result, BiomeValues::MushroomFields);
}

TEST_F(ShoreLayerTest, SnowyBiomeAdjacentToOceanBecomesSnowyBeach)
{
    // 雪地生物群系相邻海洋变成雪地海滩
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Ocean,      // north - ocean
        BiomeValues::Desert,     // east
        BiomeValues::Desert,     // south
        BiomeValues::Desert,     // west
        BiomeValues::SnowyPlains // center
    );
    EXPECT_EQ(result, BiomeValues::SnowyBeach);
}

TEST_F(ShoreLayerTest, JungleAdjacentToIncompatibleBecomesJungleEdge)
{
    // 丛林相邻不兼容生物群系变成丛林边缘
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Desert, // north - not jungle compatible
        BiomeValues::Jungle, // east
        BiomeValues::Jungle, // south
        BiomeValues::Jungle, // west
        BiomeValues::Jungle  // center
    );
    EXPECT_EQ(result, BiomeValues::JungleEdge);
}

TEST_F(ShoreLayerTest, MountainsAdjacentToOceanBecomesStoneShore)
{
    // 山地相邻海洋变成石岸
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Ocean,    // north - ocean
        BiomeValues::Desert,   // east
        BiomeValues::Desert,   // south
        BiomeValues::Desert,   // west
        BiomeValues::Mountains // center
    );
    EXPECT_EQ(result, BiomeValues::StoneShore);
}

TEST_F(ShoreLayerTest, NormalBiomeAdjacentToOceanBecomesBeach)
{
    // 普通生物群系相邻海洋变成海滩
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Ocean,  // north - ocean
        BiomeValues::Desert, // east
        BiomeValues::Desert, // south
        BiomeValues::Desert, // west
        BiomeValues::Plains  // center
    );
    EXPECT_EQ(result, BiomeValues::Beach);
}

TEST_F(ShoreLayerTest, RiverNotAffected)
{
    // 河流不受海岸层影响
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Ocean,  // north
        BiomeValues::Desert, // east
        BiomeValues::Desert, // south
        BiomeValues::Desert, // west
        BiomeValues::River   // center
    );
    EXPECT_EQ(result, BiomeValues::River);
}

TEST_F(ShoreLayerTest, SwampNotAffected)
{
    // 沼泽不受海岸层影响
    i32 result = m_shoreLayer->apply(*m_context,
        BiomeValues::Ocean,  // north
        BiomeValues::Desert, // east
        BiomeValues::Desert, // south
        BiomeValues::Desert, // west
        BiomeValues::Swamp   // center
    );
    EXPECT_EQ(result, BiomeValues::Swamp);
}

// ============================================================================
// BiomeValues 辅助函数测试
// ============================================================================

class BiomeValuesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(BiomeValuesTest, IsOceanCorrect)
{
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::Ocean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::DeepOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::WarmOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::LukewarmOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::ColdOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::FrozenOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::DeepWarmOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::DeepLukewarmOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::DeepColdOcean));
    EXPECT_TRUE(BiomeValues::isOcean(BiomeValues::DeepFrozenOcean));

    EXPECT_FALSE(BiomeValues::isOcean(BiomeValues::Plains));
    EXPECT_FALSE(BiomeValues::isOcean(BiomeValues::Desert));
    EXPECT_FALSE(BiomeValues::isOcean(BiomeValues::River));
}

TEST_F(BiomeValuesTest, IsShallowOceanCorrect)
{
    EXPECT_TRUE(BiomeValues::isShallowOcean(BiomeValues::Ocean));
    EXPECT_TRUE(BiomeValues::isShallowOcean(BiomeValues::WarmOcean));
    EXPECT_TRUE(BiomeValues::isShallowOcean(BiomeValues::LukewarmOcean));
    EXPECT_TRUE(BiomeValues::isShallowOcean(BiomeValues::ColdOcean));
    EXPECT_TRUE(BiomeValues::isShallowOcean(BiomeValues::FrozenOcean));

    EXPECT_FALSE(BiomeValues::isShallowOcean(BiomeValues::DeepOcean));
    EXPECT_FALSE(BiomeValues::isShallowOcean(BiomeValues::DeepWarmOcean));
    EXPECT_FALSE(BiomeValues::isShallowOcean(BiomeValues::Plains));
}

TEST_F(BiomeValuesTest, IsBadlandsCorrect)
{
    EXPECT_TRUE(BiomeValues::isBadlands(BiomeValues::Badlands));
    EXPECT_TRUE(BiomeValues::isBadlands(BiomeValues::WoodedBadlandsPlateau));
    EXPECT_TRUE(BiomeValues::isBadlands(BiomeValues::BadlandsPlateau));
    EXPECT_TRUE(BiomeValues::isBadlands(BiomeValues::ErodedBadlands));
    EXPECT_TRUE(BiomeValues::isBadlands(BiomeValues::ModifiedWoodedBadlandsPlateau));
    EXPECT_TRUE(BiomeValues::isBadlands(BiomeValues::ModifiedBadlandsPlateau));

    EXPECT_FALSE(BiomeValues::isBadlands(BiomeValues::Desert));
    EXPECT_FALSE(BiomeValues::isBadlands(BiomeValues::Plains));
}

TEST_F(BiomeValuesTest, IsJungleCorrect)
{
    EXPECT_TRUE(BiomeValues::isJungle(BiomeValues::Jungle));
    EXPECT_TRUE(BiomeValues::isJungle(BiomeValues::JungleHills));
    EXPECT_TRUE(BiomeValues::isJungle(BiomeValues::JungleEdge));
    EXPECT_TRUE(BiomeValues::isJungle(BiomeValues::BambooJungle));
    EXPECT_TRUE(BiomeValues::isJungle(BiomeValues::BambooJungleHills));
    EXPECT_TRUE(BiomeValues::isJungle(BiomeValues::ModifiedJungle));
    EXPECT_TRUE(BiomeValues::isJungle(BiomeValues::ModifiedJungleEdge));

    EXPECT_FALSE(BiomeValues::isJungle(BiomeValues::Forest));
    EXPECT_FALSE(BiomeValues::isJungle(BiomeValues::Desert));
}

TEST_F(BiomeValuesTest, IsJungleCompatibleCorrect)
{
    // 丛林兼容：丛林类、森林、针叶林、海洋
    EXPECT_TRUE(BiomeValues::isJungleCompatible(BiomeValues::Jungle));
    EXPECT_TRUE(BiomeValues::isJungleCompatible(BiomeValues::Forest));
    EXPECT_TRUE(BiomeValues::isJungleCompatible(BiomeValues::Taiga));
    EXPECT_TRUE(BiomeValues::isJungleCompatible(BiomeValues::Ocean));
    EXPECT_TRUE(BiomeValues::isJungleCompatible(BiomeValues::DeepOcean));

    EXPECT_FALSE(BiomeValues::isJungleCompatible(BiomeValues::Desert));
    EXPECT_FALSE(BiomeValues::isJungleCompatible(BiomeValues::Plains));
}

TEST_F(BiomeValuesTest, IsSnowyCorrect)
{
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::SnowyPlains));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::SnowyMountains));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::SnowyBeach));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::SnowyTaiga));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::SnowyTaigaHills));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::SnowyTaigaMountains));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::IceSpikes));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::FrozenOcean));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::DeepFrozenOcean));
    EXPECT_TRUE(BiomeValues::isSnowy(BiomeValues::FrozenRiver));

    EXPECT_FALSE(BiomeValues::isSnowy(BiomeValues::Plains));
    EXPECT_FALSE(BiomeValues::isSnowy(BiomeValues::Desert));
    EXPECT_FALSE(BiomeValues::isSnowy(BiomeValues::Taiga));
}

TEST_F(BiomeValuesTest, IsMountainCorrect)
{
    EXPECT_TRUE(BiomeValues::isMountain(BiomeValues::Mountains));
    EXPECT_TRUE(BiomeValues::isMountain(BiomeValues::WoodedMountains));
    EXPECT_TRUE(BiomeValues::isMountain(BiomeValues::GravellyMountains));
    EXPECT_TRUE(BiomeValues::isMountain(BiomeValues::ModifiedGravellyMountains));
    EXPECT_TRUE(BiomeValues::isMountain(BiomeValues::MountainEdge));

    EXPECT_FALSE(BiomeValues::isMountain(BiomeValues::Plains));
    EXPECT_FALSE(BiomeValues::isMountain(BiomeValues::Forest));
}

TEST_F(BiomeValuesTest, AreBiomesSimilarCorrect)
{
    // 相同生物群系
    EXPECT_TRUE(BiomeValues::areBiomesSimilar(BiomeValues::Plains, BiomeValues::Plains));
    EXPECT_TRUE(BiomeValues::areBiomesSimilar(BiomeValues::Desert, BiomeValues::Desert));

    // 同类别生物群系
    EXPECT_TRUE(BiomeValues::areBiomesSimilar(BiomeValues::Plains, BiomeValues::SunflowerPlains));
    EXPECT_TRUE(BiomeValues::areBiomesSimilar(BiomeValues::Forest, BiomeValues::BirchForest));
    EXPECT_TRUE(BiomeValues::areBiomesSimilar(BiomeValues::Ocean, BiomeValues::DeepOcean));
    // 注意：在 MC 1.16.5 中，JungleHills (17) 被映射到 Desert 类别，不是 Jungle 类别
    // 这是 MC 的历史原因，所以 Jungle 和 JungleHills 不是相似的
    EXPECT_TRUE(BiomeValues::areBiomesSimilar(BiomeValues::Jungle, BiomeValues::ModifiedJungle));

    // 不同类别生物群系
    EXPECT_FALSE(BiomeValues::areBiomesSimilar(BiomeValues::Plains, BiomeValues::Desert));
    EXPECT_FALSE(BiomeValues::areBiomesSimilar(BiomeValues::Forest, BiomeValues::Ocean));
}

TEST_F(BiomeValuesTest, SpecialBitsExtraction)
{
    // 测试特殊位提取
    i32 value = BiomeValues::Climate::Warm | 0x300; // special = 3
    EXPECT_EQ(BiomeValues::SpecialBits::extract(value), 3);

    value = BiomeValues::Climate::Cool; // no special bits
    EXPECT_EQ(BiomeValues::SpecialBits::extract(value), 0);
}

TEST_F(BiomeValuesTest, SpecialBitsSet)
{
    // 测试特殊位设置
    i32 value = BiomeValues::Climate::Warm;
    i32 result = BiomeValues::SpecialBits::set(value, 5);
    EXPECT_EQ(BiomeValues::SpecialBits::extract(result), 5);
    EXPECT_EQ(result & ~BiomeValues::SpecialBits::Mask, BiomeValues::Climate::Warm);
}
