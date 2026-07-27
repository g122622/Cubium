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
 *
 */

#include <gtest/gtest.h>

#include "common/world/gen/structure/StructureSet.hpp"

using namespace mc;
using namespace mc::world::gen::structure;

// ============================================================================
// StructureSetRegistry::findByStructure 测试
// ============================================================================

// 注册表是 Meyers 单例,默认空,需显式 initialize() 填充原版 20 个结构集;
// 否则 findByStructure 一律返回 nullptr。用例间 clear() 避免相互污染。
class StructureSetRegistryTest : public ::testing::Test {
protected:
    void SetUp() override { StructureSetRegistry::instance().initialize(); }
    void TearDown() override { StructureSetRegistry::instance().clear(); }
};

TEST_F(StructureSetRegistryTest, FindByStructure_KnownStructures)
{
    auto& registry = StructureSetRegistry::instance();

    // 验证已知结构能通过 findByStructure 找到对应的 StructureSet
    auto* villages = registry.findByStructure(ResourceLocation("minecraft", "village_plains"));
    ASSERT_NE(villages, nullptr);
    EXPECT_EQ(villages->id().toString(), "minecraft:villages");

    auto* desertPyramid = registry.findByStructure(ResourceLocation("minecraft", "desert_pyramid"));
    ASSERT_NE(desertPyramid, nullptr);
    EXPECT_EQ(desertPyramid->id().toString(), "minecraft:desert_pyramids");

    auto* stronghold = registry.findByStructure(ResourceLocation("minecraft", "stronghold"));
    ASSERT_NE(stronghold, nullptr);
    EXPECT_EQ(stronghold->id().toString(), "minecraft:strongholds");

    auto* monument = registry.findByStructure(ResourceLocation("minecraft", "monument"));
    ASSERT_NE(monument, nullptr);
    EXPECT_EQ(monument->id().toString(), "minecraft:ocean_monuments");

    auto* endCity = registry.findByStructure(ResourceLocation("minecraft", "end_city"));
    ASSERT_NE(endCity, nullptr);
    EXPECT_EQ(endCity->id().toString(), "minecraft:end_cities");

    auto* fortress = registry.findByStructure(ResourceLocation("minecraft", "fortress"));
    ASSERT_NE(fortress, nullptr);
    EXPECT_EQ(fortress->id().toString(), "minecraft:nether_complexes");
}

TEST_F(StructureSetRegistryTest, FindByStructure_UnknownStructure)
{
    auto& registry = StructureSetRegistry::instance();

    // 验证未知结构返回 nullptr
    auto* unknown = registry.findByStructure(ResourceLocation("minecraft", "nonexistent_structure"));
    EXPECT_EQ(unknown, nullptr);
}

TEST_F(StructureSetRegistryTest, FindByStructure_MultiEntrySet)
{
    auto& registry = StructureSetRegistry::instance();

    // 验证同一 StructureSet 中的多个结构都能找到同一集合
    auto* villagePlains = registry.findByStructure(ResourceLocation("minecraft", "village_plains"));
    auto* villageDesert = registry.findByStructure(ResourceLocation("minecraft", "village_desert"));
    auto* villageSnowy = registry.findByStructure(ResourceLocation("minecraft", "village_snowy"));

    ASSERT_NE(villagePlains, nullptr);
    ASSERT_NE(villageDesert, nullptr);
    ASSERT_NE(villageSnowy, nullptr);

    // 所有村庄变体应该映射到同一个 StructureSet
    EXPECT_EQ(villagePlains, villageDesert);
    EXPECT_EQ(villageDesert, villageSnowy);
    EXPECT_EQ(villagePlains->id().toString(), "minecraft:villages");
}

TEST_F(StructureSetRegistryTest, FindByStructure_PlacementTypes)
{
    auto& registry = StructureSetRegistry::instance();

    // 验证 RandomSpread 结构集
    auto* villages = registry.findByStructure(ResourceLocation("minecraft", "village_plains"));
    ASSERT_NE(villages, nullptr);
    auto& villagePlacement = villages->placement();
    EXPECT_TRUE(dynamic_cast<const placement::RandomSpreadStructurePlacement*>(&villagePlacement) != nullptr);

    // 验证 ConcentricRings 结构集（要塞）
    auto* stronghold = registry.findByStructure(ResourceLocation("minecraft", "stronghold"));
    ASSERT_NE(stronghold, nullptr);
    auto& strongholdPlacement = stronghold->placement();
    EXPECT_TRUE(dynamic_cast<const placement::ConcentricRingsStructurePlacement*>(&strongholdPlacement) != nullptr);
}
