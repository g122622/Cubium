/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "server/core/OpListManager.hpp"
#include "common/TempDirHelper.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::server::core;

class OpListManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 助手以 PID 组合 token 保证跨进程唯一，避免 CTest -j16 并发覆盖
        testDir_ = mc::test::makeUniqueTestDir("mc_op_list_test");
        testFile_ = testDir_ / "ops.json";
    }

    void TearDown() override
    {
        // 清理临时目录
        mc::test::removeTestDir(testDir_);
    }

    std::filesystem::path testDir_;
    std::filesystem::path testFile_;
};

// ========== 基本功能测试 ==========

TEST_F(OpListManagerTest, DefaultState)
{
    OpListManager opList;

    EXPECT_TRUE(opList.empty());
    EXPECT_EQ(opList.size(), 0);
}

TEST_F(OpListManagerTest, SetEntry)
{
    OpListManager opList;

    OpEntry entry("uuid-123", "Player1", OpLevel::GameMaster, false);
    EXPECT_TRUE(opList.setEntry(entry));
    EXPECT_EQ(opList.size(), 1);
    EXPECT_TRUE(opList.isOp("uuid-123"));
    EXPECT_EQ(opList.getLevel("uuid-123"), OpLevel::GameMaster);
}

TEST_F(OpListManagerTest, SetEntryUpdatesExisting)
{
    OpListManager opList;

    OpEntry entry1("uuid-123", "Player1", OpLevel::Moderator, false);
    EXPECT_TRUE(opList.setEntry(entry1));
    EXPECT_EQ(opList.size(), 1);
    EXPECT_EQ(opList.getLevel("uuid-123"), OpLevel::Moderator);

    // 更新已有条目（提升权限）
    OpEntry entry2("uuid-123", "Player1", OpLevel::Admin, true);
    EXPECT_TRUE(opList.setEntry(entry2));
    EXPECT_EQ(opList.size(), 1); // 大小不变
    EXPECT_EQ(opList.getLevel("uuid-123"), OpLevel::Admin);
}

TEST_F(OpListManagerTest, RemoveEntry)
{
    OpListManager opList;

    OpEntry entry("uuid-123", "Player1", OpLevel::GameMaster, false);
    opList.setEntry(entry);

    EXPECT_TRUE(opList.removeEntry("uuid-123"));
    EXPECT_EQ(opList.size(), 0);
    EXPECT_FALSE(opList.isOp("uuid-123"));
    EXPECT_EQ(opList.getLevel("uuid-123"), OpLevel::Normal);
}

TEST_F(OpListManagerTest, RemoveNonExistentEntry)
{
    OpListManager opList;

    EXPECT_FALSE(opList.removeEntry("non-existent-uuid"));
}

TEST_F(OpListManagerTest, GetEntry)
{
    OpListManager opList;

    OpEntry entry("uuid-123", "Player1", OpLevel::Admin, true);
    opList.setEntry(entry);

    auto result = opList.getEntry("uuid-123");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->uuid, "uuid-123");
    EXPECT_EQ(result->name, "Player1");
    EXPECT_EQ(result->level, OpLevel::Admin);
    EXPECT_TRUE(result->bypassesPlayerLimit);

    auto result2 = opList.getEntry("non-existent");
    EXPECT_FALSE(result2.has_value());
}

TEST_F(OpListManagerTest, GetAllEntries)
{
    OpListManager opList;

    opList.setEntry(OpEntry("uuid-1", "Player1", OpLevel::Moderator, false));
    opList.setEntry(OpEntry("uuid-2", "Player2", OpLevel::GameMaster, false));
    opList.setEntry(OpEntry("uuid-3", "Player3", OpLevel::Admin, true));

    auto entries = opList.getAllEntries();
    EXPECT_EQ(entries.size(), 3);
}

TEST_F(OpListManagerTest, Clear)
{
    OpListManager opList;

    opList.setEntry(OpEntry("uuid-1", "Player1", OpLevel::Moderator, false));
    opList.setEntry(OpEntry("uuid-2", "Player2", OpLevel::GameMaster, false));

    opList.clear();
    EXPECT_EQ(opList.size(), 0);
    EXPECT_TRUE(opList.empty());
}

// ========== OpLevel 测试 ==========

TEST_F(OpListManagerTest, OpLevelValues)
{
    EXPECT_EQ(static_cast<int>(OpLevel::Normal), 0);
    EXPECT_EQ(static_cast<int>(OpLevel::Moderator), 1);
    EXPECT_EQ(static_cast<int>(OpLevel::GameMaster), 2);
    EXPECT_EQ(static_cast<int>(OpLevel::Admin), 3);
    EXPECT_EQ(static_cast<int>(OpLevel::Owner), 4);
}

TEST_F(OpListManagerTest, DifferentOpLevels)
{
    OpListManager opList;

    opList.setEntry(OpEntry("uuid-1", "Mod", OpLevel::Moderator, false));
    opList.setEntry(OpEntry("uuid-2", "GM", OpLevel::GameMaster, false));
    opList.setEntry(OpEntry("uuid-3", "Admin", OpLevel::Admin, true));
    opList.setEntry(OpEntry("uuid-4", "Owner", OpLevel::Owner, true));

    EXPECT_EQ(opList.getLevel("uuid-1"), OpLevel::Moderator);
    EXPECT_EQ(opList.getLevel("uuid-2"), OpLevel::GameMaster);
    EXPECT_EQ(opList.getLevel("uuid-3"), OpLevel::Admin);
    EXPECT_EQ(opList.getLevel("uuid-4"), OpLevel::Owner);
    EXPECT_EQ(opList.getLevel("non-existent"), OpLevel::Normal);
}

TEST_F(OpListManagerTest, BypassesPlayerLimit)
{
    OpListManager opList;

    OpEntry entry1("uuid-1", "Player1", OpLevel::GameMaster, false);
    opList.setEntry(entry1);
    auto result1 = opList.getEntry("uuid-1");
    EXPECT_TRUE(result1.has_value());
    EXPECT_FALSE(result1->bypassesPlayerLimit);

    OpEntry entry2("uuid-2", "Player2", OpLevel::Admin, true);
    opList.setEntry(entry2);
    auto result2 = opList.getEntry("uuid-2");
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result2->bypassesPlayerLimit);
}

// ========== 文件操作测试 ==========

TEST_F(OpListManagerTest, SaveAndLoad)
{
    // 创建并保存
    {
        OpListManager opList;
        opList.setEntry(OpEntry("uuid-1", "Player1", OpLevel::Moderator, false));
        opList.setEntry(OpEntry("uuid-2", "Player2", OpLevel::GameMaster, false));
        opList.setEntry(OpEntry("uuid-3", "Player3", OpLevel::Admin, true));

        auto result = opList.save(testFile_);
        EXPECT_TRUE(result.success()) << result.error().message();
    }

    // 验证文件存在
    EXPECT_TRUE(std::filesystem::exists(testFile_));

    // 加载并验证
    {
        OpListManager opList;
        auto result = opList.load(testFile_);
        EXPECT_TRUE(result.success()) << result.error().message();

        EXPECT_EQ(opList.size(), 3);
        EXPECT_TRUE(opList.isOp("uuid-1"));
        EXPECT_TRUE(opList.isOp("uuid-2"));
        EXPECT_TRUE(opList.isOp("uuid-3"));

        EXPECT_EQ(opList.getLevel("uuid-1"), OpLevel::Moderator);
        EXPECT_EQ(opList.getLevel("uuid-2"), OpLevel::GameMaster);
        EXPECT_EQ(opList.getLevel("uuid-3"), OpLevel::Admin);

        auto entry = opList.getEntry("uuid-3");
        EXPECT_TRUE(entry.has_value());
        EXPECT_TRUE(entry->bypassesPlayerLimit);
    }
}

TEST_F(OpListManagerTest, LoadNonExistentFile)
{
    OpListManager opList;

    auto result = opList.load(testDir_ / "non_existent.json");
    EXPECT_TRUE(result.success()); // 应该成功，创建空列表
    EXPECT_TRUE(opList.empty());
}

TEST_F(OpListManagerTest, Reload)
{
    OpListManager opList;

    // 初始保存
    opList.setEntry(OpEntry("uuid-1", "Player1", OpLevel::Moderator, false));
    opList.save(testFile_);

    // 手动修改文件
    std::ofstream file(testFile_, std::ios::trunc);
    file << R"([
        {"uuid": "uuid-new", "name": "NewPlayer", "level": 3, "bypassesPlayerLimit": true}
    ])";
    file.close();

    // 重新加载
    auto result = opList.reload();
    EXPECT_TRUE(result.success()) << result.error().message();

    EXPECT_EQ(opList.size(), 1);
    EXPECT_TRUE(opList.isOp("uuid-new"));
    EXPECT_FALSE(opList.isOp("uuid-1"));
}

TEST_F(OpListManagerTest, LoadInvalidJson)
{
    // 创建无效 JSON 文件
    std::ofstream file(testFile_);
    file << "not a valid json";
    file.close();

    OpListManager opList;
    auto result = opList.load(testFile_);
    EXPECT_FALSE(result.success());
}

TEST_F(OpListManagerTest, LoadNonArrayJson)
{
    // 创建非数组 JSON 文件
    std::ofstream file(testFile_);
    file << R"({"uuid": "uuid-1", "name": "Player1"})";
    file.close();

    OpListManager opList;
    auto result = opList.load(testFile_);
    EXPECT_FALSE(result.success());
}

TEST_F(OpListManagerTest, LoadWithMissingFields)
{
    // 创建缺少字段的 JSON 文件
    std::ofstream file(testFile_);
    file << R"([
        {"uuid": "uuid-1", "name": "Player1"},
        {"uuid": "uuid-2", "name": "Player2", "level": 2}
    ])";
    file.close();

    OpListManager opList;
    auto result = opList.load(testFile_);
    EXPECT_TRUE(result.success()); // 缺少可选字段应该被接受
    EXPECT_EQ(opList.size(), 2);

    // 验证默认值
    auto entry1 = opList.getEntry("uuid-1");
    EXPECT_TRUE(entry1.has_value());
    EXPECT_EQ(entry1->level, OpLevel::GameMaster); // 默认等级 2
    EXPECT_FALSE(entry1->bypassesPlayerLimit);     // 默认 false

    auto entry2 = opList.getEntry("uuid-2");
    EXPECT_TRUE(entry2.has_value());
    EXPECT_EQ(entry2->level, OpLevel::GameMaster);
    EXPECT_FALSE(entry2->bypassesPlayerLimit);
}

TEST_F(OpListManagerTest, FilePathTracking)
{
    OpListManager opList;

    opList.load(testFile_);
    EXPECT_EQ(opList.filePath(), testFile_);

    // reload 应该使用上次加载的路径
    std::ofstream file(testFile_);
    file << R"([{"uuid": "uuid-1", "name": "Player1", "level": 2, "bypassesPlayerLimit": false}])";
    file.close();

    auto result = opList.reload();
    EXPECT_TRUE(result.success());
    EXPECT_EQ(opList.size(), 1);
}

// ========== 线程安全测试 ==========

TEST_F(OpListManagerTest, ThreadSafety)
{
    OpListManager opList;
    constexpr int numThreads = 4;
    constexpr int entriesPerThread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&opList, t, entriesPerThread]() {
            for (int i = 0; i < entriesPerThread; ++i) {
                std::string uuid = "uuid-" + std::to_string(t) + "-" + std::to_string(i);
                std::string name = "Player" + std::to_string(t) + "_" + std::to_string(i);
                opList.setEntry(OpEntry(uuid, name, OpLevel::GameMaster, false));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 应该恰好有 numThreads * entriesPerThread 个条目
    EXPECT_EQ(opList.size(), static_cast<size_t>(numThreads * entriesPerThread));
}

// ========== 复杂场景测试 ==========

TEST_F(OpListManagerTest, AddRemoveMultiple)
{
    OpListManager opList;

    // 添加多个
    opList.setEntry(OpEntry("uuid-1", "Player1", OpLevel::Moderator, false));
    opList.setEntry(OpEntry("uuid-2", "Player2", OpLevel::GameMaster, false));
    opList.setEntry(OpEntry("uuid-3", "Player3", OpLevel::Admin, true));

    EXPECT_EQ(opList.size(), 3);

    // 删除中间的
    EXPECT_TRUE(opList.removeEntry("uuid-2"));
    EXPECT_EQ(opList.size(), 2);
    EXPECT_FALSE(opList.isOp("uuid-2"));
    EXPECT_TRUE(opList.isOp("uuid-1"));
    EXPECT_TRUE(opList.isOp("uuid-3"));

    // 再次添加已删除的
    EXPECT_TRUE(opList.setEntry(OpEntry("uuid-2", "Player2", OpLevel::Owner, true)));
    EXPECT_EQ(opList.size(), 3);
    EXPECT_TRUE(opList.isOp("uuid-2"));
    EXPECT_EQ(opList.getLevel("uuid-2"), OpLevel::Owner);
}

TEST_F(OpListManagerTest, JsonFormatCompatibility)
{
    // 测试与 Minecraft 1.16.5 ops.json 格式的兼容性
    std::ofstream file(testFile_);
    file << R"([
        {
            "uuid": "550e8400-e29b-41d4-a716-446655440000",
            "name": "TestPlayer",
            "level": 4,
            "bypassesPlayerLimit": true
        },
        {
            "uuid": "6ba7b810-9dad-11d1-80b4-00c04fd430c8",
            "name": "AnotherPlayer",
            "level": 2,
            "bypassesPlayerLimit": false
        }
    ])";
    file.close();

    OpListManager opList;
    auto result = opList.load(testFile_);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(opList.size(), 2);

    auto entry1 = opList.getEntry("550e8400-e29b-41d4-a716-446655440000");
    EXPECT_TRUE(entry1.has_value());
    EXPECT_EQ(entry1->name, "TestPlayer");
    EXPECT_EQ(entry1->level, OpLevel::Owner);
    EXPECT_TRUE(entry1->bypassesPlayerLimit);

    auto entry2 = opList.getEntry("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
    EXPECT_TRUE(entry2.has_value());
    EXPECT_EQ(entry2->name, "AnotherPlayer");
    EXPECT_EQ(entry2->level, OpLevel::GameMaster);
    EXPECT_FALSE(entry2->bypassesPlayerLimit);
}

TEST_F(OpListManagerTest, SaveProducesValidFormat)
{
    OpListManager opList;
    opList.setEntry(OpEntry("550e8400-e29b-41d4-a716-446655440000", "TestPlayer", OpLevel::Owner, true));
    opList.setEntry(OpEntry("6ba7b810-9dad-11d1-80b4-00c04fd430c8", "AnotherPlayer", OpLevel::GameMaster, false));

    auto result = opList.save(testFile_);
    EXPECT_TRUE(result.success());

    // 读取文件内容验证格式
    std::ifstream file(testFile_);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // 验证 JSON 格式
    EXPECT_TRUE(content.find("\"uuid\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"name\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"level\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"bypassesPlayerLimit\"") != std::string::npos);
}
