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
 * The above notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <gtest/gtest.h>

#include "common/resource/ResourceLocation.hpp"
#include "server/command/commands/LocateCommand.hpp"

using namespace mc;
using namespace mc::command;

// ============================================================================
// LocateCommand::_normalizeToResourceLocation 测试
// ============================================================================

TEST(LocateCommandNormalizeTest, WithNamespace)
{
    // 带命名空间的输入应直接解析
    auto result = LocateCommand::_normalizeToResourceLocation("minecraft:village_plains");
    EXPECT_EQ(result.toString(), "minecraft:village_plains");
}

TEST(LocateCommandNormalizeTest, AliasVillage)
{
    auto result = LocateCommand::_normalizeToResourceLocation("village");
    EXPECT_EQ(result.toString(), "minecraft:village_plains");
}

TEST(LocateCommandNormalizeTest, AliasMansion)
{
    auto result = LocateCommand::_normalizeToResourceLocation("mansion");
    EXPECT_EQ(result.toString(), "minecraft:mansion");
}

TEST(LocateCommandNormalizeTest, AliasWoodlandMansion)
{
    auto result = LocateCommand::_normalizeToResourceLocation("woodland_mansion");
    EXPECT_EQ(result.toString(), "minecraft:mansion");
}

TEST(LocateCommandNormalizeTest, AliasDesertTemple)
{
    auto result = LocateCommand::_normalizeToResourceLocation("desert_temple");
    EXPECT_EQ(result.toString(), "minecraft:desert_pyramid");
}

TEST(LocateCommandNormalizeTest, AliasWitchHut)
{
    auto result = LocateCommand::_normalizeToResourceLocation("witch_hut");
    EXPECT_EQ(result.toString(), "minecraft:swamp_hut");
}

TEST(LocateCommandNormalizeTest, AliasStronghold)
{
    auto result = LocateCommand::_normalizeToResourceLocation("stronghold");
    EXPECT_EQ(result.toString(), "minecraft:stronghold");
}

TEST(LocateCommandNormalizeTest, AliasNetherFortress)
{
    auto result = LocateCommand::_normalizeToResourceLocation("nether_fortress");
    EXPECT_EQ(result.toString(), "minecraft:fortress");
}

TEST(LocateCommandNormalizeTest, AliasEndCity)
{
    auto result = LocateCommand::_normalizeToResourceLocation("end_city");
    EXPECT_EQ(result.toString(), "minecraft:end_city");
}

TEST(LocateCommandNormalizeTest, AliasOutpost)
{
    auto result = LocateCommand::_normalizeToResourceLocation("outpost");
    EXPECT_EQ(result.toString(), "minecraft:pillager_outpost");
}

TEST(LocateCommandNormalizeTest, AliasOceanRuin)
{
    auto result = LocateCommand::_normalizeToResourceLocation("ocean_ruin");
    EXPECT_EQ(result.toString(), "minecraft:ocean_ruin_cold");
}

TEST(LocateCommandNormalizeTest, AliasOceanRuins)
{
    auto result = LocateCommand::_normalizeToResourceLocation("ocean_ruins");
    EXPECT_EQ(result.toString(), "minecraft:ocean_ruin_cold");
}

TEST(LocateCommandNormalizeTest, AliasShipwreck)
{
    auto result = LocateCommand::_normalizeToResourceLocation("shipwreck");
    EXPECT_EQ(result.toString(), "minecraft:shipwreck");
}

TEST(LocateCommandNormalizeTest, AliasBuriedTreasure)
{
    auto result = LocateCommand::_normalizeToResourceLocation("buried_treasure");
    EXPECT_EQ(result.toString(), "minecraft:buried_treasure");
}

TEST(LocateCommandNormalizeTest, AliasMonument)
{
    auto result = LocateCommand::_normalizeToResourceLocation("monument");
    EXPECT_EQ(result.toString(), "minecraft:monument");
}

TEST(LocateCommandNormalizeTest, UnknownName)
{
    // 未知名称应添加 minecraft: 命名空间
    auto result = LocateCommand::_normalizeToResourceLocation("unknown_structure");
    EXPECT_EQ(result.toString(), "minecraft:unknown_structure");
}

TEST(LocateCommandNormalizeTest, ExplicitNamespacePreserved)
{
    // 显式命名空间应保留
    auto result = LocateCommand::_normalizeToResourceLocation("custommod:my_structure");
    EXPECT_EQ(result.toString(), "custommod:my_structure");
}

TEST(LocateCommandNormalizeTest, AllVillageVariants)
{
    EXPECT_EQ(LocateCommand::_normalizeToResourceLocation("village_plains").toString(), "minecraft:village_plains");
    EXPECT_EQ(LocateCommand::_normalizeToResourceLocation("village_desert").toString(), "minecraft:village_desert");
    EXPECT_EQ(LocateCommand::_normalizeToResourceLocation("village_savanna").toString(), "minecraft:village_savanna");
    EXPECT_EQ(LocateCommand::_normalizeToResourceLocation("village_snowy").toString(), "minecraft:village_snowy");
    EXPECT_EQ(LocateCommand::_normalizeToResourceLocation("village_taiga").toString(), "minecraft:village_taiga");
}

TEST(LocateCommandNormalizeTest, TrialChambers)
{
    auto result = LocateCommand::_normalizeToResourceLocation("trial_chambers");
    EXPECT_EQ(result.toString(), "minecraft:trial_chambers");
}

TEST(LocateCommandNormalizeTest, AliasEndcity)
{
    auto result = LocateCommand::_normalizeToResourceLocation("endcity");
    EXPECT_EQ(result.toString(), "minecraft:end_city");
}

TEST(LocateCommandNormalizeTest, AliasBastion)
{
    auto result = LocateCommand::_normalizeToResourceLocation("bastion");
    EXPECT_EQ(result.toString(), "minecraft:bastion_remnant");
}

TEST(LocateCommandNormalizeTest, AliasFortress)
{
    auto result = LocateCommand::_normalizeToResourceLocation("fortress");
    EXPECT_EQ(result.toString(), "minecraft:fortress");
}
