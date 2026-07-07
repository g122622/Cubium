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

    // 森林动物生成列表：sheep / pig / cow / chicken / wolf
    const auto& creatures = info.getCreatureSpawns();
    bool hasWolf = false;
    int chickenCount = 0;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
    }
    EXPECT_TRUE(hasWolf);
    // 原版 Forest chicken 仅一条 weight=10 条目；此前源码误加两条，已收敛
    EXPECT_EQ(chickenCount, 1);
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

    // 海洋应该有水生生物（squid + dolphin 归 WaterCreature）
    const auto& waterCreatures = info.getWaterCreatureSpawns();
    EXPECT_FALSE(waterCreatures.empty());

    // 鱼类（cod）原版归 WaterAmbient，非 WaterCreature
    const auto& waterAmbient = info.getWaterAmbientSpawns();
    bool hasCod = false;
    for (const auto& entry : waterAmbient) {
        if (entry.entityTypeId == "minecraft:cod") hasCod = true;
    }
    EXPECT_TRUE(hasCod);

    // 应该有溺尸
    const auto& monsters = info.getMonsterSpawns();
    bool hasDrowned = false;
    for (const auto& entry : monsters) {
        if (entry.entityTypeId == "minecraft:drowned") hasDrowned = true;
    }
    EXPECT_TRUE(hasDrowned);
}

TEST_F(WorldGenSpawnerTest, JungleSpawnInfo)
{
    // 对应 MC 1.16.5 OverworldBiomes.jungle()
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createJungle();

    // baseJungleSpawns 在标准 farmAnimals 之外额外加一条 chicken，故 Jungle 应有两条
    const auto& creatures = info.getCreatureSpawns();
    int chickenCount = 0;
    bool hasCow = false, hasParrot = false, hasPanda = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
        if (entry.entityTypeId == "minecraft:cow") hasCow = true;
        if (entry.entityTypeId == "minecraft:parrot") hasParrot = true;
        if (entry.entityTypeId == "minecraft:panda") hasPanda = true;
    }
    EXPECT_EQ(chickenCount, 2);
    EXPECT_TRUE(hasCow); // farmAnimals 含 cow（旧实现误删，已收敛）
    EXPECT_TRUE(hasParrot);
    EXPECT_TRUE(hasPanda);

    // jungle() 将 ocelot 加入 MONSTER 分类（weight=2, pack=1-3）
    const auto& monsters = info.getMonsterSpawns();
    bool hasOcelot = false;
    for (const auto& entry : monsters) {
        if (entry.entityTypeId == "minecraft:ocelot") {
            hasOcelot = true;
            EXPECT_EQ(entry.weight, 2);
            EXPECT_EQ(entry.minCount, 1);
            EXPECT_EQ(entry.maxCount, 3);
        }
    }
    EXPECT_TRUE(hasOcelot);
}

TEST_F(WorldGenSpawnerTest, BambooJungleSpawnInfo)
{
    // 对应 MC 1.16.5 OverworldBiomes.bambooJungle()
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createBambooJungle();

    // bambooJungle panda weight=80（与普通 jungle 的 1 不同）
    const auto& creatures = info.getCreatureSpawns();
    int pandaWeight = 0;
    int chickenCount = 0;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:panda") pandaWeight = entry.weight;
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
    }
    EXPECT_EQ(pandaWeight, 80);
    EXPECT_EQ(chickenCount, 2); // baseJungleSpawns 额外鸡规则

    // bambooJungle ocelot pack=1（与普通 jungle 的 3 不同）
    const auto& monsters = info.getMonsterSpawns();
    bool ocelotOk = false;
    for (const auto& entry : monsters) {
        if (entry.entityTypeId == "minecraft:ocelot") {
            if (entry.minCount == 1 && entry.maxCount == 1) ocelotOk = true;
        }
    }
    EXPECT_TRUE(ocelotOk);
}

TEST_F(WorldGenSpawnerTest, SparseJungleSpawnInfo)
{
    // 对应 MC 1.16.5 BiomeRegistry.func_244204_a(23, JUNGLE_EDGE, BiomeMaker.func_244227_b())
    //   func_244227_b 内部仅调用 baseJungleSpawns，不额外添加 wolf/parrot/ocelot/panda。
    //   注：1.21.11 sparseJungle() 额外添加了 wolf(8,2,4)，但 1.16.5 中无 wolf。本项目对齐 1.16.5。
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createSparseJungle();

    // sparseJungle = baseJungleSpawns（farmAnimals + 额外 chicken + commonSpawns）
    const auto& creatures = info.getCreatureSpawns();
    bool hasWolf = false;
    int chickenCount = 0;
    bool hasOcelotInCreatures = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
        if (entry.entityTypeId == "minecraft:chicken") ++chickenCount;
        if (entry.entityTypeId == "minecraft:ocelot") hasOcelotInCreatures = true;
    }
    EXPECT_FALSE(hasWolf);              // 1.16.5 JungleEdge 不含 wolf
    EXPECT_EQ(chickenCount, 2);         // baseJungleSpawns 额外鸡规则
    EXPECT_FALSE(hasOcelotInCreatures); // sparseJungle 没有 ocelot
}

TEST_F(WorldGenSpawnerTest, SavannaSpawnInfo)
{
    // 对应 MC 1.16.5 BiomeMaker.func_244211_a(..., false, false)
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createSavanna();

    // 普通 Savanna：farmAnimals + horse + donkey，无 llama/wolf
    const auto& creatures = info.getCreatureSpawns();
    bool hasHorse = false, hasDonkey = false, hasLlama = false, hasWolf = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:horse" && entry.weight == 1 && entry.minCount == 2 &&
            entry.maxCount == 6) {
            hasHorse = true;
        }
        if (entry.entityTypeId == "minecraft:donkey" && entry.weight == 1 && entry.minCount == 1 &&
            entry.maxCount == 1) {
            hasDonkey = true;
        }
        if (entry.entityTypeId == "minecraft:llama") hasLlama = true;
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_TRUE(hasHorse);
    EXPECT_TRUE(hasDonkey);
    EXPECT_FALSE(hasLlama);
    EXPECT_FALSE(hasWolf);
}

TEST_F(WorldGenSpawnerTest, SavannaPlateauSpawnInfo)
{
    // 对应 MC 1.16.5 BiomeMaker.func_244247_m()：在 func_244258_x 基础上加 llama(8,4,4)
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createSavannaPlateau();

    const auto& creatures = info.getCreatureSpawns();
    bool hasLlama = false, hasWolf = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:llama" && entry.weight == 8 && entry.minCount == 4 &&
            entry.maxCount == 4) {
            hasLlama = true;
        }
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_TRUE(hasLlama); // SavannaPlateau 含 llama
    EXPECT_FALSE(hasWolf); // 1.16.5 SavannaPlateau 不含 wolf
}

TEST_F(WorldGenSpawnerTest, ShatteredSavannaSpawnInfo)
{
    // 对应 MC 1.16.5 BiomeMaker.func_244211_a(..., true, true)：spawn list 与普通 Savanna 相同
    world::spawn::MobSpawnInfo info = world::spawn::MobSpawnInfo::createShatteredSavanna();

    const auto& creatures = info.getCreatureSpawns();
    bool hasLlama = false, hasWolf = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:llama") hasLlama = true;
        if (entry.entityTypeId == "minecraft:wolf") hasWolf = true;
    }
    EXPECT_FALSE(hasLlama);
    EXPECT_FALSE(hasWolf);
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
