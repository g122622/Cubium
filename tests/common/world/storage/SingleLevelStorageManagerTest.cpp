#include "world/storage/SingleLevelStorageManager.hpp"
#include "core/Types.hpp"
#include "world/storage/db/SectionKey.hpp"
#include <ctime>
#include <filesystem>
#include <gtest/gtest.h>

namespace mc::world::storage {
namespace {

namespace TestBiomes {
constexpr BiomeId PLAINS = 1;
}

class StorageTestBase : public ::testing::Test {
protected:
    std::filesystem::path testDir;

    void SetUp() override
    {
        testDir = std::filesystem::temp_directory_path() / "mc_storage_test" / std::to_string(std::time(nullptr));
        std::filesystem::create_directories(testDir);
    }

    void TearDown() override
    {
        if (std::filesystem::exists(testDir)) {
            std::error_code ec;
            std::filesystem::remove_all(testDir, ec);
        }
    }

    SectionData createTestSectionData(ChunkCoord x, ChunkCoord z, i8 sectionY, DimensionId dim, u32 fillBlockId = 1)
    {
        SectionData data;
        data.key = SectionKey(x, z, sectionY, dim);
        data.initializeDefaults();

        for (size_t i = 0; i < 100; ++i) {
            data.blockStates[i] = fillBlockId;
        }
        data.nonEmptyBlockCount = 100;
        data.biomes.resize(64, TestBiomes::PLAINS);
        return data;
    }
};

class SingleLevelStorageManagerTest : public StorageTestBase {};

TEST_F(SingleLevelStorageManagerTest, OpenClose)
{
    SingleLevelStorageManager storage;

    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;
    config.sectionCacheCapacity = 100;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success()) << result.error().message();

    EXPECT_TRUE(storage.isOpen());

    storage.close();
    EXPECT_FALSE(storage.isOpen());
}

TEST_F(SingleLevelStorageManagerTest, SectionManagerAccess)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    EXPECT_NO_THROW({
        auto& mgr = storage.sectionManager(0);
        (void)mgr;
    });

    EXPECT_NO_THROW({
        auto& mgr = storage.sectionManager(1);
        (void)mgr;
    });

    EXPECT_NO_THROW({
        auto& mgr = storage.sectionManager(2);
        (void)mgr;
    });

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, SaveAndLoadSection)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.sectionCacheCapacity = 10;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);
    SectionKey key(0, 0, 0, 0);
    SectionData originalData = createTestSectionData(0, 0, 0, 0, 42);

    auto saveResult = mgr.saveSectionSync(key, originalData);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    auto loadResult = mgr.loadSectionSync(key);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().message();

    const auto loadedData = loadResult.value();
    ASSERT_NE(loadedData, nullptr);

    EXPECT_EQ(loadedData->key.chunkX, 0);
    EXPECT_EQ(loadedData->key.chunkZ, 0);
    EXPECT_EQ(loadedData->key.sectionY, 0);
    EXPECT_EQ(loadedData->nonEmptyBlockCount, 100u);
    EXPECT_EQ(loadedData->getBlockStateId(0, 0, 0), 42u);

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, MultipleSections)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);

    for (i32 cx = -1; cx <= 1; ++cx) {
        for (i32 cz = -1; cz <= 1; ++cz) {
            for (i8 sy = 0; sy < 3; ++sy) {
                SectionKey key(cx, cz, sy, 0);
                SectionData data = createTestSectionData(cx, cz, sy, 0, static_cast<u32>(cx * 100 + cz * 10 + sy));

                auto saveResult = mgr.saveSectionSync(key, data);
                ASSERT_TRUE(saveResult.success())
                    << "Failed to save section at (" << cx << ", " << cz << ", " << static_cast<int>(sy) << ")";
            }
        }
    }

    for (i32 cx = -1; cx <= 1; ++cx) {
        for (i32 cz = -1; cz <= 1; ++cz) {
            for (i8 sy = 0; sy < 3; ++sy) {
                SectionKey key(cx, cz, sy, 0);

                auto loadResult = mgr.loadSectionSync(key);
                ASSERT_TRUE(loadResult.success());

                const auto sectionSnapshot = loadResult.value();
                ASSERT_NE(sectionSnapshot, nullptr);
                u32 expectedBlockId = static_cast<u32>(cx * 100 + cz * 10 + sy);
                EXPECT_EQ(sectionSnapshot->getBlockStateId(0, 0, 0), expectedBlockId);
            }
        }
    }

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, FlushAllDirty)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Strong;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);

    for (int i = 0; i < 10; ++i) {
        SectionKey key(i, 0, 0, 0);
        SectionData data = createTestSectionData(i, 0, 0, 0, i);
        auto saveResult = mgr.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    auto flushResult = storage.flushAllDirty();
    ASSERT_TRUE(flushResult.success());

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, DifferentDimensions)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    {
        auto& overworld = storage.sectionManager(0);
        SectionKey key(0, 0, 0, 0);
        SectionData data = createTestSectionData(0, 0, 0, 0, 1);
        auto saveResult = overworld.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    {
        auto& nether = storage.sectionManager(1);
        SectionKey key(0, 0, 0, 1);
        SectionData data = createTestSectionData(0, 0, 0, 1, 2);
        auto saveResult = nether.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    {
        auto& theEnd = storage.sectionManager(2);
        SectionKey key(0, 0, 0, 2);
        SectionData data = createTestSectionData(0, 0, 0, 2, 3);
        auto saveResult = theEnd.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    {
        auto& overworld = storage.sectionManager(0);
        auto loadResult = overworld.loadSectionSync(SectionKey(0, 0, 0, 0));
        ASSERT_TRUE(loadResult.success());

        const auto sectionSnapshot = loadResult.value();
        ASSERT_NE(sectionSnapshot, nullptr);
        EXPECT_EQ(sectionSnapshot->getBlockStateId(0, 0, 0), 1u);
    }

    {
        auto& nether = storage.sectionManager(1);
        auto loadResult = nether.loadSectionSync(SectionKey(0, 0, 0, 1));
        ASSERT_TRUE(loadResult.success());

        const auto sectionSnapshot = loadResult.value();
        ASSERT_NE(sectionSnapshot, nullptr);
        EXPECT_EQ(sectionSnapshot->getBlockStateId(0, 0, 0), 2u);
    }

    {
        auto& theEnd = storage.sectionManager(2);
        auto loadResult = theEnd.loadSectionSync(SectionKey(0, 0, 0, 2));
        ASSERT_TRUE(loadResult.success());

        const auto sectionSnapshot = loadResult.value();
        ASSERT_NE(sectionSnapshot, nullptr);
        EXPECT_EQ(sectionSnapshot->getBlockStateId(0, 0, 0), 3u);
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

        auto& mgr = storage.sectionManager(0);

        SectionKey key(10, 20, 5, 0);
        SectionData data = createTestSectionData(10, 20, 5, 0, 999);

        auto saveResult = mgr.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());

        auto flushResult = storage.flushAllDirty();
        ASSERT_TRUE(flushResult.success());

        storage.close();
    }

    {
        SingleLevelStorageManager storage;
        SingleLevelStorageConfig config;

        auto result = storage.open(testDir, config);
        ASSERT_TRUE(result.success());

        auto& mgr = storage.sectionManager(0);

        SectionKey key(10, 20, 5, 0);
        auto loadResult = mgr.loadSectionSync(key);
        ASSERT_TRUE(loadResult.success());

        const auto sectionSnapshot = loadResult.value();
        ASSERT_NE(sectionSnapshot, nullptr);

        EXPECT_EQ(sectionSnapshot->getBlockStateId(0, 0, 0), 999u);
        EXPECT_EQ(sectionSnapshot->key.chunkX, 10);
        EXPECT_EQ(sectionSnapshot->key.chunkZ, 20);
        EXPECT_EQ(sectionSnapshot->key.sectionY, 5);

        storage.close();
    }
}

TEST_F(SingleLevelStorageManagerTest, SaveAllPreservesOverwrittenSectionSnapshot)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);

    SectionKey key(30, 40, 6, 0);
    SectionData data = createTestSectionData(30, 40, 6, 0, 123);

    auto saveResult = mgr.saveSectionSync(key, data);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    auto loadResult = mgr.loadSectionSync(key);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().message();

    const auto initialSnapshot = loadResult.value();
    ASSERT_NE(initialSnapshot, nullptr);
    EXPECT_EQ(initialSnapshot->getBlockStateId(0, 0, 0), 123u);

    SectionData updatedData = createTestSectionData(30, 40, 6, 0, 777);
    auto overwriteResult = mgr.saveSectionSync(key, updatedData);
    ASSERT_TRUE(overwriteResult.success()) << overwriteResult.error().message();

    EXPECT_EQ(initialSnapshot->getBlockStateId(0, 0, 0), 123u);

    auto fullSaveResult = storage.saveAll();
    ASSERT_TRUE(fullSaveResult.success()) << fullSaveResult.error().message();

    storage.close();

    {
        SingleLevelStorageManager reopenedStorage;
        auto reopenResult = reopenedStorage.open(testDir, config);
        ASSERT_TRUE(reopenResult.success()) << reopenResult.error().message();

        auto& reopenedMgr = reopenedStorage.sectionManager(0);
        auto reopenedLoadResult = reopenedMgr.loadSectionSync(key);
        ASSERT_TRUE(reopenedLoadResult.success()) << reopenedLoadResult.error().message();
        const auto reopenedSnapshot = reopenedLoadResult.value();
        ASSERT_NE(reopenedSnapshot, nullptr);

        EXPECT_EQ(reopenedSnapshot->getBlockStateId(0, 0, 0), 777u);

        reopenedStorage.close();
    }
}

TEST_F(SingleLevelStorageManagerTest, FlushAllDirtyPersistsOverwrittenSectionSnapshot)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);

    SectionKey key(50, 60, 7, 0);
    SectionData data = createTestSectionData(50, 60, 7, 0, 456);

    auto saveResult = mgr.saveSectionSync(key, data);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    auto loadResult = mgr.loadSectionSync(key);
    ASSERT_TRUE(loadResult.success()) << loadResult.error().message();

    const auto initialSnapshot = loadResult.value();
    ASSERT_NE(initialSnapshot, nullptr);
    EXPECT_EQ(initialSnapshot->getBlockStateId(0, 0, 0), 456u);

    SectionData updatedData = createTestSectionData(50, 60, 7, 0, 888);
    auto overwriteResult = mgr.saveSectionSync(key, updatedData);
    ASSERT_TRUE(overwriteResult.success()) << overwriteResult.error().message();

    EXPECT_EQ(initialSnapshot->getBlockStateId(0, 0, 0), 456u);

    auto flushResult = storage.flushAllDirty();
    ASSERT_TRUE(flushResult.success()) << flushResult.error().message();

    storage.close();

    {
        SingleLevelStorageManager reopenedStorage;
        auto reopenResult = reopenedStorage.open(testDir, config);
        ASSERT_TRUE(reopenResult.success()) << reopenResult.error().message();

        auto& reopenedMgr = reopenedStorage.sectionManager(0);
        auto reopenedLoadResult = reopenedMgr.loadSectionSync(key);
        ASSERT_TRUE(reopenedLoadResult.success()) << reopenedLoadResult.error().message();
        const auto reopenedSnapshot = reopenedLoadResult.value();
        ASSERT_NE(reopenedSnapshot, nullptr);

        EXPECT_EQ(reopenedSnapshot->getBlockStateId(0, 0, 0), 888u);

        reopenedStorage.close();
    }
}

TEST_F(SingleLevelStorageManagerTest, InvalidSectionKey)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);
    SectionKey nonexistentKey(99999, 99999, 15, 0);
    auto loadResult = mgr.loadSectionSync(nonexistentKey);

    EXPECT_TRUE(loadResult.success());
    EXPECT_EQ(loadResult.value(), nullptr);

    storage.close();
}

} // namespace
} // namespace mc::world::storage
