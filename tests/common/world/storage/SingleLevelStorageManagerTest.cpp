#include "world/storage/SingleLevelStorageManager.hpp"
#include "core/Types.hpp"
#include "world/storage/db/SectionKey.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
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

TEST_F(SingleLevelStorageManagerTest, SaveAndLoadChunk)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.sectionCacheCapacity = 10;

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

TEST_F(SingleLevelStorageManagerTest, FlushAllDirty)
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
        section->setBlockStateId(0, 0, 0, static_cast<u32>(i));
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 0).success());
    }

    auto flushResult = storage.flushAllDirty();
    ASSERT_TRUE(flushResult.success());

    storage.close();
}

TEST_F(SingleLevelStorageManagerTest, FlushAllDirtyIncludesEntityAndBlockEntityEntrypoints)
{
    SingleLevelStorageManager storage;
    SingleLevelStorageConfig config;
    config.consistencyMode = ConsistencyMode::Strong;

    auto result = storage.open(testDir, config);
    ASSERT_TRUE(result.success());

    auto flushResult = storage.flushAllDirty();
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
        section->setBlockStateId(0, 0, 0, 1);
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 0).success());
    }

    {
        ChunkData chunk(0, 0);
        ChunkSection* section = chunk.createSection(0);
        ASSERT_NE(section, nullptr);
        section->setBlockStateId(0, 0, 0, 2);
        chunk.setLoaded(true);
        chunk.setFullyGenerated(true);
        chunk.setDirty(false);
        ASSERT_TRUE(storage.saveChunk(chunk, 1).success());
    }

    {
        ChunkData chunk(0, 0);
        ChunkSection* section = chunk.createSection(0);
        ASSERT_NE(section, nullptr);
        section->setBlockStateId(0, 0, 0, 3);
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
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), 1u);
    }

    {
        auto loadResult = storage.loadChunk(0, 0, 1);
        ASSERT_TRUE(loadResult.success());
        ASSERT_TRUE(loadResult.value().has_value());
        const ChunkSection* section = loadResult.value().value().getSection(0);
        ASSERT_NE(section, nullptr);
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), 2u);
    }

    {
        auto loadResult = storage.loadChunk(0, 0, 2);
        ASSERT_TRUE(loadResult.success());
        ASSERT_TRUE(loadResult.value().has_value());
        const ChunkSection* section = loadResult.value().value().getSection(0);
        ASSERT_NE(section, nullptr);
        EXPECT_EQ(section->getBlockStateId(0, 0, 0), 3u);
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

        auto flushResult = storage.flushAllDirty();
        ASSERT_TRUE(flushResult.success());

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

TEST_F(SingleLevelStorageManagerTest, SaveAllPreservesOverwrittenSectionSnapshot)
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

    auto fullSaveResult = storage.saveAll();
    ASSERT_TRUE(fullSaveResult.success()) << fullSaveResult.error().message();

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

TEST_F(SingleLevelStorageManagerTest, FlushAllDirtyPersistsOverwrittenSectionSnapshot)
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
    auto overwriteResult = storage.saveChunk(updatedChunk, 0);
    ASSERT_TRUE(overwriteResult.success()) << overwriteResult.error().message();

    auto flushResult = storage.flushAllDirty();
    ASSERT_TRUE(flushResult.success()) << flushResult.error().message();

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
    section->setBlockStateId(0, 0, 0, 55);
    chunk.setLoaded(true);
    chunk.setFullyGenerated(true);

    auto saveResult = storage.saveChunk(chunk, 0);
    ASSERT_TRUE(saveResult.success()) << saveResult.error().message();

    auto flushResult = storage.flushAllDirty();
    ASSERT_TRUE(flushResult.success()) << flushResult.error().message();
    EXPECT_EQ(flushResult.value(), 0u);

    auto saveAllResult = storage.saveAll();
    ASSERT_TRUE(saveAllResult.success()) << saveAllResult.error().message();
    EXPECT_EQ(saveAllResult.value(), 0u);

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
        EXPECT_EQ(levelData.spawnX, 0);
        EXPECT_EQ(levelData.spawnY, 0);
        EXPECT_EQ(levelData.spawnZ, 0);
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

        auto saveResult = bootstrap.saveLevelData(10, 20, 1, 64, 2, 30.0f, 40, 50, true, 60, false, true);
        ASSERT_TRUE(saveResult.success()) << saveResult.error().message();
        bootstrap.close();
    }

    {
        SingleLevelStorageManager readonlyStorage;
        SingleLevelStorageConfig readonlyConfig;
        readonlyConfig.readonly = true;

        auto openResult = readonlyStorage.open(testDir, readonlyConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        auto saveResult = readonlyStorage.saveLevelData(999, 888, 10, 70, 11, 12.0f, 13, 14, false, 15, true, false);
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
        EXPECT_EQ(levelData.spawnX, 1);
        EXPECT_EQ(levelData.spawnY, 64);
        EXPECT_EQ(levelData.spawnZ, 2);
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
        auto saveResult = storage.saveLevelData(0, 0, 0, 0, 0, 0.0f, 0, 0, false, 0, false, initialized);
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
