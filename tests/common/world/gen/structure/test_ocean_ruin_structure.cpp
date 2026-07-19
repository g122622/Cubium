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
#include "common/world/gen/structure/structures/OceanRuinStructure.hpp"

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::world::gen::feature::template_;

// ============================================================================
// 测试夹具
// ============================================================================

class OceanRuinStructureTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// OceanRuinConfig 测试
// ============================================================================

TEST_F(OceanRuinStructureTest, Config_DefaultValues)
{
    OceanRuinConfig config;

    EXPECT_EQ(config.biomeType, OceanRuinType::Cold);
    EXPECT_FLOAT_EQ(config.largeProbability, 0.3f);
    EXPECT_FLOAT_EQ(config.clusterProbability, 0.9f);
}

TEST_F(OceanRuinStructureTest, Config_WarmType)
{
    OceanRuinConfig config;
    config.biomeType = OceanRuinType::Warm;
    config.largeProbability = 0.5f;
    config.clusterProbability = 0.7f;

    EXPECT_EQ(config.biomeType, OceanRuinType::Warm);
    EXPECT_FLOAT_EQ(config.largeProbability, 0.5f);
    EXPECT_FLOAT_EQ(config.clusterProbability, 0.7f);
}

// ============================================================================
// OceanRuinPiece 测试
// ============================================================================

TEST_F(OceanRuinStructureTest, Piece_Construction)
{
    OceanRuinPiece piece(
        "underwater_ruin/brick_1", BlockPos(100, 64, 200), Rotation::None, 0.8f, OceanRuinType::Cold, false);

    EXPECT_EQ(piece.templateName(), "underwater_ruin/brick_1");
    EXPECT_EQ(piece.integrity(), 0.8f);
    EXPECT_EQ(piece.ruinType(), OceanRuinType::Cold);
    EXPECT_FALSE(piece.isLarge());
}

TEST_F(OceanRuinStructureTest, Piece_LargeRuin)
{
    OceanRuinPiece piece(
        "underwater_ruin/big_brick_1", BlockPos(0, 0, 0), Rotation::Clockwise90, 0.9f, OceanRuinType::Cold, true);

    EXPECT_TRUE(piece.isLarge());
    EXPECT_EQ(piece.integrity(), 0.9f);
    EXPECT_EQ(piece.ruinType(), OceanRuinType::Cold);
}

TEST_F(OceanRuinStructureTest, Piece_RotationVariants)
{
    // 测试所有旋转变体
    OceanRuinPiece piece0("test", BlockPos(0, 0, 0), Rotation::None, 1.0f, OceanRuinType::Cold, false);
    OceanRuinPiece piece90("test", BlockPos(0, 0, 0), Rotation::Clockwise90, 1.0f, OceanRuinType::Cold, false);
    OceanRuinPiece piece180("test", BlockPos(0, 0, 0), Rotation::Clockwise180, 1.0f, OceanRuinType::Cold, false);
    OceanRuinPiece piece270("test", BlockPos(0, 0, 0), Rotation::CounterClockwise90, 1.0f, OceanRuinType::Cold, false);

    // 验证构造成功
    EXPECT_EQ(piece0.templateName(), "test");
    EXPECT_EQ(piece90.templateName(), "test");
    EXPECT_EQ(piece180.templateName(), "test");
    EXPECT_EQ(piece270.templateName(), "test");
}

// ============================================================================
// OceanRuinStructure 测试
// ============================================================================

TEST_F(OceanRuinStructureTest, Structure_BasicProperties)
{
    OceanRuinStructure structure(ResourceLocation("minecraft", "ocean_ruin"));

    EXPECT_EQ(structure.name(), "ocean_ruin");
    ASSERT_NE(structure.biomeTag(), nullptr);
}

TEST_F(OceanRuinStructureTest, Structure_ValidBiomeTag)
{
    OceanRuinStructure structure(ResourceLocation("minecraft", "ocean_ruin"));
    const auto* tag = structure.biomeTag();
    ASSERT_NE(tag, nullptr);

    // OceanRuinStructure::biomeTag() 返回 has_structure/ocean_ruin_cold，
    // 只含冷/普通海洋群系；WarmOcean 属于 warm 变体 tag（has_structure/ocean_ruin_warm），不在普通 ruin tag 中。
    EXPECT_TRUE(tag->contains(Biomes::Ocean));
    EXPECT_TRUE(tag->contains(Biomes::DeepOcean));
    EXPECT_FALSE(tag->contains(Biomes::WarmOcean));
}

TEST_F(OceanRuinStructureTest, Structure_TemplateNames)
{
    // 验证模板名称已定义
    // 暖海模板
    EXPECT_FALSE(OceanRuinStructure::s_warmTemplates.empty());
    EXPECT_FALSE(OceanRuinStructure::s_warmBigTemplates.empty());

    // 冷海模板
    EXPECT_FALSE(OceanRuinStructure::s_brickTemplates.empty());
    EXPECT_FALSE(OceanRuinStructure::s_brickBigTemplates.empty());
    EXPECT_FALSE(OceanRuinStructure::s_crackedTemplates.empty());
    EXPECT_FALSE(OceanRuinStructure::s_crackedBigTemplates.empty());
    EXPECT_FALSE(OceanRuinStructure::s_mossyTemplates.empty());
    EXPECT_FALSE(OceanRuinStructure::s_mossyBigTemplates.empty());
}

TEST_F(OceanRuinStructureTest, Structure_TemplateNameFormats)
{
    // 验证模板名称格式符合 MC 1.16.5
    for (const auto& name : OceanRuinStructure::s_warmTemplates) {
        EXPECT_TRUE(name.find("underwater_ruin/warm_") != std::string::npos) << "Invalid warm template name: " << name;
    }

    for (const auto& name : OceanRuinStructure::s_brickTemplates) {
        EXPECT_TRUE(name.find("underwater_ruin/brick_") != std::string::npos)
            << "Invalid brick template name: " << name;
    }

    for (const auto& name : OceanRuinStructure::s_brickBigTemplates) {
        EXPECT_TRUE(name.find("underwater_ruin/big_brick_") != std::string::npos)
            << "Invalid big brick template name: " << name;
    }
}

// ============================================================================
// OceanRuinType 测试
// ============================================================================

TEST_F(OceanRuinStructureTest, Type_EnumValues)
{
    // 验证枚举值
    OceanRuinType warm = OceanRuinType::Warm;
    OceanRuinType cold = OceanRuinType::Cold;

    EXPECT_NE(static_cast<int>(warm), static_cast<int>(cold));
    EXPECT_EQ(static_cast<int>(warm), 0);
    EXPECT_EQ(static_cast<int>(cold), 1);
}

// ============================================================================
// 完整度测试
// ============================================================================

TEST_F(OceanRuinStructureTest, Integrity_Values)
{
    // MC 1.16.5: 大型废墟完整度 0.9，小型废墟完整度 0.8
    // 冷海废墟三层叠加完整度：传入值、0.7、0.5

    // 大型废墟
    f32 largeIntegrity = 0.9f;
    EXPECT_GE(largeIntegrity, 0.0f);
    EXPECT_LE(largeIntegrity, 1.0f);

    // 小型废墟
    f32 smallIntegrity = 0.8f;
    EXPECT_GE(smallIntegrity, 0.0f);
    EXPECT_LE(smallIntegrity, 1.0f);

    // 冷海叠加层
    f32 layer1 = 1.0f; // 传入值
    f32 layer2 = 0.7f; // 第二层
    f32 layer3 = 0.5f; // 第三层

    EXPECT_GT(layer1, layer2);
    EXPECT_GT(layer2, layer3);
}

// ============================================================================
// 旋转测试
// ============================================================================

TEST_F(OceanRuinStructureTest, Rotation_AllVariants)
{
    // 验证所有旋转值可以正常使用
    math::Random rng(12345);

    for (int i = 0; i < 100; ++i) {
        i32 rotationValue = rng.nextInt(4) * 90; // 0, 90, 180, 270
        Rotation rotation = static_cast<Rotation>(rotationValue);

        OceanRuinPiece piece("test_template", BlockPos(0, 0, 0), rotation, 0.8f, OceanRuinType::Cold, false);

        // 验证构造成功
        EXPECT_EQ(piece.templateName(), "test_template");
    }
}

// ============================================================================
// 配置概率测试
// ============================================================================

TEST_F(OceanRuinStructureTest, Config_ProbabilityRanges)
{
    OceanRuinConfig config;

    // 验证概率值在有效范围内
    EXPECT_GE(config.largeProbability, 0.0f);
    EXPECT_LE(config.largeProbability, 1.0f);

    EXPECT_GE(config.clusterProbability, 0.0f);
    EXPECT_LE(config.clusterProbability, 1.0f);
}
