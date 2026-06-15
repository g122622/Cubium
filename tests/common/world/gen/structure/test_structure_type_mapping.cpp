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

#include "common/world/gen/structure/Structure.hpp"

using namespace mc;
using namespace mc::world::gen::structure;

// ============================================================================
// Structure::typeToId 测试
// ============================================================================

TEST(StructureTypeMappingTest, TypeToId_AllTypesReturnValidResourceLocation)
{
    // 验证所有 StructureType 枚举值都能正确转换为 ResourceLocation
    auto temple = Structure::typeToId(StructureType::Temple);
    EXPECT_EQ(temple.toString(), "minecraft:temple");

    auto monument = Structure::typeToId(StructureType::Monument);
    EXPECT_EQ(monument.toString(), "minecraft:ocean_monument");

    auto stronghold = Structure::typeToId(StructureType::Stronghold);
    EXPECT_EQ(stronghold.toString(), "minecraft:stronghold");

    auto village = Structure::typeToId(StructureType::Village);
    EXPECT_EQ(village.toString(), "minecraft:village");

    auto mineshaft = Structure::typeToId(StructureType::Mineshaft);
    EXPECT_EQ(mineshaft.toString(), "minecraft:mineshaft");

    auto ruinedPortal = Structure::typeToId(StructureType::RuinedPortal);
    EXPECT_EQ(ruinedPortal.toString(), "minecraft:ruined_portal");

    auto buriedTreasure = Structure::typeToId(StructureType::BuriedTreasure);
    EXPECT_EQ(buriedTreasure.toString(), "minecraft:buried_treasure");

    auto shipwreck = Structure::typeToId(StructureType::Shipwreck);
    EXPECT_EQ(shipwreck.toString(), "minecraft:shipwreck");

    auto oceanRuin = Structure::typeToId(StructureType::OceanRuin);
    EXPECT_EQ(oceanRuin.toString(), "minecraft:ocean_ruin");

    auto mansion = Structure::typeToId(StructureType::WoodlandMansion);
    EXPECT_EQ(mansion.toString(), "minecraft:woodland_mansion");

    auto bastion = Structure::typeToId(StructureType::Bastion);
    EXPECT_EQ(bastion.toString(), "minecraft:bastion_remnant");

    auto fortress = Structure::typeToId(StructureType::Fortress);
    EXPECT_EQ(fortress.toString(), "minecraft:fortress");

    auto endCity = Structure::typeToId(StructureType::EndCity);
    EXPECT_EQ(endCity.toString(), "minecraft:end_city");

    auto outpost = Structure::typeToId(StructureType::PillagerOutpost);
    EXPECT_EQ(outpost.toString(), "minecraft:pillager_outpost");

    auto trial = Structure::typeToId(StructureType::TrialChambers);
    EXPECT_EQ(trial.toString(), "minecraft:trial_chambers");
}

// ============================================================================
// Structure::nameToStructureType 测试 — 标准名称
// ============================================================================

TEST(StructureTypeMappingTest, NameToType_StandardNames)
{
    // 验证标准结构名称（无 minecraft: 前缀）能正确转换
    EXPECT_EQ(Structure::nameToStructureType("village").value(), StructureType::Village);
    EXPECT_EQ(Structure::nameToStructureType("stronghold").value(), StructureType::Stronghold);
    EXPECT_EQ(Structure::nameToStructureType("mineshaft").value(), StructureType::Mineshaft);
    EXPECT_EQ(Structure::nameToStructureType("ocean_monument").value(), StructureType::Monument);
    EXPECT_EQ(Structure::nameToStructureType("buried_treasure").value(), StructureType::BuriedTreasure);
    EXPECT_EQ(Structure::nameToStructureType("shipwreck").value(), StructureType::Shipwreck);
    EXPECT_EQ(Structure::nameToStructureType("ocean_ruin").value(), StructureType::OceanRuin);
    EXPECT_EQ(Structure::nameToStructureType("woodland_mansion").value(), StructureType::WoodlandMansion);
    EXPECT_EQ(Structure::nameToStructureType("bastion_remnant").value(), StructureType::Bastion);
    EXPECT_EQ(Structure::nameToStructureType("fortress").value(), StructureType::Fortress);
    EXPECT_EQ(Structure::nameToStructureType("end_city").value(), StructureType::EndCity);
    EXPECT_EQ(Structure::nameToStructureType("pillager_outpost").value(), StructureType::PillagerOutpost);
    EXPECT_EQ(Structure::nameToStructureType("trial_chambers").value(), StructureType::TrialChambers);
    EXPECT_EQ(Structure::nameToStructureType("temple").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("ruined_portal").value(), StructureType::RuinedPortal);
}

// ============================================================================
// Structure::nameToStructureType 测试 — 带前缀名称
// ============================================================================

TEST(StructureTypeMappingTest, NameToType_WithMinecraftPrefix)
{
    // 验证带 minecraft: 前缀的名称能正确转换
    EXPECT_EQ(Structure::nameToStructureType("minecraft:village").value(), StructureType::Village);
    EXPECT_EQ(Structure::nameToStructureType("minecraft:fortress").value(), StructureType::Fortress);
    EXPECT_EQ(Structure::nameToStructureType("minecraft:end_city").value(), StructureType::EndCity);
    EXPECT_EQ(Structure::nameToStructureType("minecraft:bastion_remnant").value(), StructureType::Bastion);
}

// ============================================================================
// Structure::nameToStructureType 测试 — 别名映射
// ============================================================================

TEST(StructureTypeMappingTest, NameToType_Aliases)
{
    // 验证常见别名能正确映射
    EXPECT_EQ(Structure::nameToStructureType("mansion").value(), StructureType::WoodlandMansion);
    EXPECT_EQ(Structure::nameToStructureType("monument").value(), StructureType::Monument);
    EXPECT_EQ(Structure::nameToStructureType("nether_fortress").value(), StructureType::Fortress);
    EXPECT_EQ(Structure::nameToStructureType("ocean_ruins").value(), StructureType::OceanRuin);
    EXPECT_EQ(Structure::nameToStructureType("endcity").value(), StructureType::EndCity);
    EXPECT_EQ(Structure::nameToStructureType("bastion").value(), StructureType::Bastion);
    EXPECT_EQ(Structure::nameToStructureType("desert_pyramid").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("desert_temple").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("jungle_temple").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("jungle_pyramid").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("igloo").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("swamp_hut").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("witch_hut").value(), StructureType::Temple);
    EXPECT_EQ(Structure::nameToStructureType("nether_fossil").value(), StructureType::Temple);
}

// ============================================================================
// Structure::nameToStructureType 测试 — 无效输入
// ============================================================================

TEST(StructureTypeMappingTest, NameToType_InvalidInput)
{
    // 验证无效名称返回 std::nullopt
    EXPECT_FALSE(Structure::nameToStructureType("nonexistent_structure").has_value());
    EXPECT_FALSE(Structure::nameToStructureType("").has_value());
    EXPECT_FALSE(Structure::nameToStructureType("minecraft:nonexistent").has_value());
    EXPECT_FALSE(Structure::nameToStructureType("VILLAGE").has_value()); // 大小写敏感
    EXPECT_FALSE(Structure::nameToStructureType("Village").has_value());
}

// ============================================================================
// Structure::typeToId 与 nameToStructureType 往返一致性测试
// ============================================================================

TEST(StructureTypeMappingTest, RoundTrip_AllTypes)
{
    // 验证 typeToId 的结果可以被 nameToStructureType 反向解析
    // 即：nameToStructureType(typeToId(type).path()) == type
    const StructureType types[] = {
        StructureType::Temple,
        StructureType::Monument,
        StructureType::Stronghold,
        StructureType::Village,
        StructureType::Mineshaft,
        StructureType::RuinedPortal,
        StructureType::BuriedTreasure,
        StructureType::Shipwreck,
        StructureType::OceanRuin,
        StructureType::WoodlandMansion,
        StructureType::Bastion,
        StructureType::Fortress,
        StructureType::EndCity,
        StructureType::PillagerOutpost,
        StructureType::TrialChambers,
    };

    for (auto type : types) {
        auto id = Structure::typeToId(type);
        auto result = Structure::nameToStructureType(id.path());
        ASSERT_TRUE(result.has_value()) << "typeToId(" << static_cast<int>(type) << ") returned path '" << id.path()
                                        << "' which nameToStructureType cannot resolve";
        EXPECT_EQ(result.value(), type) << "Round-trip failed for type " << static_cast<int>(type) << ": typeToId -> '"
                                        << id.path() << "' -> nameToStructureType returned type "
                                        << static_cast<int>(result.value());
    }
}
