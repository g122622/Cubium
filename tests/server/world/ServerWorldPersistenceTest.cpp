#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/CraftingTableEntity.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <ctime>
#include <filesystem>
#include <gtest/gtest.h>

namespace mc::server {
namespace {

class ServerWorldPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_testDir = std::filesystem::temp_directory_path() / "mc_server_world_persistence_test" /
            std::to_string(std::time(nullptr));
        std::filesystem::create_directories(m_testDir);

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        // 初始化方块注册表（NoiseChunkGenerator 依赖）
        VanillaBlocks::initialize();
    }

    void TearDown() override
    {
        m_storage.close();
        if (std::filesystem::exists(m_testDir)) {
            std::error_code ec;
            std::filesystem::remove_all(m_testDir, ec);
        }
    }

    [[nodiscard]] std::unique_ptr<ServerWorld> createWorld()
    {
        ServerWorldConfig config;
        config.dimension = 0;
        config.seed = 12345;
        config.viewDistance = 2;

        auto world = std::make_unique<ServerWorld>(config);
        world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));

        auto initResult = world->initialize();
        EXPECT_TRUE(initResult.success()) << initResult.error().message();
        return world;
    }

    std::filesystem::path m_testDir;
    world::storage::SingleLevelStorageManager m_storage;
};

TEST_F(ServerWorldPersistenceTest, SaveAllPersistsRuntimeEntitiesAndBlockEntities)
{
    auto world = createWorld();

    ChunkData chunk(0, 0);
    ChunkSection* section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);
    section->setBlockStateId(0, 0, 0, 1);
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);
    auto saveChunkResult = m_storage.saveChunk(chunk, 0);
    ASSERT_TRUE(saveChunkResult.success()) << saveChunkResult.error().message();

    auto entity = std::make_unique<Entity>(0, world.get());
    entity->setTypeId("minecraft:unknown");
    entity->setPosition(1.0f, 64.0f, 1.0f);
    EntityId entityId = world->spawnEntity(std::move(entity));
    ASSERT_NE(entityId, 0);

    auto blockEntity = std::make_unique<CraftingTableEntity>(BlockPos(1, 64, 1));
    // 注意：不能在同一表达式里同时用 blockEntity->getPos() 和 blockEntity.release()。
    // 函数实参求值顺序未指定：若 release() 先求值，unique_ptr 内部裸指针被置空，
    // 随后 getPos() 在空指针上内联读取 m_pos（偏移 0x0C/0x10/0x14），在 m_pos.z
    // (偏移 0x14) 处触发 ACCESS_VIOLATION。先取 pos 再 release 以解耦求值顺序。
    const BlockPos blockEntityPos = blockEntity->getPos();
    world->setBlockEntity(blockEntityPos, blockEntity.release());

    auto saveAllResult = world->saveAll();
    ASSERT_TRUE(saveAllResult.success()) << saveAllResult.error().message();

    auto entityLoadResult = m_storage.entityStorage()->loadEntitiesInChunk(0, 0, 0, world.get());
    ASSERT_TRUE(entityLoadResult.success()) << entityLoadResult.error().message();
    EXPECT_EQ(entityLoadResult.value().size(), 1u);

    auto blockEntityLoadResult = m_storage.blockEntityStorage()->loadBlockEntitiesInChunk(0, 0, 0);
    ASSERT_TRUE(blockEntityLoadResult.success()) << blockEntityLoadResult.error().message();
    EXPECT_EQ(blockEntityLoadResult.value().size(), 1u);
}

TEST_F(ServerWorldPersistenceTest, ChunkUnloadPersistsMovedEntityToNewChunkWithoutLeavingStaleRecord)
{
    auto world = createWorld();

    auto entity = std::make_unique<Entity>(0, world.get());
    entity->setTypeId("minecraft:unknown");
    entity->setPosition(1.0f, 64.0f, 1.0f);
    EntityId entityId = world->spawnEntity(std::move(entity));
    ASSERT_NE(entityId, 0);

    world->tick();

    Entity* runtimeEntity = world->getEntity(entityId);
    ASSERT_NE(runtimeEntity, nullptr);
    runtimeEntity->setPosition(33.0f, 64.0f, 1.0f);

    world->tick();

    auto trackedChunk = world->entityChunkTracker().getEntityChunk(entityId);
    ASSERT_TRUE(trackedChunk.has_value());
    EXPECT_EQ(trackedChunk->first, 2);
    EXPECT_EQ(trackedChunk->second, 0);

    world->onChunkUnloading(2, 0);

    EXPECT_EQ(world->getEntity(entityId), nullptr);

    auto oldChunkResult = m_storage.entityStorage()->loadEntitiesInChunk(0, 0, 0, world.get());
    ASSERT_TRUE(oldChunkResult.success()) << oldChunkResult.error().message();
    EXPECT_TRUE(oldChunkResult.value().empty());

    auto newChunkResult = m_storage.entityStorage()->loadEntitiesInChunk(2, 0, 0, world.get());
    ASSERT_TRUE(newChunkResult.success()) << newChunkResult.error().message();
    ASSERT_EQ(newChunkResult.value().size(), 1u);
    EXPECT_NEAR(newChunkResult.value()[0]->x(), 33.0f, 0.001f);
}

} // namespace
} // namespace mc::server
