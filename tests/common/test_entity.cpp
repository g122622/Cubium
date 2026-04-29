#include <gtest/gtest.h>

#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/food/FoodStats.hpp"
#include "network/packet/PacketSerializer.hpp"

using namespace mc;

// ============================================================================
// Entity 测试
// ============================================================================

TEST(Entity, Construction) {
    Entity entity(LegacyEntityType::Player, 1);

    EXPECT_EQ(entity.id(), 1u);
    EXPECT_EQ(entity.legacyType(), LegacyEntityType::Player);
    EXPECT_FALSE(entity.uuid().empty());
    EXPECT_FALSE(entity.isRemoved());
}

TEST(Entity, Position) {
    Entity entity(LegacyEntityType::Player, 1);

    entity.setPosition(100.5, 64.0, -200.25);
    EXPECT_FLOAT_EQ(entity.x(), 100.5f);
    EXPECT_FLOAT_EQ(entity.y(), 64.0f);
    EXPECT_FLOAT_EQ(entity.z(), -200.25f);

    auto pos = entity.position();
    EXPECT_FLOAT_EQ(pos.x, 100.5f);
    EXPECT_FLOAT_EQ(pos.y, 64.0f);
    EXPECT_FLOAT_EQ(pos.z, -200.25f);
}

TEST(Entity, Rotation) {
    Entity entity(LegacyEntityType::Player, 1);

    entity.setRotation(90.0f, 45.0f);
    EXPECT_FLOAT_EQ(entity.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), 45.0f);
}

TEST(Entity, Velocity) {
    Entity entity(LegacyEntityType::Player, 1);

    entity.setVelocity(1.0, 2.0, 3.0);
    auto vel = entity.velocity();
    EXPECT_FLOAT_EQ(vel.x, 1.0f);
    EXPECT_FLOAT_EQ(vel.y, 2.0f);
    EXPECT_FLOAT_EQ(vel.z, 3.0f);
}

TEST(Entity, Move) {
    Entity entity(LegacyEntityType::Player, 1);
    entity.setPosition(0.0, 0.0, 0.0);

    entity.move(10.0, 5.0, -3.0);
    EXPECT_FLOAT_EQ(entity.x(), 10.0f);
    EXPECT_FLOAT_EQ(entity.y(), 5.0f);
    EXPECT_FLOAT_EQ(entity.z(), -3.0f);
}

TEST(Entity, Rotate) {
    Entity entity(LegacyEntityType::Player, 1);
    entity.setRotation(0.0f, 0.0f);

    entity.rotate(90.0f, 45.0f);
    EXPECT_FLOAT_EQ(entity.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), 45.0f);

    // 测试俯仰角限制
    entity.rotate(0.0f, 100.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), 90.0f);

    entity.rotate(0.0f, -200.0f);
    EXPECT_FLOAT_EQ(entity.pitch(), -90.0f);
}

TEST(Entity, BoundingBox) {
    Entity entity(LegacyEntityType::Player, 1);
    entity.setPosition(0.0, 0.0, 0.0);

    auto box = entity.boundingBox();
    EXPECT_FLOAT_EQ(box.width(), 0.6f);
    EXPECT_FLOAT_EQ(box.height(), 1.8f);
}

TEST(Entity, Flags) {
    Entity entity(LegacyEntityType::Player, 1);

    entity.addFlag(EntityFlags::OnFire);
    EXPECT_TRUE(entity.hasFlag(EntityFlags::OnFire));
    EXPECT_FALSE(entity.hasFlag(EntityFlags::Sprinting));

    entity.addFlag(EntityFlags::Sprinting);
    EXPECT_TRUE(entity.hasFlag(EntityFlags::Sprinting));

    entity.removeFlag(EntityFlags::OnFire);
    EXPECT_FALSE(entity.hasFlag(EntityFlags::OnFire));
    EXPECT_TRUE(entity.hasFlag(EntityFlags::Sprinting));
}

TEST(Entity, Tick) {
    Entity entity(LegacyEntityType::Player, 1);

    EXPECT_EQ(entity.ticksExisted(), 0u);

    entity.tick();
    EXPECT_EQ(entity.ticksExisted(), 1u);

    entity.tick();
    entity.tick();
    EXPECT_EQ(entity.ticksExisted(), 3u);
}

// ============================================================================
// Player 测试
// ============================================================================

TEST(Player, Construction) {
    Player player(1, "TestPlayer");

    EXPECT_EQ(player.id(), 1u);
    EXPECT_EQ(player.playerId(), 0u);  // 默认为0，需要手动设置
    EXPECT_EQ(player.username(), "TestPlayer");
    EXPECT_EQ(player.gameMode(), GameMode::Survival);
    EXPECT_FLOAT_EQ(player.health(), 20.0f);
}

TEST(Player, Health) {
    Player player(1, "TestPlayer");

    EXPECT_FLOAT_EQ(player.health(), 20.0f);
    EXPECT_FALSE(player.isDead());

    player.damage(5.0f);
    EXPECT_FLOAT_EQ(player.health(), 15.0f);
    EXPECT_FALSE(player.isDead());

    player.heal(3.0f);
    EXPECT_FLOAT_EQ(player.health(), 18.0f);

    player.damage(25.0f);
    EXPECT_FLOAT_EQ(player.health(), 0.0f);
    EXPECT_TRUE(player.isDead());
}

TEST(Player, GameMode) {
    Player player(1, "TestPlayer");

    player.setGameMode(GameMode::Creative);
    EXPECT_EQ(player.gameMode(), GameMode::Creative);
    EXPECT_TRUE(player.abilities().creativeMode);
    EXPECT_TRUE(player.abilities().canFly);

    player.setGameMode(GameMode::Spectator);
    EXPECT_EQ(player.gameMode(), GameMode::Spectator);
    EXPECT_TRUE(player.abilities().invulnerable);
    EXPECT_TRUE(player.abilities().flying);
}

TEST(Player, Experience) {
    Player player(1, "TestPlayer");

    EXPECT_EQ(player.experienceLevel(), 0);
    EXPECT_FLOAT_EQ(player.experienceProgress(), 0.0f);

    player.addExperience(10);
    EXPECT_GT(player.experienceLevel(), 0);

    player.setExperienceLevel(10);
    EXPECT_EQ(player.experienceLevel(), 10);
}

TEST(Player, ExperienceBarCapacity) {
    Player player(1, "TestPlayer");

    // Level 0-14: 7 + level * 2
    player.setExperienceLevel(0);
    EXPECT_EQ(player.experienceBarCapacity(), 7);

    player.setExperienceLevel(5);
    EXPECT_EQ(player.experienceBarCapacity(), 17);

    // Level 15-29: 37 + (level - 15) * 5
    player.setExperienceLevel(15);
    EXPECT_EQ(player.experienceBarCapacity(), 37);

    player.setExperienceLevel(20);
    EXPECT_EQ(player.experienceBarCapacity(), 62);

    // Level 30+: 112 + (level - 30) * 9
    player.setExperienceLevel(30);
    EXPECT_EQ(player.experienceBarCapacity(), 112);
}

TEST(Player, Food) {
    Player player(1, "TestPlayer");

    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 5.0f);

    player.foodStats().addExhaustion(10.0f);
    // 4次消耗触发，每次消耗1饱和度或1饥饿值
    EXPECT_LT(player.foodStats().saturationLevel(), 5.0f);

    player.foodStats().addStats(5, 3.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20); // 最大20
}

TEST(FoodStats, ExhaustionConsumption) {
    Player player(1, "TestPlayer");

    // 初始状态：foodLevel=20, saturation=5.0
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 5.0f);

    // 消耗 4.0 饱和度 -> 消耗 1 饱和度
    player.foodStats().addExhaustion(4.0f);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 4.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20); // 饥饿值不变

    // 消耗剩余饱和度
    player.foodStats().addExhaustion(16.0f); // 4次消耗
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 0.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20);

    // 饱和度为0后开始消耗饥饿值
    player.foodStats().addExhaustion(4.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 19);
}

TEST(FoodStats, SaturationCalculation) {
    Player player(1, "TestPlayer");

    // 重置到低饥饿值
    player.foodStats().setFoodLevel(10);
    player.foodStats().setSaturationLevel(0.0f);

    // 吃苹果：food=4, modifier=0.3
    // saturation = 4 * 0.3 * 2.0 = 2.4
    player.foodStats().addStats(4, 0.3f);
    EXPECT_EQ(player.foodStats().foodLevel(), 14);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 2.4f);

    // 吃熟牛排：food=8, modifier=0.8
    // saturation = 8 * 0.8 * 2.0 = 12.8，但上限为 foodLevel
    player.foodStats().addStats(8, 0.8f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 20.0f); // 上限为 foodLevel
}

TEST(FoodStats, NeedsFood) {
    Player player(1, "TestPlayer");

    // 饱食时不需要食物
    EXPECT_FALSE(player.foodStats().needsFood());

    // 饥饿值降低后需要食物
    player.foodStats().setFoodLevel(15);
    EXPECT_TRUE(player.foodStats().needsFood());

    player.foodStats().setFoodLevel(0);
    EXPECT_TRUE(player.foodStats().needsFood());
}

TEST(FoodStats, FoodTimer) {
    Player player(1, "TestPlayer");

    EXPECT_EQ(player.foodStats().foodTimer(), 0);

    player.foodStats().setFoodTimer(50);
    EXPECT_EQ(player.foodStats().foodTimer(), 50);
}

TEST(FoodStats, PrevFoodLevel) {
    Player player(1, "TestPlayer");

    EXPECT_EQ(player.foodStats().prevFoodLevel(), 20);

    player.foodStats().setFoodLevel(15);
    // prevFoodLevel 在 tick() 中更新，不在 setFoodLevel 中更新
    EXPECT_EQ(player.foodStats().prevFoodLevel(), 20);
}

TEST(FoodStats, ExhaustionCap) {
    Player player(1, "TestPlayer");

    // 消耗值上限为 40.0
    player.foodStats().addExhaustion(50.0f);
    EXPECT_FLOAT_EQ(player.foodStats().exhaustionLevel(), 40.0f);
}

TEST(FoodStats, Serialization) {
    Player original(1, "TestPlayer");
    original.foodStats().setFoodLevel(15);
    original.foodStats().setSaturationLevel(3.5f);
    original.foodStats().addExhaustion(2.0f);

    network::PacketSerializer ser;
    original.foodStats().serialize(ser);

    network::PacketDeserializer deser(ser.buffer());
    auto result = FoodStats::deserialize(deser);
    ASSERT_TRUE(result.success());

    FoodStats& loaded = result.value();
    EXPECT_EQ(loaded.foodLevel(), 15);
    EXPECT_FLOAT_EQ(loaded.saturationLevel(), 3.5f);
    EXPECT_FLOAT_EQ(loaded.exhaustionLevel(), 2.0f);
}

TEST(FoodStats, FastRegeneration) {
    // 快速恢复条件：foodLevel >= 20 且 saturation > 0，每 10 ticks 恢复 saturation/6 生命
    Player player(1, "TestPlayer");

    // 设置满饥饿和饱和度
    player.foodStats().setFoodLevel(20);
    player.foodStats().setSaturationLevel(6.0f);
    player.setHealth(10.0f); // 受伤状态

    // 验证初始条件
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 6.0f);
    EXPECT_FLOAT_EQ(player.health(), 10.0f);

    // tick() 需要调用多次触发快速恢复 (每 10 ticks)
    // 注意：tick() 需要 Player 实例，且会检查是否有饥饿效果
    for (int i = 0; i < 15; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证生命恢复
    EXPECT_GT(player.health(), 10.0f); // 生命应该恢复
    EXPECT_LT(player.foodStats().saturationLevel(), 6.0f); // 饱和度应该消耗
}

TEST(FoodStats, SlowRegeneration) {
    // 慢速恢复条件：foodLevel >= 18 且 saturation == 0，每 80 ticks 恢复 1 生命
    Player player(1, "TestPlayer");

    // 设置高饥饿值但无饱和度
    player.foodStats().setFoodLevel(18);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(15.0f);

    // 验证初始条件
    EXPECT_EQ(player.foodStats().foodLevel(), 18);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel(), 0.0f);
    EXPECT_FLOAT_EQ(player.health(), 15.0f);

    // tick() 需要调用多次触发慢速恢复 (每 80 ticks)
    for (int i = 0; i < 85; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证生命恢复
    EXPECT_GT(player.health(), 15.0f); // 生命应该恢复
    EXPECT_EQ(player.foodStats().foodLevel(), 18); // 饥饿值不变（消耗值消耗）
}

TEST(FoodStats, StarvationDamage) {
    // 饥饿伤害条件：foodLevel <= 0，每 80 ticks 造成 1 点伤害
    Player player(1, "TestPlayer");

    // 设置零饥饿值
    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(20.0f);

    // 验证初始条件
    EXPECT_EQ(player.foodStats().foodLevel(), 0);
    EXPECT_FLOAT_EQ(player.health(), 20.0f);

    // tick() 需要调用多次触发饥饿伤害 (每 80 ticks)
    for (int i = 0; i < 85; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证饥饿伤害
    EXPECT_LT(player.health(), 20.0f); // 应该受到伤害
}

TEST(FoodStats, StarvationDamageEasyMode) {
    // 简单模式：饥饿伤害最低保留 10 点生命
    Player player(1, "TestPlayer");

    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(12.0f); // 12 点生命

    // 触发多次饥饿伤害
    for (int i = 0; i < 170; ++i) { // 足够触发多次伤害
        player.foodStats().tick(player, Difficulty::Easy, true);
    }

    // 简单模式：生命最低 10
    EXPECT_GE(player.health(), 10.0f);
}

TEST(FoodStats, StarvationDamageNormalMode) {
    // 普通模式：饥饿伤害最低保留 1 点生命
    Player player(1, "TestPlayer");

    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(5.0f);

    // 触发多次饥饿伤害
    for (int i = 0; i < 500; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 普通模式：生命最低 1
    EXPECT_GE(player.health(), 1.0f);
}

TEST(FoodStats, PeacefulMode) {
    // 和平模式：自动恢复生命和饥饿值
    Player player(1, "TestPlayer");

    player.foodStats().setFoodLevel(10);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(15.0f);

    // 和平模式 tick
    for (int i = 0; i < 25; ++i) { // 20 ticks = 1秒，应该恢复生命
        player.foodStats().tick(player, Difficulty::Peaceful, true);
    }

    // 和平模式：生命恢复
    EXPECT_GT(player.health(), 15.0f);

    // 和平模式：饥饿值恢复（每 10 ticks 恢复 1）
    EXPECT_GT(player.foodStats().foodLevel(), 10);
}

TEST(FoodStats, PeacefulModeNoStarvation) {
    // 和平模式：即使饥饿值为 0 也不会造成伤害
    Player player(1, "TestPlayer");

    player.foodStats().setFoodLevel(0);
    player.foodStats().setSaturationLevel(0.0f);
    player.setHealth(20.0f);

    // 和平模式 tick 多次
    for (int i = 0; i < 200; ++i) {
        player.foodStats().tick(player, Difficulty::Peaceful, true);
    }

    // 和平模式：不会受到饥饿伤害
    EXPECT_FLOAT_EQ(player.health(), 20.0f);
    // 饥饿值应该恢复
    EXPECT_GT(player.foodStats().foodLevel(), 0);
}

TEST(FoodStats, NoRegenerationWithHungerEffect) {
    // 有饥饿效果时不恢复生命
    // 注意：此测试需要 Player 支持 addEffect() 方法
    // 目前仅验证基础逻辑框架
    Player player(1, "TestPlayer");

    player.foodStats().setFoodLevel(20);
    player.foodStats().setSaturationLevel(6.0f);
    player.setHealth(10.0f);

    // 正常情况下应该恢复
    for (int i = 0; i < 15; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, true);
    }

    // 验证在无饥饿效果时可以恢复
    EXPECT_GT(player.health(), 10.0f);
}

TEST(FoodStats, NaturalRegenerationDisabled) {
    // naturalRegeneration=false 时不应恢复生命
    Player player(1, "TestPlayer");

    player.foodStats().setFoodLevel(20);
    player.foodStats().setSaturationLevel(6.0f);
    player.setHealth(10.0f);

    // naturalRegeneration=false
    for (int i = 0; i < 15; ++i) {
        player.foodStats().tick(player, Difficulty::Normal, false);
    }

    // 禁用自然恢复时，生命不应恢复
    EXPECT_FLOAT_EQ(player.health(), 10.0f);
}


TEST(Player, PoseHeight) {
    Player player(1, "TestPlayer");

    // 站立
    player.setPose(EntityPose::Standing);
    EXPECT_FLOAT_EQ(player.height(), 1.8f);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 1.62f);

    // 潜行
    player.setPose(EntityPose::Crouching);
    EXPECT_FLOAT_EQ(player.height(), 1.5f);

    // 游泳
    player.setPose(EntityPose::Swimming);
    EXPECT_FLOAT_EQ(player.height(), 0.6f);

    // 睡觉
    player.setPose(EntityPose::Sleeping);
    EXPECT_FLOAT_EQ(player.height(), 0.2f);
}

TEST(Player, SprintingSneaking) {
    Player player(1, "TestPlayer");

    player.setSprinting(true);
    EXPECT_TRUE(player.isSprinting());
    EXPECT_TRUE(player.hasFlag(EntityFlags::Sprinting));

    player.setSprinting(false);
    EXPECT_FALSE(player.isSprinting());

    player.setSneaking(true);
    EXPECT_TRUE(player.isSneaking());
    EXPECT_TRUE(player.hasFlag(EntityFlags::Crouching));
    EXPECT_EQ(player.pose(), EntityPose::Crouching);
}

TEST(Player, Respawn) {
    Player player(1, "TestPlayer");

    player.damage(30.0f);
    EXPECT_TRUE(player.isDead());

    player.respawn();
    EXPECT_FALSE(player.isDead());
    EXPECT_FLOAT_EQ(player.health(), 20.0f);
    EXPECT_EQ(player.foodStats().foodLevel(), 20);
}

TEST(Player, SerializeDeserialize) {
    Player original(1, "TestPlayer");
    original.setPlayerId(12345);
    original.setPosition(100.5, 64.0, -200.25);
    original.setRotation(90.0f, 45.0f);
    original.setHealth(15.0f);
    original.setGameMode(GameMode::Creative);
    original.setExperienceLevel(10);

    network::PacketSerializer ser;
    original.serialize(ser);

    network::PacketDeserializer deser(ser.buffer());
    auto result = Player::deserialize(deser);

    EXPECT_TRUE(result.success());

    auto restored = result.value();
    EXPECT_EQ(restored->playerId(), 12345u);
    EXPECT_EQ(restored->username(), "TestPlayer");
    EXPECT_FLOAT_EQ(static_cast<float>(restored->x()), 100.5f);
    EXPECT_FLOAT_EQ(static_cast<float>(restored->y()), 64.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(restored->z()), -200.25f);
    EXPECT_FLOAT_EQ(restored->yaw(), 90.0f);
    EXPECT_FLOAT_EQ(restored->pitch(), 45.0f);
    EXPECT_FLOAT_EQ(restored->health(), 15.0f);
    EXPECT_EQ(restored->gameMode(), GameMode::Creative);
    EXPECT_EQ(restored->experienceLevel(), 10);
}
