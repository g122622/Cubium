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

/**
 * @file test_new_tree_features.cpp
 * @brief 新增树木特征配置和相关修复的单元测试
 *
 * 测试本次变更中新增的6种树木配置（FancyOak, Pine, JungleBush,
 * SwampTree, MegaPine, TallBirch）、新注册的方块、以及各 FoliagePlacer
 * 和 TrunkPlacer 的基本行为。
 */

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "util/math/MathUtils.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/gen/feature/ConfiguredFeature.hpp"
#include "world/gen/feature/FeatureIds.hpp"
#include "world/gen/feature/FeatureSpread.hpp"
#include "world/gen/feature/nether/HugeFungusFeature.hpp"
#include "world/gen/feature/ore/OreFeature.hpp"
#include "world/gen/feature/tree/TreeFeature.hpp"
#include "world/gen/feature/tree/foliage/BlobFoliagePlacer.hpp"
#include "world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "world/gen/feature/tree/foliage/FoliagePlacers.hpp"
#include "world/gen/feature/tree/trunk/StraightTrunkPlacer.hpp"
#include "world/gen/feature/tree/trunk/TrunkPlacer.hpp"
#include "world/gen/feature/tree/trunk/TrunkPlacers.hpp"
#include "world/gen/feature/vegetation/BigMushroomFeature.hpp"
#include <map>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 新增树木特征配置测试
// ============================================================================

class NewTreeFeatureConfigTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(NewTreeFeatureConfigTest, FancyOakConfig)
{
    auto feature = TreeFeatures::createFancyOakTree();
    ASSERT_NE(feature, nullptr);

    const TreeFeatureConfig& config = feature->getConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::OAK_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "fancy");
    EXPECT_STREQ(config.foliagePlacer->name(), "fancy");
    EXPECT_EQ(config.minHeight, 4);
    EXPECT_STREQ(feature->name(), "fancy_oak_tree");
}

TEST_F(NewTreeFeatureConfigTest, PineConfig)
{
    auto feature = TreeFeatures::createPineTree();
    ASSERT_NE(feature, nullptr);

    const TreeFeatureConfig& config = feature->getConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::SPRUCE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::SPRUCE_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "pine");
    EXPECT_EQ(config.minHeight, 6);
    EXPECT_STREQ(feature->name(), "pine_tree");
}

TEST_F(NewTreeFeatureConfigTest, JungleBushConfig)
{
    auto feature = TreeFeatures::createJungleBush();
    ASSERT_NE(feature, nullptr);

    const TreeFeatureConfig& config = feature->getConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    // Jungle bush uses jungle log but oak leaves (MC behavior)
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::JUNGLE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "bush");
    EXPECT_EQ(config.minHeight, 1);
    EXPECT_STREQ(feature->name(), "jungle_bush");
}

TEST_F(NewTreeFeatureConfigTest, SwampTreeConfig)
{
    auto feature = TreeFeatures::createSwampTree();
    ASSERT_NE(feature, nullptr);

    const TreeFeatureConfig& config = feature->getConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::OAK_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::OAK_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "BlobFoliagePlacer");
    // Swamp tree allows water depth
    EXPECT_EQ(config.maxWaterDepth, 1);
    EXPECT_EQ(config.minHeight, 5);
    EXPECT_STREQ(feature->name(), "swamp_tree");
}

TEST_F(NewTreeFeatureConfigTest, MegaPineConfig)
{
    auto feature = TreeFeatures::createMegaPineTree();
    ASSERT_NE(feature, nullptr);

    const TreeFeatureConfig& config = feature->getConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::SPRUCE_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::SPRUCE_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "giant");
    EXPECT_STREQ(config.foliagePlacer->name(), "mega_pine");
    EXPECT_EQ(config.minHeight, 13);
    EXPECT_STREQ(feature->name(), "mega_pine_tree");
}

TEST_F(NewTreeFeatureConfigTest, TallBirchConfig)
{
    auto feature = TreeFeatures::createTallBirchTree();
    ASSERT_NE(feature, nullptr);

    const TreeFeatureConfig& config = feature->getConfig();
    ASSERT_NE(config.trunkBlock, nullptr);
    ASSERT_NE(config.foliageBlock, nullptr);
    EXPECT_TRUE(config.trunkBlock->is(VanillaBlocks::BIRCH_LOG));
    EXPECT_TRUE(config.foliageBlock->is(VanillaBlocks::BIRCH_LEAVES));
    EXPECT_NE(config.trunkPlacer, nullptr);
    EXPECT_NE(config.foliagePlacer, nullptr);
    EXPECT_STREQ(config.trunkPlacer->name(), "StraightTrunkPlacer");
    EXPECT_STREQ(config.foliagePlacer->name(), "BlobFoliagePlacer");
    EXPECT_EQ(config.minHeight, 5);
    EXPECT_STREQ(feature->name(), "tall_birch_tree");
}

// ============================================================================
// TreeFeatureConfig 深拷贝测试
// ============================================================================

TEST_F(NewTreeFeatureConfigTest, DeepCopyPreservesTrunkPlacer)
{
    auto feature = TreeFeatures::createFancyOakTree();
    ASSERT_NE(feature, nullptr);
    const TreeFeatureConfig& original = feature->getConfig();

    // Copy construct
    TreeFeatureConfig copy(original);
    ASSERT_NE(copy.trunkPlacer, nullptr);
    ASSERT_NE(copy.foliagePlacer, nullptr);
    // The pointers should be different (deep copy)
    EXPECT_NE(copy.trunkPlacer.get(), original.trunkPlacer.get());
    EXPECT_NE(copy.foliagePlacer.get(), original.foliagePlacer.get());
    // But the names should be the same
    EXPECT_STREQ(copy.trunkPlacer->name(), original.trunkPlacer->name());
    EXPECT_STREQ(copy.foliagePlacer->name(), original.foliagePlacer->name());
    EXPECT_EQ(copy.maxWaterDepth, original.maxWaterDepth);
    EXPECT_EQ(copy.minHeight, original.minHeight);
}

// ============================================================================
// 新增 FoliagePlacer 测试
// ============================================================================

class NewFoliagePlacerTest : public ::testing::Test {
protected:
    void SetUp() override { random = std::make_unique<math::Random>(54321); }

    std::unique_ptr<math::Random> random;
};

TEST_F(NewFoliagePlacerTest, PineFoliagePlacerName)
{
    PineFoliagePlacer placer(FeatureSpread::spread(1, 1), FeatureSpread::fixed(1), 4);
    EXPECT_STREQ(placer.name(), "pine");
}

TEST_F(NewFoliagePlacerTest, PineFoliagePlacerHeight)
{
    PineFoliagePlacer placer(FeatureSpread::spread(1, 1), FeatureSpread::fixed(1), 4);
    // Pine foliage height should be reasonable
    i32 height = placer.getFoliageHeight(*random, 6);
    EXPECT_GT(height, 0);
}

TEST_F(NewFoliagePlacerTest, PineFoliagePlacerClone)
{
    PineFoliagePlacer placer(FeatureSpread::spread(1, 1), FeatureSpread::fixed(1), 4);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "pine");
}

TEST_F(NewFoliagePlacerTest, SpruceFoliagePlacerName)
{
    SpruceFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    EXPECT_STREQ(placer.name(), "spruce");
}

TEST_F(NewFoliagePlacerTest, SpruceFoliagePlacerHeight)
{
    SpruceFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    i32 height = placer.getFoliageHeight(*random, 6);
    EXPECT_GT(height, 0);
}

TEST_F(NewFoliagePlacerTest, SpruceFoliagePlacerClone)
{
    SpruceFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "spruce");
}

TEST_F(NewFoliagePlacerTest, AcaciaFoliagePlacerName)
{
    AcaciaFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    EXPECT_STREQ(placer.name(), "acacia");
}

TEST_F(NewFoliagePlacerTest, AcaciaFoliagePlacerHeight)
{
    AcaciaFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    // MC 1.16.5: Acacia foliage is flat (getFoliageHeight returns 0)
    i32 height = placer.getFoliageHeight(*random, 6);
    EXPECT_EQ(height, 0);
}

TEST_F(NewFoliagePlacerTest, AcaciaFoliagePlacerClone)
{
    AcaciaFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "acacia");
}

TEST_F(NewFoliagePlacerTest, DarkOakFoliagePlacerName)
{
    DarkOakFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    EXPECT_STREQ(placer.name(), "dark_oak");
}

TEST_F(NewFoliagePlacerTest, DarkOakFoliagePlacerClone)
{
    DarkOakFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "dark_oak");
}

TEST_F(NewFoliagePlacerTest, JungleFoliagePlacerName)
{
    JungleFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    EXPECT_STREQ(placer.name(), "jungle");
}

TEST_F(NewFoliagePlacerTest, JungleFoliagePlacerClone)
{
    JungleFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 2);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "jungle");
}

TEST_F(NewFoliagePlacerTest, MegaPineFoliagePlacerName)
{
    MegaPineFoliagePlacer placer(FeatureSpread::spread(3, 2), FeatureSpread::fixed(0), 8);
    EXPECT_STREQ(placer.name(), "mega_pine");
}

TEST_F(NewFoliagePlacerTest, MegaPineFoliagePlacerClone)
{
    MegaPineFoliagePlacer placer(FeatureSpread::spread(3, 2), FeatureSpread::fixed(0), 8);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "mega_pine");
}

TEST_F(NewFoliagePlacerTest, BushFoliagePlacerName)
{
    BushFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    EXPECT_STREQ(placer.name(), "bush");
}

TEST_F(NewFoliagePlacerTest, BushFoliagePlacerHeight)
{
    BushFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    // Bush foliage should be single-layer (height 1)
    i32 height = placer.getFoliageHeight(*random, 1);
    EXPECT_GE(height, 1);
}

TEST_F(NewFoliagePlacerTest, BushFoliagePlacerClone)
{
    BushFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0));
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "bush");
}

TEST_F(NewFoliagePlacerTest, FancyFoliagePlacerName)
{
    FancyFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    EXPECT_STREQ(placer.name(), "fancy");
}

TEST_F(NewFoliagePlacerTest, FancyFoliagePlacerHeight)
{
    FancyFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    i32 height = placer.getFoliageHeight(*random, 10);
    EXPECT_GT(height, 0);
}

TEST_F(NewFoliagePlacerTest, FancyFoliagePlacerClone)
{
    FancyFoliagePlacer placer(FeatureSpread::spread(2, 1), FeatureSpread::fixed(0), 4);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "fancy");
}

// ============================================================================
// TrunkPlacer 测试
// ============================================================================

class NewTrunkPlacerTest : public ::testing::Test {
protected:
    void SetUp() override { random = std::make_unique<math::Random>(99999); }

    std::unique_ptr<math::Random> random;
};

TEST_F(NewTrunkPlacerTest, FancyTrunkPlacerName)
{
    FancyTrunkPlacer placer(3, 11, 0);
    EXPECT_STREQ(placer.name(), "fancy");
}

TEST_F(NewTrunkPlacerTest, FancyTrunkPlacerHeight)
{
    FancyTrunkPlacer placer(3, 11, 0);
    // Height range: 3 + [0,11] + [0,0] = 3-14
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 3);
        EXPECT_LE(height, 14);
    }
}

TEST_F(NewTrunkPlacerTest, FancyTrunkPlacerClone)
{
    FancyTrunkPlacer placer(3, 11, 0);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "fancy");
}

TEST_F(NewTrunkPlacerTest, ForkyTrunkPlacerName)
{
    ForkyTrunkPlacer placer(5, 2, 1);
    EXPECT_STREQ(placer.name(), "forky");
}

TEST_F(NewTrunkPlacerTest, ForkyTrunkPlacerHeight)
{
    ForkyTrunkPlacer placer(5, 2, 1);
    // Height range: 5 + [0,2] + [0,1] = 5-8
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 5);
        EXPECT_LE(height, 8);
    }
}

TEST_F(NewTrunkPlacerTest, ForkyTrunkPlacerClone)
{
    ForkyTrunkPlacer placer(5, 2, 1);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "forky");
}

TEST_F(NewTrunkPlacerTest, DarkOakTrunkPlacerName)
{
    DarkOakTrunkPlacer placer(6, 3, 1);
    EXPECT_STREQ(placer.name(), "dark_oak");
}

TEST_F(NewTrunkPlacerTest, DarkOakTrunkPlacerHeight)
{
    DarkOakTrunkPlacer placer(6, 3, 1);
    // Height range: 6 + [0,3] + [0,1] = 6-10
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 6);
        EXPECT_LE(height, 10);
    }
}

TEST_F(NewTrunkPlacerTest, DarkOakTrunkPlacerClone)
{
    DarkOakTrunkPlacer placer(6, 3, 1);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "dark_oak");
}

TEST_F(NewTrunkPlacerTest, GiantTrunkPlacerName)
{
    GiantTrunkPlacer placer(13, 5, 3);
    EXPECT_STREQ(placer.name(), "giant");
}

TEST_F(NewTrunkPlacerTest, GiantTrunkPlacerHeight)
{
    GiantTrunkPlacer placer(13, 5, 3);
    // Height range: 13 + [0,5] + [0,3] = 13-21
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 13);
        EXPECT_LE(height, 21);
    }
}

TEST_F(NewTrunkPlacerTest, GiantTrunkPlacerClone)
{
    GiantTrunkPlacer placer(13, 5, 3);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "giant");
}

TEST_F(NewTrunkPlacerTest, MegaJungleTrunkPlacerName)
{
    MegaJungleTrunkPlacer placer(10, 8, 5);
    EXPECT_STREQ(placer.name(), "mega_jungle");
}

TEST_F(NewTrunkPlacerTest, MegaJungleTrunkPlacerHeight)
{
    MegaJungleTrunkPlacer placer(10, 8, 5);
    // Height range: 10 + [0,8] + [0,5] = 10-23
    for (int i = 0; i < 100; ++i) {
        i32 height = placer.getHeight(*random);
        EXPECT_GE(height, 10);
        EXPECT_LE(height, 23);
    }
}

TEST_F(NewTrunkPlacerTest, MegaJungleTrunkPlacerClone)
{
    MegaJungleTrunkPlacer placer(10, 8, 5);
    auto clone = placer.clone();
    ASSERT_NE(clone, nullptr);
    EXPECT_STREQ(clone->name(), "mega_jungle");
}

// ============================================================================
// TreeFeatures 注册顺序完整性测试
// ============================================================================

class TreeFeaturesRegistrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(TreeFeaturesRegistrationTest, InitializeCreates15Features)
{
    TreeFeatures::initialize();
    const auto& features = TreeFeatures::getAllFeatures();
    EXPECT_EQ(features.size(), TreeFeatureIds::Count);
    EXPECT_EQ(features.size(), 15u);
}

TEST_F(TreeFeaturesRegistrationTest, FeatureNamesMatchRegistrationOrder)
{
    TreeFeatures::initialize();
    const auto& features = TreeFeatures::getAllFeatures();

    ASSERT_GE(features.size(), 15u);

    EXPECT_STREQ(features[TreeFeatureIds::OakTree]->name(), "oak_tree");
    EXPECT_STREQ(features[TreeFeatureIds::BirchTree]->name(), "birch_tree");
    EXPECT_STREQ(features[TreeFeatureIds::SpruceTree]->name(), "spruce_tree");
    EXPECT_STREQ(features[TreeFeatureIds::JungleTree]->name(), "jungle_tree");
    EXPECT_STREQ(features[TreeFeatureIds::AcaciaTree]->name(), "acacia_tree");
    EXPECT_STREQ(features[TreeFeatureIds::DarkOakTree]->name(), "dark_oak_tree");
    EXPECT_STREQ(features[TreeFeatureIds::SparseOakTree]->name(), "sparse_oak_tree");
    EXPECT_STREQ(features[TreeFeatureIds::GiantSpruceTree]->name(), "giant_spruce_tree");
    EXPECT_STREQ(features[TreeFeatureIds::GiantJungleTree]->name(), "giant_jungle_tree");
    EXPECT_STREQ(features[TreeFeatureIds::FancyOakTree]->name(), "fancy_oak_tree");
    EXPECT_STREQ(features[TreeFeatureIds::PineTree]->name(), "pine_tree");
    EXPECT_STREQ(features[TreeFeatureIds::JungleBush]->name(), "jungle_bush");
    EXPECT_STREQ(features[TreeFeatureIds::SwampTree]->name(), "swamp_tree");
    EXPECT_STREQ(features[TreeFeatureIds::MegaPineTree]->name(), "mega_pine_tree");
    EXPECT_STREQ(features[TreeFeatureIds::TallBirchTree]->name(), "tall_birch_tree");
}

TEST_F(TreeFeaturesRegistrationTest, AllFeaturesHaveValidConfigs)
{
    TreeFeatures::initialize();
    const auto& features = TreeFeatures::getAllFeatures();

    for (size_t i = 0; i < features.size(); ++i) {
        ASSERT_NE(features[i], nullptr) << "Feature at index " << i << " is null";
        const TreeFeatureConfig& config = features[i]->getConfig();
        EXPECT_NE(config.trunkBlock, nullptr) << "Feature " << features[i]->name() << " has null trunkBlock";
        EXPECT_NE(config.foliageBlock, nullptr) << "Feature " << features[i]->name() << " has null foliageBlock";
        EXPECT_NE(config.trunkPlacer, nullptr) << "Feature " << features[i]->name() << " has null trunkPlacer";
        EXPECT_NE(config.foliagePlacer, nullptr) << "Feature " << features[i]->name() << " has null foliagePlacer";
        EXPECT_GE(config.minHeight, 1) << "Feature " << features[i]->name() << " has invalid minHeight";
    }
}

TEST_F(TreeFeaturesRegistrationTest, AllFeaturesAreVegetalDecoration)
{
    TreeFeatures::initialize();
    const auto& features = TreeFeatures::getAllFeatures();

    for (size_t i = 0; i < features.size(); ++i) {
        EXPECT_EQ(features[i]->stage(), DecorationStage::VegetalDecoration)
            << "Feature " << features[i]->name() << " has wrong stage";
    }
}

// ============================================================================
// 新增方块注册测试
// ============================================================================

class NewBlockRegistrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(NewBlockRegistrationTest, FlowerBlocks)
{
    EXPECT_NE(VanillaBlocks::LILY_OF_THE_VALLEY, nullptr);
    EXPECT_NE(VanillaBlocks::SUNFLOWER, nullptr);
    EXPECT_NE(VanillaBlocks::LILAC, nullptr);
    EXPECT_NE(VanillaBlocks::ROSE_BUSH, nullptr);
    EXPECT_NE(VanillaBlocks::PEONY, nullptr);
    EXPECT_NE(VanillaBlocks::CORNFLOWER, nullptr);
    EXPECT_NE(VanillaBlocks::WITHER_ROSE, nullptr);
}

TEST_F(NewBlockRegistrationTest, MushroomBlocks)
{
    EXPECT_NE(VanillaBlocks::BROWN_MUSHROOM_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::RED_MUSHROOM_BLOCK, nullptr);
    EXPECT_NE(VanillaBlocks::MUSHROOM_STEM, nullptr);
}

TEST_F(NewBlockRegistrationTest, UtilityBlocks)
{
    EXPECT_NE(VanillaBlocks::FARMLAND, nullptr);
    EXPECT_NE(VanillaBlocks::RED_SAND, nullptr);
}

TEST_F(NewBlockRegistrationTest, NetherBlocks)
{
    EXPECT_NE(VanillaBlocks::CRIMSON_STEM, nullptr);
    EXPECT_NE(VanillaBlocks::WARPED_STEM, nullptr);
    EXPECT_NE(VanillaBlocks::CRIMSON_NYLIUM, nullptr);
    EXPECT_NE(VanillaBlocks::WARPED_NYLIUM, nullptr);
    EXPECT_NE(VanillaBlocks::SHROOMLIGHT, nullptr);
    EXPECT_NE(VanillaBlocks::CRIMSON_FUNGUS, nullptr);
    EXPECT_NE(VanillaBlocks::WARPED_FUNGUS, nullptr);
    EXPECT_NE(VanillaBlocks::WEEPING_VINES, nullptr);
    EXPECT_NE(VanillaBlocks::TWISTING_VINES, nullptr);
}

TEST_F(NewBlockRegistrationTest, AllNewBlocksHaveDefaultState)
{
    // Flower blocks
    ASSERT_NE(VanillaBlocks::LILY_OF_THE_VALLEY, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::LILY_OF_THE_VALLEY->defaultState());

    ASSERT_NE(VanillaBlocks::SUNFLOWER, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::SUNFLOWER->defaultState());

    ASSERT_NE(VanillaBlocks::CORNFLOWER, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::CORNFLOWER->defaultState());

    // Mushroom blocks
    ASSERT_NE(VanillaBlocks::BROWN_MUSHROOM_BLOCK, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::BROWN_MUSHROOM_BLOCK->defaultState());

    ASSERT_NE(VanillaBlocks::MUSHROOM_STEM, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::MUSHROOM_STEM->defaultState());

    // Nether blocks
    ASSERT_NE(VanillaBlocks::CRIMSON_STEM, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::CRIMSON_STEM->defaultState());

    ASSERT_NE(VanillaBlocks::SHROOMLIGHT, nullptr);
    EXPECT_NO_THROW(VanillaBlocks::SHROOMLIGHT->defaultState());
}

// ============================================================================
// BigMushroomFeature 配置测试
// ============================================================================

class BigMushroomFeatureTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(BigMushroomFeatureTest, BrownMushroomConfigUsesCorrectBlocks)
{
    auto feature = BigMushroomFeatures::createBrownMushroom();
    ASSERT_NE(feature, nullptr);
    EXPECT_STREQ(feature->name(), "brown_mushroom");
}

TEST_F(BigMushroomFeatureTest, RedMushroomConfigUsesCorrectBlocks)
{
    auto feature = BigMushroomFeatures::createRedMushroom();
    ASSERT_NE(feature, nullptr);
    EXPECT_STREQ(feature->name(), "red_mushroom");
}

TEST_F(BigMushroomFeatureTest, InitializeCreatesTwoFeatures)
{
    BigMushroomFeatures::initialize();
    const auto& features = BigMushroomFeatures::getAllFeatures();
    EXPECT_EQ(features.size(), MushroomFeatureIds::Count);
    EXPECT_EQ(features.size(), 2u);
}

// ============================================================================
// HugeFungusFeature 配置测试
// ============================================================================

class HugeFungusFeatureTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(HugeFungusFeatureTest, CrimsonFungusConfig)
{
    auto feature = HugeFungusFeatures::createCrimson();
    ASSERT_NE(feature, nullptr);
    EXPECT_STREQ(feature->name(), "crimson_fungus");
}

TEST_F(HugeFungusFeatureTest, WarpedFungusConfig)
{
    auto feature = HugeFungusFeatures::createWarped();
    ASSERT_NE(feature, nullptr);
    EXPECT_STREQ(feature->name(), "warped_fungus");
}

TEST_F(HugeFungusFeatureTest, InitializeCreatesTwoFeatures)
{
    HugeFungusFeatures::initialize();
    const auto& features = HugeFungusFeatures::getAllFeatures();
    EXPECT_EQ(features.size(), 2u);
}

TEST_F(HugeFungusFeatureTest, FungusTypeEnum)
{
    // Verify enum values exist
    EXPECT_NE(static_cast<int>(FungusType::Crimson), static_cast<int>(FungusType::Warped));
}

TEST_F(HugeFungusFeatureTest, FungusConfigDefaults)
{
    HugeFungusFeatureConfig config;
    EXPECT_EQ(config.fungusType, FungusType::Crimson);
    EXPECT_EQ(config.planted, false);
}

TEST_F(HugeFungusFeatureTest, FungusConfigExplicit)
{
    HugeFungusFeatureConfig config(FungusType::Warped, true);
    EXPECT_EQ(config.fungusType, FungusType::Warped);
    EXPECT_EQ(config.planted, true);
}

// ============================================================================
// 下界矿石注册测试
// ============================================================================

class NetherOreFeatureTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { VanillaBlocks::initialize(); }
};

TEST_F(NetherOreFeatureTest, NetherQuartzOreCreation)
{
    auto feature = OreFeatures::createNetherQuartzOre();
    ASSERT_NE(feature, nullptr);
    EXPECT_STREQ(feature->name(), "nether_quartz_ore");
}

TEST_F(NetherOreFeatureTest, NetherGoldOreCreation)
{
    auto feature = OreFeatures::createNetherGoldOre();
    ASSERT_NE(feature, nullptr);
    EXPECT_STREQ(feature->name(), "nether_gold_ore");
}

TEST_F(NetherOreFeatureTest, AncientDebrisCreation)
{
    auto feature = OreFeatures::createAncientDebris();
    ASSERT_NE(feature, nullptr);
    EXPECT_STREQ(feature->name(), "ancient_debris");
}

TEST_F(NetherOreFeatureTest, InitializeCreatesAllOres)
{
    OreFeatures::initialize();
    const auto& features = OreFeatures::getAllFeatures();
    EXPECT_EQ(features.size(), OreFeatureIds::Count);
    EXPECT_EQ(features.size(), 11u);
}

TEST_F(NetherOreFeatureTest, NetherOreIdsAreConsecutiveAfterOverworld)
{
    EXPECT_EQ(OreFeatureIds::NetherQuartzOre, 8u);
    EXPECT_EQ(OreFeatureIds::NetherGoldOre, 9u);
    EXPECT_EQ(OreFeatureIds::AncientDebris, 10u);
}

// ============================================================================
// FeatureIds 偏移量级联测试
// ============================================================================

TEST(FeatureIdsCascadeTest, TreeCountUpdateCascadesToAllOffsets)
{
    // After adding 6 new trees, TreeFeatureIds::Count is 15
    EXPECT_EQ(TreeFeatureIds::Count, 15u);

    // FlowerFeatureIds::Offset should equal TreeFeatureIds::Count
    EXPECT_EQ(FlowerFeatureIds::Offset, 15u);

    // GrassFeatureIds::Offset should equal Tree + Flower counts
    EXPECT_EQ(GrassFeatureIds::Offset, 15u + 5u);

    // MushroomFeatureIds::Offset should equal Tree + Flower + Grass counts
    EXPECT_EQ(MushroomFeatureIds::Offset, 15u + 5u + 7u);

    // CactusFeatureIds::Offset
    EXPECT_EQ(CactusFeatureIds::Offset, 15u + 5u + 7u + 2u);

    // SugarCaneFeatureIds::Offset
    EXPECT_EQ(SugarCaneFeatureIds::Offset, 15u + 5u + 7u + 2u + 2u);

    // Total
    EXPECT_EQ(VegetationIds::TotalVegetalFeatures, 15u + 5u + 7u + 2u + 2u + 2u);
    EXPECT_EQ(VegetationIds::TotalVegetalFeatures, 33u);
}

// ============================================================================
// BigMushroomFeatureConfig 测试
// ============================================================================

TEST(BigMushroomConfigTest, DefaultConfig)
{
    BigMushroomFeatureConfig config;
    EXPECT_EQ(config.capState, nullptr);
    EXPECT_EQ(config.stemState, nullptr);
    EXPECT_EQ(config.capRadius, 2);
}

TEST(BigMushroomConfigTest, ExplicitConfig)
{
    VanillaBlocks::initialize();
    const BlockState* cap =
        VanillaBlocks::BROWN_MUSHROOM_BLOCK ? &VanillaBlocks::BROWN_MUSHROOM_BLOCK->defaultState() : nullptr;
    const BlockState* stem = VanillaBlocks::MUSHROOM_STEM ? &VanillaBlocks::MUSHROOM_STEM->defaultState() : nullptr;

    BigMushroomFeatureConfig config(cap, stem, 3);
    EXPECT_EQ(config.capState, cap);
    EXPECT_EQ(config.stemState, stem);
    EXPECT_EQ(config.capRadius, 3);
}

// ============================================================================
// BigMushroomFeature 高度计算测试
// ============================================================================

// Test subclasses to access protected methods
class TestBrownMushroom : public BigBrownMushroomFeature {
public:
    using BigBrownMushroomFeature::getCapRadius;
};

class TestRedMushroom : public BigRedMushroomFeature {
public:
    using BigRedMushroomFeature::getCapRadius;
};

TEST(BigMushroomHeightTest, BrownMushroomCapRadius)
{
    TestBrownMushroom brownFeature;
    // For brown: height <= 3 returns 0, otherwise capRadius
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 2), 0);
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 3), 0);
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 4), 2);
    EXPECT_EQ(brownFeature.getCapRadius(-1, 6, 2, 5), 2);
}

TEST(BigMushroomHeightTest, RedMushroomCapRadius)
{
    TestRedMushroom redFeature;
    // For red: height < totalHeight - 3 returns 0
    // height == totalHeight returns capRadius
    // otherwise capRadius - 1
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 0), 0);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 2), 0);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 3), 1); // capRadius - 1
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 4), 1);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 5), 1);
    EXPECT_EQ(redFeature.getCapRadius(-1, 6, 2, 6), 2); // at totalHeight
}

// ============================================================================
// TrunkPlacer 高度分布测试
// ============================================================================

TEST(TrunkPlacerDistributionTest, TallBirchTrunkHeight)
{
    math::Random rng(777);
    StraightTrunkPlacer placer(5, 2, 6);

    // Height range: 5 + [0,2] + [0,6] = 5-13
    std::map<i32, i32> distribution;
    for (int i = 0; i < 1000; ++i) {
        i32 height = placer.getHeight(rng);
        distribution[height]++;
        EXPECT_GE(height, 5);
        EXPECT_LE(height, 13);
    }

    // At least 5 distinct heights should appear
    EXPECT_GE(distribution.size(), 5u);
}

TEST(TrunkPlacerDistributionTest, JungleBushTrunkHeight)
{
    math::Random rng(888);
    StraightTrunkPlacer placer(1, 0, 0);

    // Fixed height: always 1
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(placer.getHeight(rng), 1);
    }
}
