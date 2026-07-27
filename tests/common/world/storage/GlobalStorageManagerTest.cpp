/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following further conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file GlobalStorageManagerTest.cpp
 * @brief GlobalStorageManager 单元测试
 *
 * 测试覆盖：
 * - deleteWorld: 删除不存在的世界、删除存在的世界、递归删除子目录
 * - listWorlds: 空目录、通过 createWorld 创建后的列表
 * - deleteWorld 与 listWorlds 的交互
 */

#include "world/storage/GlobalStorageManager.hpp"
#include "common/TempDirHelper.hpp"
#include "world/storage/core/LevelDatCodec.hpp"
#include "world/storage/core/WorldSessionLock.hpp"
#include "world/storage/core/WorldStoragePaths.hpp"
#include "world/storage/list/WorldListService.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

namespace mc::world::storage {
namespace {

class GlobalStorageManagerTest : public ::testing::Test {
protected:
    std::filesystem::path testDir;
    std::filesystem::path savesDir;
    std::unique_ptr<GlobalStorageManager> storage;

    void SetUp() override
    {
        // PID + 纳秒 + 计数器组合，跨进程唯一，避免 CTest -j16 同名目录撞锁
        testDir = mc::test::makeUniqueTestDir("mc_global_storage_test");
        savesDir = testDir / "saves";
        std::filesystem::create_directories(savesDir);

        WorldStoragePaths paths(savesDir, testDir / "backups");
        storage = std::make_unique<GlobalStorageManager>(paths);
    }

    void TearDown() override
    {
        storage.reset();
        mc::test::removeTestDir(testDir);
    }

    /**
     * @brief 通过 WorldListService::createWorld 创建一个合法的世界目录
     */
    Result<std::string> createTestWorld(const std::string& displayName)
    {
        WorldStoragePaths paths(savesDir, testDir / "backups");
        WorldListService service(paths);

        CreateWorldRequest request(displayName,
            "",
            12345,
            WorldType::Default,
            resource::ResourceLocation("minecraft", "default"),
            GameMode::Survival,
            Difficulty::Normal,
            false,
            false,
            12);
        return service.createWorld(request);
    }
};

// ============================================================================
// deleteWorld 测试
// ============================================================================

TEST_F(GlobalStorageManagerTest, DeleteWorld_NonexistentWorld_ReturnsError)
{
    // 删除一个不存在的世界应该返回错误
    auto result = storage->deleteWorld("nonexistent_world");
    EXPECT_FALSE(result.success()) << "Deleting a non-existent world should fail";
    EXPECT_EQ(result.error().code(), ErrorCode::FileNotFound);
}

TEST_F(GlobalStorageManagerTest, DeleteWorld_ExistingWorld_Succeeds)
{
    // 通过 createWorld 创建一个合法的世界
    auto createResult = createTestWorld("Test World");
    ASSERT_TRUE(createResult.success()) << "createWorld failed: " << createResult.error().message();
    std::string levelId = createResult.value();
    EXPECT_FALSE(levelId.empty());

    // 确认世界目录存在
    std::filesystem::path worldDir = savesDir / levelId;
    EXPECT_TRUE(std::filesystem::exists(worldDir)) << "World directory should exist after creation";

    // 删除世界
    auto result = storage->deleteWorld(levelId);
    EXPECT_TRUE(result.success()) << "Deleting an existing world should succeed: " << result.error().message();

    // 确认世界目录已不存在
    EXPECT_FALSE(std::filesystem::exists(worldDir)) << "World directory should not exist after deletion";
}

TEST_F(GlobalStorageManagerTest, DeleteWorld_MultipleWorlds_OnlyDeletesTarget)
{
    // 创建三个世界
    auto createAlpha = createTestWorld("World Alpha");
    auto createBeta = createTestWorld("World Beta");
    auto createGamma = createTestWorld("World Gamma");
    ASSERT_TRUE(createAlpha.success());
    ASSERT_TRUE(createBeta.success());
    ASSERT_TRUE(createGamma.success());

    std::string alphaId = createAlpha.value();
    std::string betaId = createBeta.value();
    std::string gammaId = createGamma.value();

    // 确认所有世界目录存在
    EXPECT_TRUE(std::filesystem::exists(savesDir / alphaId));
    EXPECT_TRUE(std::filesystem::exists(savesDir / betaId));
    EXPECT_TRUE(std::filesystem::exists(savesDir / gammaId));

    // 只删除 beta
    auto result = storage->deleteWorld(betaId);
    EXPECT_TRUE(result.success()) << "Deleting world_beta should succeed: " << result.error().message();

    // beta 目录应已删除
    EXPECT_FALSE(std::filesystem::exists(savesDir / betaId)) << "Deleted world directory should not exist";

    // alpha 和 gamma 目录应仍存在
    EXPECT_TRUE(std::filesystem::exists(savesDir / alphaId)) << "Other worlds should still exist";
    EXPECT_TRUE(std::filesystem::exists(savesDir / gammaId)) << "Other worlds should still exist";
}

TEST_F(GlobalStorageManagerTest, DeleteWorld_DirectoryWithSubdirectories_Succeeds)
{
    // 通过 createWorld 创建一个包含子目录的世界
    auto createResult = createTestWorld("World With Subdirs");
    ASSERT_TRUE(createResult.success()) << createResult.error().message();
    std::string levelId = createResult.value();

    // 在世界目录中创建额外的子目录和文件（模拟 db/ 目录）
    std::filesystem::path worldDir = savesDir / levelId;
    std::filesystem::create_directories(worldDir / "db" / "chunks");
    std::filesystem::create_directories(worldDir / "db" / "entities");

    std::ofstream ofs(worldDir / "db" / "test.dat", std::ios::binary);
    ofs << "test data";
    ofs.close();

    EXPECT_TRUE(std::filesystem::exists(worldDir / "db" / "test.dat"));

    // 删除世界
    auto result = storage->deleteWorld(levelId);
    EXPECT_TRUE(result.success()) << "Deleting world with subdirectories should succeed: " << result.error().message();

    // 确认整个目录被递归删除
    EXPECT_FALSE(std::filesystem::exists(worldDir)) << "World directory with subdirs should be fully removed";
}

TEST_F(GlobalStorageManagerTest, ListWorlds_EmptyDirectory_ReturnsEmptyList)
{
    // 没有任何世界时，应返回空列表
    auto result = storage->listWorlds();
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().empty()) << "Empty saves directory should return empty list";
}

TEST_F(GlobalStorageManagerTest, ListWorlds_AfterCreateWorld_ContainsWorld)
{
    // 通过 createWorld 创建世界
    auto createResult = createTestWorld("My Test World");
    ASSERT_TRUE(createResult.success()) << createResult.error().message();

    // 列表应包含该世界
    auto result = storage->listWorlds();
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().size(), 1u) << "Should have exactly 1 world after creating one";
    EXPECT_EQ(result.value()[0].levelId, createResult.value());
    EXPECT_EQ(result.value()[0].displayName, "My Test World");
}

TEST_F(GlobalStorageManagerTest, DeleteWorld_AfterDelete_ListNoLongerContainsWorld)
{
    // 创建世界并确认它出现在列表中
    auto createResult = createTestWorld("To Be Deleted");
    ASSERT_TRUE(createResult.success()) << createResult.error().message();
    std::string levelId = createResult.value();

    auto listBefore = storage->listWorlds();
    ASSERT_TRUE(listBefore.success());
    EXPECT_EQ(listBefore.value().size(), 1u);

    // 删除世界
    auto deleteResult = storage->deleteWorld(levelId);
    EXPECT_TRUE(deleteResult.success()) << deleteResult.error().message();

    // 列表应为空
    auto listAfter = storage->listWorlds();
    ASSERT_TRUE(listAfter.success());
    EXPECT_TRUE(listAfter.value().empty()) << "World list should be empty after deleting the only world";
}

} // namespace
} // namespace mc::world::storage
