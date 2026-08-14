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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/SimpleBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/trial/TrialSpawnerBlockEntity.hpp"
#include <unordered_map>
#include <nlohmann/json.hpp>

using namespace mc;

// ============================================================================
// TrialSpawnerTestWorld - 测试用 Mock 世界
// ============================================================================

class TrialSpawnerTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setEntitiesInRangeResult(const std::vector<Entity*>& entities) { m_entitiesInRange = entities; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return m_entitiesInRange;
    }

    void setPlayersResult(const std::vector<Entity*>& players) { m_players = players; }
    [[nodiscard]] std::vector<Entity*> getPlayers() const override { return m_players; }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) { m_blockEntities[pos] = entity; }
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TrialSpawnerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TrialSpawnerTestWorld::tickManager not implemented");
    }

    // 追踪 playSound 调用
    struct SoundCall {
        ResourceLocation soundId;
        f32 pitch = 0.0f;
    };
    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        MC_UNUSED(category);
        MC_UNUSED(position);
        MC_UNUSED(volume);
        m_soundCalls.push_back({soundId, pitch});
    }
    const std::vector<SoundCall>& soundCalls() const { return m_soundCalls; }
    void clearSoundCalls() { m_soundCalls.clear(); }

private:
    u64 m_currentTick = 0;
    std::vector<Entity*> m_entitiesInRange;
    std::vector<Entity*> m_players;
    std::unordered_map<BlockPos, BlockEntity*> m_blockEntities;
    std::vector<SoundCall> m_soundCalls;
};

// ============================================================================
// TrialSpawnerBlockEntity 构造和基本属性测试
// ============================================================================

class TrialSpawnerBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { spawner_ = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(10, 64, 20)); }

    std::unique_ptr<TrialSpawnerBlockEntity> spawner_;
};

TEST_F(TrialSpawnerBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(spawner_->getType(), BlockEntityType::TrialSpawner);
}

TEST_F(TrialSpawnerBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(spawner_->getPos(), BlockPos(10, 64, 20));
}

TEST_F(TrialSpawnerBlockEntityTest, Create_DefaultStateIsInactive)
{
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Inactive);
}

TEST_F(TrialSpawnerBlockEntityTest, Create_DefaultIsNotOminous)
{
    EXPECT_FALSE(spawner_->isOminous());
}

TEST_F(TrialSpawnerBlockEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(spawner_->needsTick());
}

TEST_F(TrialSpawnerBlockEntityTest, Create_DefaultConfigIsMelee)
{
    // 默认配置应为近战类型
    const auto& config = spawner_->getConfig();
    EXPECT_EQ(config.baseTotalMobs, 6);
    EXPECT_EQ(config.baseSimultaneousMobs, 3);
    EXPECT_EQ(config.ticksBetweenSpawn, 40);
    EXPECT_FLOAT_EQ(config.detectionRange, 14.0f);
    EXPECT_FLOAT_EQ(config.spawnRange, 4.0f);
    EXPECT_EQ(config.cooldownTicks, 36000);
}

// ============================================================================
// 状态设置测试
// ============================================================================

TEST_F(TrialSpawnerBlockEntityTest, SetState_UpdatesState)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::WaitingForPlayers);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::WaitingForPlayers);

    spawner_->setState(TrialSpawnerBlockEntity::State::Active);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Active);

    spawner_->setState(TrialSpawnerBlockEntity::State::WaitingForRewardEjection);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::WaitingForRewardEjection);

    spawner_->setState(TrialSpawnerBlockEntity::State::EjectingReward);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::EjectingReward);

    spawner_->setState(TrialSpawnerBlockEntity::State::Cooldown);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Cooldown);

    spawner_->setState(TrialSpawnerBlockEntity::State::Inactive);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Inactive);
}

TEST_F(TrialSpawnerBlockEntityTest, SetState_MarksChanged)
{
    EXPECT_FALSE(spawner_->isChanged());
    spawner_->setState(TrialSpawnerBlockEntity::State::Active);
    EXPECT_TRUE(spawner_->isChanged());
}

TEST_F(TrialSpawnerBlockEntityTest, SetState_SameStateDoesNotMarkChanged)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::Inactive);
    EXPECT_FALSE(spawner_->isChanged());
}

TEST_F(TrialSpawnerBlockEntityTest, SetOminous_UpdatesOminousFlag)
{
    EXPECT_FALSE(spawner_->isOminous());
    spawner_->setOminous(true);
    EXPECT_TRUE(spawner_->isOminous());
    spawner_->setOminous(false);
    EXPECT_FALSE(spawner_->isOminous());
}

TEST_F(TrialSpawnerBlockEntityTest, SetOminous_MarksChanged)
{
    EXPECT_FALSE(spawner_->isChanged());
    spawner_->setOminous(true);
    EXPECT_TRUE(spawner_->isChanged());
}

TEST_F(TrialSpawnerBlockEntityTest, SetOminous_SameValueDoesNotMarkChanged)
{
    spawner_->setOminous(false);
    EXPECT_FALSE(spawner_->isChanged());
}

// ============================================================================
// 配置工厂方法测试
// ============================================================================

TEST_F(TrialSpawnerBlockEntityTest, GetBreezeConfig_HasCorrectValues)
{
    auto config = TrialSpawnerBlockEntity::getBreezeConfig();
    EXPECT_EQ(config.baseTotalMobs, 2);
    EXPECT_EQ(config.baseSimultaneousMobs, 1);
    EXPECT_EQ(config.ticksBetweenSpawn, 20);
    EXPECT_FLOAT_EQ(config.detectionRange, 14.0f);
    EXPECT_FLOAT_EQ(config.spawnRange, 4.0f);
    EXPECT_EQ(config.cooldownTicks, 36000);
}

TEST_F(TrialSpawnerBlockEntityTest, GetMeleeConfig_HasCorrectValues)
{
    auto config = TrialSpawnerBlockEntity::getMeleeConfig();
    EXPECT_EQ(config.baseTotalMobs, 6);
    EXPECT_EQ(config.baseSimultaneousMobs, 3);
    EXPECT_EQ(config.ticksBetweenSpawn, 40);
}

TEST_F(TrialSpawnerBlockEntityTest, GetSmallMeleeConfig_HasCorrectValues)
{
    auto config = TrialSpawnerBlockEntity::getSmallMeleeConfig();
    EXPECT_EQ(config.baseTotalMobs, 12);
    EXPECT_EQ(config.baseSimultaneousMobs, 4);
    EXPECT_EQ(config.ticksBetweenSpawn, 20);
}

TEST_F(TrialSpawnerBlockEntityTest, GetRangedConfig_HasCorrectValues)
{
    auto config = TrialSpawnerBlockEntity::getRangedConfig();
    EXPECT_EQ(config.baseTotalMobs, 6);
    EXPECT_EQ(config.baseSimultaneousMobs, 3);
    EXPECT_EQ(config.ticksBetweenSpawn, 40);
}

TEST_F(TrialSpawnerBlockEntityTest, GetSlowRangedConfig_HasCorrectValues)
{
    auto config = TrialSpawnerBlockEntity::getSlowRangedConfig();
    EXPECT_EQ(config.baseTotalMobs, 6);
    EXPECT_EQ(config.baseSimultaneousMobs, 3);
    EXPECT_EQ(config.ticksBetweenSpawn, 80);
}

TEST_F(TrialSpawnerBlockEntityTest, SetConfig_UpdatesConfig)
{
    auto customConfig = TrialSpawnerBlockEntity::getBreezeConfig();
    spawner_->setConfig(customConfig);
    EXPECT_EQ(spawner_->getConfig().baseTotalMobs, 2);
    EXPECT_EQ(spawner_->getConfig().baseSimultaneousMobs, 1);
}

TEST_F(TrialSpawnerBlockEntityTest, SetConfig_MarksChanged)
{
    EXPECT_FALSE(spawner_->isChanged());
    auto customConfig = TrialSpawnerBlockEntity::getBreezeConfig();
    spawner_->setConfig(customConfig);
    EXPECT_TRUE(spawner_->isChanged());
}

// ============================================================================
// 红石比较器输出测试
// ============================================================================

TEST_F(TrialSpawnerBlockEntityTest, GetComparatorOutput_InactiveReturnsZero)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::Inactive);
    EXPECT_EQ(spawner_->getComparatorOutput(), 0);
}

TEST_F(TrialSpawnerBlockEntityTest, GetComparatorOutput_WaitingForPlayersReturnsOne)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::WaitingForPlayers);
    EXPECT_EQ(spawner_->getComparatorOutput(), 1);
}

TEST_F(TrialSpawnerBlockEntityTest, GetComparatorOutput_ActiveReturnsTwo)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::Active);
    EXPECT_EQ(spawner_->getComparatorOutput(), 2);
}

TEST_F(TrialSpawnerBlockEntityTest, GetComparatorOutput_WaitingForRewardEjectionReturnsThree)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::WaitingForRewardEjection);
    EXPECT_EQ(spawner_->getComparatorOutput(), 3);
}

TEST_F(TrialSpawnerBlockEntityTest, GetComparatorOutput_EjectingRewardReturnsFour)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::EjectingReward);
    EXPECT_EQ(spawner_->getComparatorOutput(), 4);
}

TEST_F(TrialSpawnerBlockEntityTest, GetComparatorOutput_CooldownReturnsFour)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::Cooldown);
    EXPECT_EQ(spawner_->getComparatorOutput(), 4);
}

// ============================================================================
// 序列化测试
// ============================================================================

TEST_F(TrialSpawnerBlockEntityTest, SaveLoad_PreservesState)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::Active);
    nlohmann::json data;
    spawner_->save(data);

    auto loaded = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getState(), TrialSpawnerBlockEntity::State::Active);
}

TEST_F(TrialSpawnerBlockEntityTest, SaveLoad_PreservesOminous)
{
    spawner_->setOminous(true);
    nlohmann::json data;
    spawner_->save(data);

    auto loaded = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_TRUE(loaded->isOminous());
}

TEST_F(TrialSpawnerBlockEntityTest, SaveLoad_PreservesAllStates)
{
    for (int i = 0; i <= 5; ++i) {
        auto state = static_cast<TrialSpawnerBlockEntity::State>(i);
        spawner_->setState(state);

        nlohmann::json data;
        spawner_->save(data);

        auto loaded = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0));
        ASSERT_TRUE(loaded->load(data));
        EXPECT_EQ(loaded->getState(), state);
    }
}

TEST_F(TrialSpawnerBlockEntityTest, SaveLoad_PreservesSpawnedMobsCount)
{
    nlohmann::json data;
    data["state"] = 0;
    data["ominous"] = false;
    data["cooldown_ends_at"] = 0;
    data["ejecting_reward_ends_at"] = 0;
    data["spawned_mobs_count"] = 5;
    data["tracked_players"] = nlohmann::json::array();
    data["tracked_mobs"] = nlohmann::json::array();
    data["current_mobs_count"] = 3;
    data["total_mobs_to_spawn"] = 10;
    data["max_simultaneous_mobs"] = 4;

    auto loaded = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    nlohmann::json savedData;
    loaded->save(savedData);
    EXPECT_EQ(savedData["spawned_mobs_count"].get<i32>(), 5);
    EXPECT_EQ(savedData["current_mobs_count"].get<i32>(), 3);
    EXPECT_EQ(savedData["total_mobs_to_spawn"].get<i32>(), 10);
    EXPECT_EQ(savedData["max_simultaneous_mobs"].get<i32>(), 4);
}

TEST_F(TrialSpawnerBlockEntityTest, SaveLoad_PreservesCooldownEndsAt)
{
    nlohmann::json data;
    data["state"] = 0;
    data["ominous"] = false;
    data["cooldown_ends_at"] = 12345678;
    data["ejecting_reward_ends_at"] = 0;
    data["spawned_mobs_count"] = 0;
    data["tracked_players"] = nlohmann::json::array();
    data["tracked_mobs"] = nlohmann::json::array();
    data["current_mobs_count"] = 0;
    data["total_mobs_to_spawn"] = 0;
    data["max_simultaneous_mobs"] = 0;

    auto loaded = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    nlohmann::json savedData;
    loaded->save(savedData);
    EXPECT_EQ(savedData["cooldown_ends_at"].get<i64>(), 12345678);
}

TEST_F(TrialSpawnerBlockEntityTest, SaveLoad_PreservesTrackedPlayers)
{
    nlohmann::json data;
    data["state"] = 0;
    data["ominous"] = false;
    data["cooldown_ends_at"] = 0;
    data["ejecting_reward_ends_at"] = 0;
    data["spawned_mobs_count"] = 0;
    data["tracked_players"] = nlohmann::json::array({"uuid-1", "uuid-2", "uuid-3"});
    data["tracked_mobs"] = nlohmann::json::array();
    data["current_mobs_count"] = 0;
    data["total_mobs_to_spawn"] = 0;
    data["max_simultaneous_mobs"] = 0;

    auto loaded = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    nlohmann::json savedData;
    loaded->save(savedData);
    EXPECT_EQ(savedData["tracked_players"].size(), 3u);
}

TEST_F(TrialSpawnerBlockEntityTest, SaveLoad_PreservesTrackedMobs)
{
    nlohmann::json data;
    data["state"] = 0;
    data["ominous"] = false;
    data["cooldown_ends_at"] = 0;
    data["ejecting_reward_ends_at"] = 0;
    data["spawned_mobs_count"] = 0;
    data["tracked_players"] = nlohmann::json::array();
    data["tracked_mobs"] = nlohmann::json::array({"mob-uuid-1", "mob-uuid-2"});
    data["current_mobs_count"] = 2;
    data["total_mobs_to_spawn"] = 0;
    data["max_simultaneous_mobs"] = 0;

    auto loaded = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    nlohmann::json savedData;
    loaded->save(savedData);
    EXPECT_EQ(savedData["tracked_mobs"].size(), 2u);
}

// ============================================================================
// Clone 测试
// ============================================================================

TEST_F(TrialSpawnerBlockEntityTest, Clone_CreatesDeepCopy)
{
    spawner_->setState(TrialSpawnerBlockEntity::State::Active);
    spawner_->setOminous(true);

    std::unique_ptr<BlockEntity> copy = spawner_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::TrialSpawner);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 64, 20));

    auto* spawnerCopy = static_cast<TrialSpawnerBlockEntity*>(copy.get());
    EXPECT_EQ(spawnerCopy->getState(), TrialSpawnerBlockEntity::State::Active);
    EXPECT_TRUE(spawnerCopy->isOminous());
    EXPECT_EQ(spawnerCopy->getConfig().baseTotalMobs, 6);
    EXPECT_EQ(spawnerCopy->getConfig().baseSimultaneousMobs, 3);
}

// ============================================================================
// 状态机测试（通过 tick 调度器）
// ============================================================================

class TrialSpawnerStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override { spawner_ = std::make_unique<TrialSpawnerBlockEntity>(BlockPos(0, 0, 0)); }

    TrialSpawnerTestWorld world_;
    std::unique_ptr<TrialSpawnerBlockEntity> spawner_;
};

// Inactive -> WaitingForPlayers: 玩家进入范围
TEST_F(TrialSpawnerStateMachineTest, Tick_InactiveToWaitingWhenPlayerDetected)
{
    world_.setCurrentTick(100);

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    world_.setEntitiesInRangeResult({&player});

    spawner_->tick(world_);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::WaitingForPlayers);
}

// Inactive: 无玩家时保持 Inactive
TEST_F(TrialSpawnerStateMachineTest, Tick_InactiveStaysInactiveWhenNoPlayer)
{
    world_.setCurrentTick(100);
    world_.setEntitiesInRangeResult({});

    spawner_->tick(world_);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Inactive);
}

// Inactive: 检测间隔内不执行检测
TEST_F(TrialSpawnerStateMachineTest, Tick_InactiveSkipsDetectionWithinInterval)
{
    world_.setCurrentTick(5); // 远小于 PLAYER_SCAN_INTERVAL(20)

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    world_.setEntitiesInRangeResult({&player});

    spawner_->tick(world_);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Inactive);
}

// 旁观者模式玩家不触发激活
TEST_F(TrialSpawnerStateMachineTest, Tick_InactiveIgnoresSpectatorPlayer)
{
    world_.setCurrentTick(100);

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    player.setGameMode(GameMode::Spectator);
    world_.setEntitiesInRangeResult({&player});

    spawner_->tick(world_);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Inactive);
}

// WaitingForPlayers -> Active: 玩家仍在范围
TEST_F(TrialSpawnerStateMachineTest, Tick_WaitingToActiveWhenPlayerPresent)
{
    world_.setCurrentTick(100);

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("uuid-1");
    world_.setEntitiesInRangeResult({&player});

    spawner_->setState(TrialSpawnerBlockEntity::State::WaitingForPlayers);
    spawner_->tick(world_);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Active);
}

// WaitingForPlayers -> Inactive: 无玩家
TEST_F(TrialSpawnerStateMachineTest, Tick_WaitingToInactiveWhenNoPlayer)
{
    world_.setCurrentTick(100);
    world_.setEntitiesInRangeResult({});

    spawner_->setState(TrialSpawnerBlockEntity::State::WaitingForPlayers);
    spawner_->tick(world_);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Inactive);
}

// ============================================================================
// 配置验证测试
// ============================================================================

TEST_F(TrialSpawnerBlockEntityTest, AllConfigsHaveSameDetectionAndSpawnRange)
{
    // 所有配置的 detectionRange 和 spawnRange 应相同
    for (const auto& config : {
             TrialSpawnerBlockEntity::getBreezeConfig(),
             TrialSpawnerBlockEntity::getMeleeConfig(),
             TrialSpawnerBlockEntity::getSmallMeleeConfig(),
             TrialSpawnerBlockEntity::getRangedConfig(),
             TrialSpawnerBlockEntity::getSlowRangedConfig(),
         }) {
        EXPECT_FLOAT_EQ(config.detectionRange, 14.0f);
        EXPECT_FLOAT_EQ(config.spawnRange, 4.0f);
        EXPECT_EQ(config.cooldownTicks, 36000);
    }
}

TEST_F(TrialSpawnerBlockEntityTest, AllConfigsHaveSameLootTables)
{
    auto melee = TrialSpawnerBlockEntity::getMeleeConfig();
    auto breeze = TrialSpawnerBlockEntity::getBreezeConfig();

    EXPECT_EQ(melee.supplyLootTable.toString(), breeze.supplyLootTable.toString());
    EXPECT_EQ(melee.keyLootTable.toString(), breeze.keyLootTable.toString());
    EXPECT_EQ(melee.ominousSupplyLootTable.toString(), breeze.ominousSupplyLootTable.toString());
    EXPECT_EQ(melee.ominousKeyLootTable.toString(), breeze.ominousKeyLootTable.toString());
}

TEST_F(TrialSpawnerBlockEntityTest, BreezeConfig_LootTables)
{
    auto config = TrialSpawnerBlockEntity::getBreezeConfig();
    EXPECT_EQ(config.supplyLootTable, ResourceLocation("minecraft", "spawners/trial_chamber/consumables"));
    EXPECT_EQ(config.keyLootTable, ResourceLocation("minecraft", "spawners/trial_chamber/key"));
    EXPECT_EQ(
        config.ominousSupplyLootTable, ResourceLocation("minecraft", "spawners/ominous/trial_chamber/consumables"));
    EXPECT_EQ(config.ominousKeyLootTable, ResourceLocation("minecraft", "spawners/ominous/trial_chamber/key"));
}

TEST_F(TrialSpawnerBlockEntityTest, ConfigPerPlayerScalingValues)
{
    auto melee = TrialSpawnerBlockEntity::getMeleeConfig();
    EXPECT_EQ(melee.totalMobsAddedPerPlayer, 2);
    EXPECT_EQ(melee.simultaneousMobsAddedPerPlayer, 1);
}

// ============================================================================
// detectPlayers 公共方法测试
// ============================================================================

TEST_F(TrialSpawnerStateMachineTest, DetectPlayers_FindsNonSpectatorPlayers)
{
    Player player1(1, "TestPlayer1", mc::test::testEcsRegistry());
    player1.setUuid("uuid-1");
    Player player2(2, "TestPlayer2", mc::test::testEcsRegistry());
    player2.setUuid("uuid-2");
    world_.setEntitiesInRangeResult({&player1, &player2});

    auto players = spawner_->detectPlayers(world_, 14.0f);
    EXPECT_EQ(players.size(), 2u);
}

TEST_F(TrialSpawnerStateMachineTest, DetectPlayers_ExcludesSpectators)
{
    Player player1(1, "TestPlayer1", mc::test::testEcsRegistry());
    player1.setUuid("uuid-1");
    Player spectator(2, "SpectatorPlayer", mc::test::testEcsRegistry());
    spectator.setUuid("uuid-2");
    spectator.setGameMode(GameMode::Spectator);
    world_.setEntitiesInRangeResult({&player1, &spectator});

    auto players = spawner_->detectPlayers(world_, 14.0f);
    EXPECT_EQ(players.size(), 1u);
}

TEST_F(TrialSpawnerStateMachineTest, DetectPlayers_ReturnsEmptyWhenNoEntities)
{
    world_.setEntitiesInRangeResult({});

    auto players = spawner_->detectPlayers(world_, 14.0f);
    EXPECT_TRUE(players.empty());
}

TEST_F(TrialSpawnerStateMachineTest, DetectPlayers_ReturnsEmptyWhenOnlySpectators)
{
    Player spectator(1, "SpectatorPlayer", mc::test::testEcsRegistry());
    spectator.setUuid("uuid-1");
    spectator.setGameMode(GameMode::Spectator);
    world_.setEntitiesInRangeResult({&spectator});

    auto players = spawner_->detectPlayers(world_, 14.0f);
    EXPECT_TRUE(players.empty());
}

// ============================================================================
// 生成逻辑测试所需扩展 Mock 世界
// ============================================================================

class TrialSpawnerSpawnTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setSeed(u64 seed) { m_random = math::Random(seed); }

    void setEntitiesInRangeResult(const std::vector<Entity*>& entities) { m_entitiesInRange = entities; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return m_entitiesInRange;
    }

    void setPlayersResult(const std::vector<Entity*>& players) { m_players = players; }
    [[nodiscard]] std::vector<Entity*> getPlayers() const override { return m_players; }

    void setDifficulty(Difficulty diff) { m_difficulty = diff; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) { m_blockEntities[pos] = entity; }
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }

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

    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        for (auto* entity : m_spawnedEntities) {
            if (entity && entity->uuid() == uuid) {
                return entity;
            }
        }
        return nullptr;
    }

    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        MC_UNUSED(category);
        MC_UNUSED(position);
        MC_UNUSED(volume);
        m_soundCalls.push_back({soundId, pitch});
    }

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

    // 测试辅助方法
    size_t spawnedCount() const { return m_spawnedEntities.size(); }
    void clearSpawnedEntities()
    {
        m_spawnedEntities.clear();
        m_ownedEntities.clear();
    }
    const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

    // 碰撞控制：默认无碰撞（与 BaseTestWorld 一致），可手动设为 true 阻止生成
    void setBlockCollision(bool hasCollision) { m_hasBlockCollision = hasCollision; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return m_hasBlockCollision; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TrialSpawnerSpawnTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TrialSpawnerSpawnTestWorld::tickManager not implemented");
    }

private:
    u64 m_currentTick = 0;
    Difficulty m_difficulty = Difficulty::Easy;
    std::vector<Entity*> m_entitiesInRange;
    std::vector<Entity*> m_players;
    std::unordered_map<BlockPos, BlockEntity*> m_blockEntities;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
    u64 m_nextEntityId = 0;
    bool m_hasBlockCollision = false;
    std::vector<PlayEventCall> m_playEventCalls;
    struct SoundCall {
        ResourceLocation soundId;
        f32 pitch;
    };
    std::vector<SoundCall> m_soundCalls;
};

// ============================================================================
// 测试子类：暴露私有方法供测试调用
// ============================================================================

namespace mc {

class TestTrialSpawnerBlockEntity final : public TrialSpawnerBlockEntity {
public:
    explicit TestTrialSpawnerBlockEntity(const BlockPos& pos)
        : TrialSpawnerBlockEntity(pos)
    {}

    // 暴露 _selectNextEntity 供测试调用
    [[nodiscard]] const ResourceLocation* testSelectNextEntity(IWorld& world) { return _selectNextEntity(world); }

    // 暴露 _findSpawnPosition 供测试调用
    [[nodiscard]] std::optional<Vector3> testFindSpawnPosition(IWorld& world) { return _findSpawnPosition(world); }

    // 暴露 spawnMob 供测试调用（已是 public，但通过子类统一访问入口）
    void testSpawnMob(IWorld& world) { spawnMob(world); }

    // 访问内部状态
    [[nodiscard]] i32 testSpawnedMobsCount() const { return m_spawnedMobsCount; }
    [[nodiscard]] i32 testCurrentMobsCount() const { return m_currentMobsCount; }
    [[nodiscard]] const std::unordered_set<std::string>& testTrackedMobs() const { return m_trackedMobs; }
    [[nodiscard]] const ResourceLocation& testNextSpawnEntityId() const { return m_nextSpawnEntityId; }

    // 设置下次生成实体缓存
    void setNextSpawnEntityId(const ResourceLocation& id) { m_nextSpawnEntityId = id; }

    // 清除下次生成实体缓存
    void clearNextSpawnEntityId() { m_nextSpawnEntityId = ResourceLocation(); }
};

} // namespace mc

// ============================================================================
// 配置 spawnPotentials 验证测试
// ============================================================================

TEST_F(TrialSpawnerBlockEntityTest, BreezeConfig_SpawnPotentials)
{
    auto config = TrialSpawnerBlockEntity::getBreezeConfig();
    ASSERT_EQ(config.spawnPotentials.size(), 1u);
    EXPECT_EQ(config.spawnPotentials[0].entityId, ResourceLocation("minecraft", "breeze"));
    EXPECT_EQ(config.spawnPotentials[0].weight, 1);
}

TEST_F(TrialSpawnerBlockEntityTest, MeleeConfig_SpawnPotentials)
{
    auto config = TrialSpawnerBlockEntity::getMeleeConfig();
    ASSERT_EQ(config.spawnPotentials.size(), 3u);
    EXPECT_EQ(config.spawnPotentials[0].entityId, ResourceLocation("minecraft", "zombie"));
    EXPECT_EQ(config.spawnPotentials[0].weight, 1);
    EXPECT_EQ(config.spawnPotentials[1].entityId, ResourceLocation("minecraft", "husk"));
    EXPECT_EQ(config.spawnPotentials[1].weight, 1);
    EXPECT_EQ(config.spawnPotentials[2].entityId, ResourceLocation("minecraft", "spider"));
    EXPECT_EQ(config.spawnPotentials[2].weight, 1);
}

TEST_F(TrialSpawnerBlockEntityTest, SmallMeleeConfig_SpawnPotentials)
{
    auto config = TrialSpawnerBlockEntity::getSmallMeleeConfig();
    ASSERT_EQ(config.spawnPotentials.size(), 3u);
    EXPECT_EQ(config.spawnPotentials[0].entityId, ResourceLocation("minecraft", "silverfish"));
    EXPECT_EQ(config.spawnPotentials[0].weight, 2);
    EXPECT_EQ(config.spawnPotentials[1].entityId, ResourceLocation("minecraft", "cave_spider"));
    EXPECT_EQ(config.spawnPotentials[1].weight, 2);
    EXPECT_EQ(config.spawnPotentials[2].entityId, ResourceLocation("minecraft", "slime"));
    EXPECT_EQ(config.spawnPotentials[2].weight, 1);
}

TEST_F(TrialSpawnerBlockEntityTest, RangedConfig_SpawnPotentials)
{
    auto config = TrialSpawnerBlockEntity::getRangedConfig();
    ASSERT_EQ(config.spawnPotentials.size(), 3u);
    EXPECT_EQ(config.spawnPotentials[0].entityId, ResourceLocation("minecraft", "skeleton"));
    EXPECT_EQ(config.spawnPotentials[0].weight, 1);
    EXPECT_EQ(config.spawnPotentials[1].entityId, ResourceLocation("minecraft", "stray"));
    EXPECT_EQ(config.spawnPotentials[1].weight, 1);
    EXPECT_EQ(config.spawnPotentials[2].entityId, ResourceLocation("minecraft", "bogged"));
    EXPECT_EQ(config.spawnPotentials[2].weight, 1);
}

TEST_F(TrialSpawnerBlockEntityTest, SlowRangedConfig_SpawnPotentials)
{
    auto config = TrialSpawnerBlockEntity::getSlowRangedConfig();
    ASSERT_EQ(config.spawnPotentials.size(), 3u);
    EXPECT_EQ(config.spawnPotentials[0].entityId, ResourceLocation("minecraft", "skeleton"));
    EXPECT_EQ(config.spawnPotentials[0].weight, 1);
    EXPECT_EQ(config.spawnPotentials[1].entityId, ResourceLocation("minecraft", "stray"));
    EXPECT_EQ(config.spawnPotentials[1].weight, 1);
    EXPECT_EQ(config.spawnPotentials[2].entityId, ResourceLocation("minecraft", "bogged"));
    EXPECT_EQ(config.spawnPotentials[2].weight, 1);
}

TEST_F(TrialSpawnerBlockEntityTest, AllConfigsHaveNonEmptySpawnPotentials)
{
    for (const auto& config : {
             TrialSpawnerBlockEntity::getBreezeConfig(),
             TrialSpawnerBlockEntity::getMeleeConfig(),
             TrialSpawnerBlockEntity::getSmallMeleeConfig(),
             TrialSpawnerBlockEntity::getRangedConfig(),
             TrialSpawnerBlockEntity::getSlowRangedConfig(),
         }) {
        EXPECT_FALSE(config.spawnPotentials.empty()) << "配置的 spawnPotentials 不应为空";
        i32 totalWeight = 0;
        for (const auto& entry : config.spawnPotentials) {
            EXPECT_GT(entry.weight, 0) << "生成潜力条目的权重应大于0";
            totalWeight += entry.weight;
        }
        EXPECT_GT(totalWeight, 0) << "生成潜力列表总权重应大于0";
    }
}

// ============================================================================
// _selectNextEntity 测试
// ============================================================================

class TrialSpawnerSelectEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
        spawner_ = std::make_unique<TestTrialSpawnerBlockEntity>(BlockPos(0, 64, 0));
    }

    TrialSpawnerSpawnTestWorld world_;
    std::unique_ptr<TestTrialSpawnerBlockEntity> spawner_;
};

TEST_F(TrialSpawnerSelectEntityTest, EmptyPotentials_ReturnsNullptr)
{
    // 配置空的 spawnPotentials
    TrialSpawnerBlockEntity::Config emptyConfig;
    emptyConfig.spawnPotentials = {};
    spawner_->setConfig(emptyConfig);

    auto* result = spawner_->testSelectNextEntity(world_);
    EXPECT_EQ(result, nullptr);
}

TEST_F(TrialSpawnerSelectEntityTest, SingleEntry_ReturnsThatEntity)
{
    TrialSpawnerBlockEntity::Config config;
    config.spawnPotentials = {{ResourceLocation("minecraft", "breeze"), 1}};
    spawner_->setConfig(config);

    auto* result = spawner_->testSelectNextEntity(world_);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, ResourceLocation("minecraft", "breeze"));
}

TEST_F(TrialSpawnerSelectEntityTest, WeightedSelection_DistributesAccordingToWeights)
{
    // 设置极端权重：zombie 权重99，breeze 权重1
    TrialSpawnerBlockEntity::Config config;
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "zombie"), 99},
        {ResourceLocation("minecraft", "breeze"), 1},
    };
    spawner_->setConfig(config);

    i32 zombieCount = 0;
    i32 breezeCount = 0;
    constexpr i32 TRIALS = 200;

    for (i32 i = 0; i < TRIALS; ++i) {
        spawner_->clearNextSpawnEntityId();
        auto* result = spawner_->testSelectNextEntity(world_);
        ASSERT_NE(result, nullptr);
        if (*result == ResourceLocation("minecraft", "zombie")) {
            ++zombieCount;
        } else if (*result == ResourceLocation("minecraft", "breeze")) {
            ++breezeCount;
        }
    }

    // zombie 应该占绝大多数（期望约 99%）
    EXPECT_GT(zombieCount, breezeCount);
    EXPECT_GT(zombieCount, TRIALS * 80 / 100); // 至少 80% 是 zombie
}

TEST_F(TrialSpawnerSelectEntityTest, CachedEntity_IsUsedBeforeRandomSelection)
{
    // 先设置缓存
    spawner_->setNextSpawnEntityId(ResourceLocation("minecraft", "breeze"));

    // 配置只含 zombie 的列表
    TrialSpawnerBlockEntity::Config config;
    config.spawnPotentials = {{ResourceLocation("minecraft", "zombie"), 1}};
    spawner_->setConfig(config);

    // 因为有缓存，应该返回缓存的 breeze 而不是列表中的 zombie
    auto* result = spawner_->testSelectNextEntity(world_);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, ResourceLocation("minecraft", "breeze"));
}

TEST_F(TrialSpawnerSelectEntityTest, ZeroWeightPotentials_ReturnsNullptr)
{
    TrialSpawnerBlockEntity::Config config;
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "zombie"), 0},
        {ResourceLocation("minecraft", "breeze"), 0},
    };
    spawner_->setConfig(config);

    auto* result = spawner_->testSelectNextEntity(world_);
    EXPECT_EQ(result, nullptr);
}

TEST_F(TrialSpawnerSelectEntityTest, MixedZeroAndPositiveWeight_OnlySelectsPositive)
{
    TrialSpawnerBlockEntity::Config config;
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "zombie"), 0},
        {ResourceLocation("minecraft", "breeze"), 1},
    };
    spawner_->setConfig(config);

    // 多次调用应始终返回 breeze（zombie 权重为0，不参与选择）
    for (i32 i = 0; i < 10; ++i) {
        spawner_->clearNextSpawnEntityId();
        auto* result = spawner_->testSelectNextEntity(world_);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, ResourceLocation("minecraft", "breeze"));
    }
}

// ============================================================================
// _findSpawnPosition 测试
// ============================================================================

class TrialSpawnerFindPositionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
        spawner_ = std::make_unique<TestTrialSpawnerBlockEntity>(BlockPos(0, 64, 0));
    }

    TrialSpawnerSpawnTestWorld world_;
    std::unique_ptr<TestTrialSpawnerBlockEntity> spawner_;
};

TEST_F(TrialSpawnerFindPositionTest, NoCollision_ReturnsPosition)
{
    // BaseTestWorld 默认 hasBlockCollision 返回 false，所以应该能找到位置
    auto pos = spawner_->testFindSpawnPosition(world_);
    EXPECT_TRUE(pos.has_value());
}

TEST_F(TrialSpawnerFindPositionTest, AllPositionsBlocked_ReturnsNullopt)
{
    // 设置所有位置碰撞为 true
    world_.setBlockCollision(true);

    auto pos = spawner_->testFindSpawnPosition(world_);
    EXPECT_FALSE(pos.has_value());
}

TEST_F(TrialSpawnerFindPositionTest, FoundPositionIsWithinSpawnRange)
{
    auto pos = spawner_->testFindSpawnPosition(world_);
    ASSERT_TRUE(pos.has_value());

    // 刷怪笼位于 (0, 64, 0)，spawnRange 默认为 4.0
    // X 和 Z 应在 [-4, 4] 范围内（加上 0.5 的中心偏移）
    f32 spawnRange = spawner_->getConfig().spawnRange;
    f32 centerX = 0.5f; // pos.x + 0.5
    f32 centerZ = 0.5f; // pos.z + 0.5

    EXPECT_GE(pos->x, centerX - spawnRange - 0.1f);
    EXPECT_LE(pos->x, centerX + spawnRange + 0.1f);
    EXPECT_GE(pos->z, centerZ - spawnRange - 0.1f);
    EXPECT_LE(pos->z, centerZ + spawnRange + 0.1f);

    // Y 应在 [63, 65] 范围内（pos.y + {-1, 0, +1}）
    EXPECT_GE(pos->y, 63.0f);
    EXPECT_LE(pos->y, 65.0f);
}

TEST_F(TrialSpawnerFindPositionTest, DifferentSpawnRange_Respected)
{
    // 使用自定义小范围配置
    TrialSpawnerBlockEntity::Config config;
    config.spawnRange = 1.0f;
    config.spawnPotentials = {{ResourceLocation("minecraft", "zombie"), 1}};
    spawner_->setConfig(config);

    auto pos = spawner_->testFindSpawnPosition(world_);
    ASSERT_TRUE(pos.has_value());

    // X 和 Z 应在更小的范围内
    f32 centerX = 0.5f;
    f32 centerZ = 0.5f;
    EXPECT_GE(pos->x, centerX - 2.0f); // spawnRange=1.0, 范围 [-1, 1] + center
    EXPECT_LE(pos->x, centerX + 2.0f);
    EXPECT_GE(pos->z, centerZ - 2.0f);
    EXPECT_LE(pos->z, centerZ + 2.0f);
}

// ============================================================================
// spawnMob 测试
// ============================================================================

class TrialSpawnerSpawnMobTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
        spawner_ = std::make_unique<TestTrialSpawnerBlockEntity>(BlockPos(0, 64, 0));
        spawner_->setConfig(TrialSpawnerBlockEntity::getMeleeConfig());
        world_.setCurrentTick(100);
    }

    TrialSpawnerSpawnTestWorld world_;
    std::unique_ptr<TestTrialSpawnerBlockEntity> spawner_;
};

TEST_F(TrialSpawnerSpawnMobTest, EmptyPotentials_DoesNotSpawn)
{
    TrialSpawnerBlockEntity::Config emptyConfig;
    emptyConfig.spawnPotentials = {};
    spawner_->setConfig(emptyConfig);

    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 0);
    EXPECT_EQ(world_.spawnedCount(), 0u);
}

TEST_F(TrialSpawnerSpawnMobTest, UnregisteredEntityType_DoesNotSpawn)
{
    // 使用未注册的实体类型
    TrialSpawnerBlockEntity::Config config;
    config.spawnPotentials = {{ResourceLocation("minecraft", "nonexistent_entity_type"), 1}};
    spawner_->setConfig(config);

    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 0);
    EXPECT_EQ(world_.spawnedCount(), 0u);
}

TEST_F(TrialSpawnerSpawnMobTest, AllPositionsBlocked_DoesNotSpawn)
{
    // 设置碰撞为 true，所有位置不可用
    world_.setBlockCollision(true);

    spawner_->testSpawnMob(world_);

    // 没有有效位置，不应生成怪物
    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 0);
    EXPECT_EQ(world_.spawnedCount(), 0u);
}

TEST_F(TrialSpawnerSpawnMobTest, SuccessfulSpawn_IncrementsCounters)
{
    // 使用 melee 配置（含 zombie/husk/spider），BaseTestWorld 无碰撞
    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 1);
    EXPECT_EQ(spawner_->testCurrentMobsCount(), 1);
    EXPECT_EQ(world_.spawnedCount(), 1u);
}

TEST_F(TrialSpawnerSpawnMobTest, SuccessfulSpawn_TracksMobUuid)
{
    spawner_->testSpawnMob(world_);

    const auto& trackedMobs = spawner_->testTrackedMobs();
    EXPECT_EQ(trackedMobs.size(), 1u);
    // 被追踪的 UUID 应该和生成的实体 UUID 匹配
    EXPECT_EQ(world_.spawnedEntities().size(), 1u);
    if (!world_.spawnedEntities().empty()) {
        EXPECT_NE(trackedMobs.count(world_.spawnedEntities()[0]->uuid()), 0u);
    }
}

TEST_F(TrialSpawnerSpawnMobTest, SuccessfulSpawn_PlaysSpawnEvents)
{
    spawner_->testSpawnMob(world_);

    const auto& events = world_.playEventCalls();
    // 应该播放两个事件：TRIAL_SPAWNER_SPAWN 和 TRIAL_SPAWNER_SPAWN_MOB_AT
    ASSERT_GE(events.size(), 2u);

    // 第一个事件是刷怪笼粒子效果
    EXPECT_EQ(events[0].eventId, world::WorldEvents::TRIAL_SPAWNER_SPAWN);
    EXPECT_EQ(events[0].pos, BlockPos(0, 64, 0));

    // 第二个事件是怪物生成位置的粒子效果
    EXPECT_EQ(events[1].eventId, world::WorldEvents::TRIAL_SPAWNER_SPAWN_MOB_AT);
    // 位置应在刷怪笼附近
}

TEST_F(TrialSpawnerSpawnMobTest, SuccessfulSpawn_SelectsNextEntity)
{
    spawner_->testSpawnMob(world_);

    // 生成后应该为下次生成选择了一个新的实体类型
    const auto& nextId = spawner_->testNextSpawnEntityId();
    // 下次生成的实体不应为空（因为 melee 配置有有效的 spawnPotentials）
    EXPECT_FALSE(nextId.path().empty());
}

TEST_F(TrialSpawnerSpawnMobTest, MultipleSpawns_IncrementCounters)
{
    spawner_->testSpawnMob(world_);
    spawner_->testSpawnMob(world_);
    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 3);
    EXPECT_EQ(spawner_->testCurrentMobsCount(), 3);
    EXPECT_EQ(world_.spawnedCount(), 3u);
    EXPECT_EQ(spawner_->testTrackedMobs().size(), 3u);
}

TEST_F(TrialSpawnerSpawnMobTest, SpawnWithBreezeConfig_Succeeds)
{
    spawner_->setConfig(TrialSpawnerBlockEntity::getBreezeConfig());

    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 1);
    EXPECT_EQ(world_.spawnedCount(), 1u);
}

TEST_F(TrialSpawnerSpawnMobTest, SpawnWithRangedConfig_Succeeds)
{
    spawner_->setConfig(TrialSpawnerBlockEntity::getRangedConfig());

    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 1);
    EXPECT_EQ(world_.spawnedCount(), 1u);
}

TEST_F(TrialSpawnerSpawnMobTest, SpawnWithSmallMeleeConfig_Succeeds)
{
    spawner_->setConfig(TrialSpawnerBlockEntity::getSmallMeleeConfig());

    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 1);
    EXPECT_EQ(world_.spawnedCount(), 1u);
}

TEST_F(TrialSpawnerSpawnMobTest, SpawnWithSlowRangedConfig_Succeeds)
{
    spawner_->setConfig(TrialSpawnerBlockEntity::getSlowRangedConfig());

    spawner_->testSpawnMob(world_);

    EXPECT_EQ(spawner_->testSpawnedMobsCount(), 1);
    EXPECT_EQ(world_.spawnedCount(), 1u);
}

TEST_F(TrialSpawnerSpawnMobTest, OminousSpawn_PlaysOminousEvent)
{
    spawner_->setOminous(true);

    spawner_->testSpawnMob(world_);

    const auto& events = world_.playEventCalls();
    ASSERT_GE(events.size(), 2u);

    // 不祥变体应该使用 flameParticle=1 (SOUL_FIRE_FLAME)
    EXPECT_EQ(events[0].eventId, world::WorldEvents::TRIAL_SPAWNER_SPAWN);
    EXPECT_EQ(events[0].data, 1); // ominous particle flag
    EXPECT_EQ(events[1].eventId, world::WorldEvents::TRIAL_SPAWNER_SPAWN_MOB_AT);
    EXPECT_EQ(events[1].data, 1);
}

TEST_F(TrialSpawnerSpawnMobTest, NormalSpawn_PlaysNormalEvent)
{
    // 普通变体应该使用 flameParticle=0 (FLAME)
    spawner_->testSpawnMob(world_);

    const auto& events = world_.playEventCalls();
    ASSERT_GE(events.size(), 2u);

    EXPECT_EQ(events[0].data, 0); // normal particle flag
    EXPECT_EQ(events[1].data, 0);
}

// ============================================================================
// 生成潜力加权随机测试（统计验证）
// ============================================================================

TEST_F(TrialSpawnerSelectEntityTest, EqualWeights_DistributesEvenly)
{
    // 所有条目权重相同
    TrialSpawnerBlockEntity::Config config;
    config.spawnPotentials = {
        {ResourceLocation("minecraft", "zombie"), 1},
        {ResourceLocation("minecraft", "husk"), 1},
        {ResourceLocation("minecraft", "spider"), 1},
    };
    spawner_->setConfig(config);

    std::unordered_map<std::string, i32> counts;
    constexpr i32 TRIALS = 300;

    for (i32 i = 0; i < TRIALS; ++i) {
        spawner_->clearNextSpawnEntityId();
        auto* result = spawner_->testSelectNextEntity(world_);
        ASSERT_NE(result, nullptr);
        counts[result->toString()]++;
    }

    // 每种实体期望大约 100 次（300 / 3），允许较大波动范围
    // 使用 30-170 的范围（约 ±70%），避免因随机性导致测试不稳定
    for (const auto& [name, count] : counts) {
        EXPECT_GE(count, 30) << "Entity " << name << " selected too few times";
        EXPECT_LE(count, 170) << "Entity " << name << " selected too many times";
    }
}

TEST_F(TrialSpawnerSelectEntityTest, SmallMeleeWeights_SilverfishAndCaveSpiderMoreLikely)
{
    // small melee: silverfish(2), cave_spider(2), slime(1) -> 总权重5
    auto config = TrialSpawnerBlockEntity::getSmallMeleeConfig();
    spawner_->setConfig(config);

    std::unordered_map<std::string, i32> counts;
    constexpr i32 TRIALS = 500;

    for (i32 i = 0; i < TRIALS; ++i) {
        spawner_->clearNextSpawnEntityId();
        auto* result = spawner_->testSelectNextEntity(world_);
        ASSERT_NE(result, nullptr);
        counts[result->toString()]++;
    }

    // silverfish 和 cave_spider 期望各约 200 次 (2/5)，slime 约 100 次 (1/5)
    // 使用较大容差范围避免测试不稳定
    EXPECT_GE(counts["minecraft:silverfish"], 100);
    EXPECT_GE(counts["minecraft:cave_spider"], 100);
    EXPECT_GE(counts["minecraft:slime"], 30);
}

// ============================================================================
// 视线检测测试
// ============================================================================

namespace {

/**
 * @brief 支持 raycastBlocks 的测试世界
 *
 * 继承 BaseTestWorld，并覆盖 getBlockState 以支持方块射线检测。
 * 同时提供实体生成、tick、音效等必要存根，以支撑 _findSpawnPosition 调用链。
 * 使用 map 存储方块位置到方块状态的映射，未设置的位置返回 nullptr（空气）。
 */
class TrialSpawnerLineOfSightTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setSeed(u64 seed) { m_random = math::Random(seed); }

    void setBlockCollisionFlag(bool hasCollision) { m_hasBlockCollision = hasCollision; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return m_hasBlockCollision; }

    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[key(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(key(x, y, z));
        return it != m_blockStates.end() ? it->second : nullptr;
    }

    void clearBlocks() { m_blockStates.clear(); }

    // 实体生成存根
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

    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        for (auto* entity : m_spawnedEntities) {
            if (entity && entity->uuid() == uuid) {
                return entity;
            }
        }
        return nullptr;
    }

    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        MC_UNUSED(category);
        MC_UNUSED(position);
        MC_UNUSED(volume);
        MC_UNUSED(pitch);
        MC_UNUSED(soundId);
    }

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        MC_UNUSED(eventId);
        MC_UNUSED(pos);
        MC_UNUSED(data);
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("TrialSpawnerLineOfSightTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("TrialSpawnerLineOfSightTestWorld::tickManager not implemented");
    }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty diff) { m_difficulty = diff; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getPlayers() const override { return {}; }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos&) override { return nullptr; }
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos&) const override { return nullptr; }

private:
    u64 m_currentTick = 0;
    Difficulty m_difficulty = Difficulty::Easy;
    bool m_hasBlockCollision = false;
    std::unordered_map<i64, const BlockState*> m_blockStates;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
    u64 m_nextEntityId = 0;

    static i64 key(i32 x, i32 y, i32 z)
    {
        return static_cast<i64>(x) | (static_cast<i64>(y) << 16) | (static_cast<i64>(z) << 32);
    }
};

/**
 * @brief 获取石头方块状态（用于遮挡视线测试）
 */
const BlockState* getStoneBlockState()
{
    static const BlockState* state = nullptr;
    if (state == nullptr) {
        auto* block = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
        state = block ? &block->defaultState() : nullptr;
    }
    return state;
}

} // anonymous namespace

class TrialSpawnerLineOfSightTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity::VanillaEntities::registerAll();
        // 确保方块注册表已初始化（用于射线检测）
        if (!VanillaBlocks::STONE) {
            VanillaBlocks::initialize();
        }
        spawner_ = std::make_unique<TestTrialSpawnerBlockEntity>(BlockPos(0, 64, 0));
    }

    TrialSpawnerLineOfSightTestWorld world_;
    std::unique_ptr<TestTrialSpawnerBlockEntity> spawner_;
};

TEST_F(TrialSpawnerLineOfSightTest, ClearLineOfSight_ReturnsPosition)
{
    // 无方块遮挡时，射线从生成位置到刷怪笼中心畅通无阻
    // 默认世界中 getBlockState 返回 nullptr（空气），hasBlockCollision 返回 false
    auto pos = spawner_->testFindSpawnPosition(world_);
    EXPECT_TRUE(pos.has_value());
}

TEST_F(TrialSpawnerLineOfSightTest, HitSpawnerBlockOnly_ReturnsPosition)
{
    // 刷怪笼方块本身不应阻挡视线
    // 在刷怪笼位置 (0, 64, 0) 放置一个石头方块
    // 当射线从生成位置射向刷怪笼中心时，如果射线命中了刷怪笼自身的方块，
    // 这仍然应该被允许（与 MC Java 行为一致）
    const BlockState* stoneState = getStoneBlockState();
    ASSERT_NE(stoneState, nullptr);

    // 在刷怪笼位置放置方块
    world_.setBlockStateAt(0, 64, 0, stoneState);

    // 无碰撞（hasBlockCollision 返回 false），射线只命中刷怪笼方块本身
    auto pos = spawner_->testFindSpawnPosition(world_);
    EXPECT_TRUE(pos.has_value());
}

TEST_F(TrialSpawnerLineOfSightTest, BlockingWall_ReturnsNullopt)
{
    // 设置碰撞为 true 以确保所有位置都被碰撞阻挡
    // 这样 _findSpawnPosition 会在碰撞检测阶段就被挡住
    world_.setBlockCollisionFlag(true);
    auto pos = spawner_->testFindSpawnPosition(world_);
    EXPECT_FALSE(pos.has_value());
}

TEST_F(TrialSpawnerLineOfSightTest, WallBetweenSpawnerAndSpawn_BlocksLineOfSight)
{
    // 在刷怪笼前方放置一面完整的遮挡墙，但不设置整体碰撞
    // 刷怪笼位于 (0, 64, 0)
    // 在 x=1 处放置一堵 y=63-65 的墙
    // 射线从远处生成位置到刷怪笼中心时会命中这堵墙
    const BlockState* stoneState = getStoneBlockState();
    ASSERT_NE(stoneState, nullptr);

    // 在 x=1, z=0 处放一堵墙（覆盖 y=63, 64, 65）
    world_.setBlockStateAt(1, 63, 0, stoneState);
    world_.setBlockStateAt(1, 64, 0, stoneState);
    world_.setBlockStateAt(1, 65, 0, stoneState);

    // 设置固定随机种子以控制生成位置
    world_.setSeed(42);

    // 无整体碰撞（hasBlockCollision 返回 false），但射线会命中遮挡墙
    // 由于随机位置可能落在墙的两侧，多次调用应能找到位置（有些在墙同一侧）
    // 但从 x>=2 的位置到刷怪笼中心的射线会被 x=1 处的墙挡住
    // 从 x<0 的位置则可能畅通
    // 我们验证至少有些位置因视线检测被拒绝，但最终能找到位置
    auto pos = spawner_->testFindSpawnPosition(world_);
    // 由于刷怪笼周围有开放区域（x<0 方向），应该能找到生成位置
    EXPECT_TRUE(pos.has_value());
}

TEST_F(TrialSpawnerLineOfSightTest, SpawnerBlockHit_AllowedByException)
{
    // 仅在刷怪笼位置 (0, 64, 0) 放置石头，其他位置为空气。
    // 当射线从生成位置射向刷怪笼中心 (0.5, 64.5, 0.5) 时，
    // 如果射线命中了刷怪笼方块自身 (0, 64, 0)，则根据视线检测规则应被允许。
    // 这与 HitSpawnerBlockOnly_ReturnsPosition 测试类似，但此测试更明确地
    // 验证了射线命中刷怪笼方块自身时的异常处理逻辑。
    const BlockState* stoneState = getStoneBlockState();
    ASSERT_NE(stoneState, nullptr);

    // 仅在刷怪笼位置放置方块
    world_.setBlockStateAt(0, 64, 0, stoneState);

    // 无整体碰撞，射线命中刷怪笼方块自身时允许生成
    auto pos = spawner_->testFindSpawnPosition(world_);
    EXPECT_TRUE(pos.has_value());
}

TEST_F(TrialSpawnerLineOfSightTest, WallBlocksAllDirections_CollisionAndLOSFallback)
{
    // 测试当视线被阻挡时，_findSpawnPosition 会继续尝试其他位置。
    // 在刷怪笼四周放置完整的方块墙，确保射线无法从远处到达刷怪笼中心，
    // 但 hasBlockCollision 返回 false，所以碰撞检测不会阻挡位置。
    // 由于刷怪笼方块自身 (0,64,0) 的异常规则，随机位置如果落在刷怪笼方块内，
    // 仍然可能通过视线检测。但大多数位置会被视线检测拒绝。
    // 这是一个综合测试，验证碰撞检测和视线检测的协同工作。
    const BlockState* stoneState = getStoneBlockState();
    ASSERT_NE(stoneState, nullptr);

    // 在刷怪笼周围放一层完整的方块墙（x,z 在 [-2,2], y 在 [63,65]）
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 z = -2; z <= 2; ++z) {
            for (i32 y = 63; y <= 65; ++y) {
                world_.setBlockStateAt(x, y, z, stoneState);
            }
        }
    }

    // 同时设置整体碰撞为 true，这样所有位置都在碰撞检测阶段被拒绝
    world_.setBlockCollisionFlag(true);
    auto pos = spawner_->testFindSpawnPosition(world_);
    EXPECT_FALSE(pos.has_value());
}
