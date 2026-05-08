#include <gtest/gtest.h>
#include "server/world/ServerWorld.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <thread>
#include <atomic>

using namespace mc;
using namespace mc::server;

// ============================================================================
// Mock 连接用于测试
// ============================================================================

class MockConnection : public network::IServerConnection {
public:
    MockConnection() : m_connected(true) {}

    void send(const u8* data, size_t size) override {
        (void)data;
        (void)size;
        m_sentData.insert(m_sentData.end(), data, data + size);
    }

    void disconnect(const std::string& reason = "") override {
        (void)reason;
        m_connected = false;
    }

    [[nodiscard]] bool isConnected() const override {
        return m_connected;
    }

    [[nodiscard]] std::string identifier() const override {
        return "MockConnection";
    }

    [[nodiscard]] network::ConnectionType type() const override {
        return network::ConnectionType::Local;
    }

    std::vector<u8> m_sentData;
    bool m_connected;
};

// ============================================================================
// ServerWorld 测试固件
// ============================================================================

class ServerWorldTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        config.isDebugWorld = false;
        world = std::make_unique<ServerWorld>(config);
    }

    void TearDown() override {
        world.reset();
    }

    std::unique_ptr<ServerWorld> world;
};

// ============================================================================
// 构造和初始化测试
// ============================================================================

TEST_F(ServerWorldTest, DefaultConstructor) {
    ServerWorld defaultWorld;
    EXPECT_EQ(defaultWorld.chunkCount(), 0);
}

TEST_F(ServerWorldTest, ConfigConstructor) {
    ServerWorldConfig config;
    config.viewDistance = 8;
    config.dimension = 1;

    ServerWorld configuredWorld(config);
    EXPECT_EQ(configuredWorld.config().viewDistance, 8);
    EXPECT_EQ(configuredWorld.config().dimension, 1);
}

TEST_F(ServerWorldTest, Initialize) {
    auto result = world->initialize();
    EXPECT_TRUE(result.success());
}

TEST_F(ServerWorldTest, Shutdown) {
    world->initialize();
    world->shutdown();
    EXPECT_EQ(world->chunkCount(), 0);
}

// ============================================================================
// 区块管理测试
// ============================================================================

TEST_F(ServerWorldTest, GetChunk_NotExists) {
    ChunkData* chunk = world->getChunk(0, 0);
    EXPECT_EQ(chunk, nullptr);
}

TEST_F(ServerWorldTest, HasChunk_NotExists) {
    EXPECT_FALSE(world->hasChunk(0, 0));
    EXPECT_FALSE(world->hasChunk(100, 100));
}

TEST_F(ServerWorldTest, GetChunkSync_CreatesChunk) {
    ChunkData* chunk = world->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, GetChunkSync_ReturnsSameChunk) {
    ChunkData* chunk1 = world->getChunkSync(5, 10);
    ChunkData* chunk2 = world->getChunkSync(5, 10);

    EXPECT_EQ(chunk1, chunk2);
}

TEST_F(ServerWorldTest, GetChunk_AfterGeneration) {
    world->getChunkSync(3, 7);

    ChunkData* chunk = world->getChunk(3, 7);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 3);
    EXPECT_EQ(chunk->z(), 7);
}

TEST_F(ServerWorldTest, UnloadChunk) {
    world->getChunkSync(0, 0);
    EXPECT_TRUE(world->hasChunk(0, 0));

    world->unloadChunk(0, 0);
    EXPECT_FALSE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, ChunkCount) {
    EXPECT_EQ(world->chunkCount(), 0);

    world->getChunkSync(0, 0);
    EXPECT_EQ(world->chunkCount(), 1);

    world->getChunkSync(1, 0);
    world->getChunkSync(0, 1);
    EXPECT_EQ(world->chunkCount(), 3);

    world->unloadChunk(1, 0);
    EXPECT_EQ(world->chunkCount(), 2);
}

TEST_F(ServerWorldTest, MultipleChunks) {
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            world->getChunkSync(x, z);
        }
    }

    EXPECT_EQ(world->chunkCount(), 25);

    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            EXPECT_TRUE(world->hasChunk(x, z));
        }
    }
}

// ============================================================================
// 方块操作测试
// ============================================================================

TEST_F(ServerWorldTest, SetBlock_CreatesChunk) {
    ASSERT_TRUE(world->initialize().success());

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world->setBlockState(0, 64, 0, stoneState);

    EXPECT_TRUE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, SetBlock_GetBlock) {
    ASSERT_TRUE(world->initialize().success());

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world->setBlockState(10, 50, 20, stoneState);

    const BlockState* block = world->getBlockState(10, 50, 20);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::STONE->blockId());
}

TEST_F(ServerWorldTest, GetBlock_NonExistentChunk) {
    const BlockState* block = world->getBlockState(1000, 64, 1000);
    // 不存在的区块返回 nullptr (表示空气)
    EXPECT_EQ(block, nullptr);
}

TEST_F(ServerWorldTest, SetBlock_NegativeCoordinates) {
    ASSERT_TRUE(world->initialize().success());

    const BlockState* grassState = &VanillaBlocks::GRASS_BLOCK->defaultState();
    world->setBlockState(-10, 64, -20, grassState);

    const BlockState* block = world->getBlockState(-10, 64, -20);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->blockId(), VanillaBlocks::GRASS_BLOCK->blockId());
}

TEST_F(ServerWorldTest, SetBlock_MultipleBlocks) {
    ASSERT_TRUE(world->initialize().success());

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
    const BlockState* grassState = &VanillaBlocks::GRASS_BLOCK->defaultState();

    world->setBlockState(0, 64, 0, stoneState);
    world->setBlockState(1, 64, 0, dirtState);
    world->setBlockState(0, 65, 0, grassState);

    const BlockState* block0 = world->getBlockState(0, 64, 0);
    const BlockState* block1 = world->getBlockState(1, 64, 0);
    const BlockState* block2 = world->getBlockState(0, 65, 0);

    ASSERT_NE(block0, nullptr);
    ASSERT_NE(block1, nullptr);
    ASSERT_NE(block2, nullptr);
    EXPECT_EQ(block0->blockId(), VanillaBlocks::STONE->blockId());
    EXPECT_EQ(block1->blockId(), VanillaBlocks::DIRT->blockId());
    EXPECT_EQ(block2->blockId(), VanillaBlocks::GRASS_BLOCK->blockId());
}

TEST_F(ServerWorldTest, GetHeight_ReturnsAirLayerAboveTopBlock) {
    ASSERT_TRUE(world->initialize().success());

    ChunkData* chunk = world->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    const i32 localX = 0;
    const i32 localZ = 0;
    const i32 topBlockY = chunk->getTopBlockY(HeightmapType::WorldSurfaceWG, localX, localZ);
    ASSERT_GE(topBlockY, 0);

    const i32 worldX = localX;
    const i32 worldZ = localZ;
    EXPECT_EQ(world->getHeight(worldX, worldZ), topBlockY + 1);
}

// ============================================================================
// 配置测试
// ============================================================================

TEST_F(ServerWorldTest, SetConfig) {
    ServerWorldConfig newConfig;
    newConfig.viewDistance = 16;
    newConfig.dimension = 2;

    world->setConfig(newConfig);

    EXPECT_EQ(world->config().viewDistance, 16);
    EXPECT_EQ(world->config().dimension, 2);
}

TEST_F(ServerWorldTest, Initialize_AppliesConfiguredViewDistance) {
    ServerWorldConfig config;
    config.viewDistance = 18;
    config.dimension = 0;
    config.seed = 12345;

    ServerWorld configuredWorld(config);
    auto result = configuredWorld.initialize();
    ASSERT_TRUE(result.success());
    ASSERT_NE(configuredWorld.chunkManager(), nullptr);
    EXPECT_EQ(configuredWorld.chunkManager()->viewDistance(), 18);
}

TEST_F(ServerWorldTest, SetChunkManager_AppliesConfiguredViewDistance) {
    ServerWorldConfig config;
    config.viewDistance = 18;
    config.dimension = 0;
    config.seed = 67890;

    ServerWorld configuredWorld(config);

    auto generator = std::make_unique<NoiseChunkGenerator>(config.seed, DimensionSettings::overworld());
    auto chunkManager = std::make_unique<ServerChunkManager>(configuredWorld, std::move(generator));
    ASSERT_NE(chunkManager, nullptr);

    configuredWorld.setChunkManager(std::move(chunkManager));
    ASSERT_NE(configuredWorld.chunkManager(), nullptr);
    EXPECT_EQ(configuredWorld.chunkManager()->viewDistance(), 18);
}

// ============================================================================
// Tick 测试
// ============================================================================

TEST_F(ServerWorldTest, Tick) {
    world->initialize();

    // 执行多次 tick 不应崩溃
    for (int i = 0; i < 1000; ++i) {
        world->tick();
    }
}

TEST_F(ServerWorldTest, ChunkUnloading) {
    world->initialize();

    // 创建一个区块
    world->getChunkSync(100, 100);

    // 执行多次 tick 触发卸载检查
    for (int i = 0; i < 200; ++i) {
        world->tick();
    }

    // 没有玩家订阅的区块应该被卸载
    // (取决于卸载延迟配置)
}

// ============================================================================
// 线程安全测试
// ============================================================================

TEST_F(ServerWorldTest, ConcurrentChunkAccess) {
    world->initialize();

    std::vector<std::thread> threads;

    // 多线程同时访问区块
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 100; ++j) {
                int x = (i * 100 + j) % 20 - 10;
                int z = (i * 100 + j + 50) % 20 - 10;
                world->getChunkSync(x, z);
                world->hasChunk(x, z);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 如果没有崩溃或死锁，测试通过
    EXPECT_GT(world->chunkCount(), 0);
}

TEST_F(ServerWorldTest, PlaySound_InvokesCallback) {
    std::optional<ResourceLocation> soundEventId;
    std::optional<sound::SoundCategory> category;
    std::optional<Vector3> position;
    std::optional<f32> volume;
    std::optional<f32> pitch;

    world->setOnPlaySound([&](const ResourceLocation& eventId,
                              sound::SoundCategory soundCategory,
                              const Vector3& soundPosition,
                              f32 soundVolume,
                              f32 soundPitch) {
        soundEventId = eventId;
        category = soundCategory;
        position = soundPosition;
        volume = soundVolume;
        pitch = soundPitch;
    });

    world->playSound(ResourceLocation("minecraft:test.sound"), sound::SoundCategory::Players, Vector3(1.0f, 2.0f, 3.0f), 0.75f, 1.25f);

    ASSERT_TRUE(soundEventId.has_value());
    ASSERT_TRUE(category.has_value());
    ASSERT_TRUE(position.has_value());
    ASSERT_TRUE(volume.has_value());
    ASSERT_TRUE(pitch.has_value());

    EXPECT_EQ(soundEventId->toString(), "minecraft:test.sound");
    EXPECT_EQ(*category, sound::SoundCategory::Players);
    EXPECT_FLOAT_EQ(position->x, 1.0f);
    EXPECT_FLOAT_EQ(position->y, 2.0f);
    EXPECT_FLOAT_EQ(position->z, 3.0f);
    EXPECT_FLOAT_EQ(*volume, 0.75f);
    EXPECT_FLOAT_EQ(*pitch, 1.25f);
}
