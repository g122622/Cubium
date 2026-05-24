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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/structures/IglooStructure.hpp"
#include "common/world/gen/structure/structures/SwampHutStructure.hpp"
#include "common/world/gen/structure/structures/NetherFossilStructure.hpp"
#include "common/world/gen/structure/structures/PillagerOutpostStructure.hpp"
#include "common/world/gen/structure/structures/EndCityStructure.hpp"
#include "common/world/gen/structure/structures/WoodlandMansionStructure.hpp"
#include "common/world/gen/structure/structures/BastionRemnantStructure.hpp"

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::Biomes;

// ============================================================================
// 测试夹具
// ============================================================================

class NewStructuresTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// IglooStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, Igloo_NameAndSettings)
{
    IglooStructure structure;

    EXPECT_EQ(structure.name(), "Igloo");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 14357618);

    const auto& biomes = structure.validBiomes();
    EXPECT_FALSE(biomes.empty());
    // 验证雪地生物群系
    bool hasSnowyBiome = false;
    for (auto biome : biomes) {
        if (biome == SnowyPlains || biome == SnowyTaiga) {
            hasSnowyBiome = true;
            break;
        }
    }
    EXPECT_TRUE(hasSnowyBiome);
}

TEST_F(NewStructuresTest, Igloo_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::None;
    bool hasBasement = true;

    IglooPiece piece(pos, rotation, hasBasement);

    // 验证边界框
    EXPECT_LE(piece.minX(), piece.maxX());
    EXPECT_LE(piece.minY(), piece.maxY());
    EXPECT_LE(piece.minZ(), piece.maxZ());

    // 有地下室时，Y 范围应该更大
    IglooPiece pieceWithoutBasement(pos, rotation, false);
    EXPECT_GT(piece.maxY() - piece.minY(), pieceWithoutBasement.maxY() - pieceWithoutBasement.minY());
}

// ============================================================================
// SwampHutStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, SwampHut_NameAndSettings)
{
    SwampHutStructure structure;

    EXPECT_EQ(structure.name(), "Swamp_Hut");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 14357619);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 2);
    bool hasSwamp = false;
    for (auto biome : biomes) {
        if (biome == Swamp) {
            hasSwamp = true;
            break;
        }
    }
    EXPECT_TRUE(hasSwamp);
}

TEST_F(NewStructuresTest, SwampHut_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::Clockwise90;

    SwampHutPiece piece(pos, rotation);

    // 验证边界框（沼泽小屋约 7x6x9）
    EXPECT_LE(piece.maxX() - piece.minX(), 10);
    EXPECT_LE(piece.maxY() - piece.minY(), 10);
    EXPECT_LE(piece.maxZ() - piece.minZ(), 15);
}

// ============================================================================
// NetherFossilStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, NetherFossil_NameAndSettings)
{
    NetherFossilStructure structure;

    EXPECT_EQ(structure.name(), "Nether_Fossil");
    EXPECT_EQ(structure.separationSettings().spacing, 2);
    EXPECT_EQ(structure.separationSettings().separation, 1);
    EXPECT_EQ(structure.separationSettings().salt, 14357921);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 1);
    EXPECT_EQ(biomes[0], SoulSandValley);
}

TEST_F(NewStructuresTest, NetherFossil_PieceConstruction)
{
    BlockPos pos(100, 30, 200);
    i32 fossilType = 2;
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::Clockwise180;

    NetherFossilPiece piece(pos, fossilType, rotation);

    // 验证边界框（下界化石约 15x10x15）
    EXPECT_LE(piece.maxX() - piece.minX(), 20);
    EXPECT_LE(piece.maxY() - piece.minY(), 15);
    EXPECT_LE(piece.maxZ() - piece.minZ(), 20);
}

// ============================================================================
// PillagerOutpostStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, PillagerOutpost_NameAndSettings)
{
    PillagerOutpostStructure structure;

    EXPECT_EQ(structure.name(), "Pillager_Outpost");
    EXPECT_EQ(structure.separationSettings().spacing, 32);
    EXPECT_EQ(structure.separationSettings().separation, 8);
    EXPECT_EQ(structure.separationSettings().salt, 165745296);

    const auto& biomes = structure.validBiomes();
    EXPECT_FALSE(biomes.empty());
    // 验证包含平原等生物群系
    bool hasPlains = false;
    for (auto biome : biomes) {
        if (biome == Plains) {
            hasPlains = true;
            break;
        }
    }
    EXPECT_TRUE(hasPlains);
}

// ============================================================================
// EndCityStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, EndCity_NameAndSettings)
{
    EndCityStructure structure;

    EXPECT_EQ(structure.name(), "End_City");
    EXPECT_EQ(structure.separationSettings().spacing, 20);
    EXPECT_EQ(structure.separationSettings().separation, 11);
    EXPECT_EQ(structure.separationSettings().salt, 10387313);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 2);
    bool hasEndMidlands = false;
    for (auto biome : biomes) {
        if (biome == EndMidlands) {
            hasEndMidlands = true;
            break;
        }
    }
    EXPECT_TRUE(hasEndMidlands);
}

TEST_F(NewStructuresTest, EndCity_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::CounterClockwise90;
    std::string templateName = "base_floor";

    EndCityPiece piece(pos, rotation, templateName);

    // 验证边界框
    EXPECT_LE(piece.maxX() - piece.minX(), 20);
    EXPECT_LE(piece.maxY() - piece.minY(), 25);
    EXPECT_LE(piece.maxZ() - piece.minZ(), 20);
}

// ============================================================================
// WoodlandMansionStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, WoodlandMansion_NameAndSettings)
{
    WoodlandMansionStructure structure;

    EXPECT_EQ(structure.name(), "Woodland_Mansion");
    EXPECT_EQ(structure.separationSettings().spacing, 80);
    EXPECT_EQ(structure.separationSettings().separation, 20);
    EXPECT_EQ(structure.separationSettings().salt, 10387319);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 2);
    bool hasDarkForest = false;
    for (auto biome : biomes) {
        if (biome == DarkForest) {
            hasDarkForest = true;
            break;
        }
    }
    EXPECT_TRUE(hasDarkForest);
}

TEST_F(NewStructuresTest, WoodlandMansion_PieceConstruction)
{
    BlockPos pos(100, 64, 200);
    mc::world::gen::feature::template_::Rotation rotation = mc::world::gen::feature::template_::Rotation::None;

    WoodlandMansionPiece piece(pos, rotation);

    // 验证边界框（林地府邸约 58x20x58）
    EXPECT_LE(piece.maxX() - piece.minX(), 65);
    EXPECT_LE(piece.maxY() - piece.minY(), 25);
    EXPECT_LE(piece.maxZ() - piece.minZ(), 65);
}

// ============================================================================
// BastionRemnantStructure 测试
// ============================================================================

TEST_F(NewStructuresTest, BastionRemnant_NameAndSettings)
{
    BastionRemnantStructure structure;

    EXPECT_EQ(structure.name(), "Bastion_Remnant");
    EXPECT_EQ(structure.separationSettings().spacing, 27);
    EXPECT_EQ(structure.separationSettings().separation, 4);
    EXPECT_EQ(structure.separationSettings().salt, 30084232);

    const auto& biomes = structure.validBiomes();
    EXPECT_EQ(biomes.size(), 4);
    // 验证不包含玄武岩三角洲
    for (auto biome : biomes) {
        EXPECT_NE(biome, BasaltDeltas);
    }
    // 验证包含下界荒地
    bool hasNetherWastes = false;
    for (auto biome : biomes) {
        if (biome == NetherWastes) {
            hasNetherWastes = true;
            break;
        }
    }
    EXPECT_TRUE(hasNetherWastes);
}

// ============================================================================
// 边界和边界测试
// ============================================================================

TEST_F(NewStructuresTest, AllStructures_HaveValidSeparationSettings)
{
    IglooStructure igloo;
    SwampHutStructure swampHut;
    NetherFossilStructure netherFossil;
    PillagerOutpostStructure outpost;
    EndCityStructure endCity;
    WoodlandMansionStructure mansion;
    BastionRemnantStructure bastion;

    // 分离设置应该合理
    auto validateSettings = [](const StructureSeparationSettings& settings) {
        EXPECT_GT(settings.spacing, 0);
        EXPECT_GT(settings.separation, 0);
        EXPECT_GE(settings.spacing, settings.separation);
        EXPECT_GT(settings.salt, 0);
    };

    validateSettings(igloo.separationSettings());
    validateSettings(swampHut.separationSettings());
    validateSettings(netherFossil.separationSettings());
    validateSettings(outpost.separationSettings());
    validateSettings(endCity.separationSettings());
    validateSettings(mansion.separationSettings());
    validateSettings(bastion.separationSettings());
}

TEST_F(NewStructuresTest, AllStructures_HaveValidBiomes)
{
    IglooStructure igloo;
    SwampHutStructure swampHut;
    NetherFossilStructure netherFossil;
    PillagerOutpostStructure outpost;
    EndCityStructure endCity;
    WoodlandMansionStructure mansion;
    BastionRemnantStructure bastion;

    // 所有结构应该有有效的生物群系列表
    EXPECT_FALSE(igloo.validBiomes().empty());
    EXPECT_FALSE(swampHut.validBiomes().empty());
    EXPECT_FALSE(netherFossil.validBiomes().empty());
    EXPECT_FALSE(outpost.validBiomes().empty());
    EXPECT_FALSE(endCity.validBiomes().empty());
    EXPECT_FALSE(mansion.validBiomes().empty());
    EXPECT_FALSE(bastion.validBiomes().empty());
}

TEST_F(NewStructuresTest, AllStructures_HaveValidNames)
{
    IglooStructure igloo;
    SwampHutStructure swampHut;
    NetherFossilStructure netherFossil;
    PillagerOutpostStructure outpost;
    EndCityStructure endCity;
    WoodlandMansionStructure mansion;
    BastionRemnantStructure bastion;

    // 名称应该非空
    EXPECT_FALSE(igloo.name().empty());
    EXPECT_FALSE(swampHut.name().empty());
    EXPECT_FALSE(netherFossil.name().empty());
    EXPECT_FALSE(outpost.name().empty());
    EXPECT_FALSE(endCity.name().empty());
    EXPECT_FALSE(mansion.name().empty());
    EXPECT_FALSE(bastion.name().empty());
}
