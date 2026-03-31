#include <gtest/gtest.h>

#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/player/PlayerManager.hpp"
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

    EXPECT_EQ(player.foodStats().foodLevel, 20);
    EXPECT_FLOAT_EQ(player.foodStats().saturationLevel, 5.0f);

    player.foodStats().addExhaustion(10.0f);
    // 4次消耗触发，每次消耗1饱和度或1饥饿值
    EXPECT_LT(player.foodStats().saturationLevel, 5.0f);

    player.foodStats().addStats(5, 3.0f);
    EXPECT_EQ(player.foodStats().foodLevel, 20); // 最大20
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
    EXPECT_EQ(player.foodStats().foodLevel, 20);
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

    auto& restored = result.value();
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

// ============================================================================
// PlayerManager 测试
// ============================================================================

TEST(PlayerManager, CreatePlayer) {
    PlayerManager manager;

    auto player = manager.createPlayer("TestPlayer");
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->username(), "TestPlayer");
    EXPECT_EQ(manager.playerCount(), 1u);
}

TEST(PlayerManager, CreatePlayerWithId) {
    PlayerManager manager;

    auto player = manager.createPlayer(100, "TestPlayer");
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->playerId(), 100u);
}

TEST(PlayerManager, DuplicateUsername) {
    PlayerManager manager;

    auto player1 = manager.createPlayer("TestPlayer");
    ASSERT_NE(player1, nullptr);

    auto player2 = manager.createPlayer("TestPlayer");
    EXPECT_EQ(player2, nullptr);
    EXPECT_EQ(manager.playerCount(), 1u);
}

TEST(PlayerManager, RemovePlayer) {
    PlayerManager manager;

    auto player = manager.createPlayer("TestPlayer");
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(manager.playerCount(), 1u);

    manager.removePlayer(player->playerId());
    EXPECT_EQ(manager.playerCount(), 0u);
}

TEST(PlayerManager, GetPlayer) {
    PlayerManager manager;

    auto player = manager.createPlayer(100, "TestPlayer");
    ASSERT_NE(player, nullptr);

    auto found = manager.getPlayer(100);
    EXPECT_EQ(found, player);

    auto foundByUsername = manager.getPlayerByUsername("TestPlayer");
    EXPECT_EQ(foundByUsername, player);

    auto notFound = manager.getPlayer(999);
    EXPECT_EQ(notFound, nullptr);
}

TEST(PlayerManager, MaxPlayers) {
    PlayerManager manager;

    for (size_t i = 0; i < PlayerManager::MAX_PLAYERS; ++i) {
        auto player = manager.createPlayer("Player" + std::to_string(i));
        ASSERT_NE(player, nullptr) << "Failed at player " << i;
    }

    EXPECT_EQ(manager.playerCount(), PlayerManager::MAX_PLAYERS);

    // 尝试创建超过最大数量
    auto overflow = manager.createPlayer("Overflow");
    EXPECT_EQ(overflow, nullptr);
}

TEST(PlayerManager, ForEachPlayer) {
    PlayerManager manager;

    (void)manager.createPlayer("Player1");
    (void)manager.createPlayer("Player2");
    (void)manager.createPlayer("Player3");

    int count = 0;
    manager.forEachPlayer([&count](const std::shared_ptr<Player>&) {
        count++;
    });

    EXPECT_EQ(count, 3);
}

TEST(PlayerManager, GetAllPlayers) {
    PlayerManager manager;

    (void)manager.createPlayer("Player1");
    (void)manager.createPlayer("Player2");
    (void)manager.createPlayer("Player3");

    auto players = manager.getAllPlayers();
    EXPECT_EQ(players.size(), 3u);
}

TEST(PlayerManager, Clear) {
    PlayerManager manager;

    (void)manager.createPlayer("Player1");
    (void)manager.createPlayer("Player2");
    EXPECT_EQ(manager.playerCount(), 2u);

    manager.clear();
    EXPECT_EQ(manager.playerCount(), 0u);
}
