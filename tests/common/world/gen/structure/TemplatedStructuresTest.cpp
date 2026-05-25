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

#include "world/gen/structure/structures/IglooStructure.hpp"
#include "world/gen/structure/structures/NetherFossilStructure.hpp"
#include "world/gen/structure/structures/RuinedPortalStructure.hpp"
#include "world/gen/structure/structures/ShipwreckStructure.hpp"
#include "world/gen/structure/structures/OceanRuinStructure.hpp"
#include "world/biome/Biome.hpp"
#include <set>

using namespace mc::world::gen::structure;
using namespace mc::world::gen::feature::template_;

namespace {

// 测试模板名称列表的完整性

TEST(TemplatedStructuresTest, NetherFossilHasCorrectTemplateCount)
{
    // MC 1.16.5: 下界化石有14个模板
    EXPECT_EQ(NetherFossilStructure::s_fossilTemplates.size(), 14u);

    // 验证模板名称格式
    for (const auto& name : NetherFossilStructure::s_fossilTemplates) {
        EXPECT_TRUE(name.find("nether_fossils/fossil_") != std::string::npos)
            << "Invalid template name: " << name;
    }
}

TEST(TemplatedStructuresTest, ShipwreckHasCorrectTemplateCount)
{
    // MC 1.16.5: 搁浅沉船有11个模板
    EXPECT_EQ(ShipwreckStructure::s_beachedTemplates.size(), 11u);

    // MC 1.16.5: 所有沉船有20个模板
    EXPECT_EQ(ShipwreckStructure::s_allTemplates.size(), 20u);

    // 验证模板名称格式
    for (const auto& name : ShipwreckStructure::s_beachedTemplates) {
        EXPECT_TRUE(name.find("shipwreck/") != std::string::npos)
            << "Invalid template name: " << name;
    }
}

TEST(TemplatedStructuresTest, OceanRuinHasCorrectTemplateCount)
{
    // MC 1.16.5: 暖海废墟
    EXPECT_EQ(OceanRuinStructure::s_warmTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_warmBigTemplates.size(), 4u);

    // MC 1.16.5: 冷海废墟（砖、裂纹、苔藓）
    EXPECT_EQ(OceanRuinStructure::s_brickTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_brickBigTemplates.size(), 4u);
    EXPECT_EQ(OceanRuinStructure::s_crackedTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_crackedBigTemplates.size(), 4u);
    EXPECT_EQ(OceanRuinStructure::s_mossyTemplates.size(), 8u);
    EXPECT_EQ(OceanRuinStructure::s_mossyBigTemplates.size(), 4u);
}

TEST(TemplatedStructuresTest, IglooHasCorrectTemplateNames)
{
    // MC 1.16.5: 雪屋有3个模板
    EXPECT_EQ(IglooStructure::s_topTemplateName, "igloo/top");
    EXPECT_EQ(IglooStructure::s_middleTemplateName, "igloo/middle");
    EXPECT_EQ(IglooStructure::s_bottomTemplateName, "igloo/bottom");
}

TEST(TemplatedStructuresTest, RuinedPortalHasCorrectTemplateCount)
{
    // MC 1.16.5: 普通传送门10个，巨型传送门3个
    EXPECT_EQ(RuinedPortalStructure::getNormalTemplates().size(), 10u);
    EXPECT_EQ(RuinedPortalStructure::getGiantTemplates().size(), 3u);

    // 验证模板名称格式
    for (const auto& name : RuinedPortalStructure::getNormalTemplates()) {
        EXPECT_TRUE(name.find("ruined_portal/portal_") != std::string::npos)
            << "Invalid template name: " << name;
    }
    for (const auto& name : RuinedPortalStructure::getGiantTemplates()) {
        EXPECT_TRUE(name.find("ruined_portal/giant_portal_") != std::string::npos)
            << "Invalid template name: " << name;
    }
}

TEST(TemplatedStructuresTest, RuinedPortalTypeDetectionWorks)
{
    using namespace mc::Biomes;

    // 沙漠类型
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Desert), RuinedPortalType::Desert);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(DesertHills), RuinedPortalType::Desert);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(DesertLakes), RuinedPortalType::Desert);

    // 丛林类型
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Jungle), RuinedPortalType::Jungle);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(JungleHills), RuinedPortalType::Jungle);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(JungleEdge), RuinedPortalType::Jungle);

    // 沼泽类型
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Swamp), RuinedPortalType::Swamp);

    // 山地类型
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Mountains), RuinedPortalType::Mountain);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(WoodedMountains), RuinedPortalType::Mountain);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(SnowyMountains), RuinedPortalType::Mountain);

    // 海洋类型
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Ocean), RuinedPortalType::Ocean);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(DeepOcean), RuinedPortalType::Ocean);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(WarmOcean), RuinedPortalType::Ocean);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(FrozenOcean), RuinedPortalType::Ocean);

    // 标准类型（默认）
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Plains), RuinedPortalType::Standard);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Forest), RuinedPortalType::Standard);
    EXPECT_EQ(RuinedPortalStructure::getPortalType(Taiga), RuinedPortalType::Standard);
}

// 测试结构片段属性

TEST(TemplatedStructuresTest, IglooPieceHasCorrectDefaults)
{
    IglooPiece piece(mc::BlockPos(0, 64, 0), Rotation::None, false, 0);

    EXPECT_FALSE(piece.hasBasement());
    EXPECT_EQ(piece.middleCount(), 0);
}

TEST(TemplatedStructuresTest, IglooPieceBasementGeneration)
{
    // 有地下室时，中间层数为1-2
    IglooPiece pieceWithBasement(mc::BlockPos(0, 64, 0),
        Rotation::None, true, 2);

    EXPECT_TRUE(pieceWithBasement.hasBasement());
    EXPECT_EQ(pieceWithBasement.middleCount(), 2);
}

TEST(TemplatedStructuresTest, OceanRuinConfigDefaults)
{
    OceanRuinConfig config;
    EXPECT_FLOAT_EQ(config.largeProbability, 0.3f);
    EXPECT_FLOAT_EQ(config.clusterProbability, 0.9f);
    EXPECT_EQ(config.biomeType, OceanRuinType::Cold);
}

TEST(TemplatedStructuresTest, ShipwreckPieceProperties)
{
    ShipwreckPiece piece("shipwreck/with_mast",
        mc::BlockPos(100, 50, 200),
        Rotation::Clockwise90,
        true);

    EXPECT_EQ(piece.templateName(), "shipwreck/with_mast");
    EXPECT_TRUE(piece.isBeached());
}

TEST(TemplatedStructuresTest, NetherFossilPieceProperties)
{
    NetherFossilPiece piece("nether_fossils/fossil_5",
        mc::BlockPos(0, 45, 0),
        Rotation::Clockwise180);

    EXPECT_EQ(piece.templateName(), "nether_fossils/fossil_5");
}

TEST(TemplatedStructuresTest, RuinedPortalPieceProperties)
{
    RuinedPortalProperties props;
    props.cold = true;
    props.mossiness = 0.8f;
    props.airPocket = true;
    props.vines = true;

    RuinedPortalPiece piece("ruined_portal/portal_1",
        mc::BlockPos(0, 64, 0),
        Rotation::None,
        Mirror::None,
        RuinedPortalLocation::OnLandSurface,
        props);

    EXPECT_EQ(piece.templateName(), "ruined_portal/portal_1");
    EXPECT_EQ(piece.location(), RuinedPortalLocation::OnLandSurface);
    EXPECT_TRUE(piece.properties().cold);
    EXPECT_FLOAT_EQ(piece.properties().mossiness, 0.8f);
    EXPECT_TRUE(piece.properties().airPocket);
    EXPECT_TRUE(piece.properties().vines);
}

TEST(TemplatedStructuresTest, OceanRuinPieceProperties)
{
    OceanRuinPiece piece("underwater_ruin/brick_1",
        mc::BlockPos(0, 30, 0),
        Rotation::Clockwise90,
        0.8f,
        OceanRuinType::Cold,
        false);

    EXPECT_EQ(piece.templateName(), "underwater_ruin/brick_1");
    EXPECT_FLOAT_EQ(piece.integrity(), 0.8f);
    EXPECT_EQ(piece.ruinType(), OceanRuinType::Cold);
    EXPECT_FALSE(piece.isLarge());
}

} // namespace
