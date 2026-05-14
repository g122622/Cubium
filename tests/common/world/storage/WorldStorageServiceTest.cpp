#include "world/storage/WorldStorageService.hpp"
#include "core/Types.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/storage/db/SectionCodec.hpp"
#include "world/storage/db/SectionKey.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace mc::world::storage {
namespace {

// 测试用生物群系 ID
namespace TestBiomes {
constexpr BiomeId PLAINS = 1;
}

// 测试用临时目录
class StorageTestBase : public ::testing::Test {
protected:
    std::filesystem::path testDir;

    void SetUp() override
    {
        // 创建临时测试目录
        testDir = std::filesystem::temp_directory_path() / "mc_storage_test" / std::to_string(std::time(nullptr));
        std::filesystem::create_directories(testDir);
    }

    void TearDown() override
    {
        // 清理测试目录
        if (std::filesystem::exists(testDir)) {
            std::error_code ec;
            std::filesystem::remove_all(testDir, ec);
        }
    }

    // 创建简单的测试 Section 数据
    SectionData createTestSectionData(ChunkCoord x, ChunkCoord z, i8 sectionY, DimensionId dim, u32 fillBlockId = 1)
    {
        SectionData data;
        data.key = SectionKey(x, z, sectionY, dim);

        // 初始化默认数据
        data.initializeDefaults();

        // 填充一些方块
        for (size_t i = 0; i < 100; ++i) {
            data.blockStates[i] = fillBlockId;
        }
        data.nonEmptyBlockCount = 100;

        // 设置生物群系
        data.biomes.resize(64, TestBiomes::PLAINS);

        return data;
    }
};

class WorldStorageServiceTest : public StorageTestBase {
protected:
    void SetUp() override { StorageTestBase::SetUp(); }
};

TEST_F(WorldStorageServiceTest, OpenClose)
{
    WorldStorageService storage;

    WorldStorageConfig config;
    config.consistencyMode = ConsistencyMode::Eventual;
    config.sectionCacheCapacity = 100;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success()) << result.error().message();

    EXPECT_TRUE(storage.isOpen());

    storage.close();
    EXPECT_FALSE(storage.isOpen());
}

TEST_F(WorldStorageServiceTest, SectionManagerAccess)
{
    WorldStorageService storage;
    WorldStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    // 访问主世界的 SectionManager
    EXPECT_NO_THROW({
        auto& mgr = storage.sectionManager(0); // Overworld
        (void)mgr;
    });

    // 访问下界的 SectionManager
    EXPECT_NO_THROW({
        auto& mgr = storage.sectionManager(1); // Nether
        (void)mgr;
    });

    // 访问末地的 SectionManager
    EXPECT_NO_THROW({
        auto& mgr = storage.sectionManager(2); // The End
        (void)mgr;
    });

    storage.close();
}

TEST_F(WorldStorageServiceTest, SaveAndLoadSection)
{
    WorldStorageService storage;
    WorldStorageConfig config;
    config.sectionCacheCapacity = 10;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0); // Overworld

    // 创建并保存 Section
    SectionKey key(0, 0, 0, 0);
    SectionData originalData = createTestSectionData(0, 0, 0, 0, 42);

    auto saveResult = mgr.saveSectionSync(key, originalData);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    // 加载 Section
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

TEST_F(WorldStorageServiceTest, MultipleSections)
{
    WorldStorageService storage;
    WorldStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0); // Overworld

    // 保存多个 Section
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

    // 验证加载
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

TEST_F(WorldStorageServiceTest, FlushAllDirty)
{
    WorldStorageService storage;
    WorldStorageConfig config;
    config.consistencyMode = ConsistencyMode::Strong;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);

    // 保存多个 Section
    for (int i = 0; i < 10; ++i) {
        SectionKey key(i, 0, 0, 0);
        SectionData data = createTestSectionData(i, 0, 0, 0, i);
        auto saveResult = mgr.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    // 刷新所有脏数据
    auto flushResult = storage.flushAllDirty();
    ASSERT_TRUE(flushResult.success());

    storage.close();
}

TEST_F(WorldStorageServiceTest, DifferentDimensions)
{
    WorldStorageService storage;
    WorldStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    // 在不同维度保存 Section
    {
        auto& overworld = storage.sectionManager(0); // Overworld
        SectionKey key(0, 0, 0, 0);
        SectionData data = createTestSectionData(0, 0, 0, 0, 1);
        auto saveResult = overworld.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    {
        auto& nether = storage.sectionManager(1); // Nether
        SectionKey key(0, 0, 0, 1);
        SectionData data = createTestSectionData(0, 0, 0, 1, 2);
        auto saveResult = nether.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    {
        auto& theEnd = storage.sectionManager(2); // The End
        SectionKey key(0, 0, 0, 2);
        SectionData data = createTestSectionData(0, 0, 0, 2, 3);
        auto saveResult = theEnd.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());
    }

    // 验证各维度数据独立
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

TEST_F(WorldStorageServiceTest, ReopenPreservesData)
{
    {
        WorldStorageService storage;
        WorldStorageConfig config;
        config.consistencyMode = ConsistencyMode::Strong;

        auto result = storage.open(testDir, config);
        ASSERT_TRUE(result.success());

        auto& mgr = storage.sectionManager(0);

        SectionKey key(10, 20, 5, 0);
        SectionData data = createTestSectionData(10, 20, 5, 0, 999);

        auto saveResult = mgr.saveSectionSync(key, data);
        ASSERT_TRUE(saveResult.success());

        // 强制刷新
        auto flushResult = storage.flushAllDirty();
        ASSERT_TRUE(flushResult.success());

        storage.close();
    }

    // 重新打开验证数据持久化
    {
        WorldStorageService storage;
        WorldStorageConfig config;

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

TEST_F(WorldStorageServiceTest, SaveAllPreservesOverwrittenSectionSnapshot)
{
    WorldStorageService storage;
    WorldStorageConfig config;
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

    // 通过正常保存覆盖同一 Section，验证已返回的快照仍保持原值。
    SectionData updatedData = createTestSectionData(30, 40, 6, 0, 777);
    auto overwriteResult = mgr.saveSectionSync(key, updatedData);
    ASSERT_TRUE(overwriteResult.success()) << overwriteResult.error().message();

    EXPECT_EQ(initialSnapshot->getBlockStateId(0, 0, 0), 123u);

    auto fullSaveResult = storage.saveAll();
    ASSERT_TRUE(fullSaveResult.success()) << fullSaveResult.error().message();

    storage.close();

    {
        WorldStorageService reopenedStorage;
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

TEST_F(WorldStorageServiceTest, FlushAllDirtyPersistsOverwrittenSectionSnapshot)
{
    WorldStorageService storage;
    WorldStorageConfig config;
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

    // 通过正常保存覆盖同一 Section，验证已返回的快照不受后续缓存替换影响。
    SectionData updatedData = createTestSectionData(50, 60, 7, 0, 888);
    auto overwriteResult = mgr.saveSectionSync(key, updatedData);
    ASSERT_TRUE(overwriteResult.success()) << overwriteResult.error().message();

    EXPECT_EQ(initialSnapshot->getBlockStateId(0, 0, 0), 456u);

    auto flushResult = storage.flushAllDirty();
    ASSERT_TRUE(flushResult.success()) << flushResult.error().message();

    storage.close();

    {
        WorldStorageService reopenedStorage;
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

TEST_F(WorldStorageServiceTest, InvalidSectionKey)
{
    WorldStorageService storage;
    WorldStorageConfig config;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto& mgr = storage.sectionManager(0);

    // 尝试加载不存在的 Section
    SectionKey nonexistentKey(99999, 99999, 15, 0);
    auto loadResult = mgr.loadSectionSync(nonexistentKey);

    // 应该返回 nullptr 表示不存在
    EXPECT_TRUE(loadResult.success());
    EXPECT_EQ(loadResult.value(), nullptr);

    storage.close();
}

} // namespace
} // namespace mc::world::storage
