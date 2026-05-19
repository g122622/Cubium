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

#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace test {

/**
 * @brief WorldGenSpawner 测试套件
 *
 * 测试区块生成时的生物放置逻辑。
 */
class WorldGenSpawnerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化生物群系注册表
        BiomeRegistry::instance().initialize();
    }

    void TearDown() override
    {
        // 清理
    }
};

// ========== 基本功能测试 ==========

TEST_F(WorldGenSpawnerTest, CreateSpawner)
{
    WorldGenSpawner spawner;
    EXPECT_TRUE(spawner.isEnabled());
}

TEST_F(WorldGenSpawnerTest, EnableDisable)
{
    WorldGenSpawner spawner;
    EXPECT_TRUE(spawner.isEnabled());

    spawner.setEnabled(false);
    EXPECT_FALSE(spawner.isEnabled());

    spawner.setEnabled(true);
    EXPECT_TRUE(spawner.isEnabled());
}

// ========== MobSpawnInfo 测试 ==========

TEST_F(WorldGenSpawnerTest, PlainsSpawnInfo)
{
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createPlains();

    // 检查动物生成条目
    const auto& creatures = info.getCreatureSpawns();
    EXPECT_FALSE(creatures.empty());

    // 应该包含羊、猪、牛、鸡
    bool hasSheep = false, hasPig = false, hasCow = false, hasChicken = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:sheep") hasSheep = true;
        if (entry.entityTypeId == "minecraft:pig") hasPig = true;
        if (entry.entityTypeId == "minecraft:cow") hasCow = true;
        if (entry.entityTypeId == "minecraft:chicken") hasChicken = true;
    }
    EXPECT_TRUE(hasSheep);
    EXPECT_TRUE(hasPig);
    EXPECT_TRUE(hasCow);
    EXPECT_TRUE(hasChicken);

    // 检查怪物生成条目
    const auto& monsters = info.getMonsterSpawns();
    EXPECT_FALSE(monsters.empty());
}

TEST_F(WorldGenSpawnerTest, ForestSpawnInfo)
{
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createForest();

    // 森林应该有狼
    const auto& creatures = info.getCreatureSpawns();
    bool hasWolf = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_TRUE(hasWolf);
}

TEST_F(WorldGenSpawnerTest, DesertSpawnInfo)
{
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createDesert();

    // 沙漠应该有尸壳
    const auto& monsters = info.getMonsterSpawns();
    bool hasHusk = false;
    for (const auto& entry : monsters) {
        if (entry.entityTypeId == "minecraft:husk") hasHusk = true;
    }
    EXPECT_TRUE(hasHusk);

    // 沙漠动物应该很少
    const auto& creatures = info.getCreatureSpawns();
    EXPECT_LE(creatures.size(), 2);
}

TEST_F(WorldGenSpawnerTest, OceanSpawnInfo)
{
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createOcean();

    // 海洋应该有水生生物
    const auto& waterCreatures = info.getWaterCreatureSpawns();
    EXPECT_FALSE(waterCreatures.empty());

    // 应该有溺尸
    const auto& monsters = info.getMonsterSpawns();
    bool hasDrowned = false;
    for (const auto& entry : monsters) {
        if (entry.entityTypeId == "minecraft:drowned") hasDrowned = true;
    }
    EXPECT_TRUE(hasDrowned);
}

// ========== SpawnEntry 测试 ==========

TEST_F(WorldGenSpawnerTest, SpawnEntryDefaults)
{
    world::spawn::SpawnEntry entry;
    EXPECT_EQ(entry.weight, 0);
    EXPECT_EQ(entry.minCount, 1);
    EXPECT_EQ(entry.maxCount, 4);
}

TEST_F(WorldGenSpawnerTest, SpawnEntryCustomValues)
{
    world::spawn::SpawnEntry entry("minecraft:pig", 100, 2, 5);
    EXPECT_EQ(entry.entityTypeId, "minecraft:pig");
    EXPECT_EQ(entry.weight, 100);
    EXPECT_EQ(entry.minCount, 2);
    EXPECT_EQ(entry.maxCount, 5);
}

TEST_F(WorldGenSpawnerTest, SpawnEntryWithCosts)
{
    world::spawn::SpawnCosts costs(0.5, 1.0);
    world::spawn::SpawnEntry entry("minecraft:zombie", 80, 3, 6, costs);
    EXPECT_EQ(entry.costs.energyBudget, 0.5);
    EXPECT_EQ(entry.costs.charge, 1.0);
}

// ========== Biome 集成测试 ==========

TEST_F(WorldGenSpawnerTest, BiomeSpawnInfo)
{
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);

    // 生物群系应该有生成概率
    EXPECT_GT(plains.creatureSpawnProbability(), 0.0f);
    EXPECT_LT(plains.creatureSpawnProbability(), 1.0f);
}

} // namespace test
} // namespace mc
