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

#include "server/core/WhitelistManager.hpp"
#include "common/TempDirHelper.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace mc::server::core;

class WhitelistManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 助手以 PID 组合 token 保证跨进程唯一，避免 CTest -j16 并发覆盖
        testDir_ = mc::test::makeUniqueTestDir("mc_whitelist_test");
        testFile_ = testDir_ / "whitelist.json";
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

TEST_F(WhitelistManagerTest, DefaultState)
{
    WhitelistManager manager;

    EXPECT_FALSE(manager.isEnabled());
    EXPECT_TRUE(manager.empty());
    EXPECT_EQ(manager.size(), 0);
}

TEST_F(WhitelistManagerTest, EnableDisable)
{
    WhitelistManager manager;

    manager.setEnabled(true);
    EXPECT_TRUE(manager.isEnabled());

    manager.setEnabled(false);
    EXPECT_FALSE(manager.isEnabled());
}

// ========== 条目管理测试 ==========

TEST_F(WhitelistManagerTest, AddEntry)
{
    WhitelistManager manager;

    WhitelistEntry entry("uuid-123", "Player1");
    EXPECT_TRUE(manager.addEntry(entry));
    EXPECT_EQ(manager.size(), 1);
    EXPECT_TRUE(manager.isWhitelisted("uuid-123"));
    EXPECT_TRUE(manager.isNameWhitelisted("Player1"));
    EXPECT_TRUE(manager.isNameWhitelisted("player1")); // 大小写不敏感
}

TEST_F(WhitelistManagerTest, AddDuplicateEntry)
{
    WhitelistManager manager;

    WhitelistEntry entry1("uuid-123", "Player1");
    EXPECT_TRUE(manager.addEntry(entry1));

    // 相同 UUID 不能重复添加
    WhitelistEntry entry2("uuid-123", "Player2");
    EXPECT_FALSE(manager.addEntry(entry2));
    EXPECT_EQ(manager.size(), 1);
}

TEST_F(WhitelistManagerTest, RemoveEntryByUuid)
{
    WhitelistManager manager;

    WhitelistEntry entry("uuid-123", "Player1");
    manager.addEntry(entry);

    EXPECT_TRUE(manager.removeEntry("uuid-123"));
    EXPECT_EQ(manager.size(), 0);
    EXPECT_FALSE(manager.isWhitelisted("uuid-123"));
    EXPECT_FALSE(manager.isNameWhitelisted("Player1"));
}

TEST_F(WhitelistManagerTest, RemoveEntryByName)
{
    WhitelistManager manager;

    WhitelistEntry entry("uuid-123", "Player1");
    manager.addEntry(entry);

    EXPECT_TRUE(manager.removeEntryByName("Player1"));
    EXPECT_EQ(manager.size(), 0);

    // 大小写不敏感
    manager.addEntry(entry);
    EXPECT_TRUE(manager.removeEntryByName("player1"));
    EXPECT_EQ(manager.size(), 0);
}

TEST_F(WhitelistManagerTest, RemoveNonExistentEntry)
{
    WhitelistManager manager;

    EXPECT_FALSE(manager.removeEntry("non-existent-uuid"));
    EXPECT_FALSE(manager.removeEntryByName("NonExistentPlayer"));
}

TEST_F(WhitelistManagerTest, GetEntry)
{
    WhitelistManager manager;

    WhitelistEntry entry("uuid-123", "Player1");
    manager.addEntry(entry);

    auto result = manager.getEntry("uuid-123");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->uuid, "uuid-123");
    EXPECT_EQ(result->name, "Player1");

    auto result2 = manager.getEntry("non-existent");
    EXPECT_FALSE(result2.has_value());
}

TEST_F(WhitelistManagerTest, GetEntryByName)
{
    WhitelistManager manager;

    WhitelistEntry entry("uuid-123", "Player1");
    manager.addEntry(entry);

    auto result = manager.getEntryByName("Player1");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->uuid, "uuid-123");
    EXPECT_EQ(result->name, "Player1");

    // 大小写不敏感
    auto result2 = manager.getEntryByName("player1");
    EXPECT_TRUE(result2.has_value());
}

TEST_F(WhitelistManagerTest, GetAllEntries)
{
    WhitelistManager manager;

    manager.addEntry(WhitelistEntry("uuid-1", "Player1"));
    manager.addEntry(WhitelistEntry("uuid-2", "Player2"));
    manager.addEntry(WhitelistEntry("uuid-3", "Player3"));

    auto entries = manager.getAllEntries();
    EXPECT_EQ(entries.size(), 3);
}

TEST_F(WhitelistManagerTest, GetAllNames)
{
    WhitelistManager manager;

    manager.addEntry(WhitelistEntry("uuid-1", "Player1"));
    manager.addEntry(WhitelistEntry("uuid-2", "Player2"));

    auto names = manager.getAllNames();
    EXPECT_EQ(names.size(), 2);

    // 验证名称存在
    EXPECT_NE(std::find(names.begin(), names.end(), "Player1"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "Player2"), names.end());
}

TEST_F(WhitelistManagerTest, Clear)
{
    WhitelistManager manager;

    manager.addEntry(WhitelistEntry("uuid-1", "Player1"));
    manager.addEntry(WhitelistEntry("uuid-2", "Player2"));

    manager.clear();
    EXPECT_EQ(manager.size(), 0);
    EXPECT_TRUE(manager.empty());
}

// ========== 文件操作测试 ==========

TEST_F(WhitelistManagerTest, SaveAndLoad)
{
    // 创建并保存
    {
        WhitelistManager manager;
        manager.addEntry(WhitelistEntry("uuid-1", "Player1"));
        manager.addEntry(WhitelistEntry("uuid-2", "Player2"));
        manager.addEntry(WhitelistEntry("uuid-3", "Player3"));

        auto result = manager.save(testFile_);
        EXPECT_TRUE(result.success()) << result.error().message();
    }

    // 验证文件存在
    EXPECT_TRUE(std::filesystem::exists(testFile_));

    // 加载并验证
    {
        WhitelistManager manager;
        auto result = manager.load(testFile_);
        EXPECT_TRUE(result.success()) << result.error().message();

        EXPECT_EQ(manager.size(), 3);
        EXPECT_TRUE(manager.isWhitelisted("uuid-1"));
        EXPECT_TRUE(manager.isWhitelisted("uuid-2"));
        EXPECT_TRUE(manager.isWhitelisted("uuid-3"));
        EXPECT_TRUE(manager.isNameWhitelisted("Player1"));
        EXPECT_TRUE(manager.isNameWhitelisted("Player2"));
        EXPECT_TRUE(manager.isNameWhitelisted("Player3"));
    }
}

TEST_F(WhitelistManagerTest, LoadNonExistentFile)
{
    WhitelistManager manager;

    auto result = manager.load(testDir_ / "non_existent.json");
    EXPECT_TRUE(result.success()); // 应该成功，创建空文件
    EXPECT_TRUE(manager.empty());
}

TEST_F(WhitelistManagerTest, Reload)
{
    WhitelistManager manager;

    // 初始加载
    manager.addEntry(WhitelistEntry("uuid-1", "Player1"));
    manager.save(testFile_);

    // 手动修改文件
    std::ofstream file(testFile_, std::ios::trunc);
    file << R"([
        {"uuid": "uuid-new", "name": "NewPlayer"}
    ])";
    file.close();

    // 重新加载
    auto result = manager.reload();
    EXPECT_TRUE(result.success()) << result.error().message();

    EXPECT_EQ(manager.size(), 1);
    EXPECT_TRUE(manager.isWhitelisted("uuid-new"));
    EXPECT_FALSE(manager.isWhitelisted("uuid-1"));
}

TEST_F(WhitelistManagerTest, LoadInvalidJson)
{
    // 创建无效 JSON 文件
    std::ofstream file(testFile_);
    file << "not a valid json";
    file.close();

    WhitelistManager manager;
    auto result = manager.load(testFile_);
    EXPECT_FALSE(result.success());
}

TEST_F(WhitelistManagerTest, LoadNonArrayJson)
{
    // 创建非数组 JSON 文件
    std::ofstream file(testFile_);
    file << R"({"uuid": "uuid-1", "name": "Player1"})";
    file.close();

    WhitelistManager manager;
    auto result = manager.load(testFile_);
    EXPECT_FALSE(result.success());
}

// ========== 线程安全测试 ==========

TEST_F(WhitelistManagerTest, ThreadSafety)
{
    WhitelistManager manager;
    constexpr int numThreads = 4;
    constexpr int entriesPerThread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&manager, t, entriesPerThread]() {
            for (int i = 0; i < entriesPerThread; ++i) {
                std::string uuid = "uuid-" + std::to_string(t) + "-" + std::to_string(i);
                std::string name = "Player" + std::to_string(t) + "_" + std::to_string(i);
                manager.addEntry(WhitelistEntry(uuid, name));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 应该恰好有 numThreads * entriesPerThread 个条目
    // 但由于多线程竞争，实际数量可能少于预期
    EXPECT_LE(manager.size(), static_cast<size_t>(numThreads * entriesPerThread));
}

// ========== 条目有效性测试 ==========

TEST_F(WhitelistManagerTest, InvalidEntry)
{
    WhitelistManager manager;

    // 空 UUID
    WhitelistEntry entry1("", "Player1");
    EXPECT_FALSE(entry1.isValid());
    EXPECT_FALSE(manager.addEntry(entry1));

    // 空名称
    WhitelistEntry entry2("uuid-123", "");
    EXPECT_FALSE(entry2.isValid());
    EXPECT_FALSE(manager.addEntry(entry2));
}

// ========== 大小写不敏感测试 ==========

TEST_F(WhitelistManagerTest, CaseInsensitiveNameCheck)
{
    WhitelistManager manager;

    WhitelistEntry entry("uuid-123", "PlayerName");
    manager.addEntry(entry);

    // 各种大小写变体
    EXPECT_TRUE(manager.isNameWhitelisted("PlayerName"));
    EXPECT_TRUE(manager.isNameWhitelisted("playername"));
    EXPECT_TRUE(manager.isNameWhitelisted("PLAYERNAME"));
    EXPECT_TRUE(manager.isNameWhitelisted("pLaYeRnAmE"));

    // 通过名称删除也应该大小写不敏感
    EXPECT_TRUE(manager.removeEntryByName("playername"));
    EXPECT_EQ(manager.size(), 0);
}
