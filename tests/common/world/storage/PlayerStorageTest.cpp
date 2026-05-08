#include <gtest/gtest.h>
#include "world/storage/player/PlayerSaveData.hpp"
#include "world/storage/player/PlayerDataManager.hpp"
#include "world/storage/db/RocksDBDatabase.hpp"
#include "world/storage/db/ColumnFamilies.hpp"
#include "server/core/ServerPlayerData.hpp"
#include <filesystem>
#include <fstream>

namespace mc::world::storage {
namespace {

// 测试临时目录
class PlayerStorageTest : public ::testing::Test {
protected:
    std::filesystem::path m_testDir;

    void SetUp() override {
        // 创建临时测试目录
        m_testDir = std::filesystem::temp_directory_path() / "player_storage_test";
        std::filesystem::create_directories(m_testDir);
    }

    void TearDown() override {
        // 清理临时目录
        if (std::filesystem::exists(m_testDir)) {
            std::filesystem::remove_all(m_testDir);
        }
    }
};

// ============================================================================
// PlayerSaveData 测试
// ============================================================================

class PlayerSaveDataTest : public PlayerStorageTest {
};

TEST_F(PlayerSaveDataTest, DefaultConstruction) {
    PlayerSaveData data;

    EXPECT_TRUE(data.uuid.empty());
    EXPECT_TRUE(data.username.empty());
    EXPECT_EQ(data.posX, 0.0);
    EXPECT_EQ(data.posY, 64.0);  // 默认出生高度
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

TEST_F(PlayerSaveDataTest, ParameterizedConstruction) {
    PlayerSaveData data("test-uuid-123", "TestPlayer");

    EXPECT_EQ(data.uuid, "test-uuid-123");
    EXPECT_EQ(data.username, "TestPlayer");
}

TEST_F(PlayerSaveDataTest, NbtSerializationBasic) {
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

TEST_F(PlayerSaveDataTest, NbtRoundTrip) {
    PlayerSaveData original;
    original.uuid = "roundtrip-test";
    original.username = "Alex";
    original.posX = 1234.567;
    original.posY = 70.0;
    original.posZ = -567.890;
    original.yaw = 180.0f;
    original.pitch = -30.0f;
    original.dimension = -1;  // Nether
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

TEST_F(PlayerSaveDataTest, BinarySerialization) {
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

TEST_F(PlayerSaveDataTest, SpawnPointRoundTrip) {
    PlayerSaveData original;
    original.uuid = "spawn-test";
    original.spawnPoint = GlobalPos(-1, BlockPos(-100, 70, 300));  // Nether spawn
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

TEST_F(PlayerSaveDataTest, AbilitiesRoundTrip) {
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

TEST_F(PlayerSaveDataTest, ExperienceRoundTrip) {
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

TEST_F(PlayerSaveDataTest, EmptyEffects) {
    PlayerSaveData original;
    original.uuid = "effects-empty";
    // 不添加任何效果

    nbt::tags::compound_tag nbt = original.toNbt();
    auto result = PlayerSaveData::fromNbt(nbt);
    ASSERT_TRUE(result.success());

    const PlayerSaveData& restored = result.value();
    EXPECT_TRUE(restored.effects.empty());
}

TEST_F(PlayerSaveDataTest, EnteredNetherPosition) {
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

TEST_F(PlayerSaveDataTest, SleepingState) {
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

// ============================================================================
// PlayerDataManager 测试
// ============================================================================

class PlayerDataManagerTest : public PlayerStorageTest {
protected:
    std::unique_ptr<RocksDBDatabase> m_db;
    std::unique_ptr<PlayerDataManager> m_manager;

    void SetUp() override {
        PlayerStorageTest::SetUp();

        // 创建测试数据库
        std::string dbPath = (m_testDir / "players_test.db").string();
        auto dbResult = RocksDBDatabase::open(dbPath);
        ASSERT_TRUE(dbResult.success()) << "Failed to open test database";
        m_db = std::move(dbResult.value());
        m_manager = std::make_unique<PlayerDataManager>(*m_db);
    }

    void TearDown() override {
        m_manager.reset();
        m_db.reset();
        PlayerStorageTest::TearDown();
    }
};

TEST_F(PlayerDataManagerTest, SaveAndLoad) {
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

TEST_F(PlayerDataManagerTest, LoadNonexistentPlayer) {
    auto result = m_manager->loadPlayer("nonexistent-uuid");
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), nullptr);
}

TEST_F(PlayerDataManagerTest, HasPlayer) {
    EXPECT_FALSE(m_manager->hasPlayer("has-player-test"));

    PlayerSaveData data;
    data.uuid = "has-player-test";
    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    EXPECT_TRUE(m_manager->hasPlayer("has-player-test"));
}

TEST_F(PlayerDataManagerTest, DeletePlayer) {
    PlayerSaveData data;
    data.uuid = "delete-test";
    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    EXPECT_TRUE(m_manager->hasPlayer("delete-test"));

    auto deleteResult = m_manager->deletePlayer("delete-test");
    ASSERT_TRUE(deleteResult.success());

    EXPECT_FALSE(m_manager->hasPlayer("delete-test"));
}

TEST_F(PlayerDataManagerTest, CacheBehavior) {
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

TEST_F(PlayerDataManagerTest, DirtyTracking) {
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

TEST_F(PlayerDataManagerTest, SaveAllDirty) {
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

TEST_F(PlayerDataManagerTest, SaveAll) {
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

TEST_F(PlayerDataManagerTest, ClearCache) {
    PlayerSaveData data;
    data.uuid = "clear-cache-test";
    auto saveResult = m_manager->savePlayerImmediate(data);
    ASSERT_TRUE(saveResult.success());

    EXPECT_EQ(m_manager->cacheSize(), 1u);

    m_manager->clearCache();

    EXPECT_EQ(m_manager->cacheSize(), 0u);
    EXPECT_EQ(m_manager->dirtyCount(), 0u);
}

TEST_F(PlayerDataManagerTest, UpdatePlayer) {
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

TEST_F(PlayerDataManagerTest, FromServerPlayerDataConversion) {
    // 创建简化的测试数据
    server::ServerPlayerData serverData;
    serverData.playerId = 12345;
    serverData.username = "ServerPlayer";
    serverData.x = 100.0;
    serverData.y = 64.0;
    serverData.z = 200.0;
    serverData.yaw = 90.0f;
    serverData.pitch = 45.0f;
    serverData.gameMode = GameMode::Survival;
    serverData.onGround = true;

    PlayerSaveData saveData = PlayerDataManager::fromServerPlayerData(serverData);

    EXPECT_EQ(saveData.uuid, "12345");
    EXPECT_EQ(saveData.username, "ServerPlayer");
    EXPECT_DOUBLE_EQ(saveData.posX, 100.0);
    EXPECT_DOUBLE_EQ(saveData.posY, 64.0);
    EXPECT_DOUBLE_EQ(saveData.posZ, 200.0);
    EXPECT_FLOAT_EQ(saveData.yaw, 90.0f);
    EXPECT_FLOAT_EQ(saveData.pitch, 45.0f);
    EXPECT_EQ(saveData.gameMode, GameMode::Survival);
    EXPECT_TRUE(saveData.onGround);
}

TEST_F(PlayerDataManagerTest, CallbackTest) {
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

} // namespace
} // namespace mc::world::storage
