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

#include <gtest/gtest.h>

#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/structure/structures/ShipwreckStructure.hpp"

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::world::gen::feature::template_;

// ============================================================================
// 测试夹具
// ============================================================================

class ShipwreckStructureTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// ShipwreckConfig 测试
// ============================================================================

TEST_F(ShipwreckStructureTest, Config_DefaultValues)
{
    ShipwreckConfig config;

    EXPECT_FALSE(config.isBeached);
}

TEST_F(ShipwreckStructureTest, Config_BeachedFlag)
{
    ShipwreckConfig config;
    config.isBeached = true;

    EXPECT_TRUE(config.isBeached);
}

// ============================================================================
// ShipwreckPiece 测试
// ============================================================================

TEST_F(ShipwreckStructureTest, Piece_Construction)
{
    ShipwreckPiece piece("shipwreck/with_mast", BlockPos(100, 64, 200), Rotation::None, false);

    EXPECT_EQ(piece.templateName(), "shipwreck/with_mast");
    EXPECT_FALSE(piece.isBeached());
}

TEST_F(ShipwreckStructureTest, Piece_Beached)
{
    ShipwreckPiece piece("shipwreck/rightsideup_full", BlockPos(0, 0, 0), Rotation::Clockwise90, true);

    EXPECT_TRUE(piece.isBeached());
    EXPECT_EQ(piece.templateName(), "shipwreck/rightsideup_full");
}

TEST_F(ShipwreckStructureTest, Piece_StructureOffset)
{
    // MC 1.16.5: BlockPos(4, 0, 15)
    EXPECT_EQ(ShipwreckPiece::STRUCTURE_OFFSET.x, 4);
    EXPECT_EQ(ShipwreckPiece::STRUCTURE_OFFSET.y, 0);
    EXPECT_EQ(ShipwreckPiece::STRUCTURE_OFFSET.z, 15);
}

TEST_F(ShipwreckStructureTest, Piece_RotationVariants)
{
    // 测试所有旋转变体
    ShipwreckPiece piece0("test", BlockPos(0, 0, 0), Rotation::None, false);
    ShipwreckPiece piece90("test", BlockPos(0, 0, 0), Rotation::Clockwise90, false);
    ShipwreckPiece piece180("test", BlockPos(0, 0, 0), Rotation::Clockwise180, false);
    ShipwreckPiece piece270("test", BlockPos(0, 0, 0), Rotation::CounterClockwise90, false);

    // 验证构造成功
    EXPECT_EQ(piece0.templateName(), "test");
    EXPECT_EQ(piece90.templateName(), "test");
    EXPECT_EQ(piece180.templateName(), "test");
    EXPECT_EQ(piece270.templateName(), "test");
}

// ============================================================================
// ShipwreckStructure 测试
// ============================================================================

TEST_F(ShipwreckStructureTest, Structure_BasicProperties)
{
    ShipwreckStructure structure(ResourceLocation("minecraft", "shipwreck"));

    EXPECT_EQ(structure.name(), "shipwreck");
    ASSERT_NE(structure.biomeTag(), nullptr);
}

TEST_F(ShipwreckStructureTest, Structure_ValidBiomeTag)
{
    ShipwreckStructure structure(ResourceLocation("minecraft", "shipwreck"));
    const auto* tag = structure.biomeTag();
    ASSERT_NE(tag, nullptr);

    // 普通沉船 tag（has_structure/shipwreck）只含海洋生物群系；
    // Beach/SnowyBeach 属于搁浅变体 tag（has_structure/shipwreck_beached），不在普通沉船 tag 中。
    EXPECT_TRUE(tag->contains(Biomes::Ocean));
    EXPECT_TRUE(tag->contains(Biomes::DeepOcean));
    EXPECT_FALSE(tag->contains(Biomes::Beach));
    EXPECT_FALSE(tag->contains(Biomes::SnowyBeach));
}

TEST_F(ShipwreckStructureTest, Structure_TemplateNames)
{
    // 验证模板名称已定义
    // 搁浅沉船模板（11 种）
    EXPECT_EQ(ShipwreckStructure::s_beachedTemplates.size(), 11u);

    // 所有沉船模板（20 种）
    EXPECT_EQ(ShipwreckStructure::s_allTemplates.size(), 20u);
}

TEST_F(ShipwreckStructureTest, Structure_BeachedTemplateList)
{
    // MC 1.16.5: 搁浅沉船变体
    const auto& templates = ShipwreckStructure::s_beachedTemplates;

    // 验证包含关键的搁浅模板
    bool hasWithMast = false;
    bool hasSidewaysFull = false;
    bool hasRightsideupFull = false;
    bool hasDegraded = false;

    for (const auto& name : templates) {
        if (name.find("with_mast") != std::string::npos && name.find("degraded") == std::string::npos) {
            hasWithMast = true;
        }
        if (name.find("sideways_full") != std::string::npos) {
            hasSidewaysFull = true;
        }
        if (name.find("rightsideup_full") != std::string::npos && name.find("degraded") == std::string::npos) {
            hasRightsideupFull = true;
        }
        if (name.find("degraded") != std::string::npos) {
            hasDegraded = true;
        }
    }

    EXPECT_TRUE(hasWithMast) << "Missing with_mast template";
    EXPECT_TRUE(hasSidewaysFull) << "Missing sideways_full template";
    EXPECT_TRUE(hasRightsideupFull) << "Missing rightsideup_full template";
    EXPECT_TRUE(hasDegraded) << "Missing degraded template";
}

TEST_F(ShipwreckStructureTest, Structure_AllTemplateList)
{
    // MC 1.16.5: 所有沉船变体（包括水下和搁浅）
    const auto& templates = ShipwreckStructure::s_allTemplates;

    // 验证包含关键的模板类型
    bool hasUpsidedown = false;
    bool hasSideways = false;
    bool hasRightsideup = false;
    bool hasWithMast = false;

    for (const auto& name : templates) {
        if (name.find("upsidedown") != std::string::npos) hasUpsidedown = true;
        if (name.find("sideways") != std::string::npos) hasSideways = true;
        if (name.find("rightsideup") != std::string::npos) hasRightsideup = true;
        if (name.find("with_mast") != std::string::npos) hasWithMast = true;
    }

    EXPECT_TRUE(hasUpsidedown) << "Missing upsidedown template";
    EXPECT_TRUE(hasSideways) << "Missing sideways template";
    EXPECT_TRUE(hasRightsideup) << "Missing rightsideup template";
    EXPECT_TRUE(hasWithMast) << "Missing with_mast template";
}

TEST_F(ShipwreckStructureTest, Structure_TemplateNameFormats)
{
    // 验证模板名称格式符合 MC 1.16.5
    for (const auto& name : ShipwreckStructure::s_beachedTemplates) {
        EXPECT_TRUE(name.find("shipwreck/") != std::string::npos) << "Invalid beached template name: " << name;
    }

    for (const auto& name : ShipwreckStructure::s_allTemplates) {
        EXPECT_TRUE(name.find("shipwreck/") != std::string::npos) << "Invalid template name: " << name;
    }
}

TEST_F(ShipwreckStructureTest, Structure_BeachedTemplatesSubsetOfAll)
{
    // 验证搁浅模板是所有模板的子集
    const auto& beached = ShipwreckStructure::s_beachedTemplates;
    const auto& all = ShipwreckStructure::s_allTemplates;

    for (const auto& beachedName : beached) {
        bool found = false;
        for (const auto& allName : all) {
            if (beachedName == allName) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Beached template not in all templates: " << beachedName;
    }
}

// ============================================================================
// 旋转测试
// ============================================================================

TEST_F(ShipwreckStructureTest, Rotation_AllVariants)
{
    // 验证所有旋转值可以正常使用
    math::Random rng(12345);

    for (int i = 0; i < 100; ++i) {
        i32 rotationValue = rng.nextInt(4) * 90; // 0, 90, 180, 270
        Rotation rotation = static_cast<Rotation>(rotationValue);

        ShipwreckPiece piece("test_template", BlockPos(0, 0, 0), rotation, false);

        // 验证构造成功
        EXPECT_EQ(piece.templateName(), "test_template");
    }
}

// ============================================================================
// 模板变体类型测试
// ============================================================================

TEST_F(ShipwreckStructureTest, TemplateVariants_Full)
{
    const auto& all = ShipwreckStructure::s_allTemplates;

    // 验证完整变体
    bool hasFull = false;
    for (const auto& name : all) {
        if (name.find("_full") != std::string::npos && name.find("fronthalf") == std::string::npos &&
            name.find("backhalf") == std::string::npos) {
            hasFull = true;
            break;
        }
    }
    EXPECT_TRUE(hasFull) << "Missing full template variant";
}

TEST_F(ShipwreckStructureTest, TemplateVariants_Half)
{
    const auto& all = ShipwreckStructure::s_allTemplates;

    // 验证半截变体
    bool hasFrontHalf = false;
    bool hasBackHalf = false;

    for (const auto& name : all) {
        if (name.find("fronthalf") != std::string::npos) hasFrontHalf = true;
        if (name.find("backhalf") != std::string::npos) hasBackHalf = true;
    }

    EXPECT_TRUE(hasFrontHalf) << "Missing fronthalf template variant";
    EXPECT_TRUE(hasBackHalf) << "Missing backhalf template variant";
}

TEST_F(ShipwreckStructureTest, TemplateVariants_Degraded)
{
    const auto& all = ShipwreckStructure::s_allTemplates;

    // 验证破损变体
    int degradedCount = 0;
    for (const auto& name : all) {
        if (name.find("degraded") != std::string::npos) {
            ++degradedCount;
        }
    }

    // MC 1.16.5 有多个破损变体
    EXPECT_GT(degradedCount, 0) << "Missing degraded template variants";
}

// ============================================================================
// 配置测试
// ============================================================================

TEST_F(ShipwreckStructureTest, Config_SetAndGet)
{
    ShipwreckStructure structure(ResourceLocation("minecraft", "shipwreck"));

    ShipwreckConfig config;
    config.isBeached = true;
    structure.setConfig(config);

    const auto& retrievedConfig = structure.config();
    EXPECT_TRUE(retrievedConfig.isBeached);
}
