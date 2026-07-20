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

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/GlobalPos.hpp"

using namespace mc;
using namespace mc::entity::serialization;
using namespace mc::entity::serialization::nbt_keys;

// ============================================================================
// Player NBT 序列化测试
//
// 测试 Player::addAdditionalSaveData / readAdditionalSaveData 的序列化和
// 反序列化往返一致性，覆盖游戏模式、食物数据、经验、能力、冲量上下文、重生点、
// 下界入口位置等字段。边界场景包括空 NBT、缺字段、冲量位置为空等。
// ============================================================================

namespace {

// 辅助函数：序列化 Player 到 NBT
std::unique_ptr<nbt::tags::compound_tag> savePlayerToNbt(const Player& player)
{
    auto tag = std::make_unique<nbt::tags::compound_tag>();
    player.addAdditionalSaveData(*tag);
    return tag;
}

// 辅助函数：从 NBT 反序列化到新的 Player
std::unique_ptr<Player> loadPlayerFromNbt(const nbt::tags::compound_tag& tag)
{
    auto player = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer");
    auto result = player->readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success()) << "readAdditionalSaveData should succeed";
    return player;
}

} // namespace

// ========== 游戏模式序列化测试 ==========

TEST(PlayerSerializationTest, GameMode_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative);

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_EQ(loaded->gameMode(), GameMode::Creative);
}

TEST(PlayerSerializationTest, GameMode_AllModes)
{
    for (auto mode : {GameMode::Survival, GameMode::Creative, GameMode::Adventure, GameMode::Spectator}) {
        Player player(EntityInstanceId(1), "TestPlayer");
        player.setGameMode(mode);

        auto tag = savePlayerToNbt(player);
        auto loaded = loadPlayerFromNbt(*tag);

        EXPECT_EQ(loaded->gameMode(), mode) << "GameMode round-trip failed for mode " << static_cast<i32>(mode);
    }
}

// ========== 食物数据序列化测试 ==========

TEST(PlayerSerializationTest, FoodStats_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.foodStats().setFoodLevel(15);
    player.foodStats().setSaturationLevel(3.5f);
    player.foodStats().setExhaustionLevel(1.2f);
    player.foodStats().setFoodTimer(100);

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_EQ(loaded->foodStats().foodLevel(), 15);
    EXPECT_FLOAT_EQ(loaded->foodStats().saturationLevel(), 3.5f);
    EXPECT_FLOAT_EQ(loaded->foodStats().exhaustionLevel(), 1.2f);
    EXPECT_EQ(loaded->foodStats().foodTimer(), 100);
}

TEST(PlayerSerializationTest, FoodStats_DefaultValuesPreservedWhenMissing)
{
    // 空 NBT 不应改变默认值
    Player player(EntityInstanceId(1), "TestPlayer");
    i32 defaultFoodLevel = player.foodStats().foodLevel();
    f32 defaultSaturation = player.foodStats().saturationLevel();

    nbt::tags::compound_tag emptyTag;
    auto result = player.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());

    EXPECT_EQ(player.foodStats().foodLevel(), defaultFoodLevel);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), defaultSaturation);
}

// ========== 经验序列化测试 ==========

TEST(PlayerSerializationTest, Experience_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.experienceManager().setExperience(30, 0.75f, 1200);
    player.experienceManager().setXpSeed(42);

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_EQ(loaded->experienceManager().getLevel(), 30);
    EXPECT_FLOAT_EQ(loaded->experienceManager().getProgress(), 0.75f);
    EXPECT_EQ(loaded->experienceManager().getTotalExperience(), 1200);
    EXPECT_EQ(loaded->experienceManager().getXpSeed(), 42);
}

// ========== 玩家能力序列化测试 ==========

TEST(PlayerSerializationTest, Abilities_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Creative); // 设置为创造模式，canFly 和 invulnerable 应该为 true

    // 手动覆盖能力值
    auto& abilities = player.abilities();
    abilities.invulnerable = true;
    abilities.flying = true;
    abilities.canFly = true;
    abilities.flySpeed = 0.1f;
    abilities.walkSpeed = 0.2f;

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    const auto& loadedAbilities = loaded->abilities();
    EXPECT_TRUE(loadedAbilities.invulnerable);
    EXPECT_TRUE(loadedAbilities.flying);
    EXPECT_TRUE(loadedAbilities.canFly);
    EXPECT_FLOAT_EQ(loadedAbilities.flySpeed, 0.1f);
    EXPECT_FLOAT_EQ(loadedAbilities.walkSpeed, 0.2f);
}

TEST(PlayerSerializationTest, Abilities_DefaultSurvivalMode)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    // 默认生存模式：invulnerable=false, flying=false, canFly=false
    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    const auto& loadedAbilities = loaded->abilities();
    EXPECT_FALSE(loadedAbilities.invulnerable);
    EXPECT_FALSE(loadedAbilities.flying);
    EXPECT_FALSE(loadedAbilities.canFly);
    EXPECT_FLOAT_EQ(loadedAbilities.flySpeed, 0.05f);
    EXPECT_FLOAT_EQ(loadedAbilities.walkSpeed, 0.1f);
}

TEST(PlayerSerializationTest, Abilities_MissingAbilitiesTag_PreservesDefaults)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    nbt::tags::compound_tag emptyTag;
    auto result = player.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());

    // 没有 abilities 标签时，默认值应保持不变
    const auto& abilities = player.abilities();
    EXPECT_FLOAT_EQ(abilities.flySpeed, 0.05f);
    EXPECT_FLOAT_EQ(abilities.walkSpeed, 0.1f);
}

// ========== 冲量上下文序列化测试 ==========

TEST(PlayerSerializationTest, ImpulseContext_NoImpulse_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    // 默认状态：无冲量上下文
    EXPECT_FALSE(player.isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_FALSE(player.currentImpulseImpactPos().has_value());

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_FALSE(loaded->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_FALSE(loaded->currentImpulseImpactPos().has_value());
}

TEST(PlayerSerializationTest, ImpulseContext_WithImpactPosition_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setCurrentImpulseImpactPos(Vector3(100.0f, 64.0f, 200.0f));
    player.setIgnoreFallDamageFromCurrentImpulse(true);

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_TRUE(loaded->isIgnoringFallDamageFromCurrentImpulse());
    auto impactPos = loaded->currentImpulseImpactPos();
    ASSERT_TRUE(impactPos.has_value());
    EXPECT_FLOAT_EQ(impactPos->x, 100.0f);
    EXPECT_FLOAT_EQ(impactPos->y, 64.0f);
    EXPECT_FLOAT_EQ(impactPos->z, 200.0f);
}

TEST(PlayerSerializationTest, ImpulseContext_GraceTime_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setIgnoreFallDamageFromCurrentImpulse(true);
    // 宽限期应该是 40（由 setIgnoreFallDamageFromCurrentImpulse(true) 设置）
    EXPECT_EQ(player.currentImpulseContextResetGraceTime(), 40);

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_TRUE(loaded->isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_EQ(loaded->currentImpulseContextResetGraceTime(), 40);
}

TEST(PlayerSerializationTest, ImpulseContext_ExtendedGraceTime_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setIgnoreFallDamageFromCurrentImpulse(true);
    player.applyPostImpulseGraceTime(10); // 不改变 40 tick 宽限期（取最大值）
    EXPECT_EQ(player.currentImpulseContextResetGraceTime(), 40);

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_EQ(loaded->currentImpulseContextResetGraceTime(), 40);
}

TEST(PlayerSerializationTest, ImpulseContext_EmptyNbt_ClearsImpulseState)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setCurrentImpulseImpactPos(Vector3(50.0f, 70.0f, 80.0f));
    player.setIgnoreFallDamageFromCurrentImpulse(true);

    // 读取空 NBT：MC Java 语义中缺失的键会重置为默认值
    // current_explosion_impact_pos → null（else 分支）
    // ignore_fall_damage_from_current_explosion → false（getBooleanOr 默认值）
    // current_impulse_context_reset_grace_time → 0（getIntOr 默认值）
    nbt::tags::compound_tag emptyTag;
    auto result = player.readAdditionalSaveData(emptyTag);
    EXPECT_TRUE(result.success());

    // 冲量冲击位置应该被清除
    EXPECT_FALSE(player.currentImpulseImpactPos().has_value());
    // MC Java 语义：缺失的 NBT 键重置为默认值
    EXPECT_FALSE(player.isIgnoringFallDamageFromCurrentImpulse());
    EXPECT_EQ(player.currentImpulseContextResetGraceTime(), 0);
}

// ========== 重生点序列化测试 ==========

TEST(PlayerSerializationTest, SpawnPoint_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setSpawnPoint(DimensionId(0), BlockPos(100, 64, 200), true);

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    auto spawnPoint = loaded->getSpawnPoint();
    ASSERT_TRUE(spawnPoint.has_value());
    EXPECT_EQ(spawnPoint->getDimensionId(), DimensionId(0));
    EXPECT_EQ(spawnPoint->getPos().x, 100);
    EXPECT_EQ(spawnPoint->getPos().y, 64);
    EXPECT_EQ(spawnPoint->getPos().z, 200);
    EXPECT_TRUE(loaded->isSpawnForced());
}

TEST(PlayerSerializationTest, SpawnPoint_NoSpawnPoint_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    // 默认没有重生点
    EXPECT_FALSE(player.getSpawnPoint().has_value());

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_FALSE(loaded->getSpawnPoint().has_value());
}

// ========== 下界入口位置序列化测试 ==========

TEST(PlayerSerializationTest, EnteredNetherPosition_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setEnteredNetherPosition(Vector3d(100.5, 64.0, -200.3));

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    auto netherPos = loaded->getEnteredNetherPosition();
    ASSERT_TRUE(netherPos.has_value());
    EXPECT_DOUBLE_EQ(netherPos->x, 100.5);
    EXPECT_DOUBLE_EQ(netherPos->y, 64.0);
    EXPECT_DOUBLE_EQ(netherPos->z, -200.3);
}

TEST(PlayerSerializationTest, EnteredNetherPosition_NoPosition_RoundTrip)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    EXPECT_FALSE(player.getEnteredNetherPosition().has_value());

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    EXPECT_FALSE(loaded->getEnteredNetherPosition().has_value());
}

// ========== NBT 键名验证测试 ==========

TEST(PlayerSerializationTest, NbtKeys_CorrectKeyNames)
{
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setGameMode(GameMode::Survival);

    auto tag = savePlayerToNbt(player);

    // 验证序列化使用的键名是 MC Java 标准键名
    EXPECT_NE(tag->value.find(PLAYER_GAME_TYPE), tag->value.end()) << "Should contain playerGameType key";
    EXPECT_NE(tag->value.find(FOOD_LEVEL), tag->value.end()) << "Should contain foodLevel key";
    EXPECT_NE(tag->value.find(FOOD_SATURATION_LEVEL), tag->value.end()) << "Should contain foodSaturationLevel key";
    EXPECT_NE(tag->value.find(XP_LEVEL), tag->value.end()) << "Should contain XpLevel key";
    EXPECT_NE(tag->value.find(ABILITIES), tag->value.end()) << "Should contain abilities key";
}

// ========== 综合 Round-Trip 测试 ==========

TEST(PlayerSerializationTest, FullRoundTrip_AllFields)
{
    Player player(EntityInstanceId(1), "TestPlayer");

    // 设置各种状态
    player.setGameMode(GameMode::Creative);
    player.foodStats().setFoodLevel(12);
    player.foodStats().setSaturationLevel(2.5f);
    player.foodStats().setExhaustionLevel(0.8f);
    player.foodStats().setFoodTimer(50);
    player.experienceManager().setExperience(25, 0.5f, 800);
    player.experienceManager().setXpSeed(123);
    player.abilities().flying = true;
    player.abilities().flySpeed = 0.08f;
    player.setCurrentImpulseImpactPos(Vector3(150.0f, 70.0f, 250.0f));
    player.setIgnoreFallDamageFromCurrentImpulse(true);
    player.setSpawnPoint(DimensionId(-1), BlockPos(50, 30, 100), false);
    player.setEnteredNetherPosition(Vector3d(200.0, 50.0, 300.0));

    auto tag = savePlayerToNbt(player);
    auto loaded = loadPlayerFromNbt(*tag);

    // 游戏模式
    EXPECT_EQ(loaded->gameMode(), GameMode::Creative);

    // 食物数据
    EXPECT_EQ(loaded->foodStats().foodLevel(), 12);
    EXPECT_FLOAT_EQ(loaded->foodStats().saturationLevel(), 2.5f);
    EXPECT_FLOAT_EQ(loaded->foodStats().exhaustionLevel(), 0.8f);
    EXPECT_EQ(loaded->foodStats().foodTimer(), 50);

    // 经验
    EXPECT_EQ(loaded->experienceManager().getLevel(), 25);
    EXPECT_FLOAT_EQ(loaded->experienceManager().getProgress(), 0.5f);
    EXPECT_EQ(loaded->experienceManager().getTotalExperience(), 800);
    EXPECT_EQ(loaded->experienceManager().getXpSeed(), 123);

    // 能力
    EXPECT_TRUE(loaded->abilities().flying);
    EXPECT_FLOAT_EQ(loaded->abilities().flySpeed, 0.08f);

    // 冲量上下文
    EXPECT_TRUE(loaded->isIgnoringFallDamageFromCurrentImpulse());
    auto impactPos = loaded->currentImpulseImpactPos();
    ASSERT_TRUE(impactPos.has_value());
    EXPECT_FLOAT_EQ(impactPos->x, 150.0f);
    EXPECT_FLOAT_EQ(impactPos->y, 70.0f);
    EXPECT_FLOAT_EQ(impactPos->z, 250.0f);

    // 重生点
    auto spawnPoint = loaded->getSpawnPoint();
    ASSERT_TRUE(spawnPoint.has_value());
    EXPECT_EQ(spawnPoint->getDimensionId(), DimensionId(-1));
    EXPECT_EQ(spawnPoint->getPos().x, 50);
    EXPECT_EQ(spawnPoint->getPos().y, 30);
    EXPECT_EQ(spawnPoint->getPos().z, 100);
    EXPECT_FALSE(loaded->isSpawnForced());

    // 下界入口位置
    auto netherPos = loaded->getEnteredNetherPosition();
    ASSERT_TRUE(netherPos.has_value());
    EXPECT_DOUBLE_EQ(netherPos->x, 200.0);
    EXPECT_DOUBLE_EQ(netherPos->y, 50.0);
    EXPECT_DOUBLE_EQ(netherPos->z, 300.0);
}
