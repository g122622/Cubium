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
#include "common/world/block/BlockPos.hpp"
#include "common/world/gen/structure/StructureSet.hpp"

using namespace mc;
using namespace mc::world::gen::structure;

/**
 * @brief 结构定位测试
 *
 * 测试 StructureSetRegistry::findByStructure 和相关功能
 */
class FindNearestStructureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化结构集合注册表
        StructureSetRegistry::instance().initialize();
    }
};

// ============================================================================
// StructureSetRegistry::findByStructure 测试
// ============================================================================

TEST_F(FindNearestStructureTest, FindByStructure_VillagePlains)
{
    auto& registry = StructureSetRegistry::instance();
    auto* villageSet = registry.findByStructure(ResourceLocation("minecraft", "village_plains"));
    ASSERT_NE(villageSet, nullptr);
    EXPECT_EQ(villageSet->id().toString(), "minecraft:villages");
}

TEST_F(FindNearestStructureTest, FindByStructure_Shipwreck)
{
    auto& registry = StructureSetRegistry::instance();
    auto* shipwreckSet = registry.findByStructure(ResourceLocation("minecraft", "shipwreck"));
    ASSERT_NE(shipwreckSet, nullptr);
    // 沉船应该属于 shipwrecks 集合
    EXPECT_TRUE(shipwreckSet->id().toString().find("shipwreck") != std::string::npos);
}

TEST_F(FindNearestStructureTest, FindByStructure_OceanRuin)
{
    auto& registry = StructureSetRegistry::instance();
    auto* ruinSet = registry.findByStructure(ResourceLocation("minecraft", "ocean_ruin_cold"));
    ASSERT_NE(ruinSet, nullptr);
    // 海底废墟应该属于 ocean_ruins 集合
    EXPECT_TRUE(ruinSet->id().toString().find("ocean_ruin") != std::string::npos);
}

TEST_F(FindNearestStructureTest, FindByStructure_Stronghold)
{
    auto& registry = StructureSetRegistry::instance();
    auto* strongholdSet = registry.findByStructure(ResourceLocation("minecraft", "stronghold"));
    ASSERT_NE(strongholdSet, nullptr);
    EXPECT_EQ(strongholdSet->id().toString(), "minecraft:strongholds");
}

TEST_F(FindNearestStructureTest, FindByStructure_Monument)
{
    auto& registry = StructureSetRegistry::instance();
    auto* monumentSet = registry.findByStructure(ResourceLocation("minecraft", "monument"));
    ASSERT_NE(monumentSet, nullptr);
    EXPECT_EQ(monumentSet->id().toString(), "minecraft:ocean_monuments");
}

TEST_F(FindNearestStructureTest, FindByStructure_UnknownReturnsNull)
{
    auto& registry = StructureSetRegistry::instance();
    auto* unknown = registry.findByStructure(ResourceLocation("minecraft", "nonexistent_structure"));
    EXPECT_EQ(unknown, nullptr);
}

// ============================================================================
// StructurePlacement 类型测试
// ============================================================================

TEST_F(FindNearestStructureTest, VillageUsesRandomSpreadPlacement)
{
    auto& registry = StructureSetRegistry::instance();
    auto* villageSet = registry.findByStructure(ResourceLocation("minecraft", "village_plains"));
    ASSERT_NE(villageSet, nullptr);
    auto& placement = villageSet->placement();
    EXPECT_TRUE(dynamic_cast<const placement::RandomSpreadStructurePlacement*>(&placement) != nullptr);
}

TEST_F(FindNearestStructureTest, StrongholdUsesConcentricRingsPlacement)
{
    auto& registry = StructureSetRegistry::instance();
    auto* strongholdSet = registry.findByStructure(ResourceLocation("minecraft", "stronghold"));
    ASSERT_NE(strongholdSet, nullptr);
    auto& placement = strongholdSet->placement();
    EXPECT_TRUE(dynamic_cast<const placement::ConcentricRingsStructurePlacement*>(&placement) != nullptr);
}

// ============================================================================
// BlockPos 距离计算测试
// ============================================================================

TEST_F(FindNearestStructureTest, BlockPosBasic)
{
    BlockPos pos(100, 64, -200);
    EXPECT_EQ(pos.x, 100);
    EXPECT_EQ(pos.y, 64);
    EXPECT_EQ(pos.z, -200);
}

TEST_F(FindNearestStructureTest, BlockPosDistanceCalculation)
{
    BlockPos pos1(0, 0, 0);
    BlockPos pos2(3, 0, 4);

    i32 dx = pos2.x - pos1.x;
    i32 dz = pos2.z - pos1.z;
    f64 distSq = static_cast<f64>(dx * dx + dz * dz);

    // 3^2 + 4^2 = 25
    EXPECT_DOUBLE_EQ(distSq, 25.0);
}
