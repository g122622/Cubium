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
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/trial/TrialSpawnerBlockEntity.hpp"
#include <nlohmann/json.hpp>

using namespace mc;

// ============================================================================
// TrialSpawnerTestWorld - 测试用 Mock 世界
// ============================================================================

class TrialSpawnerTestWorld final : public test::BaseTestWorld {
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

    Player player(1, "TestPlayer");
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

    Player player(1, "TestPlayer");
    player.setUuid("test-uuid-1");
    world_.setEntitiesInRangeResult({&player});

    spawner_->tick(world_);
    EXPECT_EQ(spawner_->getState(), TrialSpawnerBlockEntity::State::Inactive);
}

// 旁观者模式玩家不触发激活
TEST_F(TrialSpawnerStateMachineTest, Tick_InactiveIgnoresSpectatorPlayer)
{
    world_.setCurrentTick(100);

    Player player(1, "TestPlayer");
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

    Player player(1, "TestPlayer");
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
    Player player1(1, "TestPlayer1");
    player1.setUuid("uuid-1");
    Player player2(2, "TestPlayer2");
    player2.setUuid("uuid-2");
    world_.setEntitiesInRangeResult({&player1, &player2});

    auto players = spawner_->detectPlayers(world_, 14.0f);
    EXPECT_EQ(players.size(), 2u);
}

TEST_F(TrialSpawnerStateMachineTest, DetectPlayers_ExcludesSpectators)
{
    Player player1(1, "TestPlayer1");
    player1.setUuid("uuid-1");
    Player spectator(2, "SpectatorPlayer");
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
    Player spectator(1, "SpectatorPlayer");
    spectator.setUuid("uuid-1");
    spectator.setGameMode(GameMode::Spectator);
    world_.setEntitiesInRangeResult({&spectator});

    auto players = spawner_->detectPlayers(world_, 14.0f);
    EXPECT_TRUE(players.empty());
}
