#include "server/world/storage/SingleLevelStorageManager.hpp"
#include "common/TempDirHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Types.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/storage/db/SectionKey.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <optional>
#include <thread>
#include <gtest/gtest.h>

namespace mc::world::storage {
namespace {

namespace TestBiomes {
constexpr BiomeId PLAINS = 1;
}

/// 测试用方块状态的基准值。
///
/// 【必须避开 0/1/2】方块状态 0/1/2 分别是空气、洞穴空气、虚空空气，**它们本身就是空气**。
/// 用它们当"已放置的方块"写进区块，该段会被序列化判定为空段而丢弃方块数据；读回来自然
/// 还是空气。旧实现读路径命中 SectionCache、拿到的是内存里的原对象而绕过了序列化，
/// 于是这类测试长期"通过"。段缓存移除后读路径必须真正走一遍序列化/反序列化，
/// 测试数据必须落在确定非空气的范围内才能表达"这里有方块"。
constexpr u32 NON_AIR_STATE_BASE = 100;

class StorageTestBase : public ::testing::Test {
protected:
    std::filesystem::path testDir;

    void SetUp() override
    {
        // 复用 mc::test 共享 helper：token 含 PID + steady_clock 纳秒 + 线程 ID + 计数器，
        // 跨进程 / 跨线程 / 同 tick 都唯一，根治 CTest -j16 并行下秒级时间戳撞目录的竞态。
        // 前缀带上用例名便于人工排查目录归属。
        const ::testing::TestInfo* const info = ::testing::UnitTest::GetInstance()->current_test_info();
        const std::string prefix = "mc_storage_test/" + std::string(info->test_suite_name()) + "_" + info->name();
        testDir = mc::test::makeUniqueTestDir(prefix);
    }

    void TearDown() override { mc::test::removeTestDir(testDir); }
};

class SingleLevelStorageManagerTest : public StorageTestBase {};

void saveChunkSections(
    SingleLevelStorageManager& storage, const ChunkData& chunk, DimensionId dimension, bool sync = false)
{
    const std::array<const ChunkData*, 1> chunks{&chunk};
    auto result = storage.saveChunks(dimension, chunks, sync);
    ASSERT_TRUE(result.success()) << result.error().message();
}

size_t saveChunkSectionsForCount(SingleLevelStorageManager& storage, const ChunkData& chunk, DimensionId dimension)
{
    const std::array<const ChunkData*, 1> chunks{&chunk};
    auto result = storage.saveChunks(dimension, chunks, false);
    EXPECT_TRUE(result.success()) << result.error().message();
    return result.success() ? result.value() : 0;
}

TEST_F(SingleLevelStorageManagerTest, OpenClose)
{
    SingleLevelStorageManager storage;

    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success()) << result.error().message();

    EXPECT_TRUE(storage.isOpen());

    storage.close();
    EXPECT_FALSE(storage.isOpen());
}

TEST_F(SingleLevelStorageManagerTest, SaveAndLoadChunk)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    ChunkData chunk(0, 0);
    ChunkSection* section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);
    section->setBlockStateId(0, 0, 0, 42);
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);

    auto saveResult = storage.saveChunk(chunk, 0);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    auto loadResult = storage.loadChunk(0, 0, 0);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().message();
    ASSERT_TRUE(loadResult.value().has_value());

    const ChunkData& loadedChunk = loadResult.value().value();
    const ChunkSection* loadedSection = loadedChunk.getSection(0);
    ASSERT_NE(loadedSection, nullptr);
    EXPECT_EQ(loadedSection->getBlockStateId(0, 0, 0), 42u);

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, AsyncLoadPreparesHeightmapsBeforeCallback)
{
    VanillaBlocks::initialize();
    util::UniversalWorkerPool ioPool(1, "HeightmapTestIO", 900);
    util::UniversalWorkerPool computePool(1, "HeightmapTestCompute", 910);
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    ASSERT_TRUE(storage.open(testDir, config).success());
    storage.setIoWorkerPool(&ioPool);
    storage.setComputeWorkerPool(&computePool);

    ChunkData chunk(-2, 3);
    const i32 sectionIndex = world::toSectionIndex(64);
    chunk.createSection(sectionIndex)->setBlockState(2, 5, 7, &VanillaBlocks::STONE->defaultState());
    ASSERT_TRUE(storage.saveChunk(chunk, 0).success());
    ioPool.start();
    computePool.start();

    std::promise<Result<std::optional<ChunkData>>> promise;
    auto future = promise.get_future();
    std::thread::id callbackThread;
    storage.loadChunkAsyncCallback(
        -2,
        3,
        0,
        [&promise, &callbackThread](ChunkCoord, ChunkCoord, DimensionId, Result<std::optional<ChunkData>> result) {
            callbackThread = std::this_thread::get_id();
            promise.set_value(std::move(result));
        },
        nullptr,
        util::TaskPriority::Normal);
    const auto status = future.wait_for(std::chrono::seconds(10));
    ioPool.shutdown();
    computePool.shutdown();
    ASSERT_EQ(status, std::future_status::ready);
    EXPECT_NE(callbackThread, std::this_thread::get_id());
    auto result = future.get();
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());
    for (size_t i = 0; i < mc::world::chunk::HEIGHTMAP_TYPE_COUNT; ++i) {
        const auto type = static_cast<HeightmapType>(i);
        EXPECT_TRUE(result.value()->isHeightmapInitialized(type));
        EXPECT_EQ(result.value()->getHeightmapFirstAvailable(type, 2, 7), world::sectionToY(sectionIndex) + 6);
        EXPECT_EQ(result.value()->getHeightmapFirstAvailable(type, 7, 2), Heightmap::NO_BLOCK_SENTINEL);
    }

    // 运行时包装复用加载阶段准备好的高度值，不覆盖或丢失存档结果。
    mc::world::chunk::ChunkPrimer primer(std::make_unique<ChunkData>(std::move(*result.value())));
    EXPECT_EQ(
        primer.getHeightmapFirstAvailable(HeightmapType::WorldSurface, 2, 7), world::sectionToY(sectionIndex) + 6);
    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, AsyncLoadAfterSaveReadsSavedDataWithSingleIoThread)
{
    VanillaBlocks::initialize();
    // 单线程 IO 池：保存与加载共用，若加载在池线程内阻塞等待保存会直接死锁
    util::UniversalWorkerPool ioPool(1, "SaveThenLoadIO", 911);
    util::UniversalWorkerPool computePool(1, "SaveThenLoadCompute", 912);
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    ASSERT_TRUE(storage.open(testDir, config).success());
    storage.setIoWorkerPool(&ioPool);
    storage.setComputeWorkerPool(&computePool);
    ioPool.start();
    computePool.start();

    auto chunk = std::make_shared<ChunkData>(5, -6);
    const i32 sectionIndex = world::toSectionIndex(64);
    chunk->createSection(sectionIndex)->setBlockState(2, 5, 7, &VanillaBlocks::STONE->defaultState());
    chunk->setLoaded(true);
    chunk->setFullyGenerated(true);

    // 保存尚未完成就立刻发起同区块的异步加载
    std::promise<bool> saveDone;
    auto saveFuture = saveDone.get_future();
    storage.saveChunkAsyncCallback(
        chunk, 0, [&saveDone](std::optional<ChunkPos>, Result<void> result) { saveDone.set_value(result.success()); });

    std::promise<Result<std::optional<ChunkData>>> loadDone;
    auto loadFuture = loadDone.get_future();
    storage.loadChunkAsyncCallback(
        5,
        -6,
        0,
        [&loadDone](ChunkCoord, ChunkCoord, DimensionId, Result<std::optional<ChunkData>> result) {
            loadDone.set_value(std::move(result));
        },
        nullptr,
        util::TaskPriority::Normal);

    ASSERT_EQ(saveFuture.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    EXPECT_TRUE(saveFuture.get());
    ASSERT_EQ(loadFuture.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    auto result = loadFuture.get();
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());
    const ChunkSection* loaded = result.value()->getSection(sectionIndex);
    ASSERT_NE(loaded, nullptr);
    const BlockState* blockState = loaded->getBlockState(2, 5, 7);
    ASSERT_NE(blockState, nullptr);
    EXPECT_EQ(blockState->blockId(), VanillaBlocks::STONE->defaultState().blockId());

    ioPool.shutdown();
    computePool.shutdown();
    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, RejectedSaveCallsCompletionExactlyOnce)
{
    util::UniversalWorkerPool ioPool(1, "RejectedSaveIO", 919);
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    ASSERT_TRUE(storage.open(testDir, config).success());
    storage.setIoWorkerPool(&ioPool);
    auto chunk = std::make_shared<ChunkData>(3, 4);
    chunk->createSection(0)->fill(100);
    i32 completions = 0;
    storage.saveChunkAsyncCallback(
        chunk,
        0,
        [&completions](std::optional<ChunkPos> position, Result<void> result) {
            ++completions;
            EXPECT_TRUE(position.has_value());
            EXPECT_TRUE(result.failed());
        },
        util::TaskPriority::Normal);
    EXPECT_EQ(completions, 1);
    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, CloseWaitsForPinnedReadAssemblyBeforeDestroyingDatabase)
{
    VanillaBlocks::initialize();
    util::UniversalWorkerPool ioPool(1, "PinnedCloseIO", 920);
    util::UniversalWorkerPool computePool(1, "PinnedCloseCompute", 921);
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    ASSERT_TRUE(storage.open(testDir, config).success());
    storage.setIoWorkerPool(&ioPool);
    storage.setComputeWorkerPool(&computePool);
    ChunkData source(3, 4);
    source.createSection(0)->fill(100);
    ASSERT_TRUE(storage.saveChunk(source, 0).success());
    ioPool.start();
    computePool.start();

    std::promise<void> releaseCompute;
    auto gate = releaseCompute.get_future().share();
    auto blocker = std::make_unique<util::FunctionTask>(
        util::TaskType::ChunkLoad,
        "PinnedCloseGate",
        [gate](const std::atomic<bool>&) {
            gate.wait();
            return true;
        },
        "storage.test");
    computePool.submit(std::move(blocker), nullptr, util::TaskPriority::High, nullptr);
    auto loaded = storage.loadChunkAsync(3, 4, 0, nullptr, util::TaskPriority::Normal);
    ioPool.waitForCompletion();
    auto closing = std::async(std::launch::async, [&storage] { storage.close(); });
    EXPECT_EQ(closing.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
    releaseCompute.set_value();
    ASSERT_EQ(closing.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    closing.get();
    ASSERT_EQ(loaded.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    auto result = loaded.get();
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());
    EXPECT_EQ(result.value()->getSection(0)->getBlockStateIdFast(0), 100u);
    ioPool.shutdown();
    computePool.shutdown();
}

TEST_F(SingleLevelStorageManagerTest, SynchronousLoadPreparesHeightmaps)
{
    VanillaBlocks::initialize();
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    ASSERT_TRUE(storage.open(testDir, config).success());
    ChunkData chunk(0, 0);
    chunk.createSection(0)->setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    ASSERT_TRUE(storage.saveChunk(chunk, 0).success());
    auto result = storage.loadChunk(0, 0, 0);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());
    EXPECT_TRUE(result.value()->isHeightmapInitialized(HeightmapType::OceanFloor));
    EXPECT_EQ(result.value()->getHeightmapFirstAvailable(HeightmapType::OceanFloor, 0, 0), world::MIN_BUILD_HEIGHT + 1);
    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, MultipleSections)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    for (i32 cx = -1; cx <= 1; ++cx) {
        for (i32 cz = -1; cz <= 1; ++cz) {
            ChunkData chunk(cx, cz);
            ChunkSection* section = chunk.createSection(0);
            ASSERT_NE(section, nullptr);
            u32 expectedBlockId = static_cast<u32>((cx + 2) * 100 + (cz + 2) * 10);
            section->setBlockStateId(0, 0, 0, expectedBlockId);
            chunk.setLoaded(true);
            chunk.setFullyGenerated(true);
            chunk.setDirty(false);
            ASSERT_TRUE(storage.saveChunk(chunk, 0).success());
        }
    }

    for (i32 cx = -1; cx <= 1; ++cx) {
        for (i32 cz = -1; cz <= 1; ++cz) {
            auto loadResult = storage.loadChunk(cx, cz, 0);
            ASSERT_TRUE(loadResult.success());
            ASSERT_TRUE(loadResult.value().has_value());
            const ChunkSection* section = loadResult.value().value().getSection(0);
            ASSERT_NE(section, nullptr);
            u32 expectedBlockId = static_cast<u32>((cx + 2) * 100 + (cz + 2) * 10);
            EXPECT_EQ(section->getBlockStateId(0, 0, 0), expectedBlockId);
        }
    }

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, SaveChunkPersistsEverySectionImmediately)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Strong;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    for (int i = 0; i < 10; ++i) {
        ChunkData chunk(i, 0);
        ChunkSection* section = chunk.createSection(0);
        ASSERT_NE(section, nullptr);
        section->setBlockStateId(0, 0, 0, NON_AIR_STATE_BASE + static_cast<u32>(i));
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 0).success());
    }

    // 存储层不再缓存段数据，saveChunk 返回即已落盘：无需再 flush 一次。
    for (int i = 0; i < 10; ++i) {
        auto loadResult = storage.loadChunk(i, 0, 0);
        ASSERT_TRUE(loadResult.success()) << loadResult.error().message();
        ASSERT_TRUE(loadResult.value().has_value());
        const ChunkSection* section = loadResult.value().value().getSection(0);
        ASSERT_NE(section, nullptr);
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), NON_AIR_STATE_BASE + static_cast<u32>(i));
    }

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, FlushPlayerDataIsNoOpWithoutDirtyPlayers)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Strong;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto flushResult = storage.flushPlayerData();
    ASSERT_TRUE(flushResult.success()) << flushResult.error().message();
    EXPECT_EQ(flushResult.value(), 0u);

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, DifferentDimensions)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    {
        ChunkData chunk(0, 0);
        ChunkSection* section = chunk.createSection(0);
        ASSERT_NE(section, nullptr);
        section->setBlockStateId(0, 0, 0, NON_AIR_STATE_BASE + 1);
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 0).success());
    }

    {
        ChunkData chunk(0, 0);
        ChunkSection* section = chunk.createSection(0);
        ASSERT_NE(section, nullptr);
        section->setBlockStateId(0, 0, 0, NON_AIR_STATE_BASE + 2);
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 1).success());
    }

    {
        ChunkData chunk(0, 0);
        ChunkSection* section = chunk.createSection(0);
        ASSERT_NE(section, nullptr);
        section->setBlockStateId(0, 0, 0, NON_AIR_STATE_BASE + 3);
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 2).success());
    }

    {
        auto loadResult = storage.loadChunk(0, 0, 0);
        ASSERT_TRUE(loadResult.success());
        ASSERT_TRUE(loadResult.value().has_value());
        const ChunkSection* section = loadResult.value().value().getSection(0);
        ASSERT_NE(section, nullptr);
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), NON_AIR_STATE_BASE + 1);
    }

    {
        auto loadResult = storage.loadChunk(0, 0, 1);
        ASSERT_TRUE(loadResult.success());
        ASSERT_TRUE(loadResult.value().has_value());
        const ChunkSection* section = loadResult.value().value().getSection(0);
        ASSERT_NE(section, nullptr);
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), NON_AIR_STATE_BASE + 2);
    }

    {
        auto loadResult = storage.loadChunk(0, 0, 2);
        ASSERT_TRUE(loadResult.success());
        ASSERT_TRUE(loadResult.value().has_value());
        const ChunkSection* section = loadResult.value().value().getSection(0);
        ASSERT_NE(section, nullptr);
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), NON_AIR_STATE_BASE + 3);
    }

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, ReopenPreservesData)
{
    {
        SingleLevelStorageManager storage;
        SingleLevelStorageConfig config;
        config.consistencyMode = ConsistencyMode::Strong;

        auto result = storage.open(testDir, config);
        ASSERT_TRUE(result.success());

        ChunkData chunk(10, 20);
        ChunkSection* section = chunk.createSection(5);
        ASSERT_NE(section, nullptr);
        section->setBlockStateId(0, 0, 0, 999);
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 0).success());

        storage.close();
    }

    {
        SingleLevelStorageManager storage;
        SingleLevelStorageConfig config;

        auto result = storage.open(testDir, config);
        ASSERT_TRUE(result.success());

        auto loadResult = storage.loadChunk(10, 20, 0);
        ASSERT_TRUE(loadResult.success());
        ASSERT_TRUE(loadResult.value().has_value());
        const ChunkData& chunk = loadResult.value().value();
        const ChunkSection* section = chunk.getSection(5);
        ASSERT_NE(section, nullptr);
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), 999u);
        EXPECT_EQ(chunk.x(), 10);
        EXPECT_EQ(chunk.z(), 20);

        storage.close();
    }
}

TEST_F(SingleLevelStorageManagerTest, SaveSectionsBatchPreservesOverwrittenSectionSnapshot)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    ChunkData chunk(30, 40);
    ChunkSection* section = chunk.createSection(6);
    ASSERT_NE(section, nullptr);
    section->setBlockStateId(0, 0, 0, 123);
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);

    auto saveResult = storage.saveChunk(chunk, 0);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    auto loadResult = storage.loadChunk(30, 40, 0);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().message();
    ASSERT_TRUE(loadResult.value().has_value());

    const ChunkData& initialChunk = loadResult.value().value();
    const ChunkSection* initialSection = initialChunk.getSection(6);
    ASSERT_NE(initialSection, nullptr);
    EXPECT_EQ(initialSection->getBlockStateId(0, 0, 0), 123u);

    ChunkData updatedChunk(30, 40);
    ChunkSection* updatedSection = updatedChunk.createSection(6);
    ASSERT_NE(updatedSection, nullptr);
    updatedSection->setBlockStateId(0, 0, 0, 777);
    updatedChunk.setLoaded(true);
    updatedChunk.setFullyGenerated(true);
    updatedChunk.setDirty(false);
    auto overwriteResult = storage.saveChunk(updatedChunk, 0);
    ASSERT_TRUE(overwriteResult.success()) << overwriteResult.error().message();

    // 覆盖写：同一段被写入两次，第二次必须真正生效（批内不得残留上一版数据）。
    saveChunkSections(storage, updatedChunk, 0);

    storage.close();

    {
        SingleLevelStorageManager reopenedStorage;
        auto reopenResult = reopenedStorage.open(testDir, config);
        ASSERT_TRUE(reopenResult.success()) << reopenResult.error().message();

        auto reopenedLoadResult = reopenedStorage.loadChunk(30, 40, 0);
        ASSERT_TRUE(reopenedLoadResult.success()) << reopenedLoadResult.error().message();
        ASSERT_TRUE(reopenedLoadResult.value().has_value());
        const ChunkSection* reopenedSection = reopenedLoadResult.value().value().getSection(6);
        ASSERT_NE(reopenedSection, nullptr);
        EXPECT_EQ(reopenedSection->getBlockStateId(0, 0, 0), 777u);

        reopenedStorage.close();
    }
}

TEST_F(SingleLevelStorageManagerTest, SaveSectionsBatchPersistsOverwrittenSectionSnapshotWithSync)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    ChunkData chunk(50, 60);
    ChunkSection* section = chunk.createSection(7);
    ASSERT_NE(section, nullptr);
    section->setBlockStateId(0, 0, 0, 456);
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);
    chunk.setDirty(false);
    auto saveResult = storage.saveChunk(chunk, 0);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    auto loadResult = storage.loadChunk(50, 60, 0);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().message();
    ASSERT_TRUE(loadResult.value().has_value());
    const ChunkSection* initialSection = loadResult.value().value().getSection(7);
    ASSERT_NE(initialSection, nullptr);
    EXPECT_EQ(initialSection->getBlockStateId(0, 0, 0), 456u);

    ChunkData updatedChunk(50, 60);
    ChunkSection* updatedSection = updatedChunk.createSection(7);
    ASSERT_NE(updatedSection, nullptr);
    updatedSection->setBlockStateId(0, 0, 0, 888);
    updatedChunk.setLoaded(true);
    updatedChunk.setFullyGenerated(true);
    updatedChunk.setDirty(false);
    saveChunkSections(storage, updatedChunk, 0, /*sync=*/true);

    storage.close();

    {
        SingleLevelStorageManager reopenedStorage;
        auto reopenResult = reopenedStorage.open(testDir, config);
        ASSERT_TRUE(reopenResult.success()) << reopenResult.error().message();

        auto reopenedLoadResult = reopenedStorage.loadChunk(50, 60, 0);
        ASSERT_TRUE(reopenedLoadResult.success()) << reopenedLoadResult.error().message();
        ASSERT_TRUE(reopenedLoadResult.value().has_value());
        const ChunkSection* reopenedSection = reopenedLoadResult.value().value().getSection(7);
        ASSERT_NE(reopenedSection, nullptr);
        EXPECT_EQ(reopenedSection->getBlockStateId(0, 0, 0), 888u);

        reopenedStorage.close();
    }
}

/**
 * 批量落盘把整批段聚合成**一个** WriteBatch 提交（逐段 put 等于逐段等一次 WAL fsync，
 * 视野距离 16 下 2048 个段实测约 6.6 秒）。聚合写入最容易出的错是"聚合过程中漏条目"，
 * 它不会报错、只会静默少写数据，因此这里覆盖：
 * - 跨维度、多条区块列一次提交，返回条数必须精确等于写入的段数
 * - 同一批段连落两次，第二次条数必须不变（数据源是调用方传入的段列表，不是会被写空的缓存）
 * - 重开后逐段校验内容，确保条数对只是巧合的可能性被排除
 */
TEST_F(SingleLevelStorageManagerTest, SaveSectionsBatchPersistsEverySectionAcrossDimensions)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;

    auto openResult = storage.open(testDir, config);
    ASSERT_TRUE(openResult.success()) << openResult.error().message();

    constexpr i32 CHUNK_COUNT = 12;
    constexpr i8 SECTION_Y = 3;
    constexpr u32 FILL_BASE = 1000;
    const size_t expectedSections = static_cast<size_t>(CHUNK_COUNT) * 2;

    auto writeAll = [&storage]() {
        size_t written = 0;
        for (const DimensionId dimension : {DimensionId(0), DimensionId(1)}) {
            for (i32 i = 0; i < CHUNK_COUNT; ++i) {
                ChunkData chunk(i, 7);
                ChunkSection* section = chunk.createSection(SECTION_Y);
                EXPECT_NE(section, nullptr);
                section->setBlockStateId(0, 0, 0, FILL_BASE + static_cast<u32>(i));
                chunk.setLoaded(true);
                chunk.setFullyGenerated(true);
                chunk.setDirty(false);
                written += saveChunkSectionsForCount(storage, chunk, dimension);
            }
        }
        return written;
    };

    EXPECT_EQ(writeAll(), expectedSections);
    EXPECT_EQ(writeAll(), expectedSections) << "批量落盘的返回条数不应随上一次写入而变化";

    storage.close();

    SingleLevelStorageManager reopened;
    auto reopenResult = reopened.open(testDir, config);
    ASSERT_TRUE(reopenResult.success()) << reopenResult.error().message();

    for (const DimensionId dimension : {DimensionId(0), DimensionId(1)}) {
        for (i32 i = 0; i < CHUNK_COUNT; ++i) {
            auto loadResult = reopened.loadChunk(i, 7, dimension);
            ASSERT_TRUE(loadResult.success()) << loadResult.error().message();
            ASSERT_TRUE(loadResult.value().has_value()) << "chunk (" << i << ", 7) 在维度 " << dimension << " 未落盘";

            const ChunkSection* reopenedSection = loadResult.value().value().getSection(SECTION_Y);
            ASSERT_NE(reopenedSection, nullptr);
            EXPECT_EQ(reopenedSection->getBlockStateId(0, 0, 0), FILL_BASE + static_cast<u32>(i));
        }
    }

    reopened.close();
}

TEST_F(SingleLevelStorageManagerTest, InvalidSectionKey)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto loadResult = storage.loadChunk(99999, 99999, 0);

    EXPECT_TRUE(loadResult.success());
    EXPECT_FALSE(loadResult.value().has_value());

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, ReadonlySaveOperationsAreSilentAndDoNotPersist)
{
    {
        SingleLevelStorageManager bootstrap;
        SingleLevelStorageConfig bootstrapConfig;
        auto bootstrapResult = bootstrap.open(testDir, bootstrapConfig);
        ASSERT_TRUE(bootstrapResult.success());
        bootstrap.close();
    }

    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.readonly = true;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success()) << result.error().message();

    ChunkData chunk(1, 2);
    ChunkSection* section = chunk.createSection(0);
    ASSERT_NE(section, nullptr);
    section->setBlockStateId(0, 0, 0, NON_AIR_STATE_BASE + 55);
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);

    auto saveResult = storage.saveChunk(chunk, 0);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    // 只读模式下批量落盘与玩家数据刷盘都必须静默成功且不写入任何数据。
    const std::array<const ChunkData*, 1> readonlyChunks{&chunk};
    auto readonlySaveResult = storage.saveChunks(0, readonlyChunks, true);
    ASSERT_TRUE(readonlySaveResult.success()) << readonlySaveResult.error().message();
    EXPECT_EQ(readonlySaveResult.value(), 0u);

    auto flushResult = storage.flushPlayerData();
    ASSERT_TRUE(flushResult.success()) << flushResult.error().message();
    EXPECT_EQ(flushResult.value(), 0u);

    storage.close();

    SingleLevelStorageManager reopened;
    SingleLevelStorageConfig reopenConfig;
    auto reopenResult = reopened.open(testDir, reopenConfig);
    ASSERT_TRUE(reopenResult.success());
    auto loadResult = reopened.loadChunk(1, 2, 0);
    ASSERT_TRUE(loadResult.success());
    EXPECT_FALSE(loadResult.value().has_value());
    reopened.close();
}

TEST_F(SingleLevelStorageManagerTest, SaveChunkAsyncCallbackReportsUnknownPositionForNullChunk)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success()) << result.error().message();

    // 空区块没有真实坐标：回调必须回传空坐标而不是伪造的 (0, 0)
    bool called = false;
    std::optional<ChunkPos> reportedPos = ChunkPos(1, 1);
    storage.saveChunkAsyncCallback(nullptr, 0, [&](std::optional<ChunkPos> pos, Result<void> saveResult) {
        called = true;
        reportedPos = pos;
        ASSERT_TRUE(saveResult.failed());
        EXPECT_EQ(saveResult.error().code(), ErrorCode::InvalidArgument);
    });

    EXPECT_TRUE(called);
    EXPECT_FALSE(reportedPos.has_value());
    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, SaveChunkAsyncCallbackReportsChunkPosition)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success()) << result.error().message();

    auto chunk = std::make_shared<ChunkData>(3, -4);
    bool called = false;
    std::optional<ChunkPos> reportedPos;
    storage.saveChunkAsyncCallback(chunk, 0, [&](std::optional<ChunkPos> pos, Result<void> saveResult) {
        called = true;
        reportedPos = pos;
        EXPECT_TRUE(saveResult.success()) << saveResult.error().message();
    });

    EXPECT_TRUE(called);
    if (!reportedPos.has_value()) {
        ADD_FAILURE() << "save callback did not report a position";
        storage.close();
        return;
    }
    EXPECT_EQ(reportedPos->x, 3);
    EXPECT_EQ(reportedPos->z, -4);
    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, CloseDoesNotPersistLevelDataWithoutExplicitSave)
{
    {
        SingleLevelStorageManager storage;
        SingleLevelStorageConfig config;

        auto result = storage.open(testDir, config);
        ASSERT_TRUE(result.success()) << result.error().message();

        CreateWorldRequest request("test-world",
            "test-world",
            12345,
            WorldType::Default,
            resource::ResourceLocation("minecraft", "default"),
            GameMode::Survival,
            Difficulty::Normal,
            false,
            false,
            10);
        auto writeInitialResult = LevelDatCodec::writeInitial(testDir, request);
        ASSERT_TRUE(writeInitialResult.success()) << writeInitialResult.error().message();

        storage.close();
    }

    {
        SingleLevelStorageManager reopened;
        SingleLevelStorageConfig config;

        auto reopenResult = reopened.open(testDir, config);
        ASSERT_TRUE(reopenResult.success()) << reopenResult.error().message();

        auto levelDataResult = reopened.loadLevelData();
        ASSERT_TRUE(levelDataResult.success()) << levelDataResult.error().message();
        const auto& levelData = levelDataResult.value();
        EXPECT_EQ(levelData.gameTime, 0);
        EXPECT_EQ(levelData.dayTime, 0);
        EXPECT_EQ(levelData.spawnData.pos(), BlockPos(0, 0, 0));
        // writeInitial 写入未初始化标志：新世界首次启动应重算出生点
        EXPECT_FALSE(levelData.initialized);

        reopened.close();
    }
}

TEST_F(SingleLevelStorageManagerTest, ReadonlySaveLevelDataDoesNotPersist)
{
    {
        SingleLevelStorageManager bootstrap;
        SingleLevelStorageConfig config;
        auto openResult = bootstrap.open(testDir, config);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        CreateWorldRequest request("test-world",
            "test-world",
            12345,
            WorldType::Default,
            resource::ResourceLocation("minecraft", "default"),
            GameMode::Survival,
            Difficulty::Normal,
            false,
            false,
            10);
        auto writeInitialResult = LevelDatCodec::writeInitial(testDir, request);
        ASSERT_TRUE(writeInitialResult.success()) << writeInitialResult.error().message();

        auto saveResult = bootstrap.saveLevelData(10,
            20,
            RespawnData::of(0, BlockPos(1, 64, 2), 30.0f, 0.0f),
            40,
            50,
            true,
            60,
            false,
            true,
            Difficulty::Normal,
            false);
        ASSERT_TRUE(saveResult.success()) << saveResult.error().message();
        bootstrap.close();
    }

    {
        SingleLevelStorageManager readonlyStorage;
        SingleLevelStorageConfig readonlyConfig;
        readonlyConfig.readonly = true;

        auto openResult = readonlyStorage.open(testDir, readonlyConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        auto saveResult = readonlyStorage.saveLevelData(999,
            888,
            RespawnData::of(0, BlockPos(10, 70, 11), 12.0f, 0.0f),
            13,
            14,
            false,
            15,
            true,
            false,
            Difficulty::Normal,
            false);
        ASSERT_TRUE(saveResult.success()) << saveResult.error().message();
        readonlyStorage.close();
    }

    {
        SingleLevelStorageManager reopened;
        SingleLevelStorageConfig config;

        auto openResult = reopened.open(testDir, config);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        auto levelDataResult = reopened.loadLevelData();
        ASSERT_TRUE(levelDataResult.success()) << levelDataResult.error().message();
        const auto& levelData = levelDataResult.value();
        EXPECT_EQ(levelData.gameTime, 10);
        EXPECT_EQ(levelData.dayTime, 20);
        EXPECT_EQ(levelData.spawnData.pos(), BlockPos(1, 64, 2));
        EXPECT_FLOAT_EQ(levelData.spawnData.yaw, 30.0f);
        EXPECT_EQ(levelData.rainTime, 50);
        EXPECT_TRUE(levelData.raining);
        EXPECT_EQ(levelData.thunderTime, 60);
        EXPECT_FALSE(levelData.thundering);
        // 第一次 saveLevelData 传 initialized=true 已落盘；只读模式的第二次调用不应覆盖
        EXPECT_TRUE(levelData.initialized);

        reopened.close();
    }
}

// 验证 saveLevelData 的 initialized 参数能正确落盘并被 loadLevelData 读回。
// 覆盖 initialized 字段在 true/false 两种取值下的往返一致性。
TEST_F(SingleLevelStorageManagerTest, SaveLevelData_PersistsInitializedFlag)
{
    auto writeInitial = [&] {
        SingleLevelStorageManager storage;
        SingleLevelStorageConfig config;
        auto openResult = storage.open(testDir, config);
        EXPECT_TRUE(openResult.success()) << openResult.error().message();

        CreateWorldRequest request("test-world",
            "test-world",
            12345,
            WorldType::Default,
            resource::ResourceLocation("minecraft", "default"),
            GameMode::Survival,
            Difficulty::Normal,
            false,
            false,
            10);
        auto result = LevelDatCodec::writeInitial(testDir, request);
        EXPECT_TRUE(result.success()) << result.error().message();
        storage.close();
    };

    auto saveInitialized = [&](bool initialized) {
        SingleLevelStorageManager storage;
        SingleLevelStorageConfig config;
        auto openResult = storage.open(testDir, config);
        EXPECT_TRUE(openResult.success()) << openResult.error().message();
        auto saveResult =
            storage.saveLevelData(0, 0, RespawnData{}, 0, 0, false, 0, false, initialized, Difficulty::Normal, false);
        EXPECT_TRUE(saveResult.success()) << saveResult.error().message();
        storage.close();
    };

    auto loadInitialized = [&]() -> bool {
        SingleLevelStorageManager storage;
        SingleLevelStorageConfig config;
        auto openResult = storage.open(testDir, config);
        EXPECT_TRUE(openResult.success()) << openResult.error().message();
        auto levelDataResult = storage.loadLevelData();
        EXPECT_TRUE(levelDataResult.success()) << levelDataResult.error().message();
        storage.close();
        return levelDataResult.value().initialized;
    };

    // 新世界模板：initialized=false
    writeInitial();
    EXPECT_FALSE(loadInitialized());

    // 落盘 initialized=true：读回应为 true
    saveInitialized(true);
    EXPECT_TRUE(loadInitialized());

    // 再落盘 initialized=false：读回应为 false
    saveInitialized(false);
    EXPECT_FALSE(loadInitialized());
}

} // namespace
} // namespace mc::world::storage
