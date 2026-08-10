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
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/trial/VaultBlockEntity.hpp"
#include <nlohmann/json.hpp>

using namespace mc;

// ============================================================================
// VaultTestWorld - 测试用 Mock 世界
// ============================================================================

class VaultTestWorld final : public mc::test::BaseTestWorld {
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
        throw std::runtime_error("VaultTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("VaultTestWorld::tickManager not implemented");
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
// VaultBlockEntity 构造和基本属性测试
// ============================================================================

class VaultBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { vault_ = std::make_unique<VaultBlockEntity>(BlockPos(10, 64, 20)); }

    std::unique_ptr<VaultBlockEntity> vault_;
};

TEST_F(VaultBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(vault_->getType(), BlockEntityType::Vault);
}

TEST_F(VaultBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(vault_->getPos(), BlockPos(10, 64, 20));
}

TEST_F(VaultBlockEntityTest, Create_DefaultStateIsInactive)
{
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Inactive);
}

TEST_F(VaultBlockEntityTest, Create_DefaultIsNotOminous)
{
    EXPECT_FALSE(vault_->isOminous());
}

TEST_F(VaultBlockEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(vault_->needsTick());
}

TEST_F(VaultBlockEntityTest, Create_RewardedPlayersIsEmpty)
{
    // 保存后验证 rewarded_players 为空数组
    nlohmann::json data;
    vault_->save(data);
    EXPECT_TRUE(data["rewarded_players"].is_array());
    EXPECT_EQ(data["rewarded_players"].size(), 0u);
}

// ============================================================================
// 状态设置测试
// ============================================================================

TEST_F(VaultBlockEntityTest, SetState_UpdatesState)
{
    vault_->setState(VaultBlockEntity::State::Active);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Active);

    vault_->setState(VaultBlockEntity::State::Unlocking);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Unlocking);

    vault_->setState(VaultBlockEntity::State::Ejecting);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Ejecting);

    vault_->setState(VaultBlockEntity::State::Inactive);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Inactive);
}

TEST_F(VaultBlockEntityTest, SetState_MarksChanged)
{
    EXPECT_FALSE(vault_->isChanged());
    vault_->setState(VaultBlockEntity::State::Active);
    EXPECT_TRUE(vault_->isChanged());
}

TEST_F(VaultBlockEntityTest, SetState_SameStateDoesNotMarkChanged)
{
    vault_->setState(VaultBlockEntity::State::Inactive);
    EXPECT_FALSE(vault_->isChanged());
}

TEST_F(VaultBlockEntityTest, SetOminous_UpdatesOminousAndConfig)
{
    EXPECT_FALSE(vault_->isOminous());
    EXPECT_EQ(vault_->getConfig().keyItem, VaultBlockEntity::getNormalConfig().keyItem);

    vault_->setOminous(true);
    EXPECT_TRUE(vault_->isOminous());
    EXPECT_EQ(vault_->getConfig().keyItem, VaultBlockEntity::getOminousConfig().keyItem);

    vault_->setOminous(false);
    EXPECT_FALSE(vault_->isOminous());
    EXPECT_EQ(vault_->getConfig().keyItem, VaultBlockEntity::getNormalConfig().keyItem);
}

TEST_F(VaultBlockEntityTest, SetOminous_MarksChanged)
{
    EXPECT_FALSE(vault_->isChanged());
    vault_->setOminous(true);
    EXPECT_TRUE(vault_->isChanged());
}

TEST_F(VaultBlockEntityTest, SetOminous_SameValueDoesNotMarkChanged)
{
    vault_->setOminous(false);
    EXPECT_FALSE(vault_->isChanged());
}

// ============================================================================
// 配置工厂方法测试
// ============================================================================

TEST_F(VaultBlockEntityTest, GetNormalConfig_HasCorrectValues)
{
    auto config = VaultBlockEntity::getNormalConfig();
    EXPECT_EQ(config.lootTable, ResourceLocation("minecraft", "chests/trial_chambers/reward"));
    EXPECT_EQ(config.ominousLootTable, ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous"));
    EXPECT_FLOAT_EQ(config.activationRange, 4.0f);
    EXPECT_FLOAT_EQ(config.deactivationRange, 4.5f);
    EXPECT_EQ(config.keyItem, Items::TRIAL_KEY);
}

TEST_F(VaultBlockEntityTest, GetOminousConfig_HasCorrectValues)
{
    auto config = VaultBlockEntity::getOminousConfig();
    EXPECT_EQ(config.lootTable, ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous"));
    EXPECT_EQ(config.ominousLootTable, ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous"));
    EXPECT_FLOAT_EQ(config.activationRange, 4.0f);
    EXPECT_FLOAT_EQ(config.deactivationRange, 4.5f);
    EXPECT_EQ(config.keyItem, Items::OMINOUS_TRIAL_KEY);
}

// ============================================================================
// 红石比较器输出测试
// ============================================================================

TEST_F(VaultBlockEntityTest, GetComparatorOutput_InactiveReturnsZero)
{
    vault_->setState(VaultBlockEntity::State::Inactive);
    EXPECT_EQ(vault_->getComparatorOutput(), 0);
}

TEST_F(VaultBlockEntityTest, GetComparatorOutput_ActiveReturnsZero)
{
    vault_->setState(VaultBlockEntity::State::Active);
    EXPECT_EQ(vault_->getComparatorOutput(), 0);
}

TEST_F(VaultBlockEntityTest, GetComparatorOutput_UnlockingReturnsFifteen)
{
    vault_->setState(VaultBlockEntity::State::Unlocking);
    EXPECT_EQ(vault_->getComparatorOutput(), 15);
}

TEST_F(VaultBlockEntityTest, GetComparatorOutput_EjectingReturnsFifteen)
{
    vault_->setState(VaultBlockEntity::State::Ejecting);
    EXPECT_EQ(vault_->getComparatorOutput(), 15);
}

// ============================================================================
// 序列化测试
// ============================================================================

TEST_F(VaultBlockEntityTest, SaveLoad_PreservesState)
{
    vault_->setState(VaultBlockEntity::State::Active);
    nlohmann::json data;
    vault_->save(data);

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getState(), VaultBlockEntity::State::Active);
}

TEST_F(VaultBlockEntityTest, SaveLoad_PreservesOminous)
{
    vault_->setOminous(true);
    nlohmann::json data;
    vault_->save(data);

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_TRUE(loaded->isOminous());
    // 不祥配置应被恢复
    EXPECT_EQ(loaded->getConfig().keyItem, Items::OMINOUS_TRIAL_KEY);
}

TEST_F(VaultBlockEntityTest, SaveLoad_PreservesRewardedPlayers)
{
    nlohmann::json data;
    data["state"] = 0;
    data["ominous"] = false;
    data["rewarded_players"] = nlohmann::json::array({"uuid-1", "uuid-2", "uuid-3"});
    data["unlocking_start_tick"] = 0;
    data["ejection_end_tick"] = 0;
    data["unlocking_player_uuid"] = "";
    data["last_insert_fail_sound_tick"] = 0;

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    nlohmann::json savedData;
    loaded->save(savedData);
    EXPECT_TRUE(savedData.contains("rewarded_players"));
    EXPECT_EQ(savedData["rewarded_players"].size(), 3u);
}

TEST_F(VaultBlockEntityTest, SaveLoad_PreservesUnlockingState)
{
    vault_->setState(VaultBlockEntity::State::Unlocking);
    nlohmann::json data;
    vault_->save(data);

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getState(), VaultBlockEntity::State::Unlocking);
}

TEST_F(VaultBlockEntityTest, SaveLoad_PreservesEjectingState)
{
    vault_->setState(VaultBlockEntity::State::Ejecting);
    nlohmann::json data;
    vault_->save(data);

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getState(), VaultBlockEntity::State::Ejecting);
}

TEST_F(VaultBlockEntityTest, SaveLoad_PreservesTickFields)
{
    nlohmann::json data;
    data["state"] = 2;
    data["ominous"] = false;
    data["rewarded_players"] = nlohmann::json::array();
    data["unlocking_start_tick"] = 100;
    data["ejection_end_tick"] = 200;
    data["unlocking_player_uuid"] = "test-uuid";
    data["last_insert_fail_sound_tick"] = 50;

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    nlohmann::json savedData;
    loaded->save(savedData);
    EXPECT_EQ(savedData["unlocking_start_tick"].get<i64>(), 100);
    EXPECT_EQ(savedData["ejection_end_tick"].get<i64>(), 200);
    EXPECT_EQ(savedData["unlocking_player_uuid"].get<std::string>(), "test-uuid");
    EXPECT_EQ(savedData["last_insert_fail_sound_tick"].get<i64>(), 50);
}

TEST_F(VaultBlockEntityTest, SaveLoad_PreservesAllStates)
{
    for (int i = 0; i <= 3; ++i) {
        auto state = static_cast<VaultBlockEntity::State>(i);
        vault_->setState(state);
        nlohmann::json data;
        vault_->save(data);

        auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
        ASSERT_TRUE(loaded->load(data));
        EXPECT_EQ(loaded->getState(), state);
    }
}

// ============================================================================
// Clone 测试
// ============================================================================

TEST_F(VaultBlockEntityTest, Clone_CreatesDeepCopy)
{
    vault_->setState(VaultBlockEntity::State::Active);
    vault_->setOminous(true);

    std::unique_ptr<BlockEntity> copy = vault_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Vault);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 64, 20));

    auto* vaultCopy = static_cast<VaultBlockEntity*>(copy.get());
    EXPECT_EQ(vaultCopy->getState(), VaultBlockEntity::State::Active);
    EXPECT_TRUE(vaultCopy->isOminous());
}

// ============================================================================
// 状态机测试（通过 tick 调度器）
// ============================================================================

class VaultStateMachineTest : public ::testing::Test {
protected:
    void SetUp() override { vault_ = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0)); }

    VaultTestWorld world_;
    std::unique_ptr<VaultBlockEntity> vault_;
};

// Inactive -> Active: 玩家进入激活范围
TEST_F(VaultStateMachineTest, Tick_InactiveToActiveWhenPlayerInRange)
{
    world_.setCurrentTick(100);

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    world_.setEntitiesInRangeResult({&player});

    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Active);
}

// Inactive: 无玩家时保持 Inactive
TEST_F(VaultStateMachineTest, Tick_InactiveStaysInactiveWhenNoPlayer)
{
    world_.setCurrentTick(100);
    world_.setEntitiesInRangeResult({});

    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Inactive);
}

// Inactive: 检测间隔内不执行检测（tick < STATE_UPDATE_INTERVAL）
TEST_F(VaultStateMachineTest, Tick_InactiveSkipsDetectionWithinInterval)
{
    world_.setCurrentTick(5); // 远小于 STATE_UPDATE_INTERVAL(20)

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    world_.setEntitiesInRangeResult({&player});

    vault_->tick(world_);
    // 状态应保持 Inactive（检测间隔未到）
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Inactive);
}

// Active -> Inactive: 玩家离开失活范围
TEST_F(VaultStateMachineTest, Tick_ActiveToInactiveWhenNoPlayer)
{
    world_.setCurrentTick(100);
    world_.setEntitiesInRangeResult({});

    vault_->setState(VaultBlockEntity::State::Active);
    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Inactive);
}

// Active: 玩家仍在失活范围内时保持 Active
TEST_F(VaultStateMachineTest, Tick_ActiveStaysActiveWhenPlayerPresent)
{
    world_.setCurrentTick(100);

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    world_.setEntitiesInRangeResult({&player});

    vault_->setState(VaultBlockEntity::State::Active);
    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Active);
}

// Active: 检测间隔内不执行检测
TEST_F(VaultStateMachineTest, Tick_ActiveSkipsDetectionWithinInterval)
{
    world_.setCurrentTick(5);
    world_.setEntitiesInRangeResult({});

    vault_->setState(VaultBlockEntity::State::Active);
    vault_->tick(world_);
    // 间隔未到，应保持 Active
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Active);
}

// 旁观者模式玩家不触发激活
TEST_F(VaultStateMachineTest, Tick_IgnoresSpectatorPlayer)
{
    world_.setCurrentTick(100);

    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    player.setGameMode(GameMode::Spectator);
    world_.setEntitiesInRangeResult({&player});

    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Inactive);
}

// 完整的状态循环: Inactive -> Active -> Inactive
TEST_F(VaultStateMachineTest, Tick_FullCycleInactiveActiveInactive)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");

    // Tick 100: 玩家进入 -> Inactive -> Active
    world_.setCurrentTick(100);
    world_.setEntitiesInRangeResult({&player});
    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Active);

    // Tick 120: 玩家仍在 -> 保持 Active
    world_.setCurrentTick(120);
    world_.setEntitiesInRangeResult({&player});
    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Active);

    // Tick 140: 玩家离开 -> Active -> Inactive
    world_.setCurrentTick(140);
    world_.setEntitiesInRangeResult({});
    vault_->tick(world_);
    EXPECT_EQ(vault_->getState(), VaultBlockEntity::State::Inactive);
}

// tryInsertKey: Inactive 状态下拒绝插入钥匙
TEST_F(VaultStateMachineTest, TryInsertKey_RejectsWhenInactive)
{
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    player.setUuid("test-uuid-1");
    player.setWorld(&world_);

    vault_->setState(VaultBlockEntity::State::Inactive);
    EXPECT_FALSE(vault_->tryInsertKey(player));
}

// ============================================================================
// 自定义配置测试
// ============================================================================

TEST_F(VaultBlockEntityTest, SetConfig_UpdatesConfig)
{
    VaultBlockEntity::Config customConfig;
    customConfig.activationRange = 8.0f;
    customConfig.deactivationRange = 10.0f;
    customConfig.keyItem = nullptr;

    vault_->setConfig(customConfig);
    EXPECT_FLOAT_EQ(vault_->getConfig().activationRange, 8.0f);
    EXPECT_FLOAT_EQ(vault_->getConfig().deactivationRange, 10.0f);
    EXPECT_EQ(vault_->getConfig().keyItem, nullptr);
}

TEST_F(VaultBlockEntityTest, SetConfig_MarksChanged)
{
    EXPECT_FALSE(vault_->isChanged());
    VaultBlockEntity::Config customConfig;
    vault_->setConfig(customConfig);
    EXPECT_TRUE(vault_->isChanged());
}

// ============================================================================
// SaveLoad 保留配置状态
// ============================================================================

TEST_F(VaultBlockEntityTest, SaveLoad_OminousRestoresCorrectConfig)
{
    vault_->setOminous(true);
    nlohmann::json data;
    vault_->save(data);

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    // 不祥配置应被恢复
    EXPECT_TRUE(loaded->isOminous());
    EXPECT_EQ(loaded->getConfig().keyItem, Items::OMINOUS_TRIAL_KEY);
    EXPECT_EQ(loaded->getConfig().lootTable.toString(),
        ResourceLocation("minecraft", "chests/trial_chambers/reward_ominous").toString());
}

TEST_F(VaultBlockEntityTest, SaveLoad_NormalPreservesNormalConfig)
{
    // 默认为普通宝库
    nlohmann::json data;
    vault_->save(data);

    auto loaded = std::make_unique<VaultBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_FALSE(loaded->isOminous());
    EXPECT_EQ(loaded->getConfig().keyItem, Items::TRIAL_KEY);
    EXPECT_EQ(loaded->getConfig().lootTable.toString(),
        ResourceLocation("minecraft", "chests/trial_chambers/reward").toString());
}

// ============================================================================
// VaultBlockEntity 常量验证（通过配置和序列化间接测试）
// ============================================================================

TEST_F(VaultBlockEntityTest, NormalConfig_ActivationRangeIsFour)
{
    auto config = VaultBlockEntity::getNormalConfig();
    EXPECT_FLOAT_EQ(config.activationRange, 4.0f);
}

TEST_F(VaultBlockEntityTest, NormalConfig_DeactivationRangeIsFourPointFive)
{
    auto config = VaultBlockEntity::getNormalConfig();
    EXPECT_FLOAT_EQ(config.deactivationRange, 4.5f);
}

TEST_F(VaultBlockEntityTest, OminousConfig_SameRangesAsNormal)
{
    auto normalConfig = VaultBlockEntity::getNormalConfig();
    auto ominousConfig = VaultBlockEntity::getOminousConfig();
    EXPECT_FLOAT_EQ(normalConfig.activationRange, ominousConfig.activationRange);
    EXPECT_FLOAT_EQ(normalConfig.deactivationRange, ominousConfig.deactivationRange);
}

TEST_F(VaultBlockEntityTest, OminousConfig_DifferentKeyItemOrBothUninitialized)
{
    auto normalConfig = VaultBlockEntity::getNormalConfig();
    auto ominousConfig = VaultBlockEntity::getOminousConfig();
    // 如果 Items 已初始化，两个 keyItem 应该不同
    // 如果 Items 未初始化（测试环境），两个 keyItem 都是 nullptr
    if (normalConfig.keyItem != nullptr && ominousConfig.keyItem != nullptr) {
        EXPECT_NE(normalConfig.keyItem, ominousConfig.keyItem);
    } else {
        // Items 未初始化时，两个 keyItem 都应该是 nullptr
        EXPECT_EQ(normalConfig.keyItem, nullptr);
        EXPECT_EQ(ominousConfig.keyItem, nullptr);
    }
}
