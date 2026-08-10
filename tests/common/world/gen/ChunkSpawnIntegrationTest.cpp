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

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"

using namespace mc;
using namespace mc::entity;

/**
 * @brief 区块生成生物集成测试
 *
 * 测试 WorldGenSpawner 与 ChunkPrimer 的集成流程。
 */
class ChunkSpawnIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化生物群系注册表
        BiomeRegistry::instance().initialize();

        // 注册测试实体类型
        registerTestEntities();
    }

    void TearDown() override
    {
        // 清理实体注册表。clear() 同时会重置全局 VanillaEntityTypeKeys::* 缓存
        // （见 VanillaEntityTypeKeys::reset），保证"注册表空 ⇔ 指针缓存全 nullptr"不变量，
        // 避免后续测试因 entityType()==nullptr 与 VanillaEntityTypeKeys::ITEM=旧值 比较失败。
        EntityRegistry::instance().clear();
    }

    void registerTestEntities()
    {
        auto& registry = EntityRegistry::instance();

        // 注册测试猪
        registry.registerType(EntityTypeKeys::PIG,
            EntityType::Builder(
                [](IWorld*, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> {
                    return std::make_unique<Entity>(EntityInstanceId(0), nullptr, mc::test::testEcsRegistry());
                },
                EntityClassification::Creature)
                .size(0.9f, 0.9f)
                .trackingRange(10)
                .canSummon()
                .build());

        // 注册测试牛
        registry.registerType(EntityTypeKeys::COW,
            EntityType::Builder(
                [](IWorld*, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> {
                    return std::make_unique<Entity>(EntityInstanceId(0), nullptr, mc::test::testEcsRegistry());
                },
                EntityClassification::Creature)
                .size(0.9f, 1.4f)
                .trackingRange(10)
                .canSummon()
                .build());

        // 注册测试羊
        registry.registerType(EntityTypeKeys::SHEEP,
            EntityType::Builder(
                [](IWorld*, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> {
                    return std::make_unique<Entity>(EntityInstanceId(0), nullptr, mc::test::testEcsRegistry());
                },
                EntityClassification::Creature)
                .size(0.9f, 1.3f)
                .trackingRange(10)
                .canSummon()
                .build());

        // 注册测试鸡
        registry.registerType(EntityTypeKeys::CHICKEN,
            EntityType::Builder(
                [](IWorld*, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> {
                    return std::make_unique<Entity>(EntityInstanceId(0), nullptr, mc::test::testEcsRegistry());
                },
                EntityClassification::Creature)
                .size(0.4f, 0.7f)
                .trackingRange(10)
                .canSummon()
                .build());
    }
};

// 测试 SpawnedEntityData 结构
TEST_F(ChunkSpawnIntegrationTest, SpawnedEntityDataStructure)
{
    SpawnedEntityData data("minecraft:pig", 100.5f, 64.0f, 200.3f);

    EXPECT_EQ(data.entityTypeId, "minecraft:pig");
    EXPECT_FLOAT_EQ(data.x, 100.5f);
    EXPECT_FLOAT_EQ(data.y, 64.0f);
    EXPECT_FLOAT_EQ(data.z, 200.3f);
    EXPECT_EQ(data.spawnReason, world::spawn::SpawnReason::ChunkGeneration);
}

// 测试 SpawnedEntityData 默认构造
TEST_F(ChunkSpawnIntegrationTest, SpawnedEntityDataDefaultConstructor)
{
    SpawnedEntityData data;

    EXPECT_TRUE(data.entityTypeId.empty());
    EXPECT_FLOAT_EQ(data.x, 0.0f);
    EXPECT_FLOAT_EQ(data.y, 0.0f);
    EXPECT_FLOAT_EQ(data.z, 0.0f);
    EXPECT_EQ(data.spawnReason, world::spawn::SpawnReason::ChunkGeneration);
}

// 测试 ChunkPrimer 存储生成实体
TEST_F(ChunkSpawnIntegrationTest, ChunkPrimerSpawnedEntities)
{
    ChunkPrimer primer(0, 0);

    // 初始为空
    EXPECT_EQ(primer.spawnedEntityCount(), 0);
    EXPECT_TRUE(primer.spawnedEntities().empty());

    // 添加实体
    primer.addSpawnedEntity(SpawnedEntityData("minecraft:pig", 8.5, 64.0, 8.5));
    EXPECT_EQ(primer.spawnedEntityCount(), 1);

    primer.addSpawnedEntity(SpawnedEntityData("minecraft:cow", 12.0, 64.0, 4.0));
    EXPECT_EQ(primer.spawnedEntityCount(), 2);

    // 验证内容
    const auto& entities = primer.spawnedEntities();
    EXPECT_EQ(entities[0].entityTypeId, "minecraft:pig");
    EXPECT_EQ(entities[1].entityTypeId, "minecraft:cow");

    // 清空
    primer.clearSpawnedEntities();
    EXPECT_EQ(primer.spawnedEntityCount(), 0);
    EXPECT_TRUE(primer.spawnedEntities().empty());
}

// 测试 ChunkPrimer 移动实体数据
TEST_F(ChunkSpawnIntegrationTest, ChunkPrimerMoveSpawnedEntities)
{
    ChunkPrimer primer(0, 0);

    primer.addSpawnedEntity(SpawnedEntityData("minecraft:pig", 8.0, 64.0, 8.0));
    primer.addSpawnedEntity(SpawnedEntityData("minecraft:sheep", 10.0, 64.0, 10.0));
    primer.addSpawnedEntity(SpawnedEntityData("minecraft:chicken", 6.0, 64.0, 6.0));

    // 移动获取
    auto entities = std::move(primer.spawnedEntities());
    EXPECT_EQ(entities.size(), 3);

    // primer 应该为空
    EXPECT_EQ(primer.spawnedEntityCount(), 0);
}

// 测试生物群系生成信息正确关联
TEST_F(ChunkSpawnIntegrationTest, BiomeSpawnInfoCorrectlyAssociated)
{
    auto& registry = BiomeRegistry::instance();

    // 平原生物群系应该有动物生成信息
    const Biome& plains = registry.get(Biomes::Plains);
    const auto& spawnInfo = plains.spawnInfo();
    const auto& creatures = spawnInfo.getCreatureSpawns();

    // 平原应该有被动动物
    EXPECT_FALSE(creatures.empty());

    // 验证至少有猪、牛、羊中的一种
    bool hasPassiveAnimal = false;
    for (const auto& entry : creatures) {
        if (entry.entityTypeId == "minecraft:pig" || entry.entityTypeId == "minecraft:cow" ||
            entry.entityTypeId == "minecraft:sheep" || entry.entityTypeId == "minecraft:chicken") {
            hasPassiveAnimal = true;
            break;
        }
    }
    EXPECT_TRUE(hasPassiveAnimal);
}

// 测试生成概率
TEST_F(ChunkSpawnIntegrationTest, CreatureSpawnProbability)
{
    auto& registry = BiomeRegistry::instance();
    const Biome& plains = registry.get(Biomes::Plains);

    // 生物群系应该有非零的生成概率
    f32 probability = plains.creatureSpawnProbability();
    EXPECT_GT(probability, 0.0f);
    EXPECT_LE(probability, 1.0f);
}

// 测试实体注册表查找
TEST_F(ChunkSpawnIntegrationTest, EntityRegistryLookup)
{
    auto& registry = EntityRegistry::instance();

    // 验证注册的实体类型
    const EntityType* pigType = registry.getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);
    EXPECT_EQ(pigType->classification(), EntityClassification::Creature);
    EXPECT_TRUE(pigType->canSummon());

    const EntityType* cowType = registry.getType(EntityTypeKeys::COW);
    ASSERT_NE(cowType, nullptr);
    EXPECT_EQ(cowType->classification(), EntityClassification::Creature);

    const EntityType* sheepType = registry.getType(EntityTypeKeys::SHEEP);
    ASSERT_NE(sheepType, nullptr);
    EXPECT_EQ(sheepType->classification(), EntityClassification::Creature);

    const EntityType* chickenType = registry.getType(EntityTypeKeys::CHICKEN);
    ASSERT_NE(chickenType, nullptr);
    EXPECT_EQ(chickenType->classification(), EntityClassification::Creature);
}

// 测试实体创建
TEST_F(ChunkSpawnIntegrationTest, EntityCreation)
{
    auto& registry = EntityRegistry::instance();

    const EntityType* pigType = registry.getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);

    // 创建实体
    std::unique_ptr<Entity> pig = pigType->create(nullptr, mc::test::testEcsRegistry());
    ASSERT_NE(pig, nullptr);
}

// 测试 SpawnEntry 权重选择
TEST_F(ChunkSpawnIntegrationTest, SpawnEntryWeightSelection)
{
    // 创建带权重的生成条目
    std::vector<world::spawn::SpawnEntry> entries = {
        world::spawn::SpawnEntry("minecraft:pig", 100, 2, 4),  // 权重 100
        world::spawn::SpawnEntry("minecraft:cow", 50, 2, 3),   // 权重 50
        world::spawn::SpawnEntry("minecraft:sheep", 75, 2, 4), // 权重 75
    };

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& entry : entries) {
        totalWeight += entry.weight;
    }
    EXPECT_EQ(totalWeight, 225);

    // 模拟多次选择，验证分布
    math::Random rng(12345);
    std::map<std::string, i32> selectionCounts;

    const i32 iterations = 1000;
    for (i32 i = 0; i < iterations; ++i) {
        i32 weightValue = rng.nextInt(totalWeight);
        const world::spawn::SpawnEntry* selectedEntry = nullptr;
        i32 currentWeight = 0;

        for (const auto& entry : entries) {
            currentWeight += entry.weight;
            if (weightValue < currentWeight) {
                selectedEntry = &entry;
                break;
            }
        }

        if (selectedEntry) {
            selectionCounts[selectedEntry->entityTypeId]++;
        }
    }

    // 验证每种类型都被选中（概率应该很高）
    EXPECT_GT(selectionCounts["minecraft:pig"], 0);
    EXPECT_GT(selectionCounts["minecraft:cow"], 0);
    EXPECT_GT(selectionCounts["minecraft:sheep"], 0);

    // 猪应该被选中最多次（权重最高）
    EXPECT_GT(selectionCounts["minecraft:pig"], selectionCounts["minecraft:cow"]);
}

// 测试生成数量范围
TEST_F(ChunkSpawnIntegrationTest, SpawnCountRange)
{
    world::spawn::SpawnEntry entry("minecraft:pig", 100, 2, 4);

    math::Random rng(54321);

    // 生成多个样本，验证范围
    for (i32 i = 0; i < 100; ++i) {
        i32 count = entry.minCount;
        if (entry.maxCount > entry.minCount) {
            count = entry.minCount + rng.nextInt(entry.maxCount - entry.minCount + 1);
        }

        EXPECT_GE(count, entry.minCount);
        EXPECT_LE(count, entry.maxCount);
    }
}

// 测试 MobSpawnInfo 空生成信息
TEST_F(ChunkSpawnIntegrationTest, EmptyMobSpawnInfo)
{
    world::spawn::MobSpawnInfo emptyInfo;

    EXPECT_TRUE(emptyInfo.getCreatureSpawns().empty());
    EXPECT_TRUE(emptyInfo.getMonsterSpawns().empty());
    EXPECT_TRUE(emptyInfo.getAmbientSpawns().empty());
    EXPECT_TRUE(emptyInfo.getWaterCreatureSpawns().empty());
    EXPECT_TRUE(emptyInfo.getMiscSpawns().empty());
}

// 测试 ChunkPrimer 在 toChunkData 后实体数据仍可访问
TEST_F(ChunkSpawnIntegrationTest, ChunkPrimerToChunkDataPreservesEntities)
{
    ChunkPrimer primer(10, 20);

    // 添加一些生成实体
    primer.addSpawnedEntity(SpawnedEntityData("minecraft:pig", 160.5, 64.0, 320.5));
    primer.addSpawnedEntity(SpawnedEntityData("minecraft:cow", 164.0, 64.0, 324.0));

    // 在转换前获取实体数据
    std::vector<SpawnedEntityData> savedEntities = primer.spawnedEntities();
    EXPECT_EQ(savedEntities.size(), 2);

    // 转换为 ChunkData
    auto chunkData = primer.toChunkData();
    ASSERT_NE(chunkData, nullptr);
    EXPECT_EQ(chunkData->x(), 10);
    EXPECT_EQ(chunkData->z(), 20);

    // primer 内部实体数据应该被清空（因为 std::move）
    EXPECT_EQ(primer.spawnedEntityCount(), 0);

    // 但保存的实体数据仍然有效
    EXPECT_EQ(savedEntities.size(), 2);
    EXPECT_EQ(savedEntities[0].entityTypeId, "minecraft:pig");
    EXPECT_EQ(savedEntities[1].entityTypeId, "minecraft:cow");
}

// 测试 SpawnedEntityData 拷贝语义
TEST_F(ChunkSpawnIntegrationTest, SpawnedEntityDataCopySemantics)
{
    SpawnedEntityData original("minecraft:sheep", 100.0, 64.0, 200.0);

    // 拷贝构造
    SpawnedEntityData copied(original);
    EXPECT_EQ(copied.entityTypeId, "minecraft:sheep");
    EXPECT_DOUBLE_EQ(copied.x, 100.0);

    // 拷贝赋值
    SpawnedEntityData assigned;
    assigned = original;
    EXPECT_EQ(assigned.entityTypeId, "minecraft:sheep");
    EXPECT_DOUBLE_EQ(assigned.z, 200.0);
}

// 测试 SpawnedEntityData 移动语义
TEST_F(ChunkSpawnIntegrationTest, SpawnedEntityDataMoveSemantics)
{
    SpawnedEntityData original("minecraft:chicken", 50.0, 70.0, 150.0);

    // 移动构造
    SpawnedEntityData moved(std::move(original));
    EXPECT_EQ(moved.entityTypeId, "minecraft:chicken");
    EXPECT_DOUBLE_EQ(moved.x, 50.0);

    // 移动赋值
    SpawnedEntityData assigned;
    SpawnedEntityData source("minecraft:cow", 75.0, 65.0, 175.0);
    assigned = std::move(source);
    EXPECT_EQ(assigned.entityTypeId, "minecraft:cow");
}

// 测试多个 ChunkPrimer 独立存储实体
TEST_F(ChunkSpawnIntegrationTest, MultipleChunkPrimersIndependent)
{
    ChunkPrimer primer1(0, 0);
    ChunkPrimer primer2(1, 0);

    primer1.addSpawnedEntity(SpawnedEntityData("minecraft:pig", 8.0, 64.0, 8.0));
    primer2.addSpawnedEntity(SpawnedEntityData("minecraft:cow", 24.0, 64.0, 8.0));

    EXPECT_EQ(primer1.spawnedEntityCount(), 1);
    EXPECT_EQ(primer2.spawnedEntityCount(), 1);

    EXPECT_EQ(primer1.spawnedEntities()[0].entityTypeId, "minecraft:pig");
    EXPECT_EQ(primer2.spawnedEntities()[0].entityTypeId, "minecraft:cow");

    // 清空一个不影响另一个
    primer1.clearSpawnedEntities();
    EXPECT_EQ(primer1.spawnedEntityCount(), 0);
    EXPECT_EQ(primer2.spawnedEntityCount(), 1);
}

// 测试生物群系不同的生成信息
TEST_F(ChunkSpawnIntegrationTest, DifferentBiomeSpawnInfo)
{
    auto& registry = BiomeRegistry::instance();

    const Biome& plains = registry.get(Biomes::Plains);
    const Biome& desert = registry.get(Biomes::Desert);
    const Biome& ocean = registry.get(Biomes::Ocean);

    // 平原应该有被动动物
    const auto& plainsCreatures = plains.spawnInfo().getCreatureSpawns();
    EXPECT_FALSE(plainsCreatures.empty());

    // 沙漠可能没有或很少的被动动物（取决于配置）
    // 海洋可能有水生生物
    // 这里只验证方法能正常工作
    const auto& desertCreatures = desert.spawnInfo().getCreatureSpawns();
    const auto& oceanCreatures = ocean.spawnInfo().getCreatureSpawns();

    // 验证可以通过
    (void)desertCreatures;
    (void)oceanCreatures;
}

// 测试实体类型 canSummon 标志
TEST_F(ChunkSpawnIntegrationTest, EntityTypeCanSummon)
{
    auto& registry = EntityRegistry::instance();

    // 注册一个不能被召唤的实体类型
    registry.registerType("test:unsummonable",
        EntityType::Builder([](IWorld*, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> { return nullptr; },
            EntityClassification::Misc)
            .canSummon(false)
            .build());

    const EntityType* unsummonable = registry.getType("test:unsummonable");
    ASSERT_NE(unsummonable, nullptr);
    EXPECT_FALSE(unsummonable->canSummon());

    // 验证之前注册的可召唤实体
    const EntityType* pigType = registry.getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);
    EXPECT_TRUE(pigType->canSummon());
}

// 测试实体类型分类
TEST_F(ChunkSpawnIntegrationTest, EntityTypeClassification)
{
    auto& registry = EntityRegistry::instance();

    // 验证所有注册的动物都是 Creature 分类
    const EntityType* pigType = registry.getType(EntityTypeKeys::PIG);
    ASSERT_NE(pigType, nullptr);
    EXPECT_EQ(pigType->classification(), EntityClassification::Creature);

    const EntityType* cowType = registry.getType(EntityTypeKeys::COW);
    ASSERT_NE(cowType, nullptr);
    EXPECT_EQ(cowType->classification(), EntityClassification::Creature);

    // 验证分类辅助函数
    EXPECT_TRUE(isPeaceful(EntityClassification::Creature));
    EXPECT_FALSE(isPeaceful(EntityClassification::Monster));
    EXPECT_EQ(getMaxCount(EntityClassification::Creature), 10);
}

// 测试生成坐标验证
TEST_F(ChunkSpawnIntegrationTest, SpawnCoordinatesInChunkRange)
{
    // 测试生成的实体坐标在区块范围内
    ChunkCoord chunkX = 5;
    ChunkCoord chunkZ = -3;

    ChunkPrimer primer(chunkX, chunkZ);

    // 区块内的世界坐标范围
    i32 startX = chunkX * 16;
    i32 startZ = chunkZ * 16;

    // 添加在区块范围内的实体
    primer.addSpawnedEntity(
        SpawnedEntityData("minecraft:pig", static_cast<f32>(startX + 8.5), 64.0f, static_cast<f32>(startZ + 8.5)));

    primer.addSpawnedEntity(
        SpawnedEntityData("minecraft:cow", static_cast<f32>(startX + 15.0), 65.0f, static_cast<f32>(startZ + 0.0)));

    const auto& entities = primer.spawnedEntities();
    EXPECT_EQ(entities.size(), 2);

    // 验证坐标
    EXPECT_GE(entities[0].x, static_cast<f32>(startX));
    EXPECT_LT(entities[0].x, static_cast<f32>(startX + 16));
    EXPECT_GE(entities[0].z, static_cast<f32>(startZ));
    EXPECT_LT(entities[0].z, static_cast<f32>(startZ + 16));
}

// 测试批量添加实体
TEST_F(ChunkSpawnIntegrationTest, BatchAddEntities)
{
    ChunkPrimer primer(0, 0);

    // 批量添加 10 个实体
    for (int i = 0; i < 10; ++i) {
        SpawnedEntityData data(
            "minecraft:pig", static_cast<f32>(i * 1.5), static_cast<f32>(64 + i), static_cast<f32>(i * 2.0));
        primer.addSpawnedEntity(std::move(data));
    }

    EXPECT_EQ(primer.spawnedEntityCount(), 10);

    // 验证顺序保持
    const auto& entities = primer.spawnedEntities();
    for (int i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(entities[i].x, static_cast<f32>(i * 1.5));
        EXPECT_FLOAT_EQ(entities[i].y, static_cast<f32>(64 + i));
        EXPECT_FLOAT_EQ(entities[i].z, static_cast<f32>(i * 2.0));
    }
}
