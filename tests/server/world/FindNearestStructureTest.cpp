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
 */

#include <gtest/gtest.h>

#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/block/BlockPos.hpp"

using namespace mc;
using namespace mc::world::gen::structure;

/**
 * @brief 结构定位测试
 *
 * 测试 IWorld::findNearestStructure 接口和相关功能
 */
class FindNearestStructureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化结构注册表
        if (!StructureRegistry::isInitialized()) {
            StructureRegistry::initialize();
        }
    }
};

// ============================================================================
// StructureType 枚举测试
// ============================================================================

TEST_F(FindNearestStructureTest, StructureTypeEnumExists)
{
    // 验证 StructureType 枚举包含所有需要的类型
    EXPECT_NO_THROW({
        StructureType type = StructureType::Shipwreck;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::OceanRuin;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::BuriedTreasure;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::Village;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::Stronghold;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::Mineshaft;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::Monument;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::Temple;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::RuinedPortal;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::WoodlandMansion;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::Fortress;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::Bastion;
        (void)type;
    });

    EXPECT_NO_THROW({
        StructureType type = StructureType::EndCity;
        (void)type;
    });
}

// ============================================================================
// StructureSeparationSettings 测试
// ============================================================================

TEST_F(FindNearestStructureTest, StructureSeparationSettingsDefaults)
{
    // 验证结构间距设置的默认值
    StructureSeparationSettings settings;
    EXPECT_EQ(settings.spacing, 1);
    EXPECT_EQ(settings.separation, 0);
    EXPECT_EQ(settings.salt, 0);
}

TEST_F(FindNearestStructureTest, StructureSeparationSettingsCustom)
{
    // 验证自定义结构间距设置
    StructureSeparationSettings settings{24, 4, 165745295};
    EXPECT_EQ(settings.spacing, 24);
    EXPECT_EQ(settings.separation, 4);
    EXPECT_EQ(settings.salt, 165745295);
}

// ============================================================================
// StructureRegistry 测试
// ============================================================================

TEST_F(FindNearestStructureTest, StructureRegistryInitialized)
{
    EXPECT_TRUE(StructureRegistry::isInitialized());
}

TEST_F(FindNearestStructureTest, StructureRegistryGetShipwreck)
{
    const Structure* structure = StructureRegistry::get("shipwreck");
    ASSERT_NE(structure, nullptr);
    EXPECT_EQ(structure->structureType(), StructureType::Shipwreck);

    // 验证沉船的间距设置（MC 1.16.5）
    auto settings = structure->separationSettings();
    EXPECT_EQ(settings.spacing, 24);
    EXPECT_EQ(settings.separation, 4);
    EXPECT_EQ(settings.salt, 165745295);
}

TEST_F(FindNearestStructureTest, StructureRegistryGetOceanRuin)
{
    const Structure* structure = StructureRegistry::get("ocean_ruin");
    ASSERT_NE(structure, nullptr);
    EXPECT_EQ(structure->structureType(), StructureType::OceanRuin);

    // 验证海底废墟的间距设置（MC 1.16.5）
    auto settings = structure->separationSettings();
    EXPECT_EQ(settings.spacing, 20);
    EXPECT_EQ(settings.separation, 8);
    EXPECT_EQ(settings.salt, 14357621);
}

TEST_F(FindNearestStructureTest, StructureRegistryGetInvalid)
{
    const Structure* structure = StructureRegistry::get("invalid_structure");
    EXPECT_EQ(structure, nullptr);
}

// ============================================================================
// Structure::findStructureStart 测试
// ============================================================================

TEST_F(FindNearestStructureTest, FindStructureStartBasic)
{
    // 测试基本的结构起点查找
    i64 seed = 12345;
    i32 chunkX = 0;
    i32 chunkZ = 0;
    StructureSeparationSettings settings{24, 4, 165745295};

    i32 startX, startZ;
    bool result = Structure::findStructureStart(seed, chunkX, chunkZ, settings, startX, startZ, true);

    // 结果应该是确定性的（相同种子和区块坐标得到相同结果）
    if (result) {
        // 如果找到结构，验证坐标在合理范围内
        EXPECT_GE(startX, chunkX * 16 - settings.spacing * 16);
        EXPECT_LE(startX, chunkX * 16 + settings.spacing * 16);
        EXPECT_GE(startZ, chunkZ * 16 - settings.spacing * 16);
        EXPECT_LE(startZ, chunkZ * 16 + settings.spacing * 16);
    }
}

TEST_F(FindNearestStructureTest, FindStructureStartDeterministic)
{
    // 验证相同参数总是返回相同结果
    i64 seed = 67890;
    i32 chunkX = 10;
    i32 chunkZ = 20;
    StructureSeparationSettings settings{24, 4, 165745295};

    i32 startX1, startZ1;
    bool result1 = Structure::findStructureStart(seed, chunkX, chunkZ, settings, startX1, startZ1, true);

    i32 startX2, startZ2;
    bool result2 = Structure::findStructureStart(seed, chunkX, chunkZ, settings, startX2, startZ2, true);

    EXPECT_EQ(result1, result2);
    if (result1 && result2) {
        EXPECT_EQ(startX1, startX2);
        EXPECT_EQ(startZ1, startZ2);
    }
}

TEST_F(FindNearestStructureTest, FindStructureStartDifferentSeeds)
{
    // 验证不同种子可能产生不同结果
    i32 chunkX = 0;
    i32 chunkZ = 0;
    StructureSeparationSettings settings{24, 4, 165745295};

    i32 startX1, startZ1;
    bool result1 = Structure::findStructureStart(11111, chunkX, chunkZ, settings, startX1, startZ1, true);

    i32 startX2, startZ2;
    bool result2 = Structure::findStructureStart(22222, chunkX, chunkZ, settings, startX2, startZ2, true);

    // 不同种子可能产生不同的结果（但不一定总是不同）
    // 这个测试只是确保函数不会崩溃
    EXPECT_NO_THROW({
        (void)result1;
        (void)result2;
    });
}

// ============================================================================
// BlockPos 测试
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

// ============================================================================
// 常量验证测试
// ============================================================================

TEST_F(FindNearestStructureTest, MC1165ShipwreckConstants)
{
    // 验证 MC 1.16.5 沉船常量
    // spacing=24, separation=4, salt=165745295
    const Structure* structure = StructureRegistry::get("shipwreck");
    ASSERT_NE(structure, nullptr);

    auto settings = structure->separationSettings();
    EXPECT_EQ(settings.spacing, 24);
    EXPECT_EQ(settings.separation, 4);
    EXPECT_EQ(settings.salt, 165745295);
}

TEST_F(FindNearestStructureTest, MC1165OceanRuinConstants)
{
    // 验证 MC 1.16.5 海底废墟常量
    // spacing=20, separation=8, salt=14357621
    const Structure* structure = StructureRegistry::get("ocean_ruin");
    ASSERT_NE(structure, nullptr);

    auto settings = structure->separationSettings();
    EXPECT_EQ(settings.spacing, 20);
    EXPECT_EQ(settings.separation, 8);
    EXPECT_EQ(settings.salt, 14357621);
}

TEST_F(FindNearestStructureTest, MC1165BuriedTreasureConstants)
{
    // 验证 MC 1.16.5 埋藏宝藏常量
    // spacing=1, separation=0, salt=0
    const Structure* structure = StructureRegistry::get("buried_treasure");
    ASSERT_NE(structure, nullptr);

    auto settings = structure->separationSettings();
    EXPECT_EQ(settings.spacing, 1);
    EXPECT_EQ(settings.separation, 0);
    EXPECT_EQ(settings.salt, 0);
}
