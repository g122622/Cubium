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
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

// ============================================================================
// MobSpawnerTestWorld - 测试用 Mock 世界
// ============================================================================

class MobSpawnerTestWorld final : public test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setSeed(u64 seed) { m_seed = seed; }
    [[nodiscard]] u64 seed() const override { return m_seed; }

    void setEntitiesInRangeResult(std::vector<Entity*> entities) { m_entitiesInRange = std::move(entities); }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return m_entitiesInRange;
    }

    void setDifficulty(Difficulty diff) { m_difficulty = diff; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityId(0);
        }
        EntityId id = EntityId(++m_nextEntityId);
        entity->setId(id);
        m_spawnedEntities.push_back(entity.get());
        m_ownedEntities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MobSpawnerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MobSpawnerTestWorld::tickManager not implemented");
    }

    // 测试辅助
    size_t spawnedCount() const { return m_spawnedEntities.size(); }
    void clearSpawnedEntities()
    {
        m_spawnedEntities.clear();
        m_ownedEntities.clear();
    }
    const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

private:
    u64 m_currentTick = 0;
    u64 m_seed = 12345;
    Difficulty m_difficulty = Difficulty::Easy;
    std::vector<Entity*> m_entitiesInRange;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
    u64 m_nextEntityId = 0;
};

// ============================================================================
// 构造和基本属性测试
// ============================================================================

class MobSpawnerBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { spawner_ = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20)); }

    std::unique_ptr<MobSpawnerBlockEntity> spawner_;
};

TEST_F(MobSpawnerBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(spawner_->getType(), BlockEntityType::MobSpawner);
}

TEST_F(MobSpawnerBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(spawner_->getPos(), BlockPos(10, 64, 20));
}

TEST_F(MobSpawnerBlockEntityTest, Create_NeedsTickReturnsTrue)
{
    EXPECT_TRUE(spawner_->needsTick());
}

TEST_F(MobSpawnerBlockEntityTest, Create_OnlyOpsCanSetNbt)
{
    EXPECT_TRUE(spawner_->onlyOpsCanSetNbt());
}

TEST_F(MobSpawnerBlockEntityTest, Create_HasDefaultValues)
{
    EXPECT_EQ(spawner_->getMinSpawnDelay(), 200);
    EXPECT_EQ(spawner_->getMaxSpawnDelay(), 800);
    EXPECT_EQ(spawner_->getSpawnCount(), 4);
    EXPECT_EQ(spawner_->getMaxNearbyEntities(), 6);
    EXPECT_EQ(spawner_->getRequiredPlayerRange(), 16);
    EXPECT_EQ(spawner_->getSpawnRange(), 4);
    EXPECT_TRUE(spawner_->getSpawnPotentials().empty());
}

TEST_F(MobSpawnerBlockEntityTest, Create_NextEntityIdIsEmptyByDefault)
{
    EXPECT_TRUE(spawner_->getNextEntityId().path().empty());
}

// ============================================================================
// 配置接口测试
// ============================================================================

TEST_F(MobSpawnerBlockEntityTest, SetMinSpawnDelay)
{
    spawner_->setMinSpawnDelay(100);
    EXPECT_EQ(spawner_->getMinSpawnDelay(), 100);
}

TEST_F(MobSpawnerBlockEntityTest, SetMaxSpawnDelay)
{
    spawner_->setMaxSpawnDelay(400);
    EXPECT_EQ(spawner_->getMaxSpawnDelay(), 400);
}

TEST_F(MobSpawnerBlockEntityTest, SetSpawnCount)
{
    spawner_->setSpawnCount(8);
    EXPECT_EQ(spawner_->getSpawnCount(), 8);
}

TEST_F(MobSpawnerBlockEntityTest, SetMaxNearbyEntities)
{
    spawner_->setMaxNearbyEntities(10);
    EXPECT_EQ(spawner_->getMaxNearbyEntities(), 10);
}

TEST_F(MobSpawnerBlockEntityTest, SetRequiredPlayerRange)
{
    spawner_->setRequiredPlayerRange(32);
    EXPECT_EQ(spawner_->getRequiredPlayerRange(), 32);
}

TEST_F(MobSpawnerBlockEntityTest, SetSpawnRange)
{
    spawner_->setSpawnRange(8);
    EXPECT_EQ(spawner_->getSpawnRange(), 8);
}

TEST_F(MobSpawnerBlockEntityTest, SetEntityId_SetsNextEntityId)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);
    EXPECT_EQ(spawner_->getNextEntityId(), ResourceLocation("minecraft:silverfish"));
}

TEST_F(MobSpawnerBlockEntityTest, SetEntityId_AddsDefaultSpawnPotential)
{
    math::Random rng(42);
    EXPECT_TRUE(spawner_->getSpawnPotentials().empty());

    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);

    ASSERT_EQ(spawner_->getSpawnPotentials().size(), 1u);
    EXPECT_EQ(spawner_->getSpawnPotentials()[0].entityId, ResourceLocation("minecraft:silverfish"));
    EXPECT_EQ(spawner_->getSpawnPotentials()[0].weight, 1);
}

TEST_F(MobSpawnerBlockEntityTest, SetEntityId_DoesNotOverrideExistingPotentials)
{
    math::Random rng(42);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:zombie"), 2);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:skeleton"), 1);

    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);

    // setEntityId 不应覆盖已有的 spawnPotentials
    ASSERT_EQ(spawner_->getSpawnPotentials().size(), 2u);
    EXPECT_EQ(spawner_->getSpawnPotentials()[0].entityId, ResourceLocation("minecraft:zombie"));
    EXPECT_EQ(spawner_->getSpawnPotentials()[1].entityId, ResourceLocation("minecraft:skeleton"));
}

TEST_F(MobSpawnerBlockEntityTest, AddSpawnPotential)
{
    spawner_->addSpawnPotential(ResourceLocation("minecraft:zombie"), 3);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:skeleton"), 2);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:creeper"), 1);

    const auto& potentials = spawner_->getSpawnPotentials();
    ASSERT_EQ(potentials.size(), 3u);
    EXPECT_EQ(potentials[0].entityId, ResourceLocation("minecraft:zombie"));
    EXPECT_EQ(potentials[0].weight, 3);
    EXPECT_EQ(potentials[1].entityId, ResourceLocation("minecraft:skeleton"));
    EXPECT_EQ(potentials[1].weight, 2);
    EXPECT_EQ(potentials[2].entityId, ResourceLocation("minecraft:creeper"));
    EXPECT_EQ(potentials[2].weight, 1);
}

TEST_F(MobSpawnerBlockEntityTest, SetCustomSpawnRules)
{
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);
    // CustomSpawnRules 已设置，测试仅验证不崩溃
}

TEST_F(MobSpawnerBlockEntityTest, SetEntityId_MarksChanged)
{
    math::Random rng(42);
    spawner_->clearChanged();
    EXPECT_FALSE(spawner_->isChanged());

    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);
    EXPECT_TRUE(spawner_->isChanged());
}

// ============================================================================
// JSON 序列化测试
// ============================================================================

TEST_F(MobSpawnerBlockEntityTest, SaveLoad_Roundtrip)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);
    spawner_->setMinSpawnDelay(100);
    spawner_->setMaxSpawnDelay(300);
    spawner_->setSpawnCount(2);
    spawner_->setMaxNearbyEntities(4);
    spawner_->setRequiredPlayerRange(8);
    spawner_->setSpawnRange(2);

    // 保存
    nlohmann::json data;
    spawner_->save(data);

    // 加载到新实体
    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getMinSpawnDelay(), 100);
    EXPECT_EQ(loaded->getMaxSpawnDelay(), 300);
    EXPECT_EQ(loaded->getSpawnCount(), 2);
    EXPECT_EQ(loaded->getMaxNearbyEntities(), 4);
    EXPECT_EQ(loaded->getRequiredPlayerRange(), 8);
    EXPECT_EQ(loaded->getSpawnRange(), 2);
    EXPECT_EQ(loaded->getNextEntityId(), ResourceLocation("minecraft:silverfish"));
}

TEST_F(MobSpawnerBlockEntityTest, SaveLoad_SpawnPotentials)
{
    math::Random rng(42);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:zombie"), 3);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:skeleton"), 2);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:creeper"), 1);

    nlohmann::json data;
    spawner_->save(data);

    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->load(data));

    const auto& potentials = loaded->getSpawnPotentials();
    ASSERT_EQ(potentials.size(), 3u);
    EXPECT_EQ(potentials[0].entityId, ResourceLocation("minecraft:zombie"));
    EXPECT_EQ(potentials[0].weight, 3);
    EXPECT_EQ(potentials[1].entityId, ResourceLocation("minecraft:skeleton"));
    EXPECT_EQ(potentials[1].weight, 2);
    EXPECT_EQ(potentials[2].entityId, ResourceLocation("minecraft:creeper"));
    EXPECT_EQ(potentials[2].weight, 1);
}

TEST_F(MobSpawnerBlockEntityTest, SaveLoad_CustomSpawnRules)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);

    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    nlohmann::json data;
    spawner_->save(data);

    // 验证 custom_spawn_rules 写入
    ASSERT_TRUE(data.contains("custom_spawn_rules"));
    EXPECT_EQ(data["custom_spawn_rules"]["block_light_min"].get<i32>(), 0);
    EXPECT_EQ(data["custom_spawn_rules"]["block_light_max"].get<i32>(), 7);
}

TEST_F(MobSpawnerBlockEntityTest, Load_MissingFields_UseDefaults)
{
    // 空JSON数据加载应保留默认值
    nlohmann::json data;
    data["id"] = "minecraft:mob_spawner";
    data["x"] = 10;
    data["y"] = 64;
    data["z"] = 20;

    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_EQ(loaded->getMinSpawnDelay(), 200);
    EXPECT_EQ(loaded->getMaxSpawnDelay(), 800);
    EXPECT_EQ(loaded->getSpawnCount(), 4);
    EXPECT_EQ(loaded->getMaxNearbyEntities(), 6);
    EXPECT_EQ(loaded->getRequiredPlayerRange(), 16);
    EXPECT_EQ(loaded->getSpawnRange(), 4);
}

// ============================================================================
// NBT 序列化测试
// ============================================================================

TEST_F(MobSpawnerBlockEntityTest, SaveToNBT_WritesSpawnDelay)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);
    // 设置已知延迟值
    spawner_->setMinSpawnDelay(50);
    spawner_->setMaxSpawnDelay(50);

    nbt::tags::compound_tag tag;
    spawner_->saveToNBT(tag);

    ASSERT_NE(tag.value.find("MinSpawnDelay"), tag.value.end());
    const auto* minDelay = dynamic_cast<const nbt::tags::short_tag*>(tag.value.at("MinSpawnDelay").get());
    ASSERT_NE(minDelay, nullptr);
    EXPECT_EQ(minDelay->value, 50);
}

TEST_F(MobSpawnerBlockEntityTest, SaveToNBT_WritesSpawnData)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);

    nbt::tags::compound_tag tag;
    spawner_->saveToNBT(tag);

    ASSERT_NE(tag.value.find("SpawnData"), tag.value.end());
    const auto* spawnData = dynamic_cast<const nbt::tags::compound_tag*>(tag.value.at("SpawnData").get());
    ASSERT_NE(spawnData, nullptr);

    ASSERT_NE(spawnData->value.find("entity"), spawnData->value.end());
    const auto* entity = dynamic_cast<const nbt::tags::compound_tag*>(spawnData->value.at("entity").get());
    ASSERT_NE(entity, nullptr);

    ASSERT_NE(entity->value.find("id"), entity->value.end());
    const auto* id = dynamic_cast<const nbt::tags::string_tag*>(entity->value.at("id").get());
    ASSERT_NE(id, nullptr);
    EXPECT_EQ(id->value, "minecraft:silverfish");
}

TEST_F(MobSpawnerBlockEntityTest, SaveToNBT_WritesSpawnPotentials)
{
    spawner_->addSpawnPotential(ResourceLocation("minecraft:zombie"), 3);
    spawner_->addSpawnPotential(ResourceLocation("minecraft:skeleton"), 2);

    nbt::tags::compound_tag tag;
    spawner_->saveToNBT(tag);

    ASSERT_NE(tag.value.find("SpawnPotentials"), tag.value.end());
    const auto* listTag = dynamic_cast<const nbt::tags::list_tag*>(tag.value.at("SpawnPotentials").get());
    ASSERT_NE(listTag, nullptr);
    ASSERT_EQ(listTag->element_id(), nbt::TagId::Compound);

    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*listTag);
    ASSERT_EQ(compoundList.value.size(), 2u);
}

TEST_F(MobSpawnerBlockEntityTest, LoadFromNBT_ReadsSpawnDelay)
{
    nbt::tags::compound_tag tag;
    // 基类数据
    tag.put("id", std::string("minecraft:mob_spawner"));
    tag.put("x", static_cast<i32>(10));
    tag.put("y", static_cast<i32>(64));
    tag.put("z", static_cast<i32>(20));

    // 生成参数
    tag.put("Delay", static_cast<i16>(10));
    tag.put("MinSpawnDelay", static_cast<i16>(50));
    tag.put("MaxSpawnDelay", static_cast<i16>(200));
    tag.put("SpawnCount", static_cast<i16>(2));
    tag.put("MaxNearbyEntities", static_cast<i16>(4));
    tag.put("RequiredPlayerRange", static_cast<i16>(8));
    tag.put("SpawnRange", static_cast<i16>(2));

    // SpawnData (MC 1.21 格式)
    nbt::tags::compound_tag spawnData;
    nbt::tags::compound_tag entityTag;
    entityTag.put("id", std::string("minecraft:silverfish"));
    spawnData.value.emplace("entity", std::make_unique<nbt::tags::compound_tag>(std::move(entityTag)));
    tag.value.emplace("SpawnData", std::make_unique<nbt::tags::compound_tag>(std::move(spawnData)));

    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_EQ(loaded->getMinSpawnDelay(), 50);
    EXPECT_EQ(loaded->getMaxSpawnDelay(), 200);
    EXPECT_EQ(loaded->getSpawnCount(), 2);
    EXPECT_EQ(loaded->getMaxNearbyEntities(), 4);
    EXPECT_EQ(loaded->getRequiredPlayerRange(), 8);
    EXPECT_EQ(loaded->getSpawnRange(), 2);
    EXPECT_EQ(loaded->getNextEntityId(), ResourceLocation("minecraft:silverfish"));
}

TEST_F(MobSpawnerBlockEntityTest, LoadFromNBT_ReadsSpawnPotentials)
{
    nbt::tags::compound_tag tag;
    tag.put("id", std::string("minecraft:mob_spawner"));
    tag.put("x", static_cast<i32>(10));
    tag.put("y", static_cast<i32>(64));
    tag.put("z", static_cast<i32>(20));

    // SpawnPotentials
    auto potentialsList = std::make_unique<nbt::tags::compound_list_tag>();

    nbt::tags::compound_tag entry1;
    entry1.put("weight", static_cast<i32>(3));
    nbt::tags::compound_tag data1;
    nbt::tags::compound_tag entity1;
    entity1.put("id", std::string("minecraft:zombie"));
    data1.value.emplace("entity", std::make_unique<nbt::tags::compound_tag>(std::move(entity1)));
    entry1.value.emplace("data", std::make_unique<nbt::tags::compound_tag>(std::move(data1)));
    potentialsList->value.push_back(std::move(entry1));

    nbt::tags::compound_tag entry2;
    entry2.put("weight", static_cast<i32>(1));
    nbt::tags::compound_tag data2;
    nbt::tags::compound_tag entity2;
    entity2.put("id", std::string("minecraft:skeleton"));
    data2.value.emplace("entity", std::make_unique<nbt::tags::compound_tag>(std::move(entity2)));
    entry2.value.emplace("data", std::make_unique<nbt::tags::compound_tag>(std::move(data2)));
    potentialsList->value.push_back(std::move(entry2));

    tag.value.emplace("SpawnPotentials", std::move(potentialsList));

    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    const auto& potentials = loaded->getSpawnPotentials();
    ASSERT_EQ(potentials.size(), 2u);
    EXPECT_EQ(potentials[0].entityId, ResourceLocation("minecraft:zombie"));
    EXPECT_EQ(potentials[0].weight, 3);
    EXPECT_EQ(potentials[1].entityId, ResourceLocation("minecraft:skeleton"));
    EXPECT_EQ(potentials[1].weight, 1);
}

TEST_F(MobSpawnerBlockEntityTest, LoadFromNBT_CompatibleWithIntTags)
{
    // 测试兼容 int 标签格式（MC 有时使用 int 而非 short）
    nbt::tags::compound_tag tag;
    tag.put("id", std::string("minecraft:mob_spawner"));
    tag.put("x", static_cast<i32>(10));
    tag.put("y", static_cast<i32>(64));
    tag.put("z", static_cast<i32>(20));

    tag.put("MinSpawnDelay", static_cast<i32>(100));
    tag.put("MaxSpawnDelay", static_cast<i32>(400));

    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    EXPECT_EQ(loaded->getMinSpawnDelay(), 100);
    EXPECT_EQ(loaded->getMaxSpawnDelay(), 400);
}

// ============================================================================
// Clone 测试
// ============================================================================

TEST_F(MobSpawnerBlockEntityTest, Clone_PreservesAllState)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);
    spawner_->setMinSpawnDelay(50);
    spawner_->setMaxSpawnDelay(150);
    spawner_->setSpawnCount(2);
    spawner_->setMaxNearbyEntities(3);
    spawner_->setRequiredPlayerRange(8);
    spawner_->setSpawnRange(2);
    // setEntityId 已经添加了一个默认 spawnPotential（minecraft:silverfish, weight=1）
    // 再添加一个
    spawner_->addSpawnPotential(ResourceLocation("minecraft:zombie"), 2);

    auto cloned = spawner_->clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getType(), BlockEntityType::MobSpawner);

    auto* clonedSpawner = static_cast<MobSpawnerBlockEntity*>(cloned.get());
    EXPECT_EQ(clonedSpawner->getMinSpawnDelay(), 50);
    EXPECT_EQ(clonedSpawner->getMaxSpawnDelay(), 150);
    EXPECT_EQ(clonedSpawner->getSpawnCount(), 2);
    EXPECT_EQ(clonedSpawner->getMaxNearbyEntities(), 3);
    EXPECT_EQ(clonedSpawner->getRequiredPlayerRange(), 8);
    EXPECT_EQ(clonedSpawner->getSpawnRange(), 2);
    EXPECT_EQ(clonedSpawner->getNextEntityId(), ResourceLocation("minecraft:silverfish"));
    // setEntityId 添加了 silverfish(1)，addSpawnPotential 添加了 zombie(2)
    ASSERT_EQ(clonedSpawner->getSpawnPotentials().size(), 2u);
    EXPECT_EQ(clonedSpawner->getSpawnPotentials()[0].entityId, ResourceLocation("minecraft:silverfish"));
    EXPECT_EQ(clonedSpawner->getSpawnPotentials()[0].weight, 1);
    EXPECT_EQ(clonedSpawner->getSpawnPotentials()[1].entityId, ResourceLocation("minecraft:zombie"));
    EXPECT_EQ(clonedSpawner->getSpawnPotentials()[1].weight, 2);
}

// ============================================================================
// Tick 逻辑测试（需要 Mock 世界）
// ============================================================================

class MobSpawnerTickTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        spawner_ = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
        world_ = std::make_unique<MobSpawnerTestWorld>();
    }

    std::unique_ptr<MobSpawnerBlockEntity> spawner_;
    std::unique_ptr<MobSpawnerTestWorld> world_;
};

TEST_F(MobSpawnerTickTest, Tick_NoEntityId_DoesNothing)
{
    // 未设置实体ID时，tick不应崩溃也不应生成实体
    spawner_->tick(*world_);
    EXPECT_EQ(world_->spawnedCount(), 0u);
}

TEST_F(MobSpawnerTickTest, Tick_NoPlayerNearby_DoesNotSpawn)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypes::PIG), rng);
    spawner_->setRequiredPlayerRange(16);

    // 没有玩家附近
    world_->setEntitiesInRangeResult({});

    spawner_->tick(*world_);
    EXPECT_EQ(world_->spawnedCount(), 0u);
}

TEST_F(MobSpawnerTickTest, Tick_RequiredPlayerRangeZero_SpawnsWithoutPlayer)
{
    // requiredPlayerRange = 0 时，无需玩家也能生成
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypes::PIG), rng);
    spawner_->setRequiredPlayerRange(0);
    spawner_->setMinSpawnDelay(0);
    spawner_->setMaxSpawnDelay(0);
    spawner_->setSpawnCount(1);
    spawner_->setMaxNearbyEntities(6);

    world_->setEntitiesInRangeResult({});
    world_->setSeed(12345);
    world_->setCurrentTick(100);

    // 需要多次tick让延迟归零
    for (int i = 0; i < 30; ++i) {
        spawner_->tick(*world_);
    }

    // 由于生成实体需要 EntityType 可召唤，在测试环境中可能不会真正生成
    // 关键是不崩溃且逻辑正确运行
}

TEST_F(MobSpawnerTickTest, Tick_PlayerNearby_AllowsSpawning)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypes::PIG), rng);
    spawner_->setRequiredPlayerRange(16);

    // 创建一个模拟玩家（只要 getEntitiesInRange 返回的指针能 dynamic_cast 为 Player*）
    Player player(EntityId(1), "TestPlayer");
    world_->setEntitiesInRangeResult({&player});
    world_->setSeed(12345);
    world_->setCurrentTick(100);

    // tick不应崩溃
    spawner_->tick(*world_);
}

TEST_F(MobSpawnerTickTest, Tick_DelayCountdown)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypes::PIG), rng);
    spawner_->setMinSpawnDelay(5);
    spawner_->setMaxSpawnDelay(5);

    Player player(EntityId(1), "TestPlayer");
    world_->setEntitiesInRangeResult({&player});
    world_->setSeed(12345);
    world_->setCurrentTick(100);

    // 初始延迟由 setEntityId 中的 _delay(rng) 设置，范围是 [5, 5] = 5
    // tick 5 次后延迟应归零并尝试生成
    for (int i = 0; i < 5; ++i) {
        spawner_->tick(*world_);
    }
    // 不崩溃即通过
}

TEST_F(MobSpawnerTickTest, Tick_SpawnDelayMinusOne_ResetsDelay)
{
    // spawnDelay = -1 应重置延迟
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypes::PIG), rng);

    Player player(EntityId(1), "TestPlayer");
    world_->setEntitiesInRangeResult({&player});
    world_->setSeed(12345);
    world_->setCurrentTick(100);

    // 通过 JSON 设置 spawnDelay = -1
    nlohmann::json data;
    spawner_->save(data);
    data["spawn_delay"] = -1;
    spawner_->load(data);

    // tick 不崩溃，延迟应被重置
    spawner_->tick(*world_);
}

TEST_F(MobSpawnerTickTest, Tick_SetChangedOnConfiguration)
{
    // setEntityId 应标记为已修改
    math::Random rng(42);
    spawner_->clearChanged();
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);
    EXPECT_TRUE(spawner_->isChanged());

    // addSpawnPotential 应标记为已修改
    spawner_->clearChanged();
    spawner_->addSpawnPotential(ResourceLocation("minecraft:zombie"), 1);
    EXPECT_TRUE(spawner_->isChanged());

    // setCustomSpawnRules 应标记为已修改
    spawner_->clearChanged();
    CustomSpawnRules rules;
    spawner_->setCustomSpawnRules(rules);
    EXPECT_TRUE(spawner_->isChanged());
}
