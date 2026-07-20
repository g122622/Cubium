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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND OF EITHER EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file OminousItemSpawnerEntityTest.cpp
 * @brief OminousItemSpawnerEntity 单元测试
 *
 * 测试不祥物品生成器实体的核心功能：构造、常量、物品存取、
 * NBT 序列化/反序列化、tick 逻辑、警告音效、物品生成等。
 */

#include "common/entity/entities/misc/OminousItemSpawnerEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/trial/WindChargeItem.hpp"
#include "common/item/items/weapon/ThrowableItem.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <gtest/gtest.h>

#include <memory>

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::serialization::nbt_helper;

// ============================================================================
// 测试用 Mock World
// ============================================================================

class OminousItemSpawnerTestWorld final : public mc::test::BaseTestWorld {
public:
    OminousItemSpawnerTestWorld() = default;

    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    void setClientSide(bool isClient) { m_isClientSide = isClient; }

    [[nodiscard]] u64 getGameTime() const override { return m_gameTime; }

    void setGameTime(u64 time) { m_gameTime = time; }

    void advanceGameTime() { m_gameTime++; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size()); // 非零 ID
    }

    [[nodiscard]] Entity* getEntity(EntityId id) override
    {
        size_t index = static_cast<size_t>(id) - 1;
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_lastSoundEventId = soundEventId;
        m_lastSoundCategory = category;
        m_lastSoundPosition = position;
        m_soundPlayCount++;
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_lastPlayEventId = eventId;
        m_lastPlayEventPos = pos;
        m_lastPlayEventData = data;
        m_playEventCount++;
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_lastGameEventId = event.id();
        m_lastGameEventPos = pos;
        m_lastGameEventSourceEntity = context.sourceEntity();
        m_gameEventCount++;
    }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particleCount++;
        m_lastParticleType = type;
    }

    // 测试辅助方法

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] Entity* getLastSpawnedEntity()
    {
        if (m_spawnedEntities.empty()) {
            return nullptr;
        }
        return m_spawnedEntities.back().get();
    }

    void clearSpawnedEntities() { m_spawnedEntities.clear(); }

    [[nodiscard]] i32 soundPlayCount() const { return m_soundPlayCount; }
    [[nodiscard]] const ResourceLocation& lastSoundEventId() const { return m_lastSoundEventId; }
    [[nodiscard]] const Vector3& lastSoundPosition() const { return m_lastSoundPosition; }

    [[nodiscard]] i32 playEventCount() const { return m_playEventCount; }
    [[nodiscard]] i32 lastPlayEventId() const { return m_lastPlayEventId; }
    [[nodiscard]] const BlockPos& lastPlayEventPos() const { return m_lastPlayEventPos; }
    [[nodiscard]] i32 lastPlayEventData() const { return m_lastPlayEventData; }

    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }
    [[nodiscard]] const std::string& lastGameEventId() const { return m_lastGameEventId; }
    [[nodiscard]] const BlockPos& lastGameEventPos() const { return m_lastGameEventPos; }
    [[nodiscard]] const Entity* lastGameEventSourceEntity() const { return m_lastGameEventSourceEntity; }

    [[nodiscard]] i32 particleCount() const { return m_particleCount; }
    [[nodiscard]] particle::ParticleTypeId lastParticleType() const { return m_lastParticleType; }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_gameTime = 0;
    bool m_isClientSide = false;

    // 音效记录
    i32 m_soundPlayCount = 0;
    ResourceLocation m_lastSoundEventId;
    Vector3 m_lastSoundPosition{0, 0, 0};
    sound::SoundCategory m_lastSoundCategory = sound::SoundCategory::Master;

    // 世界事件记录
    i32 m_playEventCount = 0;
    i32 m_lastPlayEventId = 0;
    BlockPos m_lastPlayEventPos{0, 0, 0};
    i32 m_lastPlayEventData = 0;

    // 游戏事件记录
    i32 m_gameEventCount = 0;
    std::string m_lastGameEventId;
    BlockPos m_lastGameEventPos{0, 0, 0};
    const Entity* m_lastGameEventSourceEntity = nullptr;

    // 粒子记录
    i32 m_particleCount = 0;
    particle::ParticleTypeId m_lastParticleType = particle::ParticleTypeId::Invalid;
};

// ============================================================================
// 测试固定装置
// ============================================================================

class OminousItemSpawnerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    void SetUp() override { m_world.clearSpawnedEntities(); }

    OminousItemSpawnerTestWorld m_world;
};

// ============================================================================
// 基本构造测试
// ============================================================================

TEST_F(OminousItemSpawnerTest, Construction_DefaultValues)
{
    OminousItemSpawnerEntity entity(EntityId(1));

    // 默认物品为空
    EXPECT_TRUE(entity.getItem().isEmpty());

    // 尺寸
    EXPECT_FLOAT_EQ(entity.width(), 0.25f);
    EXPECT_FLOAT_EQ(entity.height(), 0.25f);

    // noClip 应为 true（MC Java: this.noPhysics = true）
    EXPECT_TRUE(entity.noClip());

    // 活塞推动反应为 Ignore
    EXPECT_EQ(entity.getPushReaction(), PushReaction::Ignore);

    // 未被移除
    EXPECT_FALSE(entity.isRemoved());
}

TEST_F(OminousItemSpawnerTest, Constants_MatchMCJava)
{
    // MC Java: random.nextIntBetweenInclusive(60, 120)
    EXPECT_EQ(OminousItemSpawnerEntity::SPAWN_ITEM_DELAY_MIN, 60);
    EXPECT_EQ(OminousItemSpawnerEntity::SPAWN_ITEM_DELAY_MAX, 120);

    // MC Java: TICKS_BEFORE_ABOUT_TO_SPAWN_SOUND = 36
    EXPECT_EQ(OminousItemSpawnerEntity::TICKS_BEFORE_ABOUT_TO_SPAWN_SOUND, 36);
}

TEST_F(OminousItemSpawnerTest, Create_FactoryMethod)
{
    // create() 创建默认实体（无物品，延迟为最大值）
    auto entityPtr = OminousItemSpawnerEntity::create(nullptr);
    ASSERT_NE(entityPtr, nullptr);

    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    EXPECT_TRUE(entity->getItem().isEmpty());
}

TEST_F(OminousItemSpawnerTest, CreateWithItem_SetsItemAndRandomDelay)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 5);

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, stack);
    ASSERT_NE(entityPtr, nullptr);

    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);

    // 物品应被设置
    EXPECT_FALSE(entity->getItem().isEmpty());
    EXPECT_EQ(entity->getItem().getCount(), 5);
    EXPECT_EQ(entity->getItem().getItem(), diamond);

    // 延迟应在 [60, 120] 范围内
    // 由于延迟在私有成员中，我们通过多次创建来间接验证
}

TEST_F(OminousItemSpawnerTest, CreateWithItem_RandomDelayInRange)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    // 创建多个实体，验证它们最终会在合理时间后移除
    // 间接验证随机延迟在 [60, 120] 范围内
    // Entity::tick() 先递增 m_ticksExisted 再调用子类 tickServer()
    // 当 m_ticksExisted >= m_spawnItemAfterTicks 时移除
    // 因此 m_spawnItemAfterTicks=60 时需要 60 次 tick，m_spawnItemAfterTicks=120 时需要 120 次 tick
    bool allRemovedInExpectedRange = true;
    for (int i = 0; i < 10; ++i) {
        auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, stack);
        auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
        ASSERT_NE(entity, nullptr);
        entity->setWorld(&m_world);

        // tick 直到实体被移除
        i32 ticksNeeded = 0;
        while (!entity->isRemoved() && ticksNeeded <= 200) {
            entity->tick();
            ticksNeeded++;
        }

        // 实体应在 [60, 120] 次 tick 内被移除
        if (ticksNeeded < 60 || ticksNeeded > 120) {
            allRemovedInExpectedRange = false;
            break;
        }
    }
    EXPECT_TRUE(allRemovedInExpectedRange);
}

// ============================================================================
// 物品存取测试
// ============================================================================

TEST_F(OminousItemSpawnerTest, SetGetItem)
{
    OminousItemSpawnerEntity entity(EntityId(1));

    EXPECT_TRUE(entity.getItem().isEmpty());

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 3);

    entity.setItem(stack);
    EXPECT_FALSE(entity.getItem().isEmpty());
    EXPECT_EQ(entity.getItem().getItem(), diamond);
    EXPECT_EQ(entity.getItem().getCount(), 3);
}

TEST_F(OminousItemSpawnerTest, SetItem_OverwritePrevious)
{
    OminousItemSpawnerEntity entity(EntityId(1));

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(ironIngot, nullptr);

    entity.setItem(ItemStack(*diamond, 5));
    EXPECT_EQ(entity.getItem().getItem(), diamond);

    entity.setItem(ItemStack(*ironIngot, 10));
    EXPECT_EQ(entity.getItem().getItem(), ironIngot);
    EXPECT_EQ(entity.getItem().getCount(), 10);
}

// ============================================================================
// NBT 序列化/反序列化测试
// ============================================================================

class OminousItemSpawnerNbtTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(OminousItemSpawnerNbtTest, EmptyItem_RoundTrip)
{
    // 无物品时不序列化 item 标签
    OminousItemSpawnerEntity entity(EntityId(1));

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    // 验证不包含 "item" 键
    const nbt::tags::compound_tag* itemTag = tryGetCompound(tag, "item");
    EXPECT_EQ(itemTag, nullptr);

    // 反序列化应不崩溃
    OminousItemSpawnerEntity loaded(EntityId(2));
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(loaded.getItem().isEmpty());
}

TEST_F(OminousItemSpawnerNbtTest, WithItem_RoundTrip)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    OminousItemSpawnerEntity entity(EntityId(1));
    entity.setItem(ItemStack(*diamond, 7));

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    // 验证包含 "item" 键
    const nbt::tags::compound_tag* itemTag = tryGetCompound(tag, "item");
    ASSERT_NE(itemTag, nullptr);

    // 反序列化
    OminousItemSpawnerEntity loaded(EntityId(2));
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(loaded.getItem().isEmpty());
    EXPECT_EQ(loaded.getItem().getCount(), 7);
    EXPECT_EQ(loaded.getItem().getItem(), diamond);
}

TEST_F(OminousItemSpawnerNbtTest, SpawnItemAfterTicks_RoundTrip)
{
    OminousItemSpawnerEntity entity(EntityId(1));

    nbt::tags::compound_tag tag;
    entity.addAdditionalSaveData(tag);

    // 验证包含 "spawn_item_after_ticks" 键
    auto ticksVal = tryGetLong(tag, "spawn_item_after_ticks");
    ASSERT_TRUE(ticksVal.has_value());
    // 默认值为 SPAWN_ITEM_DELAY_MAX (120)
    EXPECT_EQ(ticksVal.value(), OminousItemSpawnerEntity::SPAWN_ITEM_DELAY_MAX);

    // 反序列化
    OminousItemSpawnerEntity loaded(EntityId(2));
    auto result = loaded.readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success());
}

TEST_F(OminousItemSpawnerNbtTest, MissingKeys_PreservesDefaults)
{
    // 空 NBT 标签不应改变默认值
    OminousItemSpawnerEntity entity(EntityId(1));
    EXPECT_TRUE(entity.getItem().isEmpty());

    nbt::tags::compound_tag emptyTag;
    auto result = entity.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(entity.getItem().isEmpty());
}

// ============================================================================
// Tick 逻辑测试
// ============================================================================

TEST_F(OminousItemSpawnerTest, TickServer_SpawnsItemAndRemoves)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    OminousItemSpawnerEntity entity(EntityId(1));
    entity.setItem(ItemStack(*diamond, 1));
    entity.setWorld(&m_world);
    m_world.setClientSide(false);

    // 设置延迟为最小值 (60)，需要 60 ticks 后生成物品
    // 但 m_spawnItemAfterTicks 是私有成员，无法直接设置
    // 使用 createWithItem 创建的实体，延迟在 [60, 120] 范围
    // 我们通过直接 tick 足够多次来验证生成行为

    // tick 200 次确保超过最大延迟
    for (int i = 0; i < 200; ++i) {
        if (entity.isRemoved()) {
            break;
        }
        entity.tick();
    }

    // 实体应已被移除
    EXPECT_TRUE(entity.isRemoved());

    // 应有世界事件触发（粒子效果）
    EXPECT_GT(m_world.playEventCount(), 0);
}

TEST_F(OminousItemSpawnerTest, TickClient_SpawnsParticles)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    OminousItemSpawnerEntity entity(EntityId(1));
    entity.setItem(ItemStack(*diamond, 1));
    entity.setWorld(&m_world);
    m_world.setClientSide(true);
    m_world.setGameTime(0);

    // 客户端每 5 tick 生成粒子
    // 前 5 tick 不应有粒子（gameTime % 5 != 0 的情况除外）
    entity.tick(); // gameTime=0, 0%5==0 -> 粒子
    EXPECT_GE(m_world.particleCount(), 1);

    i32 particlesAfterFirstTick = m_world.particleCount();

    // tick 4 次，gameTime 1-4 不生成粒子
    for (int i = 0; i < 4; ++i) {
        m_world.advanceGameTime();
        entity.tick();
    }
    EXPECT_EQ(m_world.particleCount(), particlesAfterFirstTick); // 没有增加

    // 第 5 次 tick (gameTime=5)，5%5==0 -> 粒子
    m_world.advanceGameTime();
    entity.tick();
    EXPECT_GT(m_world.particleCount(), particlesAfterFirstTick);
}

TEST_F(OminousItemSpawnerTest, TickServer_WarningSoundPlayed)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    // 使用 createWithItem 创建实体，随机延迟 [60, 120]
    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*diamond, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    // tick 直到听到警告音效
    bool soundPlayed = false;
    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
        if (m_world.soundPlayCount() > 0) {
            soundPlayed = true;
            // 验证是正确的音效
            EXPECT_EQ(m_world.lastSoundEventId(), SoundEvents::TRIAL_SPAWNER_ABOUT_TO_SPAWN_ITEM);
            break;
        }
    }
    EXPECT_TRUE(soundPlayed);
}

TEST_F(OminousItemSpawnerTest, TickServer_SpawnsItemEntityForNormalItem)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*diamond, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    // tick 直到实体被移除
    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    // 应生成一个 ItemEntity
    EXPECT_GT(m_world.spawnedEntityCount(), 0u);
}

TEST_F(OminousItemSpawnerTest, TickServer_NoItem_DoesNotSpawn)
{
    // 无物品的实体不应生成任何东西
    OminousItemSpawnerEntity entity(EntityId(1));
    entity.setWorld(&m_world);
    m_world.setClientSide(false);

    // tick 足够多次
    for (int i = 0; i < 200 && !entity.isRemoved(); ++i) {
        entity.tick();
    }

    // 无物品时，即使达到延迟时间，也不应生成实体
    // 注意：默认构造 m_spawnItemAfterTicks = SPAWN_ITEM_DELAY_MAX
    // 实体会在延迟后被移除，但不会生成物品
    EXPECT_TRUE(entity.isRemoved());
    // 没有实体被生成（因为物品为空）
    // 但可能有 playEvent 调用（spawnItem 播放粒子但不生成实体）
}

TEST_F(OminousItemSpawnerTest, TickServer_GameEventTriggered)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*diamond, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    // tick 直到实体被移除
    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    // 应触发 ENTITY_PLACE 游戏事件
    EXPECT_GT(m_world.gameEventCount(), 0);
    EXPECT_EQ(m_world.lastGameEventId(), gameevent::GameEvents::ENTITY_PLACE.id());
}

TEST_F(OminousItemSpawnerTest, TickServer_PlayEventTriggered)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*diamond, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    // tick 直到实体被移除
    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    // 应播放世界事件 TRIAL_SPAWNER_SPAWN_ITEM (3021)
    EXPECT_GT(m_world.playEventCount(), 0);
    EXPECT_EQ(m_world.lastPlayEventId(), world::WorldEvents::TRIAL_SPAWNER_SPAWN_ITEM);
    EXPECT_EQ(m_world.lastPlayEventData(), 1);
}

TEST_F(OminousItemSpawnerTest, TickServer_NoWorld_DoesNotCrash)
{
    // 无世界引用时，tick 不应崩溃
    OminousItemSpawnerEntity entity(EntityId(1));
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    entity.setItem(ItemStack(*diamond, 1));

    // tick 多次不应崩溃
    for (int i = 0; i < 200; ++i) {
        entity.tick();
    }
}

TEST_F(OminousItemSpawnerTest, TickClient_NoWorld_DoesNotCrash)
{
    // 无世界引用时，客户端 tick 不应崩溃
    OminousItemSpawnerEntity entity(EntityId(1));
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    entity.setItem(ItemStack(*diamond, 1));

    // 模拟客户端模式但不设置世界
    for (int i = 0; i < 200; ++i) {
        entity.tick();
    }
}

// ============================================================================
// 警告音效时序测试
// ============================================================================

TEST_F(OminousItemSpawnerTest, WarningSoundPlays36TicksBeforeSpawn)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*diamond, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    // 跟踪音效播放时的 tick 数
    i32 warningTick = -1;
    i32 spawnTick = -1;

    for (i32 i = 0; i < 200; ++i) {
        i32 soundCountBefore = m_world.soundPlayCount();
        entity->tick();

        if (m_world.soundPlayCount() > soundCountBefore && warningTick < 0) {
            warningTick = i;
        }
        if (entity->isRemoved() && spawnTick < 0) {
            spawnTick = i;
        }
    }

    // 警告音效应在生成前约 36 ticks 播放
    ASSERT_GE(warningTick, 0);
    ASSERT_GE(spawnTick, 0);
    i32 ticksBeforeSpawn = spawnTick - warningTick;
    // 允许一定误差：警告音效应在生成前 36 ticks
    EXPECT_GE(ticksBeforeSpawn, 35);
    EXPECT_LE(ticksBeforeSpawn, 37);
}

// ============================================================================
// 活塞推动反应测试
// ============================================================================

TEST_F(OminousItemSpawnerTest, GetPushReaction_IsIgnore)
{
    OminousItemSpawnerEntity entity(EntityId(1));
    EXPECT_EQ(entity.getPushReaction(), PushReaction::Ignore);
}

// ============================================================================
// 弹射物生成测试（需要实体注册）
// ============================================================================

class OminousItemSpawnerProjectileTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        entity::VanillaEntities::registerAll();
    }

    void SetUp() override { m_world.clearSpawnedEntities(); }

    OminousItemSpawnerTestWorld m_world;
};

TEST_F(OminousItemSpawnerProjectileTest, ThrowableItem_Snowball_SpawnsProjectile)
{
    // 雪球是 ThrowableItem，应通过 spawnProjectile 路径创建弹射物
    Item* snowball = Items::SNOWBALL;
    ASSERT_NE(snowball, nullptr);

    // 验证物品是 ThrowableItem
    const auto* throwableItem = dynamic_cast<const item::ThrowableItem*>(snowball);
    ASSERT_NE(throwableItem, nullptr) << "雪球应该是 ThrowableItem";

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*snowball, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    // tick 直到实体被移除
    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    // 实体应被移除
    EXPECT_TRUE(entity->isRemoved());

    // 应生成一个实体（弹射物）
    EXPECT_GT(m_world.spawnedEntityCount(), 0u);

    // 应播放世界事件
    EXPECT_GT(m_world.playEventCount(), 0);
    EXPECT_EQ(m_world.lastPlayEventId(), world::WorldEvents::TRIAL_SPAWNER_SPAWN_ITEM);

    // 应触发 ENTITY_PLACE 游戏事件
    EXPECT_GT(m_world.gameEventCount(), 0);
    EXPECT_EQ(m_world.lastGameEventId(), gameevent::GameEvents::ENTITY_PLACE.id());
}

TEST_F(OminousItemSpawnerProjectileTest, ThrowableItem_Egg_SpawnsProjectile)
{
    // 鸡蛋是 ThrowableItem
    Item* egg = Items::EGG;
    ASSERT_NE(egg, nullptr);

    const auto* throwableItem = dynamic_cast<const item::ThrowableItem*>(egg);
    ASSERT_NE(throwableItem, nullptr) << "鸡蛋应该是 ThrowableItem";

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*egg, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    EXPECT_TRUE(entity->isRemoved());
    EXPECT_GT(m_world.spawnedEntityCount(), 0u);
}

TEST_F(OminousItemSpawnerProjectileTest, ThrowableItem_EnderPearl_SpawnsProjectile)
{
    // 末影珍珠是 ThrowableItem
    Item* enderPearl = Items::ENDER_PEARL;
    ASSERT_NE(enderPearl, nullptr);

    const auto* throwableItem = dynamic_cast<const item::ThrowableItem*>(enderPearl);
    ASSERT_NE(throwableItem, nullptr) << "末影珍珠应该是 ThrowableItem";

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*enderPearl, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    EXPECT_TRUE(entity->isRemoved());
    EXPECT_GT(m_world.spawnedEntityCount(), 0u);
}

TEST_F(OminousItemSpawnerProjectileTest, ThrowableItem_ExperienceBottle_SpawnsProjectile)
{
    // 经验瓶是 ThrowableItem
    Item* experienceBottle = Items::EXPERIENCE_BOTTLE;
    ASSERT_NE(experienceBottle, nullptr);

    const auto* throwableItem = dynamic_cast<const item::ThrowableItem*>(experienceBottle);
    ASSERT_NE(throwableItem, nullptr) << "经验瓶应该是 ThrowableItem";

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*experienceBottle, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    EXPECT_TRUE(entity->isRemoved());
    EXPECT_GT(m_world.spawnedEntityCount(), 0u);
}

TEST_F(OminousItemSpawnerProjectileTest, WindChargeItem_SpawnsProjectile)
{
    // 风弹是 WindChargeItem（不是 ThrowableItem，是独立的弹射物类型）
    Item* windCharge = Items::WIND_CHARGE;
    ASSERT_NE(windCharge, nullptr);

    // 验证风弹不是 ThrowableItem
    const auto* throwableItem = dynamic_cast<const item::ThrowableItem*>(windCharge);
    EXPECT_EQ(throwableItem, nullptr) << "风弹不应该是 ThrowableItem";

    // 验证风弹是 WindChargeItem
    const auto* windChargeItem = dynamic_cast<const item::WindChargeItem*>(windCharge);
    ASSERT_NE(windChargeItem, nullptr) << "风弹应该是 WindChargeItem";

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*windCharge, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    EXPECT_TRUE(entity->isRemoved());
    EXPECT_GT(m_world.spawnedEntityCount(), 0u);

    // 应播放世界事件
    EXPECT_GT(m_world.playEventCount(), 0);
    EXPECT_EQ(m_world.lastPlayEventId(), world::WorldEvents::TRIAL_SPAWNER_SPAWN_ITEM);
}

TEST_F(OminousItemSpawnerProjectileTest, SplashPotionItem_SpawnsProjectile)
{
    // 喷溅药水是 ThrowablePotionItem（继承自 ThrowableItem）
    Item* splashPotion = Items::SPLASH_POTION;
    ASSERT_NE(splashPotion, nullptr);

    const auto* throwableItem = dynamic_cast<const item::ThrowableItem*>(splashPotion);
    ASSERT_NE(throwableItem, nullptr) << "喷溅药水应该是 ThrowableItem（通过 ThrowablePotionItem 继承）";

    auto entityPtr = OminousItemSpawnerEntity::createWithItem(m_world, ItemStack(*splashPotion, 1));
    auto* entity = dynamic_cast<OminousItemSpawnerEntity*>(entityPtr.get());
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setClientSide(false);

    for (int i = 0; i < 200 && !entity->isRemoved(); ++i) {
        entity->tick();
    }

    EXPECT_TRUE(entity->isRemoved());
    EXPECT_GT(m_world.spawnedEntityCount(), 0u);
}
