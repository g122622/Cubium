#include "common/TestWorldHelper.hpp"
#include "common/TempDirHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <filesystem>
#include <functional>
#include <gtest/gtest.h>

namespace mc::server {
namespace {

class ServerWorldPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // PID + 纳秒时间戳保证 CTest -j16 跨进程唯一，避免同秒 token 碰撞
        m_testDir = mc::test::makeUniqueTestDir("mc_server_world_persistence_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        // 初始化方块注册表（NoiseChunkGenerator 依赖）
        VanillaBlocks::initialize();

        // 注册内置方块实体类型工厂（Piston/Chest 等）。
        // 反序列化路径 BlockEntityStorageManager -> BlockEntityDeserializer ->
        // BlockEntityRegistry::create 依赖此注册，否则 create 返回 nullptr，
        // 警告 "Failed to create block entity of type 'minecraft:piston'"。
        // 与 SignEntityTest.cpp:46 同一模式。registerBuiltinTypes 幂等，重复调用无副作用。
        blockentity::BlockEntityRegistry::instance().registerBuiltinTypes();

        // 注册测试用实体类型 "minecraft:unknown"。
        // 反序列化器 EntityDeserializer::deserialize 通过 EntityRegistry::getType(typeId)
        // 查类型工厂，未注册则报 "Unknown entity type"。原版注册表（VanillaEntities）
        // 不会注册 "minecraft:unknown"，故测试需自行注册一个工厂创建裸 Entity。
        // 与 EntitySerializationTest.cpp:86-92 同一模式。hasType 守卫避免重复注册报错。
        if (!entity::EntityRegistry::instance().hasType("minecraft:unknown")) {
            auto registerResult = entity::EntityRegistry::instance().registerType("minecraft:unknown",
                entity::EntityType::Builder(
                    [](IWorld* world, ecs::EntityRegistry& registry) -> std::unique_ptr<Entity> { return std::make_unique<Entity>(0, world, mc::test::testEcsRegistry()); },
                    entity::EntityClassification::Misc)
                    .build());
            ASSERT_TRUE(registerResult.success()) << registerResult.error().message();
        }
    }

    void TearDown() override
    {
        m_storage.close();
        // RocksDB 后台线程可能延迟释放文件句柄，helper 内置 10 次重试覆盖句柄释放窗口
        mc::test::removeTestDir(m_testDir);
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
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
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

    auto entity = std::make_unique<Entity>(0, world.get(), mc::test::testEcsRegistry());
    entity->setTypeId("minecraft:unknown");
    entity->setPosition(1.0f, 64.0f, 1.0f);
    EntityInstanceId entityId = world->spawnEntity(std::move(entity));
    ASSERT_NE(entityId, 0);

    auto blockEntity = std::make_unique<blockentity::PistonBlockEntity>(BlockPos(1, 64, 1));
    // 注意：不能在同一表达式里同时用 blockEntity->getPos() 和 blockEntity.release()。
    // 函数实参求值顺序未指定：若 release() 先求值，unique_ptr 内部裸指针被置空，
    // 随后 getPos() 在空指针上内联读取 m_pos（偏移 0x0C/0x10/0x14），在 m_pos.z
    // (偏移 0x14) 处触发 ACCESS_VIOLATION。先取 pos 再 release 以解耦求值顺序。
    const BlockPos blockEntityPos = blockEntity->getPos();
    world->setBlockEntity(blockEntityPos, blockEntity.release());

    // ServerWorld::saveAll（曾委托共享存储跨维度全量保存的越权方法）已删除。
    // 这里改为显式走存储层的两条落盘路径，等价覆盖原 saveAll 对运行时实体/方块实体的持久化：
    //   ① 运行时实体：从 EntityManager 收集后调 EntityStorageManager::saveAllEntities。
    //      （原 ServerWorld::_collectLoadedEntitiesForSave 即是 m_entityManager.forEachEntity 收集。）
    //   ② 方块实体：SingleLevelStorageManager::saveChunk 在落盘 section 后会附带逐个
    //      saveBlockEntity（见 SingleLevelStorageManager.cpp saveChunk 实现），故对内存中
    //      携带该方块实体的 chunk 再 saveChunk 一次即可落盘方块实体。
    std::vector<std::reference_wrapper<Entity>> entitiesToSave;
    world->entityManager().forEachEntity([&entitiesToSave](Entity* e) {
        if (e != nullptr) {
            entitiesToSave.emplace_back(*e);
        }
        return true;
    });
    auto entitySaveResult = m_storage.entityStorage()->saveAllEntities(entitiesToSave, 0);
    ASSERT_TRUE(entitySaveResult.success()) << entitySaveResult.error().message();

    const ChunkData* persistedChunk = world->getChunk(0, 0);
    ASSERT_NE(persistedChunk, nullptr);
    auto blockEntitySaveResult = m_storage.saveChunk(*persistedChunk, 0);
    ASSERT_TRUE(blockEntitySaveResult.success()) << blockEntitySaveResult.error().message();

    auto entityLoadResult = m_storage.entityStorage()->loadEntitiesInChunk(0, 0, 0, mc::test::testEcsRegistry());
    ASSERT_TRUE(entityLoadResult.success()) << entityLoadResult.error().message();
    EXPECT_EQ(entityLoadResult.value().size(), 1u);

    auto blockEntityLoadResult = m_storage.blockEntityStorage()->loadBlockEntitiesInChunk(0, 0, 0);
    ASSERT_TRUE(blockEntityLoadResult.success()) << blockEntityLoadResult.error().message();
    EXPECT_EQ(blockEntityLoadResult.value().size(), 1u);
}

TEST_F(ServerWorldPersistenceTest, ChunkUnloadPersistsMovedEntityToNewChunkWithoutLeavingStaleRecord)
{
    auto world = createWorld();

    auto entity = std::make_unique<Entity>(0, world.get(), mc::test::testEcsRegistry());
    entity->setTypeId("minecraft:unknown");
    entity->setPosition(1.0f, 64.0f, 1.0f);
    EntityInstanceId entityId = world->spawnEntity(std::move(entity));
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

    auto oldChunkResult = m_storage.entityStorage()->loadEntitiesInChunk(0, 0, 0, mc::test::testEcsRegistry());
    ASSERT_TRUE(oldChunkResult.success()) << oldChunkResult.error().message();
    EXPECT_TRUE(oldChunkResult.value().empty());

    auto newChunkResult = m_storage.entityStorage()->loadEntitiesInChunk(2, 0, 0, mc::test::testEcsRegistry());
    ASSERT_TRUE(newChunkResult.success()) << newChunkResult.error().message();
    ASSERT_EQ(newChunkResult.value().size(), 1u);
    EXPECT_NEAR(newChunkResult.value()[0]->x(), 33.0f, 0.001f);
}

} // namespace
} // namespace mc::server
