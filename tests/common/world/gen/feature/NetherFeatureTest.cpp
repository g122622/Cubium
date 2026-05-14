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

#include "world/gen/feature/FeatureIds.hpp"
#include "world/gen/feature/nether/BasaltFeature.hpp"
#include "world/gen/feature/nether/GlowstoneFeature.hpp"
#include "world/gen/feature/nether/MagmaPatchFeature.hpp"
#include "world/gen/feature/nether/NetherFeatures.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

// ============================================================================
// GlowstoneFeature 测试
// ============================================================================

class GlowstoneFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化特征
        GlowstoneFeatures::initialize();
    }
};

TEST_F(GlowstoneFeatureTest, InitializeFeatures)
{
    const auto& features = GlowstoneFeatures::getAllFeatures();
    EXPECT_GE(features.size(), 2); // Normal 和 Large
}

TEST_F(GlowstoneFeatureTest, CreateNormalFeature)
{
    auto feature = GlowstoneFeatures::createNormal();
    ASSERT_NE(feature, nullptr);
    EXPECT_EQ(feature->stage(), DecorationStage::UndergroundDecoration);
    EXPECT_NE(feature->name(), nullptr);
}

TEST_F(GlowstoneFeatureTest, CreateLargeFeature)
{
    auto feature = GlowstoneFeatures::createLarge();
    ASSERT_NE(feature, nullptr);
    EXPECT_EQ(feature->stage(), DecorationStage::UndergroundDecoration);
}

TEST_F(GlowstoneFeatureTest, ConfigValues)
{
    auto feature = GlowstoneFeatures::createNormal();
    const auto& config = feature->getConfig();
    EXPECT_EQ(config.maxDistance, 8);
    EXPECT_EQ(config.branchCount, 4);
    EXPECT_EQ(config.maxBranchLength, 6);
}

// ============================================================================
// BasaltColumnFeature 测试
// ============================================================================

class BasaltColumnFeatureTest : public ::testing::Test {
protected:
    void SetUp() override { BasaltColumnFeatures::initialize(); }
};

TEST_F(BasaltColumnFeatureTest, InitializeFeatures)
{
    const auto& features = BasaltColumnFeatures::getAllFeatures();
    EXPECT_GE(features.size(), 2); // Normal 和 Large
}

TEST_F(BasaltColumnFeatureTest, CreateNormalFeature)
{
    auto feature = BasaltColumnFeatures::createNormal();
    ASSERT_NE(feature, nullptr);
    EXPECT_EQ(feature->stage(), DecorationStage::UndergroundDecoration);
}

TEST_F(BasaltColumnFeatureTest, CreateLargeFeature)
{
    auto feature = BasaltColumnFeatures::createLarge();
    ASSERT_NE(feature, nullptr);
    const auto& config = feature->getConfig();
    EXPECT_GT(config.maxHeight, 5);
}

// ============================================================================
// BasaltDeltaFeature 测试
// ============================================================================

class BasaltDeltaFeatureTest : public ::testing::Test {
protected:
    void SetUp() override { BasaltDeltaFeatures::initialize(); }
};

TEST_F(BasaltDeltaFeatureTest, InitializeFeatures)
{
    const auto& features = BasaltDeltaFeatures::getAllFeatures();
    EXPECT_GE(features.size(), 1);
}

TEST_F(BasaltDeltaFeatureTest, CreateNormalFeature)
{
    auto feature = BasaltDeltaFeatures::createNormal();
    ASSERT_NE(feature, nullptr);
    EXPECT_EQ(feature->stage(), DecorationStage::UndergroundDecoration);
}

// ============================================================================
// MagmaPatchFeature 测试
// ============================================================================

class MagmaPatchFeatureTest : public ::testing::Test {
protected:
    void SetUp() override { MagmaPatchFeatures::initialize(); }
};

TEST_F(MagmaPatchFeatureTest, InitializeFeatures)
{
    const auto& features = MagmaPatchFeatures::getAllFeatures();
    EXPECT_GE(features.size(), 2); // Normal 和 Dense
}

TEST_F(MagmaPatchFeatureTest, CreateNormalFeature)
{
    auto feature = MagmaPatchFeatures::createNormal();
    ASSERT_NE(feature, nullptr);
    EXPECT_EQ(feature->stage(), DecorationStage::UndergroundDecoration);
}

TEST_F(MagmaPatchFeatureTest, CreateDenseFeature)
{
    auto feature = MagmaPatchFeatures::createDense();
    ASSERT_NE(feature, nullptr);
    // Dense feature should have different name
    EXPECT_NE(feature->name(), nullptr);
}

// ============================================================================
// NetherFireFeature 测试
// ============================================================================

class NetherFireFeatureTest : public ::testing::Test {
protected:
    void SetUp() override { NetherFireFeatures::initialize(); }
};

TEST_F(NetherFireFeatureTest, InitializeFeatures)
{
    const auto& features = NetherFireFeatures::getAllFeatures();
    EXPECT_GE(features.size(), 1);
}

TEST_F(NetherFireFeatureTest, CreateNormalFeature)
{
    auto feature = NetherFireFeatures::createNormal();
    ASSERT_NE(feature, nullptr);
    EXPECT_EQ(feature->stage(), DecorationStage::VegetalDecoration);
}

// ============================================================================
// NetherFeatureRegistry 测试
// ============================================================================

class NetherFeatureRegistryTest : public ::testing::Test {
protected:
    void SetUp() override { NetherFeatureRegistry::initialize(); }
};

TEST_F(NetherFeatureRegistryTest, InitializeRegistry)
{
    // 初始化应该不会抛出异常
    EXPECT_NO_THROW(NetherFeatureRegistry::initialize());
}

TEST_F(NetherFeatureRegistryTest, GetUndergroundFeatures)
{
    auto features = NetherFeatureRegistry::getAllUndergroundFeaturesAndClear();
    EXPECT_GE(features.size(), 5); // 至少有萤石、玄武岩柱、玄武岩三角洲、岩浆池
}

TEST_F(NetherFeatureRegistryTest, GetVegetationFeatures)
{
    // 需要重新初始化，因为上面的测试清空了
    NetherFeatureRegistry::initialize();
    auto features = NetherFeatureRegistry::getAllVegetationFeaturesAndClear();
    EXPECT_GE(features.size(), 2); // 至少有巨型真菌、下界火焰
}

TEST_F(NetherFeatureRegistryTest, FeatureStages)
{
    NetherFeatureRegistry::initialize();
    auto underground = NetherFeatureRegistry::getAllUndergroundFeaturesAndClear();

    for (const auto& feature : underground) {
        if (feature) {
            EXPECT_EQ(feature->stage(), DecorationStage::UndergroundDecoration);
        }
    }

    NetherFeatureRegistry::initialize();
    auto vegetation = NetherFeatureRegistry::getAllVegetationFeaturesAndClear();

    for (const auto& feature : vegetation) {
        if (feature) {
            EXPECT_EQ(feature->stage(), DecorationStage::VegetalDecoration);
        }
    }
}

// ============================================================================
// FeatureIds 测试
// ============================================================================

TEST(NetherFeatureIdsTest, GlowstoneIds)
{
    EXPECT_EQ(GlowstoneFeatureIds::Normal, 0);
    EXPECT_EQ(GlowstoneFeatureIds::Large, 1);
    EXPECT_EQ(GlowstoneFeatureIds::Count, 2);
}

TEST(NetherFeatureIdsTest, BasaltIds)
{
    EXPECT_EQ(BasaltFeatureIds::Offset, GlowstoneFeatureIds::Count);
    EXPECT_EQ(BasaltFeatureIds::ColumnNormal, 2);
    EXPECT_EQ(BasaltFeatureIds::ColumnLarge, 3);
    EXPECT_EQ(BasaltFeatureIds::Delta, 4);
    EXPECT_EQ(BasaltFeatureIds::Count, 3);
}

TEST(NetherFeatureIdsTest, MagmaIds)
{
    EXPECT_EQ(MagmaFeatureIds::Offset, GlowstoneFeatureIds::Count + BasaltFeatureIds::Count);
    EXPECT_EQ(MagmaFeatureIds::PatchNormal, 5);
    EXPECT_EQ(MagmaFeatureIds::PatchDense, 6);
    EXPECT_EQ(MagmaFeatureIds::Count, 2);
}

TEST(NetherFeatureIdsTest, NetherFungusIds)
{
    EXPECT_GE(NetherFungusIds::CrimsonFungus, 0);
    EXPECT_GE(NetherFungusIds::WarpedFungus, 1);
    EXPECT_GE(NetherFungusIds::NetherFire, 2);
    EXPECT_EQ(NetherFungusIds::Count, 3);
}

} // namespace
} // namespace mc
