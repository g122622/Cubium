#include <gtest/gtest.h>
#include "server/world/ServerWorld.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "common/network/connection/IServerConnection.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/blockentity/transport/HopperEntity.hpp"
#include "common/world/blockentity/processing/FurnaceEntity.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <thread>
#include <atomic>

using namespace mc;
using namespace mc::server;
using namespace mc::blockentity;

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
        // 注意：isDebugWorld 字段已移除，改用 isDebugWorld() 方法通过检测区块生成器类型判断
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
    ChunkData* chunk = world->chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, GetChunkSync_ReturnsSameChunk) {
    ChunkData* chunk1 = world->chunkManager()->getChunkSync(5, 10);
    ChunkData* chunk2 = world->chunkManager()->getChunkSync(5, 10);

    EXPECT_EQ(chunk1, chunk2);
}

TEST_F(ServerWorldTest, GetChunk_AfterGeneration) {
    world->chunkManager()->getChunkSync(3, 7);

    ChunkData* chunk = world->getChunk(3, 7);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 3);
    EXPECT_EQ(chunk->z(), 7);
}

TEST_F(ServerWorldTest, UnloadChunk) {
    world->chunkManager()->getChunkSync(0, 0);
    EXPECT_TRUE(world->hasChunk(0, 0));

    world->chunkManager()->unloadChunkSync(0, 0);
    EXPECT_FALSE(world->hasChunk(0, 0));
}

TEST_F(ServerWorldTest, ChunkCount) {
    EXPECT_EQ(world->chunkCount(), 0);

    world->chunkManager()->getChunkSync(0, 0);
    EXPECT_EQ(world->chunkCount(), 1);

    world->chunkManager()->getChunkSync(1, 0);
    world->chunkManager()->getChunkSync(0, 1);
    EXPECT_EQ(world->chunkCount(), 3);

    world->chunkManager()->unloadChunkSync(1, 0);
    EXPECT_EQ(world->chunkCount(), 2);
}

TEST_F(ServerWorldTest, MultipleChunks) {
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            world->chunkManager()->getChunkSync(x, z);
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

    ChunkData* chunk = world->chunkManager()->getChunkSync(0, 0);
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
    world->chunkManager()->getChunkSync(100, 100);

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
                world->chunkManager()->getChunkSync(x, z);
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

// ============================================================================
// isDebugWorld 测试
// ============================================================================

TEST_F(ServerWorldTest, IsDebugWorld_ReturnsFalse_WithoutChunkManager) {
    // 无区块管理器时返回 false
    EXPECT_FALSE(world->isDebugWorld());
}

TEST_F(ServerWorldTest, IsDebugWorld_ReturnsFalse_WithNoiseChunkGenerator) {
    ServerWorldConfig config;
    config.viewDistance = 10;
    config.dimension = 0;
    config.seed = 12345;

    ServerWorld testWorld(config);
    auto result = testWorld.initialize();
    ASSERT_TRUE(result.success());

    // 默认使用 NoiseChunkGenerator，不是调试世界
    EXPECT_FALSE(testWorld.isDebugWorld());
}

TEST_F(ServerWorldTest, IsDebugWorld_ReturnsTrue_WithDebugChunkGenerator) {
    ServerWorldConfig config;
    config.viewDistance = 10;
    config.dimension = 0;
    config.seed = 12345;

    ServerWorld testWorld(config);
    auto result = testWorld.initialize();
    ASSERT_TRUE(result.success());

    // 创建 DebugChunkGenerator
    auto debugGenerator = std::make_unique<DebugChunkGenerator>();
    auto chunkManager = std::make_unique<ServerChunkManager>(testWorld, std::move(debugGenerator));
    testWorld.setChunkManager(std::move(chunkManager));

    // 使用 DebugChunkGenerator 时应返回 true
    EXPECT_TRUE(testWorld.isDebugWorld());
}

// ============================================================================
// 方块实体管理测试
// ============================================================================

TEST_F(ServerWorldTest, GetBlockEntity_ReturnsNullptr_WhenNoChunk) {
    // 没有区块时返回 nullptr
    BlockEntity* entity = world->getBlockEntity(BlockPos(0, 64, 0));
    EXPECT_EQ(entity, nullptr);
}

TEST_F(ServerWorldTest, GetBlockEntity_ReturnsNullptr_WhenNoEntity) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 没有方块实体时返回 nullptr
    BlockEntity* entity = world->getBlockEntity(BlockPos(0, 64, 0));
    EXPECT_EQ(entity, nullptr);
}

TEST_F(ServerWorldTest, GetBlockEntity_OutOfWorldBounds_ReturnsNullptr) {
    ASSERT_TRUE(world->initialize().success());

    // 超出世界高度范围返回 nullptr
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 1000, 0)), nullptr);
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, -100, 0)), nullptr);
}

TEST_F(ServerWorldTest, SetBlockEntity_StoresEntity) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 创建方块实体
    auto chest = std::make_unique<blockentity::ChestEntity>(BlockPos(5, 64, 10));
    blockentity::ChestEntity* rawChest = chest.get();

    // 设置方块实体
    world->setBlockEntity(BlockPos(5, 64, 10), chest.release());

    // 验证可以获取
    BlockEntity* retrieved = world->getBlockEntity(BlockPos(5, 64, 10));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved, rawChest);
    EXPECT_EQ(retrieved->getType(), BlockEntityType::Chest);
    EXPECT_EQ(retrieved->getPos(), BlockPos(5, 64, 10));
}

TEST_F(ServerWorldTest, SetBlockEntity_OverwritesExisting) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 创建第一个方块实体
    auto chest1 = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    world->setBlockEntity(BlockPos(0, 64, 0), chest1.release());

    // 创建第二个方块实体并覆盖
    auto chest2 = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    blockentity::ChestEntity* rawChest2 = chest2.get();
    world->setBlockEntity(BlockPos(0, 64, 0), chest2.release());

    // 验证是第二个实体
    BlockEntity* retrieved = world->getBlockEntity(BlockPos(0, 64, 0));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved, rawChest2);
}

TEST_F(ServerWorldTest, SetBlockEntity_SetsWorldReference) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 创建方块实体
    auto chest = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    world->setBlockEntity(BlockPos(0, 64, 0), chest.release());

    // 验证世界引用已设置
    BlockEntity* retrieved = world->getBlockEntity(BlockPos(0, 64, 0));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getWorld(), world.get());
}

TEST_F(ServerWorldTest, RemoveBlockEntity_RemovesEntity) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 创建并设置方块实体
    auto chest = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    world->setBlockEntity(BlockPos(0, 64, 0), chest.release());

    // 验证存在
    EXPECT_NE(world->getBlockEntity(BlockPos(0, 64, 0)), nullptr);

    // 移除
    world->removeBlockEntity(BlockPos(0, 64, 0));

    // 验证已移除
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 64, 0)), nullptr);
}

TEST_F(ServerWorldTest, RemoveBlockEntity_NoEntity_NoCrash) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 移除不存在的方块实体不应崩溃
    world->removeBlockEntity(BlockPos(0, 64, 0));
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 64, 0)), nullptr);
}

TEST_F(ServerWorldTest, RemoveBlockEntity_OutOfWorldBounds_NoCrash) {
    ASSERT_TRUE(world->initialize().success());

    // 移除超出范围的方块实体不应崩溃
    world->removeBlockEntity(BlockPos(0, 1000, 0));
    world->removeBlockEntity(BlockPos(0, -100, 0));
}

TEST_F(ServerWorldTest, SetBlockEntity_MultipleEntitiesInSameChunk) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 创建多个方块实体
    auto chest1 = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    auto chest2 = std::make_unique<blockentity::ChestEntity>(BlockPos(5, 65, 10));
    auto chest3 = std::make_unique<blockentity::ChestEntity>(BlockPos(15, 70, 15));

    blockentity::ChestEntity* raw1 = chest1.get();
    blockentity::ChestEntity* raw2 = chest2.get();
    blockentity::ChestEntity* raw3 = chest3.get();

    world->setBlockEntity(BlockPos(0, 64, 0), chest1.release());
    world->setBlockEntity(BlockPos(5, 65, 10), chest2.release());
    world->setBlockEntity(BlockPos(15, 70, 15), chest3.release());

    // 验证所有实体都可以获取
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 64, 0)), raw1);
    EXPECT_EQ(world->getBlockEntity(BlockPos(5, 65, 10)), raw2);
    EXPECT_EQ(world->getBlockEntity(BlockPos(15, 70, 15)), raw3);
}

TEST_F(ServerWorldTest, SetBlockEntity_MultipleChunks) {
    ASSERT_TRUE(world->initialize().success());

    // 创建多个区块
    world->chunkManager()->getChunkSync(0, 0);
    world->chunkManager()->getChunkSync(1, 0);
    world->chunkManager()->getChunkSync(0, 1);

    // 在不同区块创建方块实体
    auto chest1 = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    auto chest2 = std::make_unique<blockentity::ChestEntity>(BlockPos(16, 64, 0));  // 区块 (1, 0)
    auto chest3 = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 16));  // 区块 (0, 1)

    blockentity::ChestEntity* raw1 = chest1.get();
    blockentity::ChestEntity* raw2 = chest2.get();
    blockentity::ChestEntity* raw3 = chest3.get();

    world->setBlockEntity(BlockPos(0, 64, 0), chest1.release());
    world->setBlockEntity(BlockPos(16, 64, 0), chest2.release());
    world->setBlockEntity(BlockPos(0, 64, 16), chest3.release());

    // 验证所有实体都可以获取
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 64, 0)), raw1);
    EXPECT_EQ(world->getBlockEntity(BlockPos(16, 64, 0)), raw2);
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 64, 16)), raw3);
}

TEST_F(ServerWorldTest, SetBlockEntity_Nullptr_DoesNothing) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 设置 nullptr 不应崩溃
    world->setBlockEntity(BlockPos(0, 64, 0), nullptr);

    // 验证没有设置任何东西
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 64, 0)), nullptr);
}

TEST_F(ServerWorldTest, ConstGetBlockEntity_ReturnsCorrectEntity) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 创建并设置方块实体
    auto chest = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    world->setBlockEntity(BlockPos(0, 64, 0), chest.release());

    // 使用 const 版本获取
    const ServerWorld& constWorld = *world;
    const BlockEntity* retrieved = constWorld.getBlockEntity(BlockPos(0, 64, 0));
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getType(), BlockEntityType::Chest);
}

TEST_F(ServerWorldTest, SetBlockEntity_DifferentBlockEntityTypes) {
    ASSERT_TRUE(world->initialize().success());

    // 创建区块
    world->chunkManager()->getChunkSync(0, 0);

    // 创建不同类型的方块实体
    auto furnace = std::make_unique<blockentity::FurnaceEntity>(BlockPos(0, 64, 0));
    auto hopper = std::make_unique<blockentity::HopperEntity>(BlockPos(1, 64, 0));
    auto sign = std::make_unique<blockentity::SignEntity>(BlockPos(2, 64, 0));

    world->setBlockEntity(BlockPos(0, 64, 0), furnace.release());
    world->setBlockEntity(BlockPos(1, 64, 0), hopper.release());
    world->setBlockEntity(BlockPos(2, 64, 0), sign.release());

    // 验证类型正确
    EXPECT_EQ(world->getBlockEntity(BlockPos(0, 64, 0))->getType(), BlockEntityType::Furnace);
    EXPECT_EQ(world->getBlockEntity(BlockPos(1, 64, 0))->getType(), BlockEntityType::Hopper);
    EXPECT_EQ(world->getBlockEntity(BlockPos(2, 64, 0))->getType(), BlockEntityType::Sign);
}

// ============================================================================
// 组件访问器测试
// ============================================================================

TEST_F(ServerWorldTest, CollisionCacheAccessor_ReturnsNullptrBeforeInitialize) {
    // 初始化前碰撞缓存应该为 nullptr
    EXPECT_EQ(world->collisionCache(), nullptr);
}

TEST_F(ServerWorldTest, CollisionCacheAccessor_ReturnsValidPointerAfterInitialize) {
    ASSERT_TRUE(world->initialize().success());

    // 初始化后碰撞缓存应该有效
    auto* cache = world->collisionCache();
    ASSERT_NE(cache, nullptr);
}

TEST_F(ServerWorldTest, CollisionCacheAccessor_ConstVersionWorks) {
    ASSERT_TRUE(world->initialize().success());

    const ServerWorld& constWorld = *world;
    const auto* cache = constWorld.collisionCache();
    ASSERT_NE(cache, nullptr);
}

TEST_F(ServerWorldTest, EntityManagerAccessor_ReturnsValidReference) {
    // 即使未初始化，entityManager 也应该有效
    auto& em = world->entityManager();
    EXPECT_EQ(em.entityCount(), 0);
}

TEST_F(ServerWorldTest, EntityManagerAccessor_ConstVersionWorks) {
    const ServerWorld& constWorld = *world;
    const auto& em = constWorld.entityManager();
    EXPECT_EQ(em.entityCount(), 0);
}

TEST_F(ServerWorldTest, ChunkManagerAccessor_ReturnsNullptrBeforeSetChunkManager) {
    // 初始化前区块管理器应该为 nullptr
    EXPECT_EQ(world->chunkManager(), nullptr);
}

TEST_F(ServerWorldTest, ChunkManagerAccessor_ReturnsValidPointerAfterInitialize) {
    ASSERT_TRUE(world->initialize().success());

    auto* cm = world->chunkManager();
    ASSERT_NE(cm, nullptr);
    EXPECT_EQ(cm->viewDistance(), 10);  // 配置中设置的值
}

// ============================================================================
// 实体移除自动追踪测试
// ============================================================================

TEST_F(ServerWorldTest, RemoveEntity_AutoUntracksFromEntityTracker) {
    ASSERT_TRUE(world->initialize().success());

    // 创建一个简单实体
    ItemStack stack;  // 空物品堆
    auto entity = std::make_unique<ItemEntity>(EntityId(1), stack, 100.0f, 64.0f, 100.0f);
    EntityId entityId = entity->id();

    // 生成实体
    EntityId spawnedId = world->spawnEntity(std::move(entity));
    EXPECT_NE(spawnedId, EntityId(0));

    // 验证实体的追踪器状态
    EXPECT_TRUE(world->entityTracker().isTracking(entityId));

    // 移除实体 - 应该自动从追踪器中移除
    auto removedEntity = world->removeEntity(entityId);

    // 验证实体被正确移除
    ASSERT_NE(removedEntity, nullptr);
    EXPECT_EQ(removedEntity->id(), entityId);

    // 验证实体不再被追踪
    EXPECT_FALSE(world->entityTracker().isTracking(entityId));

    // 验证实体已从管理器中移除
    EXPECT_EQ(world->entityManager().getEntity(entityId), nullptr);
}

TEST_F(ServerWorldTest, RemoveEntity_NonExistentEntity_ReturnsNullptr) {
    ASSERT_TRUE(world->initialize().success());

    // 移除不存在的实体应该返回 nullptr
    auto result = world->removeEntity(EntityId(99999));
    EXPECT_EQ(result, nullptr);
}

TEST_F(ServerWorldTest, RemoveEntity_MultipleEntities_OnlyTargetRemoved) {
    ASSERT_TRUE(world->initialize().success());

    // 创建多个实体
    ItemStack stack;  // 空物品堆
    auto entity1 = std::make_unique<ItemEntity>(EntityId(1), stack, 0.0f, 64.0f, 0.0f);
    auto entity2 = std::make_unique<ItemEntity>(EntityId(2), stack, 10.0f, 64.0f, 10.0f);
    auto entity3 = std::make_unique<ItemEntity>(EntityId(3), stack, 20.0f, 64.0f, 20.0f);

    EntityId id1 = entity1->id();
    EntityId id2 = entity2->id();
    EntityId id3 = entity3->id();

    world->spawnEntity(std::move(entity1));
    world->spawnEntity(std::move(entity2));
    world->spawnEntity(std::move(entity3));

    // 验证所有实体都被追踪
    EXPECT_TRUE(world->entityTracker().isTracking(id1));
    EXPECT_TRUE(world->entityTracker().isTracking(id2));
    EXPECT_TRUE(world->entityTracker().isTracking(id3));

    // 移除中间的实体
    auto removed = world->removeEntity(id2);
    ASSERT_NE(removed, nullptr);
    EXPECT_EQ(removed->id(), id2);

    // 验证只有目标实体被移除和取消追踪
    EXPECT_FALSE(world->entityTracker().isTracking(id2));
    EXPECT_EQ(world->entityManager().getEntity(id2), nullptr);

    // 验证其他实体仍然存在
    EXPECT_TRUE(world->entityTracker().isTracking(id1));
    EXPECT_TRUE(world->entityTracker().isTracking(id3));
    EXPECT_NE(world->entityManager().getEntity(id1), nullptr);
    EXPECT_NE(world->entityManager().getEntity(id3), nullptr);
}
