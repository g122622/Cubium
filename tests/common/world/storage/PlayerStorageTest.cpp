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

#include "common/entity/entities/player/Player.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "world/storage/db/ColumnFamilies.hpp"
#include "world/storage/db/RocksDBDatabase.hpp"
#include "world/storage/player/PlayerDataManager.hpp"
#include "world/storage/player/PlayerSaveData.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <gtest/gtest.h>

namespace mc::world::storage {
namespace {

// 测试临时目录
class PlayerStorageTest : public ::testing::Test {
protected:
    std::filesystem::path m_testDir;

    void SetUp() override
    {
        // 创建临时测试目录
        m_testDir = std::filesystem::temp_directory_path() / "player_storage_test";
        std::filesystem::create_directories(m_testDir);
    }

    void TearDown() override
    {
        // 清理临时目录
        if (std::filesystem::exists(m_testDir)) {
            std::filesystem::remove_all(m_testDir);
        }
    }
};

// ============================================================================
// PlayerSaveData 测试
// ============================================================================

class PlayerSaveDataTest : public PlayerStorageTest {};

TEST_F(PlayerSaveDataTest, DefaultConstruction)
{
    PlayerSaveData data;

    EXPECT_TRUE(data.uuid.empty());
    EXPECT_TRUE(data.username.empty());
    EXPECT_EQ(data.posX, 0.0);
    EXPECT_EQ(data.posY, 64.0); // 默认出生高度
    EXPECT_EQ(data.posZ, 0.0);
    EXPECT_EQ(data.yaw, 0.0f);
    EXPECT_EQ(data.pitch, 0.0f);
    EXPECT_EQ(data.dimension, 0);
    EXPECT_EQ(data.gameMode, GameMode::Survival);
    EXPECT_EQ(data.health, 20.0f);
    EXPECT_EQ(data.foodLevel, 20);
    EXPECT_EQ(data.experienceLevel, 0);
    EXPECT_EQ(data.selectedSlot, 0);
    EXPECT_EQ(data.airSupply, 300);
}

TEST_F(PlayerSaveDataTest, ParameterizedConstruction)
{
    PlayerSaveData data("test-uuid-123", "TestPlayer");

    EXPECT_EQ(data.uuid, "test-uuid-123");
    EXPECT_EQ(data.username, "TestPlayer");
}

TEST_F(PlayerSaveDataTest, NbtSerializationBasic)
{
    PlayerSaveData original;
    original.uuid = "test-uuid-456";
    original.username = "Steve";
    original.posX = 100.5;
    original.posY = 64.0;
    original.posZ = -200.25;
    original.yaw = 90.0f;
    original.pitch = 45.0f;
    original.dimension = 0;
    original.gameMode = GameMode::Creative;
    original.health = 15.0f;
    original.foodLevel = 18;
    original.saturationLevel = 4.5f;
    original.experienceLevel = 10;
    original.experienceProgress = 0.75f;
    original.totalExperience = 150;
    original.selectedSlot = 5;

    // 序列化到 NBT
    nbt::tags::compound_tag nbt = original.toNbt();

    // 验证一些基本字段
    EXPECT_EQ(nbt.get<nbt::tags::string_tag>("UUID"), "test-uuid-456");
    EXPECT_EQ(nbt.get<nbt::tags::string_tag>("Name"), "Steve");
    EXPECT_EQ(nbt.get<nbt::tags::int_tag>("Dimension"), 0);
    EXPECT_EQ(nbt.get<nbt::tags::int_tag>("playerGameType"), static_cast<i32>(GameMode::Creative));
    EXPECT_EQ(nbt.get<nbt::tags::float_tag>("Health"), 15.0f);
    EXPECT_EQ(nbt.get<nbt::tags::int_tag>("foodLevel"), 18);
    EXPECT_EQ(nbt.get<nbt::tags::int_tag>("XpLevel"), 10);
}

TEST_F(PlayerSaveDataTest, NbtRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "roundtrip-test";
    original.username = "Alex";
    original.posX = 1234.567;
    original.posY = 70.0;
    original.posZ = -567.890;
    original.yaw = 180.0f;
    original.pitch = -30.0f;
    original.dimension = -1; // Nether
    original.gameMode = GameMode::Survival;
    original.health = 12.5f;
    original.maxHealth = 20.0f;
    original.foodLevel = 15;
    original.saturationLevel = 3.2f;
    original.exhaustionLevel = 0.8f;
    original.foodTickTimer = 100;
    original.experienceLevel = 25;
    original.experienceProgress = 0.5f;
    original.totalExperience = 500;
    original.xpSeed = 12345;
    original.invulnerable = true;
    original.canFly = true;
    original.flying = false;
    original.flySpeed = 0.1f;
    original.walkSpeed = 0.2f;
    original.selectedSlot = 3;
    original.airSupply = 250;
    original.maxAirSupply = 300;
    original.onGround = true;
    original.sprinting = true;
    original.sneaking = false;

    // 设置重生点
    original.spawnPoint = GlobalPos(0, BlockPos(100, 64, 200));
    original.spawnForced = true;

    // 序列化
    nbt::tags::compound_tag nbt = original.toNbt();

    // 反序列化
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();

    // 验证所有字段
    EXPECT_EQ(restored.uuid, original.uuid);
    EXPECT_EQ(restored.username, original.username);
    EXPECT_DOUBLE_EQ(restored.posX, original.posX);
    EXPECT_DOUBLE_EQ(restored.posY, original.posY);
    EXPECT_DOUBLE_EQ(restored.posZ, original.posZ);
    EXPECT_FLOAT_EQ(restored.yaw, original.yaw);
    EXPECT_FLOAT_EQ(restored.pitch, original.pitch);
    EXPECT_EQ(restored.dimension, original.dimension);
    EXPECT_EQ(restored.gameMode, original.gameMode);
    EXPECT_FLOAT_EQ(restored.health, original.health);
    EXPECT_FLOAT_EQ(restored.maxHealth, original.maxHealth);
    EXPECT_EQ(restored.foodLevel, original.foodLevel);
    EXPECT_FLOAT_EQ(restored.saturationLevel, original.saturationLevel);
    EXPECT_FLOAT_EQ(restored.exhaustionLevel, original.exhaustionLevel);
    EXPECT_EQ(restored.foodTickTimer, original.foodTickTimer);
    EXPECT_EQ(restored.experienceLevel, original.experienceLevel);
    EXPECT_FLOAT_EQ(restored.experienceProgress, original.experienceProgress);
    EXPECT_EQ(restored.totalExperience, original.totalExperience);
    EXPECT_EQ(restored.xpSeed, original.xpSeed);
    EXPECT_EQ(restored.invulnerable, original.invulnerable);
    EXPECT_EQ(restored.canFly, original.canFly);
    EXPECT_EQ(restored.flying, original.flying);
    EXPECT_FLOAT_EQ(restored.flySpeed, original.flySpeed);
    EXPECT_FLOAT_EQ(restored.walkSpeed, original.walkSpeed);
    EXPECT_EQ(restored.selectedSlot, original.selectedSlot);
    EXPECT_EQ(restored.airSupply, original.airSupply);
    EXPECT_EQ(restored.maxAirSupply, original.maxAirSupply);
    EXPECT_EQ(restored.onGround, original.onGround);
    EXPECT_EQ(restored.sprinting, original.sprinting);
    EXPECT_EQ(restored.sneaking, original.sneaking);

    // 验证重生点
    ASSERT_TRUE(restored.spawnPoint.has_value());
    EXPECT_EQ(restored.spawnPoint->x(), original.spawnPoint->x());
    EXPECT_EQ(restored.spawnPoint->y(), original.spawnPoint->y());
    EXPECT_EQ(restored.spawnPoint->z(), original.spawnPoint->z());
    EXPECT_EQ(restored.spawnPoint->getDimensionId(), original.spawnPoint->getDimensionId());
    EXPECT_EQ(restored.spawnForced, original.spawnForced);
}

TEST_F(PlayerSaveDataTest, BinarySerialization)
{
    PlayerSaveData original;
    original.uuid = "binary-test";
    original.username = "BinaryPlayer";
    original.posX = 500.0;
    original.posY = 80.0;
    original.posZ = -300.0;
    original.health = 18.0f;
    original.foodLevel = 19;

    // 序列化到二进制（带压缩）
    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& binaryData = serializeResult.value();
    EXPECT_FALSE(binaryData.empty());

    // 反序列化
    auto deserializeResult = PlayerSaveData::deserialize(binaryData);
    ASSERT_TRUE(deserializeResult.success());

    const PlayerSaveData& restored = deserializeResult.value();

    EXPECT_EQ(restored.uuid, original.uuid);
    EXPECT_EQ(restored.username, original.username);
    EXPECT_DOUBLE_EQ(restored.posX, original.posX);
    EXPECT_DOUBLE_EQ(restored.posY, original.posY);
    EXPECT_DOUBLE_EQ(restored.posZ, original.posZ);
    EXPECT_FLOAT_EQ(restored.health, original.health);
    EXPECT_EQ(restored.foodLevel, original.foodLevel);
}

TEST_F(PlayerSaveDataTest, SpawnPointRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "spawn-test";
    original.spawnPoint = GlobalPos(-1, BlockPos(-100, 70, 300)); // Nether spawn
    original.spawnForced = true;

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.spawnPoint.has_value());
    EXPECT_EQ(restored.spawnPoint->x(), -100);
    EXPECT_EQ(restored.spawnPoint->y(), 70);
    EXPECT_EQ(restored.spawnPoint->z(), 300);
    EXPECT_EQ(restored.spawnPoint->getDimensionId(), -1);
    EXPECT_TRUE(restored.spawnForced);
}

TEST_F(PlayerSaveDataTest, AbilitiesRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "abilities-test";
    original.invulnerable = true;
    original.canFly = true;
    original.flying = true;
    original.flySpeed = 0.15f;
    original.walkSpeed = 0.25f;

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_TRUE(restored.invulnerable);
    EXPECT_TRUE(restored.canFly);
    EXPECT_TRUE(restored.flying);
    EXPECT_FLOAT_EQ(restored.flySpeed, 0.15f);
    EXPECT_FLOAT_EQ(restored.walkSpeed, 0.25f);
}

TEST_F(PlayerSaveDataTest, ExperienceRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "xp-test";
    original.experienceLevel = 30;
    original.experienceProgress = 0.75f;
    original.totalExperience = 1500;
    original.xpSeed = 99999;

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_EQ(restored.experienceLevel, 30);
    EXPECT_FLOAT_EQ(restored.experienceProgress, 0.75f);
    EXPECT_EQ(restored.totalExperience, 1500);
    EXPECT_EQ(restored.xpSeed, 99999);
}

TEST_F(PlayerSaveDataTest, EmptyEffects)
{
    PlayerSaveData original;
    original.uuid = "effects-empty";
    // 不添加任何效果

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_TRUE(restored.effects.empty());
}

TEST_F(PlayerSaveDataTest, EnteredNetherPosition)
{
    PlayerSaveData original;
    original.uuid = "nether-pos-test";
    original.enteredNetherPosition = Vector3d(100.5, 64.0, -200.5);

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.enteredNetherPosition.has_value());
    EXPECT_DOUBLE_EQ(restored.enteredNetherPosition->x, 100.5);
    EXPECT_DOUBLE_EQ(restored.enteredNetherPosition->y, 64.0);
    EXPECT_DOUBLE_EQ(restored.enteredNetherPosition->z, -200.5);
}

TEST_F(PlayerSaveDataTest, SleepingState)
{
    PlayerSaveData original;
    original.uuid = "sleep-test";
    original.sleeping = true;
    original.sleepTimer = 100;
    original.sleepingPosition = BlockPos(50, 64, 100);

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_TRUE(restored.sleeping);
    EXPECT_EQ(restored.sleepTimer, 100);
    ASSERT_TRUE(restored.sleepingPosition.has_value());
    EXPECT_EQ(restored.sleepingPosition->x, 50);
    EXPECT_EQ(restored.sleepingPosition->y, 64);
    EXPECT_EQ(restored.sleepingPosition->z, 100);
}

// ========== 冲量上下文序列化测试 ==========

TEST_F(PlayerSaveDataTest, ImpulseContext_DefaultValues_RoundTrip)
{
    // 默认状态：无冲量上下文
    PlayerSaveData original;
    original.uuid = "impulse-default-test";

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_FALSE(restored.currentImpulseImpactPos.has_value());
    EXPECT_FALSE(restored.ignoreFallDamageFromCurrentImpulse);
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 0);
}

TEST_F(PlayerSaveDataTest, ImpulseContext_WithImpactPosition_RoundTrip)
{
    PlayerSaveData original;
    original.uuid = "impulse-pos-test";
    original.currentImpulseImpactPos = Vector3(100.0f, 64.0f, 200.0f);
    original.ignoreFallDamageFromCurrentImpulse = true;
    original.currentImpulseContextResetGraceTime = 40;

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.currentImpulseImpactPos.has_value());
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->x, 100.0f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->y, 64.0f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->z, 200.0f);
    EXPECT_TRUE(restored.ignoreFallDamageFromCurrentImpulse);
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 40);
}

TEST_F(PlayerSaveDataTest, ImpulseContext_IgnoreWithoutPosition)
{
    // 可以只设置 ignoreFallDamage 标志而不设置冲击位置
    PlayerSaveData original;
    original.uuid = "impulse-ignore-only-test";
    original.ignoreFallDamageFromCurrentImpulse = true;
    original.currentImpulseContextResetGraceTime = 20;

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_FALSE(restored.currentImpulseImpactPos.has_value());
    EXPECT_TRUE(restored.ignoreFallDamageFromCurrentImpulse);
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 20);
}

TEST_F(PlayerSaveDataTest, ImpulseContext_PositionWithoutIgnore)
{
    // 可以有冲击位置但不需要忽略坠落伤害
    PlayerSaveData original;
    original.uuid = "impulse-pos-no-ignore-test";
    original.currentImpulseImpactPos = Vector3(50.0f, 70.0f, 80.0f);
    original.ignoreFallDamageFromCurrentImpulse = false;
    original.currentImpulseContextResetGraceTime = 0;

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.currentImpulseImpactPos.has_value());
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->x, 50.0f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->y, 70.0f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->z, 80.0f);
    EXPECT_FALSE(restored.ignoreFallDamageFromCurrentImpulse);
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 0);
}

TEST_F(PlayerSaveDataTest, ImpulseContext_NegativeCoordinates_RoundTrip)
{
    // 测试负坐标值
    PlayerSaveData original;
    original.uuid = "impulse-negative-test";
    original.currentImpulseImpactPos = Vector3(-100.5f, -64.0f, -200.3f);
    original.ignoreFallDamageFromCurrentImpulse = true;
    original.currentImpulseContextResetGraceTime = 10;

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.currentImpulseImpactPos.has_value());
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->x, -100.5f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->y, -64.0f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->z, -200.3f);
    EXPECT_TRUE(restored.ignoreFallDamageFromCurrentImpulse);
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 10);
}

TEST_F(PlayerSaveDataTest, ImpulseContext_MissingFieldsInNbt_Defaults)
{
    // 测试从缺少冲量上下文字段的 NBT 反序列化时使用默认值
    nbt::tags::compound_tag tag;
    tag.put("UUID", std::string("impulse-missing-test"));

    auto result = PlayerSaveData::fromNbt(tag);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_FALSE(restored.currentImpulseImpactPos.has_value());
    EXPECT_FALSE(restored.ignoreFallDamageFromCurrentImpulse);
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 0);
}

TEST_F(PlayerSaveDataTest, ImpulseContext_BinarySerializationRoundTrip)
{
    // 测试通过二进制序列化的往返一致性
    PlayerSaveData original;
    original.uuid = "impulse-binary-test";
    original.username = "TestPlayer";
    original.currentImpulseImpactPos = Vector3(123.45f, 67.89f, -234.56f);
    original.ignoreFallDamageFromCurrentImpulse = true;
    original.currentImpulseContextResetGraceTime = 35;

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = PlayerSaveData::deserialize(serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    const PlayerSaveData& restored = deserializeResult.value();
    ASSERT_TRUE(restored.currentImpulseImpactPos.has_value());
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->x, 123.45f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->y, 67.89f);
    EXPECT_FLOAT_EQ(restored.currentImpulseImpactPos->z, -234.56f);
    EXPECT_TRUE(restored.ignoreFallDamageFromCurrentImpulse);
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 35);
}

TEST_F(PlayerSaveDataTest, ImpulseContext_WindBurstExtendedGraceTime)
{
    // 测试风爆附魔扩展宽限期（10 tick）后的序列化
    PlayerSaveData original;
    original.uuid = "impulse-windburst-test";
    original.currentImpulseImpactPos = Vector3(200.0f, 50.0f, 300.0f);
    original.ignoreFallDamageFromCurrentImpulse = true;
    original.currentImpulseContextResetGraceTime = 40; // setIgnoreFallDamageFromCurrentImpulse(true) 设置的 40 tick

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_EQ(restored.currentImpulseContextResetGraceTime, 40);
    EXPECT_TRUE(restored.ignoreFallDamageFromCurrentImpulse);
}

// ========== LastDeathLocation 序列化测试 ==========

TEST_F(PlayerSaveDataTest, LastDeathLocationOverworldRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "ldl-overworld-test";
    original.lastDeathLocation = GlobalPos(0, BlockPos(100, 64, -200));

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.lastDeathLocation.has_value());
    EXPECT_EQ(restored.lastDeathLocation->getDimensionId(), 0);
    EXPECT_EQ(restored.lastDeathLocation->x(), 100);
    EXPECT_EQ(restored.lastDeathLocation->y(), 64);
    EXPECT_EQ(restored.lastDeathLocation->z(), -200);
}

TEST_F(PlayerSaveDataTest, LastDeathLocationNetherRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "ldl-nether-test";
    original.lastDeathLocation = GlobalPos(-1, BlockPos(50, 30, -100));

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.lastDeathLocation.has_value());
    EXPECT_EQ(restored.lastDeathLocation->getDimensionId(), -1);
    EXPECT_EQ(restored.lastDeathLocation->x(), 50);
    EXPECT_EQ(restored.lastDeathLocation->y(), 30);
    EXPECT_EQ(restored.lastDeathLocation->z(), -100);
}

TEST_F(PlayerSaveDataTest, LastDeathLocationEndRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "ldl-end-test";
    original.lastDeathLocation = GlobalPos(1, BlockPos(-300, 80, 150));

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.lastDeathLocation.has_value());
    EXPECT_EQ(restored.lastDeathLocation->getDimensionId(), 1);
    EXPECT_EQ(restored.lastDeathLocation->x(), -300);
    EXPECT_EQ(restored.lastDeathLocation->y(), 80);
    EXPECT_EQ(restored.lastDeathLocation->z(), 150);
}

TEST_F(PlayerSaveDataTest, LastDeathLocationEmpty)
{
    PlayerSaveData original;
    original.uuid = "ldl-empty-test";
    // lastDeathLocation 默认为 std::nullopt

    nbt::tags::compound_tag nbt = original.toNbt();

    // 验证反序列化时 lastDeathLocation 为空
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_FALSE(restored.lastDeathLocation.has_value());
}

TEST_F(PlayerSaveDataTest, LastDeathLocationBinarySerializationRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "ldl-binary-test";
    original.username = "DeathTestPlayer";
    original.lastDeathLocation = GlobalPos(-1, BlockPos(200, 50, 300));

    auto serializeResult = original.serialize();
    ASSERT_TRUE(serializeResult.success());

    auto deserializeResult = PlayerSaveData::deserialize(serializeResult.value());
    ASSERT_TRUE(deserializeResult.success());

    const PlayerSaveData& restored = deserializeResult.value();
    ASSERT_TRUE(restored.lastDeathLocation.has_value());
    EXPECT_EQ(restored.lastDeathLocation->getDimensionId(), -1);
    EXPECT_EQ(restored.lastDeathLocation->x(), 200);
    EXPECT_EQ(restored.lastDeathLocation->y(), 50);
    EXPECT_EQ(restored.lastDeathLocation->z(), 300);
}

TEST_F(PlayerSaveDataTest, LastDeathLocationNegativeCoordinatesRoundTrip)
{
    PlayerSaveData original;
    original.uuid = "ldl-negative-coords-test";
    original.lastDeathLocation = GlobalPos(0, BlockPos(-1000000, -64, -2000000));

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    ASSERT_TRUE(restored.lastDeathLocation.has_value());
    EXPECT_EQ(restored.lastDeathLocation->x(), -1000000);
    EXPECT_EQ(restored.lastDeathLocation->y(), -64);
    EXPECT_EQ(restored.lastDeathLocation->z(), -2000000);
}

// ============================================================================
// PlayerDataManager 测试
// ============================================================================

class PlayerDataManagerTest : public PlayerStorageTest {
protected:
    std::unique_ptr<RocksDBDatabase> m_db;
    std::unique_ptr<PlayerDataManager> m_manager;

    void SetUp() override
    {
        PlayerStorageTest::SetUp();

        // 创建测试数据库
        std::string dbPath = (m_testDir / "players_test.db").string();
        auto dbResult = RocksDBDatabase::open(dbPath);
        ASSERT_TRUE(dbResult.success()) << "Failed to open test database";
        m_db = std::move(dbResult.value());
        m_manager = std::make_unique<PlayerDataManager>(*m_db);
    }

    void TearDown() override
    {
        m_manager.reset();
        m_db.reset();
        PlayerStorageTest::TearDown();
    }
};

TEST_F(PlayerDataManagerTest, SaveAndLoad)
{
    PlayerSaveData original;
    original.uuid = "save-load-test";
    original.username = "TestPlayer";
    original.posX = 100.0;
    original.posY = 64.0;
    original.posZ = 200.0;
    original.health = 15.0f;

    // 保存
    auto saveResult = m_manager->savePlayerImmediate(original);
    ASSERT_TRUE(saveResult.success());

    // 加载
    auto loadResult = m_manager->loadPlayer("save-load-test");
    ASSERT_TRUE(loadResult.success());
    ASSERT_NE(loadResult.value(), nullptr);

    const PlayerSaveData* loaded = loadResult.value();
    EXPECT_EQ(loaded->uuid, original.uuid);
    EXPECT_EQ(loaded->username, original.username);
    EXPECT_DOUBLE_EQ(loaded->posX, original.posX);
    EXPECT_DOUBLE_EQ(loaded->posY, original.posY);
    EXPECT_DOUBLE_EQ(loaded->posZ, original.posZ);
    EXPECT_FLOAT_EQ(loaded->health, original.health);
}

TEST_F(PlayerDataManagerTest, LoadNonexistentPlayer)
{
    auto result = m_manager->loadPlayer("nonexistent-uuid");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), nullptr);
}

TEST_F(PlayerDataManagerTest, HasPlayer)
{
    EXPECT_FALSE(m_manager->hasPlayer("has-player-test"));

    PlayerSaveData data;
    data.uuid = "has-player-test";
    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    EXPECT_TRUE(m_manager->hasPlayer("has-player-test"));
}

TEST_F(PlayerDataManagerTest, DeletePlayer)
{
    PlayerSaveData data;
    data.uuid = "delete-test";
    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    EXPECT_TRUE(m_manager->hasPlayer("delete-test"));

    auto deleteResult = m_manager->deletePlayer("delete-test");
    ASSERT_TRUE(deleteResult.success());

    EXPECT_FALSE(m_manager->hasPlayer("delete-test"));
}

TEST_F(PlayerDataManagerTest, CacheBehavior)
{
    PlayerSaveData original;
    original.uuid = "cache-test";
    original.username = "CachePlayer";
    original.health = 10.0f;

    // 保存并缓存
    auto saveResult = m_manager->savePlayerImmediate(original);
    ASSERT_TRUE(saveResult.success());

    // 第一次加载（从数据库）
    auto loadResult1 = m_manager->loadPlayer("cache-test");
    ASSERT_TRUE(loadResult1.success());

    // 第二次加载（从缓存）
    auto loadResult2 = m_manager->loadPlayer("cache-test");
    ASSERT_TRUE(loadResult2.success());

    // 应该返回相同的指针
    EXPECT_EQ(loadResult1.value(), loadResult2.value());
}

TEST_F(PlayerDataManagerTest, DirtyTracking)
{
    PlayerSaveData data;
    data.uuid = "dirty-test";
    data.username = "DirtyPlayer";

    // 使用 savePlayer（延迟保存）
    auto saveResult = m_manager->savePlayer(data);
    ASSERT_TRUE(saveResult.success());

    // 应该被标记为脏
    EXPECT_EQ(m_manager->dirtyCount(), 1u);
    auto dirtyUuids = m_manager->getDirtyUuids();
    EXPECT_EQ(dirtyUuids.size(), 1u);
    EXPECT_EQ(dirtyUuids[0], "dirty-test");

    // 保存脏数据
    auto flushResult = m_manager->saveAllDirty();
    ASSERT_TRUE(flushResult.success());
    EXPECT_EQ(flushResult.value(), 1u);

    // 脏列表应该清空
    EXPECT_EQ(m_manager->dirtyCount(), 0u);
}

TEST_F(PlayerDataManagerTest, SaveAllDirty)
{
    // 创建多个玩家
    for (int i = 0; i < 5; ++i) {
        PlayerSaveData data;
        data.uuid = "player-" + std::to_string(i);
        data.username = "Player" + std::to_string(i);
        auto saveResult = m_manager->savePlayer(data);
        ASSERT_TRUE(saveResult.success());
    }

    EXPECT_EQ(m_manager->dirtyCount(), 5u);

    auto saveResult = m_manager->saveAllDirty();
    ASSERT_TRUE(saveResult.success());
    EXPECT_EQ(saveResult.value(), 5u);
    EXPECT_EQ(m_manager->dirtyCount(), 0u);
}

TEST_F(PlayerDataManagerTest, SaveAll)
{
    // 创建并立即保存一些玩家
    for (int i = 0; i < 3; ++i) {
        PlayerSaveData data;
        data.uuid = "immediate-" + std::to_string(i);
        auto saveResult = m_manager->savePlayerImmediate(data);
        ASSERT_TRUE(saveResult.success());
    }

    // 创建延迟保存的玩家
    for (int i = 0; i < 2; ++i) {
        PlayerSaveData data;
        data.uuid = "delayed-" + std::to_string(i);
        auto saveResult = m_manager->savePlayer(data);
        ASSERT_TRUE(saveResult.success());
    }

    // saveAll 应该保存所有缓存的玩家
    auto saveResult = m_manager->saveAll();
    ASSERT_TRUE(saveResult.success());
    EXPECT_EQ(saveResult.value(), 5u);
}

TEST_F(PlayerDataManagerTest, ClearCache)
{
    PlayerSaveData data;
    data.uuid = "clear-cache-test";
    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    EXPECT_EQ(m_manager->cacheSize(), 1u);

    m_manager->clearCache();

    EXPECT_EQ(m_manager->cacheSize(), 0u);
    EXPECT_EQ(m_manager->dirtyCount(), 0u);
}

TEST_F(PlayerDataManagerTest, UpdatePlayer)
{
    PlayerSaveData data;
    data.uuid = "update-test";
    data.username = "OriginalName";
    data.health = 20.0f;

    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    // 修改并更新
    data.username = "UpdatedName";
    data.health = 15.0f;
    auto updateResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(updateResult.success());

    // 加载并验证
    auto loadResult = m_manager->loadPlayer("update-test");
    ASSERT_TRUE(loadResult.success());
    EXPECT_EQ(loadResult.value()->username, "UpdatedName");
    EXPECT_FLOAT_EQ(loadResult.value()->health, 15.0f);
}

TEST_F(PlayerDataManagerTest, FromServerPlayerDataConversion)
{
    // 创建简化的测试数据
    server::ServerPlayerData serverData;
    serverData.playerId = 12345;
    serverData.uuid = "server-uuid-12345"; // commit f20d2df32 起使用真正的 UUID 字段
    serverData.username = "ServerPlayer";
    serverData.x = 100.0;
    serverData.y = 64.0;
    serverData.z = 200.0;
    serverData.yaw = 90.0f;
    serverData.pitch = 45.0f;
    serverData.gameMode = GameMode::Survival;
    serverData.onGround = true;

    PlayerSaveData saveData = PlayerDataManager::fromServerPlayerData(serverData);

    EXPECT_EQ(saveData.uuid, "server-uuid-12345");
    EXPECT_EQ(saveData.username, "ServerPlayer");
    EXPECT_DOUBLE_EQ(saveData.posX, 100.0);
    EXPECT_DOUBLE_EQ(saveData.posY, 64.0);
    EXPECT_DOUBLE_EQ(saveData.posZ, 200.0);
    EXPECT_FLOAT_EQ(saveData.yaw, 90.0f);
    EXPECT_FLOAT_EQ(saveData.pitch, 45.0f);
    EXPECT_EQ(saveData.gameMode, GameMode::Survival);
    EXPECT_TRUE(saveData.onGround);
}

TEST_F(PlayerDataManagerTest, CallbackTest)
{
    bool callbackCalled = false;
    std::string savedUuid;

    m_manager->setOnPlayerSaved([&](const std::string& uuid) {
        callbackCalled = true;
        savedUuid = uuid;
    });

    PlayerSaveData data;
    data.uuid = "callback-test";
    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(savedUuid, "callback-test");
}

// ============================================================================
// applyToPlayer 测试
// ============================================================================

class ApplyToPlayerTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer"); }
    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

TEST_F(ApplyToPlayerTest, RestoresPosition)
{
    PlayerSaveData data;
    data.posX = 100.5;
    data.posY = 70.0;
    data.posZ = -200.3;
    data.yaw = 90.0f;
    data.pitch = 45.0f;

    PlayerDataManager::applyToPlayer(*player, data);

    const auto& pos = player->position();
    EXPECT_FLOAT_EQ(pos.x, 100.5f);
    EXPECT_FLOAT_EQ(pos.y, 70.0f);
    EXPECT_FLOAT_EQ(pos.z, -200.3f);
    EXPECT_FLOAT_EQ(player->yaw(), 90.0f);
    EXPECT_FLOAT_EQ(player->pitch(), 45.0f);
}

TEST_F(ApplyToPlayerTest, RestoresGameMode)
{
    PlayerSaveData data;
    data.gameMode = GameMode::Creative;
    // 创造模式的能力需要显式设置（与 MC Java 的 PlayerSaveData 一致）
    data.invulnerable = true;
    data.canFly = true;

    PlayerDataManager::applyToPlayer(*player, data);

    EXPECT_EQ(player->gameMode(), GameMode::Creative);
    EXPECT_TRUE(player->abilities().canFly);
    EXPECT_TRUE(player->abilities().invulnerable);
}

TEST_F(ApplyToPlayerTest, RestoresFoodStats)
{
    PlayerSaveData data;
    data.foodLevel = 12;
    data.saturationLevel = 3.5f;
    data.exhaustionLevel = 1.2f;
    data.foodTickTimer = 50;

    PlayerDataManager::applyToPlayer(*player, data);

    EXPECT_EQ(player->foodStats().foodLevel(), 12);
    EXPECT_FLOAT_EQ(player->foodStats().saturationLevel(), 3.5f);
    EXPECT_FLOAT_EQ(player->foodStats().exhaustionLevel(), 1.2f);
    EXPECT_EQ(player->foodStats().foodTimer(), 50);
}

TEST_F(ApplyToPlayerTest, RestoresExperience)
{
    PlayerSaveData data;
    data.experienceLevel = 30;
    data.experienceProgress = 0.75f;
    data.totalExperience = 1200;
    data.xpSeed = 42;

    PlayerDataManager::applyToPlayer(*player, data);

    EXPECT_EQ(player->experienceManager().getLevel(), 30);
    EXPECT_FLOAT_EQ(player->experienceManager().getProgress(), 0.75f);
    EXPECT_EQ(player->experienceManager().getTotalExperience(), 1200);
    EXPECT_EQ(player->experienceManager().getXpSeed(), 42);
}

TEST_F(ApplyToPlayerTest, RestoresAbilities)
{
    PlayerSaveData data;
    data.invulnerable = true;
    data.canFly = true;
    data.flying = true;
    data.flySpeed = 0.1f;
    data.walkSpeed = 0.2f;

    PlayerDataManager::applyToPlayer(*player, data);

    EXPECT_TRUE(player->abilities().invulnerable);
    EXPECT_TRUE(player->abilities().canFly);
    EXPECT_TRUE(player->abilities().flying);
    EXPECT_FLOAT_EQ(player->abilities().flySpeed, 0.1f);
    EXPECT_FLOAT_EQ(player->abilities().walkSpeed, 0.2f);
}

TEST_F(ApplyToPlayerTest, RestoresImpulseContext)
{
    PlayerSaveData data;
    data.currentImpulseImpactPos = Vector3(100.0f, 64.0f, 200.0f);
    data.ignoreFallDamageFromCurrentImpulse = true;
    data.currentImpulseContextResetGraceTime = 40;

    PlayerDataManager::applyToPlayer(*player, data);

    auto impactPos = player->currentImpulseImpactPos();
    ASSERT_TRUE(impactPos.has_value());
    EXPECT_FLOAT_EQ(impactPos->x, 100.0f);
    EXPECT_FLOAT_EQ(impactPos->y, 64.0f);
    EXPECT_FLOAT_EQ(impactPos->z, 200.0f);
    EXPECT_TRUE(player->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_EQ(player->currentImpulseContextResetGraceTime(), 40);
}

TEST_F(ApplyToPlayerTest, RestoresSpawnPoint)
{
    PlayerSaveData data;
    data.spawnPoint = GlobalPos(DimensionId(-1), BlockPos(50, 30, 100));
    data.spawnForced = true;

    PlayerDataManager::applyToPlayer(*player, data);

    auto spawnPoint = player->getSpawnPoint();
    ASSERT_TRUE(spawnPoint.has_value());
    EXPECT_EQ(spawnPoint->getDimensionId(), DimensionId(-1));
    EXPECT_EQ(spawnPoint->getPos().x, 50);
    EXPECT_EQ(spawnPoint->getPos().y, 30);
    EXPECT_EQ(spawnPoint->getPos().z, 100);
    EXPECT_TRUE(player->isSpawnForced());
}

TEST_F(ApplyToPlayerTest, RestoresEnteredNetherPosition)
{
    PlayerSaveData data;
    data.enteredNetherPosition = Vector3d(200.0, 50.0, 300.0);

    PlayerDataManager::applyToPlayer(*player, data);

    auto netherPos = player->getEnteredNetherPosition();
    ASSERT_TRUE(netherPos.has_value());
    EXPECT_DOUBLE_EQ(netherPos->x, 200.0);
    EXPECT_DOUBLE_EQ(netherPos->y, 50.0);
    EXPECT_DOUBLE_EQ(netherPos->z, 300.0);
}

TEST_F(ApplyToPlayerTest, RestoresLastDeathLocation)
{
    PlayerSaveData data;
    data.lastDeathLocation = GlobalPos(DimensionId(-1), BlockPos(50, 30, -100));

    PlayerDataManager::applyToPlayer(*player, data);

    auto deathLoc = player->getLastDeathLocation();
    ASSERT_TRUE(deathLoc.has_value());
    EXPECT_EQ(deathLoc->getDimensionId(), DimensionId(-1));
    EXPECT_EQ(deathLoc->x(), 50);
    EXPECT_EQ(deathLoc->y(), 30);
    EXPECT_EQ(deathLoc->z(), -100);
}

TEST_F(ApplyToPlayerTest, ClearsLastDeathLocation)
{
    // 先设置死亡位置
    player->setLastDeathLocation(GlobalPos(DimensionId(0), BlockPos(100, 64, 200)));
    ASSERT_TRUE(player->getLastDeathLocation().has_value());

    // 使用空的 PlayerSaveData 恢复（没有 lastDeathLocation）
    PlayerSaveData data;
    data.uuid = "clear-death-loc-test";

    PlayerDataManager::applyToPlayer(*player, data);

    // 应该清除死亡位置
    EXPECT_FALSE(player->getLastDeathLocation().has_value());
}

TEST_F(ApplyToPlayerTest, DefaultDataDoesNotCrash)
{
    // 默认 PlayerSaveData 应该安全地应用到 Player 而不崩溃
    PlayerSaveData data;
    data.uuid = "default-test";

    EXPECT_NO_THROW(PlayerDataManager::applyToPlayer(*player, data));

    // 默认位置
    EXPECT_FLOAT_EQ(player->position().x, 0.0f);
    // 默认游戏模式应该是 Survival
    EXPECT_EQ(player->gameMode(), GameMode::Survival);
}

TEST_F(ApplyToPlayerTest, RestoresFullPlayerState)
{
    PlayerSaveData data;
    data.posX = 500.0;
    data.posY = 80.0;
    data.posZ = -300.0;
    data.yaw = 180.0f;
    data.pitch = -30.0f;
    data.gameMode = GameMode::Spectator;
    data.health = 10.0f;
    data.foodLevel = 8;
    data.saturationLevel = 1.0f;
    data.exhaustionLevel = 2.5f;
    data.foodTickTimer = 75;
    data.experienceLevel = 50;
    data.experienceProgress = 0.9f;
    data.totalExperience = 5000;
    data.xpSeed = 999;
    data.invulnerable = true;
    data.canFly = true;
    data.flying = false;
    data.flySpeed = 0.07f;
    data.walkSpeed = 0.15f;
    data.currentImpulseImpactPos = Vector3(10.0f, 20.0f, 30.0f);
    data.ignoreFallDamageFromCurrentImpulse = true;
    data.currentImpulseContextResetGraceTime = 50; // 大于默认的 40 tick
    data.spawnPoint = GlobalPos(DimensionId(0), BlockPos(100, 64, 200));
    data.spawnForced = false;
    data.enteredNetherPosition = Vector3d(150.0, 40.0, 250.0);
    data.onGround = true;
    data.sprinting = true;
    data.sneaking = false;
    data.airSupply = 150;

    PlayerDataManager::applyToPlayer(*player, data);

    // 验证关键字段
    EXPECT_FLOAT_EQ(player->position().x, 500.0f);
    EXPECT_EQ(player->gameMode(), GameMode::Spectator);
    EXPECT_FLOAT_EQ(player->health(), 10.0f);
    EXPECT_EQ(player->foodStats().foodLevel(), 8);
    EXPECT_EQ(player->experienceManager().getLevel(), 50);
    EXPECT_TRUE(player->abilities().invulnerable);
    EXPECT_TRUE(player->isIgnoringFallDamageFromCurrentImpulse());
    auto impactPos = player->currentImpulseImpactPos();
    ASSERT_TRUE(impactPos.has_value());
    EXPECT_FLOAT_EQ(impactPos->x, 10.0f);
    EXPECT_EQ(player->currentImpulseContextResetGraceTime(), 50);
    EXPECT_TRUE(player->isSprinting());
    EXPECT_EQ(player->air(), 150);
}

// ============================================================================
// fromPlayer 测试
//
// fromPlayer() 是关闭服务端时回写在线玩家运行时状态的核心入口：
// IntegratedServer::savePlayerRuntimeState() 和 StandaloneServer::savePlayerRuntimeState()
// 都通过遍历在线玩家实体调用 fromPlayer() + savePlayer()，将运行时状态写入缓存，
// 随后 saveAllWorldData() 会通过 PlayerDataManager::saveAll() 落盘到 RocksDB。
// 因此 fromPlayer() 必须能够完整提取 Player 的所有可持久化状态。
// ============================================================================

class FromPlayerTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(EntityInstanceId(1), "FromPlayerTest"); }
    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

TEST_F(FromPlayerTest, ExtractsDefaultPlayerState)
{
    // 默认构造的 Player 也应该能安全提取状态
    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    EXPECT_EQ(data.username, "FromPlayerTest");
    EXPECT_FLOAT_EQ(data.health, 20.0f);
    EXPECT_EQ(data.foodLevel, 20);
    EXPECT_EQ(data.gameMode, GameMode::Survival);
}

TEST_F(FromPlayerTest, ExtractsCustomPositionAndRotation)
{
    player->setPosition(100.5f, 70.0f, -200.3f);
    player->setRotation(90.0f, 45.0f);

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    EXPECT_FLOAT_EQ(static_cast<f32>(data.posX), 100.5f);
    EXPECT_FLOAT_EQ(static_cast<f32>(data.posY), 70.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(data.posZ), -200.3f);
    EXPECT_FLOAT_EQ(data.yaw, 90.0f);
    EXPECT_FLOAT_EQ(data.pitch, 45.0f);
}

TEST_F(FromPlayerTest, ExtractsHealthAndFood)
{
    player->setHealth(15.0f);
    player->foodStats().setFoodLevel(12);
    player->foodStats().setSaturationLevel(3.5f);

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    EXPECT_FLOAT_EQ(data.health, 15.0f);
    EXPECT_EQ(data.foodLevel, 12);
    EXPECT_FLOAT_EQ(data.saturationLevel, 3.5f);
}

TEST_F(FromPlayerTest, ExtractsExperience)
{
    player->experienceManager().setExperience(30, 0.75f, 1500);
    player->experienceManager().setXpSeed(42);

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    EXPECT_EQ(data.experienceLevel, 30);
    EXPECT_FLOAT_EQ(data.experienceProgress, 0.75f);
    EXPECT_EQ(data.totalExperience, 1500);
    EXPECT_EQ(data.xpSeed, 42);
}

TEST_F(FromPlayerTest, ExtractsAbilities)
{
    auto& abilities = player->abilities();
    abilities.invulnerable = true;
    abilities.canFly = true;
    abilities.flying = true;
    abilities.flySpeed = 0.1f;
    abilities.walkSpeed = 0.2f;

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    EXPECT_TRUE(data.invulnerable);
    EXPECT_TRUE(data.canFly);
    EXPECT_TRUE(data.flying);
    EXPECT_FLOAT_EQ(data.flySpeed, 0.1f);
    EXPECT_FLOAT_EQ(data.walkSpeed, 0.2f);
}

TEST_F(FromPlayerTest, ExtractsSpawnPoint)
{
    player->setSpawnPoint(DimensionId(-1), BlockPos(50, 30, 100), true);

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    ASSERT_TRUE(data.spawnPoint.has_value());
    EXPECT_EQ(data.spawnPoint->getDimensionId(), DimensionId(-1));
    EXPECT_EQ(data.spawnPoint->x(), 50);
    EXPECT_EQ(data.spawnPoint->y(), 30);
    EXPECT_EQ(data.spawnPoint->z(), 100);
    EXPECT_TRUE(data.spawnForced);
}

TEST_F(FromPlayerTest, ExtractsLastDeathLocation)
{
    player->setLastDeathLocation(GlobalPos(DimensionId(0), BlockPos(100, 64, -200)));

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    ASSERT_TRUE(data.lastDeathLocation.has_value());
    EXPECT_EQ(data.lastDeathLocation->getDimensionId(), DimensionId(0));
    EXPECT_EQ(data.lastDeathLocation->x(), 100);
    EXPECT_EQ(data.lastDeathLocation->y(), 64);
    EXPECT_EQ(data.lastDeathLocation->z(), -200);
}

TEST_F(FromPlayerTest, ExtractsEnteredNetherPosition)
{
    player->setEnteredNetherPosition(Vector3d(200.0, 50.0, 300.0));

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    ASSERT_TRUE(data.enteredNetherPosition.has_value());
    EXPECT_DOUBLE_EQ(data.enteredNetherPosition->x, 200.0);
    EXPECT_DOUBLE_EQ(data.enteredNetherPosition->y, 50.0);
    EXPECT_DOUBLE_EQ(data.enteredNetherPosition->z, 300.0);
}

TEST_F(FromPlayerTest, ExtractsImpulseContext)
{
    player->setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player->setIgnoreFallDamageFromCurrentImpulse(true);

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    ASSERT_TRUE(data.currentImpulseImpactPos.has_value());
    EXPECT_FLOAT_EQ(data.currentImpulseImpactPos->x, 100.0f);
    EXPECT_FLOAT_EQ(data.currentImpulseImpactPos->y, 64.0f);
    EXPECT_FLOAT_EQ(data.currentImpulseImpactPos->z, 200.0f);
    EXPECT_TRUE(data.ignoreFallDamageFromCurrentImpulse);
    // setIgnoreFallDamageFromCurrentImpulse(true) 设置 40 tick 宽限期
    EXPECT_EQ(data.currentImpulseContextResetGraceTime, 40);
}

TEST_F(FromPlayerTest, ExtractsStatusFlags)
{
    player->setSprinting(true);
    player->setSneaking(true);
    player->setAir(150);

    PlayerSaveData data = PlayerDataManager::fromPlayer(*player);

    EXPECT_TRUE(data.sprinting);
    EXPECT_TRUE(data.sneaking);
    EXPECT_EQ(data.airSupply, 150);
}

TEST_F(FromPlayerTest, RoundTripPreservesFullState)
{
    // 先通过 applyToPlayer 设置完整状态
    // 注意：applyToPlayer 不修改 username/uuid，这些在 Player 构造时确定
    PlayerSaveData original;
    original.uuid = "roundtrip-from-player";
    original.username = "RoundTripPlayer"; // applyToPlayer 不会改写 username
    original.posX = 500.0;
    original.posY = 80.0;
    original.posZ = -300.0;
    original.yaw = 180.0f;
    original.pitch = -30.0f;
    original.gameMode = GameMode::Spectator;
    original.health = 10.0f;
    original.foodLevel = 8;
    original.saturationLevel = 1.0f;
    original.exhaustionLevel = 2.5f;
    original.foodTickTimer = 75;
    original.experienceLevel = 50;
    original.experienceProgress = 0.9f;
    original.totalExperience = 5000;
    original.xpSeed = 999;
    original.invulnerable = true;
    original.canFly = true;
    original.flying = false;
    original.flySpeed = 0.07f;
    original.walkSpeed = 0.15f;
    original.spawnPoint = GlobalPos(DimensionId(0), BlockPos(100, 64, 200));
    original.spawnForced = false;
    original.enteredNetherPosition = Vector3d(150.0, 40.0, 250.0);
    original.onGround = true;
    original.sprinting = true;
    original.sneaking = false;
    original.airSupply = 150;

    PlayerDataManager::applyToPlayer(*player, original);

    // 用 fromPlayer 提取，应该得到等价的状态
    // （username/uuid 来自 Player 构造函数，不会被 applyToPlayer 修改，
    //  因此 fromPlayer 提取的值是构造时的值，不是 original.username）
    PlayerSaveData extracted = PlayerDataManager::fromPlayer(*player);

    EXPECT_FLOAT_EQ(static_cast<f32>(extracted.posX), static_cast<f32>(original.posX));
    EXPECT_FLOAT_EQ(static_cast<f32>(extracted.posY), static_cast<f32>(original.posY));
    EXPECT_FLOAT_EQ(static_cast<f32>(extracted.posZ), static_cast<f32>(original.posZ));
    EXPECT_FLOAT_EQ(extracted.yaw, original.yaw);
    EXPECT_FLOAT_EQ(extracted.pitch, original.pitch);
    EXPECT_EQ(extracted.gameMode, original.gameMode);
    EXPECT_FLOAT_EQ(extracted.health, original.health);
    EXPECT_EQ(extracted.foodLevel, original.foodLevel);
    EXPECT_FLOAT_EQ(extracted.saturationLevel, original.saturationLevel);
    EXPECT_EQ(extracted.experienceLevel, original.experienceLevel);
    EXPECT_FLOAT_EQ(extracted.experienceProgress, original.experienceProgress);
    EXPECT_EQ(extracted.totalExperience, original.totalExperience);
    EXPECT_EQ(extracted.xpSeed, original.xpSeed);
    EXPECT_TRUE(extracted.invulnerable);
    EXPECT_TRUE(extracted.canFly);
    EXPECT_FALSE(extracted.flying);
    EXPECT_FLOAT_EQ(extracted.flySpeed, original.flySpeed);
    EXPECT_FLOAT_EQ(extracted.walkSpeed, original.walkSpeed);

    ASSERT_TRUE(extracted.spawnPoint.has_value());
    EXPECT_EQ(extracted.spawnPoint->getDimensionId(), original.spawnPoint->getDimensionId());
    EXPECT_EQ(extracted.spawnPoint->x(), original.spawnPoint->x());
    EXPECT_EQ(extracted.spawnPoint->y(), original.spawnPoint->y());
    EXPECT_EQ(extracted.spawnPoint->z(), original.spawnPoint->z());
    EXPECT_EQ(extracted.spawnForced, original.spawnForced);

    ASSERT_TRUE(extracted.enteredNetherPosition.has_value());
    EXPECT_DOUBLE_EQ(extracted.enteredNetherPosition->x, original.enteredNetherPosition->x);
    EXPECT_DOUBLE_EQ(extracted.enteredNetherPosition->y, original.enteredNetherPosition->y);
    EXPECT_DOUBLE_EQ(extracted.enteredNetherPosition->z, original.enteredNetherPosition->z);

    EXPECT_EQ(extracted.sprinting, original.sprinting);
    EXPECT_EQ(extracted.sneaking, original.sneaking);
    EXPECT_EQ(extracted.airSupply, original.airSupply);
}

TEST_F(FromPlayerTest, RoundTripPersistsThroughStorage)
{
    // 验证完整的存档往返：applyToPlayer → fromPlayer → savePlayerImmediate → loadPlayer
    // 这正是 IntegratedServer::savePlayerRuntimeState() 的核心流程
    // 注意：必须显式设置 Player 的 uuid，否则 fromPlayer 提取的 uuid 为空，
    //       savePlayerImmediate 后 loadPlayer 将无法通过 uuid 查找
    player->setUuid("roundtrip-storage-uuid");

    PlayerSaveData original;
    original.uuid = "roundtrip-storage-uuid";
    original.username = "StorageRoundTrip";
    original.posX = 123.45;
    original.posY = 64.0;
    original.posZ = -678.9;
    original.health = 17.5f;
    original.foodLevel = 14;
    original.experienceLevel = 7;

    PlayerDataManager::applyToPlayer(*player, original);

    PlayerSaveData extracted = PlayerDataManager::fromPlayer(*player);
    EXPECT_EQ(extracted.uuid, "roundtrip-storage-uuid");

    // 创建独立存储管理器
    std::filesystem::path testDir = std::filesystem::temp_directory_path() / "player_from_player_storage_test";
    std::filesystem::create_directories(testDir);
    auto dbResult = RocksDBDatabase::open((testDir / "players.db").string());
    ASSERT_TRUE(dbResult.success());
    auto db = std::move(dbResult.value());
    PlayerDataManager manager(*db);

    auto saveResult = manager.savePlayerImmediate(extracted);
    ASSERT_TRUE(saveResult.success());

    auto loadResult = manager.loadPlayer("roundtrip-storage-uuid");
    ASSERT_TRUE(loadResult.success());
    ASSERT_NE(loadResult.value(), nullptr);

    const PlayerSaveData* loaded = loadResult.value();
    EXPECT_EQ(loaded->uuid, "roundtrip-storage-uuid");
    // posX/posZ 在 Player 中以 f32 存储，往返会丢失 double 精度，故用 f32 比较
    EXPECT_FLOAT_EQ(static_cast<f32>(loaded->posX), 123.45f);
    EXPECT_FLOAT_EQ(static_cast<f32>(loaded->posZ), -678.9f);
    EXPECT_FLOAT_EQ(loaded->health, 17.5f);
    EXPECT_EQ(loaded->foodLevel, 14);
    EXPECT_EQ(loaded->experienceLevel, 7);

    // 关闭数据库后再清理临时目录，避免文件占用
    manager.clearCache();
    db.reset();

    // 重试几次删除（Windows 上 RocksDB 后台线程可能延迟释放句柄）
    for (int i = 0; i < 5; ++i) {
        std::error_code ec;
        std::filesystem::remove_all(testDir, ec);
        if (!ec) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace
} // namespace mc::world::storage
