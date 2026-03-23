/**
 * @file test_vegetation_features.cpp
 * @brief 植被特征集成测试
 *
 * 测试植被特征的注册、配置和生物群系集成。
 */

#include <gtest/gtest.h>
#include "../src/common/world/gen/feature/FeatureIds.hpp"
#include "../src/common/world/gen/feature/ConfiguredFeature.hpp"
#include "../src/common/world/gen/feature/vegetation/VegetationFeatures.hpp"
#include "../src/common/world/biome/BiomeGenerationSettings.hpp"
#include "../src/common/world/block/VanillaBlocks.hpp"

using namespace mc;

class VegetationFeatureTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // 初始化方块系统
        VanillaBlocks::initialize();
        // 初始化特征注册表
        FeatureRegistry::instance().initialize();
    }

    static void TearDownTestSuite() {
        FeatureRegistry::instance().clear();
    }
};

// ============================================================================
// FeatureIds 测试
// ============================================================================

TEST_F(VegetationFeatureTest, OreFeatureIdsAreConsecutive) {
    // 验证矿石特征ID是连续的
    EXPECT_EQ(OreFeatureIds::CoalOre, 0u);
    EXPECT_EQ(OreFeatureIds::IronOre, 1u);
    EXPECT_EQ(OreFeatureIds::GoldOre, 2u);
    EXPECT_EQ(OreFeatureIds::RedstoneOre, 3u);
    EXPECT_EQ(OreFeatureIds::DiamondOre, 4u);
    EXPECT_EQ(OreFeatureIds::LapisOre, 5u);
    EXPECT_EQ(OreFeatureIds::EmeraldOre, 6u);
    EXPECT_EQ(OreFeatureIds::CopperOre, 7u);
    EXPECT_EQ(OreFeatureIds::Count, 8u);
}

TEST_F(VegetationFeatureTest, TreeFeatureIdsAreConsecutive) {
    // 验证树木特征ID是连续的
    // 注意：ID顺序必须与 TreeFeatures::initialize() 中注册顺序一致
    EXPECT_EQ(TreeFeatureIds::OakTree, 0u);
    EXPECT_EQ(TreeFeatureIds::BirchTree, 1u);
    EXPECT_EQ(TreeFeatureIds::SpruceTree, 2u);
    EXPECT_EQ(TreeFeatureIds::JungleTree, 3u);
    EXPECT_EQ(TreeFeatureIds::AcaciaTree, 4u);
    EXPECT_EQ(TreeFeatureIds::DarkOakTree, 5u);
    EXPECT_EQ(TreeFeatureIds::SparseOakTree, 6u);
    EXPECT_EQ(TreeFeatureIds::GiantSpruceTree, 7u);
    EXPECT_EQ(TreeFeatureIds::GiantJungleTree, 8u);
    EXPECT_EQ(TreeFeatureIds::Count, 9u);
}

TEST_F(VegetationFeatureTest, FlowerFeatureIdsHaveCorrectOffset) {
    // 验证花卉特征ID偏移量正确
    EXPECT_EQ(FlowerFeatureIds::Offset, TreeFeatureIds::Count);
    EXPECT_EQ(FlowerFeatureIds::PlainsFlowers, TreeFeatureIds::Count);
    EXPECT_EQ(FlowerFeatureIds::ForestFlowers, TreeFeatureIds::Count + 1);
    EXPECT_EQ(FlowerFeatureIds::FlowerForestFlowers, TreeFeatureIds::Count + 2);
    EXPECT_EQ(FlowerFeatureIds::SwampFlowers, TreeFeatureIds::Count + 3);
    EXPECT_EQ(FlowerFeatureIds::Sunflower, TreeFeatureIds::Count + 4);
    EXPECT_EQ(FlowerFeatureIds::Count, 5u);
}

TEST_F(VegetationFeatureTest, GrassFeatureIdsHaveCorrectOffset) {
    // 验证草丛特征ID偏移量正确
    const u32 expectedOffset = TreeFeatureIds::Count + FlowerFeatureIds::Count;
    EXPECT_EQ(GrassFeatureIds::Offset, expectedOffset);
    EXPECT_EQ(GrassFeatureIds::PlainsGrass, expectedOffset);
    EXPECT_EQ(GrassFeatureIds::ForestGrass, expectedOffset + 1);
    EXPECT_EQ(GrassFeatureIds::JungleGrass, expectedOffset + 2);
    EXPECT_EQ(GrassFeatureIds::SwampGrass, expectedOffset + 3);
    EXPECT_EQ(GrassFeatureIds::SavannaGrass, expectedOffset + 4);
    EXPECT_EQ(GrassFeatureIds::TaigaGrass, expectedOffset + 5);
    EXPECT_EQ(GrassFeatureIds::BadlandsDeadBush, expectedOffset + 6);
    EXPECT_EQ(GrassFeatureIds::Count, 7u);
}

TEST_F(VegetationFeatureTest, MushroomFeatureIdsHaveCorrectOffset) {
    // 验证蘑菇特征ID偏移量正确
    const u32 expectedOffset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count;
    EXPECT_EQ(MushroomFeatureIds::Offset, expectedOffset);
    EXPECT_EQ(MushroomFeatureIds::BrownMushroom, expectedOffset);
    EXPECT_EQ(MushroomFeatureIds::RedMushroom, expectedOffset + 1);
    EXPECT_EQ(MushroomFeatureIds::Count, 2u);
}

TEST_F(VegetationFeatureTest, CactusFeatureIdsHaveCorrectOffset) {
    // 验证仙人掌特征ID偏移量正确
    const u32 expectedOffset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count + MushroomFeatureIds::Count;
    EXPECT_EQ(CactusFeatureIds::Offset, expectedOffset);
    EXPECT_EQ(CactusFeatureIds::DesertCactus, expectedOffset);
    EXPECT_EQ(CactusFeatureIds::BadlandsCactus, expectedOffset + 1);
    EXPECT_EQ(CactusFeatureIds::Count, 2u);
}

TEST_F(VegetationFeatureTest, SugarCaneFeatureIdsHaveCorrectOffset) {
    // 验证甘蔗特征ID偏移量正确
    const u32 expectedOffset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count + MushroomFeatureIds::Count + CactusFeatureIds::Count;
    EXPECT_EQ(SugarCaneFeatureIds::Offset, expectedOffset);
    EXPECT_EQ(SugarCaneFeatureIds::Normal, expectedOffset);
    EXPECT_EQ(SugarCaneFeatureIds::Dense, expectedOffset + 1);
    EXPECT_EQ(SugarCaneFeatureIds::Count, 2u);
}

TEST_F(VegetationFeatureTest, IceSpikeFeatureIdsAreConsecutive) {
    // 验证冰刺特征ID是连续的
    EXPECT_EQ(IceSpikeFeatureIds::Spike, 0u);
    EXPECT_EQ(IceSpikeFeatureIds::Iceberg, 1u);
    EXPECT_EQ(IceSpikeFeatureIds::Count, 2u);
}

TEST_F(VegetationFeatureTest, TotalVegetalFeatureCount) {
    // 验证VegetalDecoration阶段特征总数正确
    const u32 expectedTotal =
        TreeFeatureIds::Count +
        FlowerFeatureIds::Count +
        GrassFeatureIds::Count +
        MushroomFeatureIds::Count +
        CactusFeatureIds::Count +
        SugarCaneFeatureIds::Count;

    EXPECT_EQ(VegetationIds::TotalVegetalFeatures, expectedTotal);
    EXPECT_EQ(VegetationIds::TotalVegetalFeatures, 27u); // 9+5+7+2+2+2
}

// ============================================================================
// FeatureRegistry 集成测试
// ============================================================================

TEST_F(VegetationFeatureTest, FeatureRegistryHasAllFeatures) {
    // 验证FeatureRegistry中注册了所有特征
    const auto& oreFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::UndergroundOres);
    EXPECT_EQ(oreFeatures.size(), OreFeatureIds::Count);

    const auto& vegetalFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);
    EXPECT_EQ(vegetalFeatures.size(), VegetationIds::TotalVegetalFeatures);

    const auto& surfaceFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::SurfaceStructures);
    EXPECT_EQ(surfaceFeatures.size(), IceSpikeFeatureIds::Count);
}

TEST_F(VegetationFeatureTest, FeatureRegistryFeatureNames) {
    // 验证特征名称正确设置
    const auto& oreFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::UndergroundOres);

    // 验证矿石特征名称
    EXPECT_NE(oreFeatures[OreFeatureIds::CoalOre], nullptr);
    EXPECT_STREQ(oreFeatures[OreFeatureIds::CoalOre]->name(), "coal_ore");

    EXPECT_NE(oreFeatures[OreFeatureIds::IronOre], nullptr);
    EXPECT_STREQ(oreFeatures[OreFeatureIds::IronOre]->name(), "iron_ore");

    EXPECT_NE(oreFeatures[OreFeatureIds::DiamondOre], nullptr);
    EXPECT_STREQ(oreFeatures[OreFeatureIds::DiamondOre]->name(), "diamond_ore");
}

TEST_F(VegetationFeatureTest, TreeFeatureNames) {
    const auto& vegetalFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);

    // 基础树木
    EXPECT_NE(vegetalFeatures[TreeFeatureIds::OakTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::OakTree]->name(), "oak_tree");

    EXPECT_NE(vegetalFeatures[TreeFeatureIds::BirchTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::BirchTree]->name(), "birch_tree");

    EXPECT_NE(vegetalFeatures[TreeFeatureIds::SpruceTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::SpruceTree]->name(), "spruce_tree");

    EXPECT_NE(vegetalFeatures[TreeFeatureIds::JungleTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::JungleTree]->name(), "jungle_tree");

    // 新增树木类型 (ID顺序与注册顺序一致)
    EXPECT_NE(vegetalFeatures[TreeFeatureIds::AcaciaTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::AcaciaTree]->name(), "acacia_tree");

    EXPECT_NE(vegetalFeatures[TreeFeatureIds::DarkOakTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::DarkOakTree]->name(), "dark_oak_tree");

    EXPECT_NE(vegetalFeatures[TreeFeatureIds::SparseOakTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::SparseOakTree]->name(), "sparse_oak_tree");

    EXPECT_NE(vegetalFeatures[TreeFeatureIds::GiantSpruceTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::GiantSpruceTree]->name(), "giant_spruce_tree");

    EXPECT_NE(vegetalFeatures[TreeFeatureIds::GiantJungleTree], nullptr);
    EXPECT_STREQ(vegetalFeatures[TreeFeatureIds::GiantJungleTree]->name(), "giant_jungle_tree");
}

TEST_F(VegetationFeatureTest, FlowerFeatureNames) {
    const auto& vegetalFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);

    EXPECT_NE(vegetalFeatures[FlowerFeatureIds::PlainsFlowers], nullptr);
    EXPECT_STREQ(vegetalFeatures[FlowerFeatureIds::PlainsFlowers]->name(), "plains_flowers");

    EXPECT_NE(vegetalFeatures[FlowerFeatureIds::ForestFlowers], nullptr);
    EXPECT_STREQ(vegetalFeatures[FlowerFeatureIds::ForestFlowers]->name(), "forest_flowers");

    EXPECT_NE(vegetalFeatures[FlowerFeatureIds::FlowerForestFlowers], nullptr);
    EXPECT_STREQ(vegetalFeatures[FlowerFeatureIds::FlowerForestFlowers]->name(), "flower_forest_flowers");
}

TEST_F(VegetationFeatureTest, GrassFeatureNames) {
    const auto& vegetalFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);

    EXPECT_NE(vegetalFeatures[GrassFeatureIds::PlainsGrass], nullptr);
    EXPECT_STREQ(vegetalFeatures[GrassFeatureIds::PlainsGrass]->name(), "plains_grass");

    EXPECT_NE(vegetalFeatures[GrassFeatureIds::TaigaGrass], nullptr);
    EXPECT_STREQ(vegetalFeatures[GrassFeatureIds::TaigaGrass]->name(), "taiga_grass");

    EXPECT_NE(vegetalFeatures[GrassFeatureIds::BadlandsDeadBush], nullptr);
    EXPECT_STREQ(vegetalFeatures[GrassFeatureIds::BadlandsDeadBush]->name(), "badlands_dead_bush");
}

TEST_F(VegetationFeatureTest, MushroomFeatureNames) {
    const auto& vegetalFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);

    EXPECT_NE(vegetalFeatures[MushroomFeatureIds::BrownMushroom], nullptr);
    EXPECT_STREQ(vegetalFeatures[MushroomFeatureIds::BrownMushroom]->name(), "brown_mushroom");

    EXPECT_NE(vegetalFeatures[MushroomFeatureIds::RedMushroom], nullptr);
    EXPECT_STREQ(vegetalFeatures[MushroomFeatureIds::RedMushroom]->name(), "red_mushroom");
}

TEST_F(VegetationFeatureTest, CactusFeatureNames) {
    const auto& vegetalFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);

    EXPECT_NE(vegetalFeatures[CactusFeatureIds::DesertCactus], nullptr);
    EXPECT_STREQ(vegetalFeatures[CactusFeatureIds::DesertCactus]->name(), "desert_cactus");

    EXPECT_NE(vegetalFeatures[CactusFeatureIds::BadlandsCactus], nullptr);
    EXPECT_STREQ(vegetalFeatures[CactusFeatureIds::BadlandsCactus]->name(), "badlands_cactus");
}

TEST_F(VegetationFeatureTest, SugarCaneFeatureNames) {
    const auto& vegetalFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);

    EXPECT_NE(vegetalFeatures[SugarCaneFeatureIds::Normal], nullptr);
    EXPECT_STREQ(vegetalFeatures[SugarCaneFeatureIds::Normal]->name(), "sugar_cane");

    EXPECT_NE(vegetalFeatures[SugarCaneFeatureIds::Dense], nullptr);
    EXPECT_STREQ(vegetalFeatures[SugarCaneFeatureIds::Dense]->name(), "sugar_cane_dense");
}

TEST_F(VegetationFeatureTest, IceSpikeFeatureNames) {
    const auto& surfaceFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::SurfaceStructures);

    EXPECT_NE(surfaceFeatures[IceSpikeFeatureIds::Spike], nullptr);
    EXPECT_STREQ(surfaceFeatures[IceSpikeFeatureIds::Spike]->name(), "ice_spike");

    EXPECT_NE(surfaceFeatures[IceSpikeFeatureIds::Iceberg], nullptr);
    EXPECT_STREQ(surfaceFeatures[IceSpikeFeatureIds::Iceberg]->name(), "ice_berg");
}

// ============================================================================
// BiomeGenerationSettings 集成测试
// ============================================================================

TEST_F(VegetationFeatureTest, PlainsBiomeSettings) {
    auto settings = BiomeGenerationSettings::createPlains();

    // 平原应该有矿石特征
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    EXPECT_FALSE(ores.empty());

    // 平原应该有稀疏橡树
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasSparseOakTree = false;
    for (u32 id : vegetal) {
        if (id == TreeFeatureIds::SparseOakTree) {
            hasSparseOakTree = true;
            break;
        }
    }
    EXPECT_TRUE(hasSparseOakTree);

    // 平原应该有花卉和草丛
    bool hasPlainsFlowers = false;
    bool hasPlainsGrass = false;
    for (u32 id : vegetal) {
        if (id == FlowerFeatureIds::PlainsFlowers) hasPlainsFlowers = true;
        if (id == GrassFeatureIds::PlainsGrass) hasPlainsGrass = true;
    }
    EXPECT_TRUE(hasPlainsFlowers);
    EXPECT_TRUE(hasPlainsGrass);
}

TEST_F(VegetationFeatureTest, ForestBiomeSettings) {
    auto settings = BiomeGenerationSettings::createForest();

    // 森林应该有树木
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    EXPECT_FALSE(vegetal.empty());

    // 森林应该有橡树、白桦和森林花卉/草丛
    bool hasOakTree = false;
    bool hasBirchTree = false;
    bool hasForestFlowers = false;
    bool hasForestGrass = false;

    for (u32 id : vegetal) {
        if (id == TreeFeatureIds::OakTree) hasOakTree = true;
        if (id == TreeFeatureIds::BirchTree) hasBirchTree = true;
        if (id == FlowerFeatureIds::ForestFlowers) hasForestFlowers = true;
        if (id == GrassFeatureIds::ForestGrass) hasForestGrass = true;
    }

    EXPECT_TRUE(hasOakTree);
    EXPECT_TRUE(hasBirchTree);
    EXPECT_TRUE(hasForestFlowers);
    EXPECT_TRUE(hasForestGrass);
}

TEST_F(VegetationFeatureTest, DesertBiomeSettings) {
    auto settings = BiomeGenerationSettings::createDesert();

    // 沙漠应该有仙人掌
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasCactus = false;
    bool hasDeadBush = false;

    for (u32 id : vegetal) {
        if (id == CactusFeatureIds::DesertCactus) hasCactus = true;
        if (id == GrassFeatureIds::BadlandsDeadBush) hasDeadBush = true;
    }

    EXPECT_TRUE(hasCactus);
    EXPECT_TRUE(hasDeadBush);
}

TEST_F(VegetationFeatureTest, SwampBiomeSettings) {
    auto settings = BiomeGenerationSettings::createSwamp();

    // 沼泽应该有甘蔗和蘑菇
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasDenseSugarCane = false;
    bool hasBrownMushroom = false;
    bool hasRedMushroom = false;
    bool hasSwampFlowers = false;

    for (u32 id : vegetal) {
        if (id == SugarCaneFeatureIds::Dense) hasDenseSugarCane = true;
        if (id == MushroomFeatureIds::BrownMushroom) hasBrownMushroom = true;
        if (id == MushroomFeatureIds::RedMushroom) hasRedMushroom = true;
        if (id == FlowerFeatureIds::SwampFlowers) hasSwampFlowers = true;
    }

    EXPECT_TRUE(hasDenseSugarCane);
    EXPECT_TRUE(hasBrownMushroom);
    EXPECT_TRUE(hasRedMushroom);
    EXPECT_TRUE(hasSwampFlowers);
}

TEST_F(VegetationFeatureTest, IceSpikesBiomeSettings) {
    auto settings = BiomeGenerationSettings::createIceSpikes();

    // 冰刺平原应该有冰刺结构
    const auto& surface = settings.getFeatures(DecorationStage::SurfaceStructures);
    bool hasSpike = false;
    bool hasIceberg = false;

    for (u32 id : surface) {
        if (id == IceSpikeFeatureIds::Spike) hasSpike = true;
        if (id == IceSpikeFeatureIds::Iceberg) hasIceberg = true;
    }

    EXPECT_TRUE(hasSpike);
    EXPECT_TRUE(hasIceberg);
}

TEST_F(VegetationFeatureTest, BadlandsBiomeSettings) {
    auto settings = BiomeGenerationSettings::createBadlands();

    // 恶地应该有恶地仙人掌
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasBadlandsCactus = false;
    bool hasDeadBush = false;

    for (u32 id : vegetal) {
        if (id == CactusFeatureIds::BadlandsCactus) hasBadlandsCactus = true;
        if (id == GrassFeatureIds::BadlandsDeadBush) hasDeadBush = true;
    }

    EXPECT_TRUE(hasBadlandsCactus);
    EXPECT_TRUE(hasDeadBush);
}

TEST_F(VegetationFeatureTest, FlowerForestBiomeSettings) {
    auto settings = BiomeGenerationSettings::createFlowerForest();

    // 繁花森林应该有繁花森林花卉
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasFlowerForestFlowers = false;
    int flowerForestFlowersCount = 0;

    for (u32 id : vegetal) {
        if (id == FlowerFeatureIds::FlowerForestFlowers) {
            hasFlowerForestFlowers = true;
            flowerForestFlowersCount++;
        }
    }

    EXPECT_TRUE(hasFlowerForestFlowers);
    // 繁花森林花卉应该添加两次以增加密度
    EXPECT_GE(flowerForestFlowersCount, 1);
}

TEST_F(VegetationFeatureTest, MountainsBiomeSettings) {
    auto settings = BiomeGenerationSettings::createMountains();

    // 山地应该有绿宝石矿石
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    bool hasEmeraldOre = false;

    for (u32 id : ores) {
        if (id == OreFeatureIds::EmeraldOre) {
            hasEmeraldOre = true;
            break;
        }
    }

    EXPECT_TRUE(hasEmeraldOre);

    // 山地应该有云杉树和针叶林草丛
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasSpruceTree = false;
    bool hasTaigaGrass = false;

    for (u32 id : vegetal) {
        if (id == TreeFeatureIds::SpruceTree) hasSpruceTree = true;
        if (id == GrassFeatureIds::TaigaGrass) hasTaigaGrass = true;
    }

    EXPECT_TRUE(hasSpruceTree);
    EXPECT_TRUE(hasTaigaGrass);
}

TEST_F(VegetationFeatureTest, OceanBiomeSettings) {
    auto settings = BiomeGenerationSettings::createOcean();

    // 海洋不应该有绿宝石
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    bool hasEmeraldOre = false;

    for (u32 id : ores) {
        if (id == OreFeatureIds::EmeraldOre) {
            hasEmeraldOre = true;
            break;
        }
    }

    EXPECT_FALSE(hasEmeraldOre);

    // 海洋不应该有植被
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    EXPECT_TRUE(vegetal.empty());
}

TEST_F(VegetationFeatureTest, TaigaBiomeSettings) {
    auto settings = BiomeGenerationSettings::createTaiga();

    // 针叶林应该有云杉树和针叶林草丛
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasSpruceTree = false;
    bool hasTaigaGrass = false;

    for (u32 id : vegetal) {
        if (id == TreeFeatureIds::SpruceTree) hasSpruceTree = true;
        if (id == GrassFeatureIds::TaigaGrass) hasTaigaGrass = true;
    }

    EXPECT_TRUE(hasSpruceTree);
    EXPECT_TRUE(hasTaigaGrass);
}

TEST_F(VegetationFeatureTest, JungleBiomeSettings) {
    auto settings = BiomeGenerationSettings::createJungle();

    // 丛林应该有丛林树和丛林草丛
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasJungleTree = false;
    bool hasJungleGrass = false;

    for (u32 id : vegetal) {
        if (id == TreeFeatureIds::JungleTree) hasJungleTree = true;
        if (id == GrassFeatureIds::JungleGrass) hasJungleGrass = true;
    }

    EXPECT_TRUE(hasJungleTree);
    EXPECT_TRUE(hasJungleGrass);
}

TEST_F(VegetationFeatureTest, SavannaBiomeSettings) {
    auto settings = BiomeGenerationSettings::createSavanna();

    // 稀树草原应该有稀疏橡树和稀树草原草丛
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    bool hasSparseOakTree = false;
    bool hasSavannaGrass = false;

    for (u32 id : vegetal) {
        if (id == TreeFeatureIds::SparseOakTree) hasSparseOakTree = true;
        if (id == GrassFeatureIds::SavannaGrass) hasSavannaGrass = true;
    }

    EXPECT_TRUE(hasSparseOakTree);
    EXPECT_TRUE(hasSavannaGrass);
}
