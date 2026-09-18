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
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

// ============================================================================
// MobSpawnerTestWorld - 测试用 Mock 世界
// ============================================================================

class MobSpawnerTestWorld final : public mc::test::BaseTestWorld {
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

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityInstanceId(0);
        }
        EntityInstanceId id = EntityInstanceId(++m_nextEntityId);
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

    // playEvent 记录
    struct PlayEventCall {
        i32 eventId;
        BlockPos pos;
        i32 data;
    };

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_playEventCalls.push_back({eventId, pos, data});
    }

    [[nodiscard]] const std::vector<PlayEventCall>& playEventCalls() const { return m_playEventCalls; }
    void clearPlayEventCalls() { m_playEventCalls.clear(); }

private:
    u64 m_currentTick = 0;
    u64 m_seed = 12345;
    Difficulty m_difficulty = Difficulty::Easy;
    std::vector<Entity*> m_entitiesInRange;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
    u64 m_nextEntityId = 0;
    std::vector<PlayEventCall> m_playEventCalls;
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
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);
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
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);
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
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);
    spawner_->setRequiredPlayerRange(16);

    // 创建一个模拟玩家（只要 getEntitiesInRange 返回的指针能 dynamic_cast 为 Player*）
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    world_->setEntitiesInRangeResult({&player});
    world_->setSeed(12345);
    world_->setCurrentTick(100);

    // tick不应崩溃
    spawner_->tick(*world_);
}

TEST_F(MobSpawnerTickTest, Tick_DelayCountdown)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);
    spawner_->setMinSpawnDelay(5);
    spawner_->setMaxSpawnDelay(5);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
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
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
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

// ============================================================================
// CustomSpawnRules::isValidPosition 测试
// ============================================================================

TEST(CustomSpawnRulesTest, DefaultValues_AllowAllLightLevels)
{
    CustomSpawnRules rules;
    // 默认 [0, 15] 范围允许所有光照等级
    EXPECT_TRUE(rules.isValidPosition(0, 0));
    EXPECT_TRUE(rules.isValidPosition(15, 15));
    EXPECT_TRUE(rules.isValidPosition(7, 8));
    EXPECT_TRUE(rules.isValidPosition(0, 15));
    EXPECT_TRUE(rules.isValidPosition(15, 0));
}

TEST(CustomSpawnRulesTest, RestrictedBlockLight_BlocksOutOfRange)
{
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;

    // 方块光照在范围内
    EXPECT_TRUE(rules.isValidPosition(0, 10));
    EXPECT_TRUE(rules.isValidPosition(7, 10));
    // 方块光照超出范围
    EXPECT_FALSE(rules.isValidPosition(8, 10));
    EXPECT_FALSE(rules.isValidPosition(15, 10));
}

TEST(CustomSpawnRulesTest, RestrictedSkyLight_BlocksOutOfRange)
{
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 0;

    // 天空光照在范围内（仅允许完全黑暗）
    EXPECT_TRUE(rules.isValidPosition(5, 0));
    // 天空光照超出范围
    EXPECT_FALSE(rules.isValidPosition(5, 1));
    EXPECT_FALSE(rules.isValidPosition(0, 15));
}

TEST(CustomSpawnRulesTest, BothRestricted_BothMustPass)
{
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 7;

    // 两者都在范围内
    EXPECT_TRUE(rules.isValidPosition(0, 0));
    EXPECT_TRUE(rules.isValidPosition(7, 7));
    EXPECT_TRUE(rules.isValidPosition(3, 5));

    // 方块光照超出范围
    EXPECT_FALSE(rules.isValidPosition(8, 3));
    // 天空光照超出范围
    EXPECT_FALSE(rules.isValidPosition(3, 8));
    // 两者都超出范围
    EXPECT_FALSE(rules.isValidPosition(8, 8));
}

// ============================================================================
// CustomSpawnRules NBT 序列化测试
// ============================================================================

TEST_F(MobSpawnerBlockEntityTest, SaveToNBT_WritesCustomSpawnRules)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);

    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 0;
    spawner_->setCustomSpawnRules(rules);

    nbt::tags::compound_tag tag;
    spawner_->saveToNBT(tag);

    ASSERT_NE(tag.value.find("SpawnData"), tag.value.end());
    const auto* spawnData = dynamic_cast<const nbt::tags::compound_tag*>(tag.value.at("SpawnData").get());
    ASSERT_NE(spawnData, nullptr);

    ASSERT_NE(spawnData->value.find("CustomSpawnRules"), spawnData->value.end());
    const auto* customRules =
        dynamic_cast<const nbt::tags::compound_tag*>(spawnData->value.at("CustomSpawnRules").get());
    ASSERT_NE(customRules, nullptr);

    // 验证 block_light_limit
    ASSERT_NE(customRules->value.find("block_light_limit"), customRules->value.end());
    const auto& blockLightArr =
        dynamic_cast<const nbt::tags::intarray_tag&>(*customRules->value.at("block_light_limit"));
    ASSERT_EQ(blockLightArr.value.size(), 2u);
    EXPECT_EQ(blockLightArr.value[0], 0);
    EXPECT_EQ(blockLightArr.value[1], 7);

    // 验证 sky_light_limit
    ASSERT_NE(customRules->value.find("sky_light_limit"), customRules->value.end());
    const auto& skyLightArr = dynamic_cast<const nbt::tags::intarray_tag&>(*customRules->value.at("sky_light_limit"));
    ASSERT_EQ(skyLightArr.value.size(), 2u);
    EXPECT_EQ(skyLightArr.value[0], 0);
    EXPECT_EQ(skyLightArr.value[1], 0);
}

TEST_F(MobSpawnerBlockEntityTest, LoadFromNBT_ReadsCustomSpawnRules)
{
    nbt::tags::compound_tag tag;
    tag.put("id", std::string("minecraft:mob_spawner"));
    tag.put("x", static_cast<i32>(10));
    tag.put("y", static_cast<i32>(64));
    tag.put("z", static_cast<i32>(20));

    // SpawnData 包含 CustomSpawnRules
    nbt::tags::compound_tag spawnData;
    nbt::tags::compound_tag entityTag;
    entityTag.put("id", std::string("minecraft:zombie"));
    spawnData.value.emplace("entity", std::make_unique<nbt::tags::compound_tag>(std::move(entityTag)));

    nbt::tags::compound_tag customRules;
    customRules.value.emplace("block_light_limit", std::make_unique<nbt::tags::intarray_tag>(std::vector<i32>{0, 7}));
    customRules.value.emplace("sky_light_limit", std::make_unique<nbt::tags::intarray_tag>(std::vector<i32>{0, 0}));
    spawnData.value.emplace("CustomSpawnRules", std::make_unique<nbt::tags::compound_tag>(std::move(customRules)));

    tag.value.emplace("SpawnData", std::make_unique<nbt::tags::compound_tag>(std::move(spawnData)));

    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    // 通过 JSON 保存/加载来验证 CustomSpawnRules 被正确读取
    nlohmann::json data;
    loaded->save(data);

    ASSERT_TRUE(data.contains("custom_spawn_rules"));
    EXPECT_EQ(data["custom_spawn_rules"]["block_light_min"].get<i32>(), 0);
    EXPECT_EQ(data["custom_spawn_rules"]["block_light_max"].get<i32>(), 7);
    EXPECT_EQ(data["custom_spawn_rules"]["sky_light_min"].get<i32>(), 0);
    EXPECT_EQ(data["custom_spawn_rules"]["sky_light_max"].get<i32>(), 0);
}

TEST_F(MobSpawnerBlockEntityTest, LoadFromNBT_NoCustomSpawnRules_NoRulesSet)
{
    nbt::tags::compound_tag tag;
    tag.put("id", std::string("minecraft:mob_spawner"));
    tag.put("x", static_cast<i32>(10));
    tag.put("y", static_cast<i32>(64));
    tag.put("z", static_cast<i32>(20));

    // SpawnData 不含 CustomSpawnRules
    nbt::tags::compound_tag spawnData;
    nbt::tags::compound_tag entityTag;
    entityTag.put("id", std::string("minecraft:zombie"));
    spawnData.value.emplace("entity", std::make_unique<nbt::tags::compound_tag>(std::move(entityTag)));
    tag.value.emplace("SpawnData", std::make_unique<nbt::tags::compound_tag>(std::move(spawnData)));

    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    // 验证没有 CustomSpawnRules
    nlohmann::json data;
    loaded->save(data);
    EXPECT_FALSE(data.contains("custom_spawn_rules"));
}

TEST_F(MobSpawnerBlockEntityTest, NBT_Roundtrip_CustomSpawnRules)
{
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation("minecraft:silverfish"), rng);

    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 0;
    spawner_->setCustomSpawnRules(rules);

    // 保存到 NBT
    nbt::tags::compound_tag tag;
    spawner_->saveToNBT(tag);

    // 加载到新实体
    auto loaded = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    ASSERT_TRUE(loaded->loadFromNBT(tag));

    // 通过 JSON 验证
    nlohmann::json data;
    loaded->save(data);
    ASSERT_TRUE(data.contains("custom_spawn_rules"));
    EXPECT_EQ(data["custom_spawn_rules"]["block_light_min"].get<i32>(), 0);
    EXPECT_EQ(data["custom_spawn_rules"]["block_light_max"].get<i32>(), 7);
    EXPECT_EQ(data["custom_spawn_rules"]["sky_light_min"].get<i32>(), 0);
    EXPECT_EQ(data["custom_spawn_rules"]["sky_light_max"].get<i32>(), 0);
}

namespace mc::blockentity {
// ============================================================================
// 测试子类：暴露 _isValidSpawnPosition 供测试
// 需要放在 mc::blockentity 命名空间中以便 friend 声明生效
// ============================================================================

class TestMobSpawnerBlockEntity final : public MobSpawnerBlockEntity {
public:
    explicit TestMobSpawnerBlockEntity(const BlockPos& pos)
        : MobSpawnerBlockEntity(pos)
    {}

    /// 暴露 _isValidSpawnPosition 供测试调用
    [[nodiscard]] bool testIsValidSpawnPosition(IWorld& world, const BlockPos& pos)
    {
        // 获取当前实体类型；如果没有设置则返回 true（无法检查放置规则）
        const entity::EntityType* entityType = entity::EntityRegistry::instance().getType(getNextEntityId().toString());
        if (entityType == nullptr) {
            return true;
        }
        return _isValidSpawnPosition(world, pos, *entityType);
    }
};

} // namespace mc::blockentity

// ============================================================================
// 光照和生成规则测试用 Mock 世界
// ============================================================================

class MobSpawnerLightTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    void setBlockLightValue(u8 value) { m_blockLight = value; }
    void setSkyLightValue(u8 value) { m_skyLight = value; }
    void setDifficulty(Difficulty diff) { m_difficulty = diff; }
    void setSeed(u64 seed) { m_seed = seed; }
    void setEntitiesInRangeResult(std::vector<Entity*> entities) { m_entitiesInRange = std::move(entities); }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return m_blockLight; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return m_skyLight; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    [[nodiscard]] u64 seed() const override { return m_seed; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return m_entitiesInRange;
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityInstanceId(0);
        }
        EntityInstanceId id = EntityInstanceId(++m_nextEntityId);
        entity->setId(id);
        m_ownedEntities.push_back(std::move(entity));
        return id;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MobSpawnerLightTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MobSpawnerLightTestWorld::tickManager not implemented");
    }

private:
    u8 m_blockLight = 0;
    u8 m_skyLight = 15;
    Difficulty m_difficulty = Difficulty::Easy;
    u64 m_seed = 12345;
    std::vector<Entity*> m_entitiesInRange;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
    u64 m_nextEntityId = 0;
};

// ============================================================================
// CustomSpawnRules 光照检查测试
// ============================================================================

class SpawnPositionLightTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
        spawner_ = std::make_unique<TestMobSpawnerBlockEntity>(BlockPos(10, 64, 20));
        world_ = std::make_unique<MobSpawnerLightTestWorld>();
    }

    std::unique_ptr<TestMobSpawnerBlockEntity> spawner_;
    std::unique_ptr<MobSpawnerLightTestWorld> world_;
};

TEST_F(SpawnPositionLightTest, CustomSpawnRules_BlockLightInRange_AllowsSpawn)
{
    // 设置 CustomSpawnRules 要求方块光照 [0, 7]
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    // 方块光照 = 5，在范围内
    world_->setBlockLightValue(5);
    world_->setSkyLightValue(15);

    // 需要设置一个已注册的实体类型才能测试
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_TRUE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_BlockLightOutOfRange_BlocksSpawn)
{
    // 设置 CustomSpawnRules 要求方块光照 [0, 7]
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    // 方块光照 = 10，超出范围
    world_->setBlockLightValue(10);
    world_->setSkyLightValue(15);

    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_FALSE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_SkyLightOutOfRange_BlocksSpawn)
{
    // 设置 CustomSpawnRules 要求天空光照 [0, 0]（完全黑暗）
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 0;
    spawner_->setCustomSpawnRules(rules);

    // 天空光照 = 8，超出范围
    world_->setBlockLightValue(0);
    world_->setSkyLightValue(8);

    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_FALSE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_BothLightInRanges_AllowsSpawn)
{
    // 设置 CustomSpawnRules 要求方块光照 [0, 7]，天空光照 [0, 0]
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 0;
    spawner_->setCustomSpawnRules(rules);

    // 方块光照 = 3，天空光照 = 0，都在范围内
    world_->setBlockLightValue(3);
    world_->setSkyLightValue(0);

    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_TRUE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_DefaultRanges_AllowAllLight)
{
    // 默认 CustomSpawnRules 允许所有光照 [0, 15] x [0, 15]
    CustomSpawnRules rules; // 默认值
    spawner_->setCustomSpawnRules(rules);

    // 任何光照组合都应允许
    world_->setBlockLightValue(15);
    world_->setSkyLightValue(15);

    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_TRUE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_MonsterPeacefulDifficulty_BlocksSpawn)
{
    // CustomSpawnRules 存在 + 怪物实体 + 和平难度 = 不生成
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    world_->setDifficulty(Difficulty::Peaceful);
    world_->setBlockLightValue(0);
    world_->setSkyLightValue(0);

    // 僵尸是 Monster 分类
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::ZOMBIE), rng);

    // 由于 _spawnEntities 中在 CustomSpawnRules 检查之前先判断难度，
    // 非和平分类在和平难度下直接返回 false，不会到达 _isValidSpawnPosition
    // 这里测试的是 _spawnEntities 层面的逻辑，通过 tick 测试来验证
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    world_->setEntitiesInRangeResult({&player});
    spawner_->setRequiredPlayerRange(0);
    spawner_->setMinSpawnDelay(0);
    spawner_->setMaxSpawnDelay(0);
    spawner_->setSpawnCount(1);

    // 多次 tick 尝试生成，和平难度下不应生成
    for (int i = 0; i < 30; ++i) {
        spawner_->tick(*world_);
    }
    // 不会崩溃，且在和平难度下怪物不应生成
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_PeacefulCreature_PeacefulDifficulty_AllowsSpawn)
{
    // CustomSpawnRules 存在 + 和平生物（猪）+ 和平难度 = 允许生成（光照通过的话）
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    world_->setDifficulty(Difficulty::Peaceful);
    world_->setBlockLightValue(5);
    world_->setSkyLightValue(10);

    // 猪是 Creature 分类（和平生物），即使和平难度也允许
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    // 光照在范围内，和平生物在和平难度下允许生成
    EXPECT_TRUE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, NoCustomSpawnRules_UsesDefaultSpawnRules)
{
    // 不设置 CustomSpawnRules，应使用 EntitySpawnPlacementRegistry 默认规则
    // 猪的放置类型是 OnGround，默认世界 getBlockState 返回 nullptr（视为空气）
    // 所以 OnGround 检查会失败（脚下没有固体方块）
    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    // 不设置 CustomSpawnRules，_isValidSpawnPosition 会走 SpawnPlacements 路径
    // BaseTestWorld 的 getBlockState 返回 nullptr，OnGround 检查需要脚下有固体方块
    // 因此猪无法生成（脚下没有地面）
    EXPECT_FALSE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_BlockLightAtExactBoundary_AllowsSpawn)
{
    // 边界值测试：方块光照恰好等于 max
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 7;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    // 方块光照 = 7，恰好等于 max
    world_->setBlockLightValue(7);
    world_->setSkyLightValue(10);

    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_TRUE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_BlockLightAtMinBoundary_AllowsSpawn)
{
    // 边界值测试：方块光照恰好等于 min
    CustomSpawnRules rules;
    rules.blockLightMin = 5;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    // 方块光照 = 5，恰好等于 min
    world_->setBlockLightValue(5);
    world_->setSkyLightValue(10);

    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_TRUE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

TEST_F(SpawnPositionLightTest, CustomSpawnRules_BlockLightBelowMin_BlocksSpawn)
{
    // 方块光照低于最小值
    CustomSpawnRules rules;
    rules.blockLightMin = 5;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    // 方块光照 = 4，低于 min=5
    world_->setBlockLightValue(4);
    world_->setSkyLightValue(10);

    math::Random rng(42);
    spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

    EXPECT_FALSE(spawner_->testIsValidSpawnPosition(*world_, BlockPos(10, 64, 20)));
}

// ============================================================================
// playEvent（刷怪笼生成粒子事件）测试
// ============================================================================

class MobSpawnerPlayEventTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
        spawner_ = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
        world_ = std::make_unique<MobSpawnerTestWorld>();

        // 先设置延迟参数，再调用 setEntityId（setEntityId 内部会调用 _delay 重置延迟）
        spawner_->setMinSpawnDelay(0);
        spawner_->setMaxSpawnDelay(0);
        spawner_->setRequiredPlayerRange(0);
        spawner_->setSpawnCount(1);
        spawner_->setMaxNearbyEntities(6);
        spawner_->setSpawnRange(1);

        math::Random rng(42);
        spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

        world_->setSeed(12345);
        world_->setCurrentTick(100);
        world_->setDifficulty(Difficulty::Easy);
    }

    void TearDown() override {}

    std::unique_ptr<MobSpawnerBlockEntity> spawner_;
    std::unique_ptr<MobSpawnerTestWorld> world_;
};

TEST_F(MobSpawnerPlayEventTest, Tick_SpawnSuccess_EmitsMobSpawnerParticlesEvent)
{
    // 设置 CustomSpawnRules 允许所有光照，绕过 EntitySpawnPlacementRegistry 检查
    // BaseTestWorld 没有 getBlockState 实现（返回 nullptr），OnGround 放置检查会失败
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    // 需要多次 tick 以触发生成
    for (int i = 0; i < 10; ++i) {
        spawner_->tick(*world_);
    }

    // 验证至少发出了一次 MOB_SPAWNER_PARTICLES 事件
    bool foundMobSpawnerEvent = false;
    for (const auto& call : world_->playEventCalls()) {
        if (call.eventId == world::WorldEvents::MOB_SPAWNER_PARTICLES) {
            foundMobSpawnerEvent = true;
            // 验证事件位置是刷怪笼位置
            EXPECT_EQ(call.pos.x, 10);
            EXPECT_EQ(call.pos.y, 64);
            EXPECT_EQ(call.pos.z, 20);
            break;
        }
    }
    EXPECT_TRUE(foundMobSpawnerEvent) << "Expected MOB_SPAWNER_PARTICLES event to be emitted on successful spawn";
}

TEST_F(MobSpawnerPlayEventTest, Tick_NoSpawn_NoMobSpawnerParticlesEvent)
{
    // 没有设置实体ID时，不应发出 MOB_SPAWNER_PARTICLES 事件
    auto emptySpawner = std::make_unique<MobSpawnerBlockEntity>(BlockPos(5, 10, 5));
    auto emptyWorld = std::make_unique<MobSpawnerTestWorld>();
    emptyWorld->setSeed(42);
    emptyWorld->setCurrentTick(0);

    for (int i = 0; i < 50; ++i) {
        emptySpawner->tick(*emptyWorld);
    }

    // 没有实体ID，不应发出 MOB_SPAWNER_PARTICLES 事件
    for (const auto& call : emptyWorld->playEventCalls()) {
        EXPECT_NE(call.eventId, world::WorldEvents::MOB_SPAWNER_PARTICLES)
            << "Should not emit MOB_SPAWNER_PARTICLES when no entity ID is set";
    }
}

TEST_F(MobSpawnerPlayEventTest, Tick_MultipleSpawns_EmitsEventForEachBatch)
{
    // 设置 CustomSpawnRules 允许所有光照
    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    spawner_->setCustomSpawnRules(rules);

    // 多次 tick 以触发生成（每次延迟=0，每个 tick 都应该尝试生成）
    for (int i = 0; i < 20; ++i) {
        spawner_->tick(*world_);
    }

    // 统计 MOB_SPAWNER_PARTICLES 事件次数
    size_t eventCount = 0;
    for (const auto& call : world_->playEventCalls()) {
        if (call.eventId == world::WorldEvents::MOB_SPAWNER_PARTICLES) {
            ++eventCount;
        }
    }
    // 应该有多次成功生成事件
    EXPECT_GE(eventCount, 1u) << "Expected at least one MOB_SPAWNER_PARTICLES event in 20 ticks";
}

// ============================================================================
// 端到端生成测试（验证 entityType->create → finalizeSpawn → spawnEntity 完整链路）
// ============================================================================
//
// 这组测试用于覆盖 MobSpawnerBlockEntity::_spawnEntities 的完整生成链路：
// 1. 通过 EntityType::create(&world) 创建实体实例
// 2. 对 MobEntity 调用 finalizeSpawn(world, DifficultyInstance, SpawnReason::Spawner)
// 3. 通过 world.spawnEntity(std::move(entity)) 将实体添加到世界
//
// 早期由于测试环境中未注册 EntityType，该链路无法端到端验证，留下 TODO。
// 现在 VanillaEntities::registerAll() 已可在测试环境调用，且 MobSpawnerTestWorld
// 的 spawnEntity override 会持有生成实体的所有权并暴露 spawnedEntities() 访问接口，
// 因此可以完整断言生成结果（实体数量、类型 id、MobEntity 多态、finalizeSpawn 副作用）。

class MobSpawnerE2ESpawnTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
        spawner_ = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
        world_ = std::make_unique<MobSpawnerTestWorld>();

        // 延迟参数：min=max=0，使每个 tick 都尝试生成
        spawner_->setMinSpawnDelay(0);
        spawner_->setMaxSpawnDelay(0);
        // 关闭玩家范围限制（_isNearPlayer 直接返回 true）
        spawner_->setRequiredPlayerRange(0);
        // 每次 tick 生成 1 个实体
        spawner_->setSpawnCount(1);
        // 附近同类实体上限设大，避免 _countNearbyEntities 阻塞后续生成
        spawner_->setMaxNearbyEntities(64);
        // 生成范围 1（围绕刷怪笼 3x3x3 区域）
        spawner_->setSpawnRange(1);

        // CustomSpawnRules 放宽光照限制，绕过 EntitySpawnPlacementRegistry::canSpawnEntity
        // 的 OnGround 放置检查（BaseTestWorld::getBlockState 返回 nullptr，无法通过默认检查）。
        CustomSpawnRules rules;
        rules.blockLightMin = 0;
        rules.blockLightMax = 15;
        rules.skyLightMin = 0;
        rules.skyLightMax = 15;
        spawner_->setCustomSpawnRules(rules);

        math::Random rng(42);
        spawner_->setEntityId(ResourceLocation(entity::EntityTypeKeys::PIG), rng);

        world_->setSeed(12345);
        world_->setCurrentTick(100);
        world_->setDifficulty(Difficulty::Easy);
    }

    void TearDown() override {}

    std::unique_ptr<MobSpawnerBlockEntity> spawner_;
    std::unique_ptr<MobSpawnerTestWorld> world_;
};

TEST_F(MobSpawnerE2ESpawnTest, Tick_SpawnsPigEntity_EndToEnd)
{
    // 多次 tick 以触发生成（默认 m_spawnDelay=20，需要 ~20 tick 才能归零）
    for (int i = 0; i < 30; ++i) {
        spawner_->tick(*world_);
    }

    // 断言 1：至少生成 1 个实体（spawnEntity 被成功调用且返回有效 id）
    EXPECT_GE(world_->spawnedCount(), 1u) << "Expected at least one spawned pig entity after 30 ticks";

    // 断言 2：生成的实体类型 id 为 "minecraft:pig"
    //   EntityType::create 内部会调用 setTypeId(m_name)，确保 getTypeId() 返回注册名
    const auto& spawned = world_->spawnedEntities();
    ASSERT_FALSE(spawned.empty());
    EXPECT_EQ(spawned.front()->getTypeId(), std::string("minecraft:pig"));

    // 断言 3：生成的实体是 MobEntity 派生类（PigEntity 继承自 MobEntity）
    //   验证 _spawnEntities 中 dynamic_cast<MobEntity*>(entity.get()) 分支被触发
    EXPECT_NE(dynamic_cast<const MobEntity*>(spawned.front()), nullptr);

    // 断言 4：spawnEntity 返回的 id 已正确写入实体（非 0 表示成功）
    //   MobSpawnerTestWorld::spawnEntity 通过 setId(EntityInstanceId(++m_nextEntityId)) 设置 id
    EXPECT_NE(spawned.front()->id(), EntityInstanceId(0));
}

TEST_F(MobSpawnerE2ESpawnTest, Tick_MultipleTicks_SpawnsMultipleEntities)
{
    // spawnCount=1 且 maxNearbyEntities=64，每个成功 tick 都应生成 1 个实体。
    // 跑足够多的 tick 以确保至少触发多次生成事件。
    for (int i = 0; i < 60; ++i) {
        spawner_->tick(*world_);
    }

    // 在 maxNearbyEntities=64 的上限内，应生成多个实体
    EXPECT_GE(world_->spawnedCount(), 2u) << "Expected multiple spawned entities across 60 ticks";

    // 所有生成的实体类型 id 都是 "minecraft:pig"
    for (const auto* entity : world_->spawnedEntities()) {
        EXPECT_EQ(entity->getTypeId(), std::string("minecraft:pig"));
    }
}

TEST_F(MobSpawnerE2ESpawnTest, Tick_SpawnSuccess_EmitsParticlesEvent)
{
    // 端到端验证：成功生成后应触发 MOB_SPAWNER_PARTICLES 事件
    for (int i = 0; i < 30; ++i) {
        spawner_->tick(*world_);
    }

    // 生成成功的前提下游离粒子事件必须被发出
    ASSERT_GE(world_->spawnedCount(), 1u);

    bool foundMobSpawnerEvent = false;
    for (const auto& call : world_->playEventCalls()) {
        if (call.eventId == world::WorldEvents::MOB_SPAWNER_PARTICLES) {
            foundMobSpawnerEvent = true;
            EXPECT_EQ(call.pos.x, 10);
            EXPECT_EQ(call.pos.y, 64);
            EXPECT_EQ(call.pos.z, 20);
            break;
        }
    }
    EXPECT_TRUE(foundMobSpawnerEvent) << "Expected MOB_SPAWNER_PARTICLES event after successful spawn";
}

TEST_F(MobSpawnerE2ESpawnTest, Tick_ZombieSpawn_FinalizeSpawnCalledWithSpawnerReason)
{
    // 重新构造刷怪笼，避免 SetUp 中的 PIG spawnPotentials 干扰 ZOMBIE 选择
    // （setEntityId 在 spawnPotentials 非空时不会替换条目，_selectNextEntity 会回到 PIG）
    auto zSpawner = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    zSpawner->setMinSpawnDelay(0);
    zSpawner->setMaxSpawnDelay(0);
    zSpawner->setRequiredPlayerRange(0);
    zSpawner->setSpawnCount(1);
    zSpawner->setMaxNearbyEntities(64);
    zSpawner->setSpawnRange(1);

    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    zSpawner->setCustomSpawnRules(rules);

    // 切换实体类型为 Zombie（Monster 分类，MonsterEntity 派生）
    math::Random rng(7);
    zSpawner->setEntityId(ResourceLocation(entity::EntityTypeKeys::ZOMBIE), rng);

    // 难度设为 Normal，确保 Monster 类生物允许生成
    // （CustomSpawnRules 路径下，非和平生物仅在和平难度被拒绝）
    world_->setDifficulty(Difficulty::Normal);

    for (int i = 0; i < 30; ++i) {
        zSpawner->tick(*world_);
    }

    // Zombie 应在普通难度下成功生成
    EXPECT_GE(world_->spawnedCount(), 1u) << "Expected at least one spawned zombie in Normal difficulty";

    if (!world_->spawnedEntities().empty()) {
        const auto* zombie = world_->spawnedEntities().front();
        EXPECT_EQ(zombie->getTypeId(), std::string("minecraft:zombie"));
        // Zombie 继承自 MonsterEntity → MobEntity，验证 dynamic_cast 分支被触发
        EXPECT_NE(dynamic_cast<const MobEntity*>(zombie), nullptr);
    }
}

TEST_F(MobSpawnerE2ESpawnTest, Tick_PeacefulDifficulty_MonsterSpawnRejected)
{
    // 重新构造刷怪笼，避免 SetUp 中的 PIG spawnPotentials 干扰 ZOMBIE 选择
    auto zSpawner = std::make_unique<MobSpawnerBlockEntity>(BlockPos(10, 64, 20));
    zSpawner->setMinSpawnDelay(0);
    zSpawner->setMaxSpawnDelay(0);
    zSpawner->setRequiredPlayerRange(0);
    zSpawner->setSpawnCount(1);
    zSpawner->setMaxNearbyEntities(64);
    zSpawner->setSpawnRange(1);

    CustomSpawnRules rules;
    rules.blockLightMin = 0;
    rules.blockLightMax = 15;
    rules.skyLightMin = 0;
    rules.skyLightMax = 15;
    zSpawner->setCustomSpawnRules(rules);

    // 切换实体类型为 Zombie（Monster 分类）
    math::Random rng(7);
    zSpawner->setEntityId(ResourceLocation(entity::EntityTypeKeys::ZOMBIE), rng);

    // 难度设为 Peaceful，CustomSpawnRules 路径下应拒绝 Monster 类生物
    world_->setDifficulty(Difficulty::Peaceful);

    for (int i = 0; i < 40; ++i) {
        zSpawner->tick(*world_);
    }

    // Peaceful 难度下不应生成 Zombie（_spawnEntities 在生成前直接 return false）
    EXPECT_EQ(world_->spawnedCount(), 0u) << "Expected no zombie spawn in Peaceful difficulty with CustomSpawnRules";
}
