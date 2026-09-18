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

#include "server/core/BannedIpList.hpp"
#include "common/TempDirHelper.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::server::core;

class BannedIpListTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 助手以 PID 组合 token 保证跨进程唯一，避免 CTest -j16 并发覆盖
        testDir_ = mc::test::makeUniqueTestDir("mc_banned_ips_test");
        testFile_ = testDir_ / "banned-ips.json";
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

TEST_F(BannedIpListTest, DefaultState)
{
    BannedIpList banList;

    EXPECT_TRUE(banList.empty());
    EXPECT_EQ(banList.size(), 0);
}

TEST_F(BannedIpListTest, AddEntry)
{
    BannedIpList banList;

    BannedIpEntry entry("192.168.1.100", "2024-01-15 10:00:00 +0800", "ServerAdmin", "forever", "DDoS attack");
    EXPECT_TRUE(banList.addEntry(entry));
    EXPECT_EQ(banList.size(), 1);
    EXPECT_TRUE(banList.isBanned("192.168.1.100"));
}

TEST_F(BannedIpListTest, AddDuplicateEntry)
{
    BannedIpList banList;

    BannedIpEntry entry1("192.168.1.100", "2024-01-15 10:00:00 +0800", "ServerAdmin", "forever", "DDoS attack");
    EXPECT_TRUE(banList.addEntry(entry1));

    // 相同 IP 不能重复添加
    BannedIpEntry entry2("192.168.1.100", "2024-01-15 11:00:00 +0800", "ServerAdmin", "forever", "Spam");
    EXPECT_FALSE(banList.addEntry(entry2));
    EXPECT_EQ(banList.size(), 1);
}

TEST_F(BannedIpListTest, RemoveEntry)
{
    BannedIpList banList;

    BannedIpEntry entry("192.168.1.100", "2024-01-15 10:00:00 +0800", "ServerAdmin", "forever", "DDoS attack");
    banList.addEntry(entry);

    EXPECT_TRUE(banList.removeEntry("192.168.1.100"));
    EXPECT_EQ(banList.size(), 0);
    EXPECT_FALSE(banList.isBanned("192.168.1.100"));
}

TEST_F(BannedIpListTest, RemoveNonExistentEntry)
{
    BannedIpList banList;

    EXPECT_FALSE(banList.removeEntry("192.168.1.100"));
}

TEST_F(BannedIpListTest, GetEntry)
{
    BannedIpList banList;

    BannedIpEntry entry("192.168.1.100", "2024-01-15 10:00:00 +0800", "ServerAdmin", "forever", "DDoS attack");
    banList.addEntry(entry);

    auto result = banList.getEntry("192.168.1.100");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->ip, "192.168.1.100");
    EXPECT_EQ(result->source, "ServerAdmin");
    EXPECT_EQ(result->reason, "DDoS attack");
    EXPECT_EQ(result->expires, "forever");

    auto result2 = banList.getEntry("192.168.1.200");
    EXPECT_FALSE(result2.has_value());
}

TEST_F(BannedIpListTest, GetAllEntries)
{
    BannedIpList banList;

    banList.addEntry(BannedIpEntry("192.168.1.100", "2024-01-15 10:00:00 +0800", "Admin1", "forever", "Reason1"));
    banList.addEntry(BannedIpEntry("192.168.1.101", "2024-01-15 11:00:00 +0800", "Admin2", "forever", "Reason2"));
    banList.addEntry(BannedIpEntry("192.168.1.102", "2024-01-15 12:00:00 +0800", "Admin3", "forever", "Reason3"));

    auto entries = banList.getAllEntries();
    EXPECT_EQ(entries.size(), 3);
}

TEST_F(BannedIpListTest, GetAllBannedIps)
{
    BannedIpList banList;

    banList.addEntry(BannedIpEntry("192.168.1.100", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Reason"));
    banList.addEntry(BannedIpEntry("192.168.1.101", "2024-01-15 11:00:00 +0800", "Admin", "forever", "Reason"));

    auto ips = banList.getAllBannedIps();
    EXPECT_EQ(ips.size(), 2);

    // 验证 IP 存在
    EXPECT_NE(std::find(ips.begin(), ips.end(), "192.168.1.100"), ips.end());
    EXPECT_NE(std::find(ips.begin(), ips.end(), "192.168.1.101"), ips.end());
}

TEST_F(BannedIpListTest, Clear)
{
    BannedIpList banList;

    banList.addEntry(BannedIpEntry("192.168.1.100", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Reason"));
    banList.addEntry(BannedIpEntry("192.168.1.101", "2024-01-15 11:00:00 +0800", "Admin", "forever", "Reason"));

    banList.clear();
    EXPECT_EQ(banList.size(), 0);
    EXPECT_TRUE(banList.empty());
}

// ========== 条目有效性测试 ==========

TEST_F(BannedIpListTest, InvalidEntry)
{
    BannedIpList banList;

    // 空 IP
    BannedIpEntry entry1("", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Reason");
    EXPECT_FALSE(entry1.isValid());
    EXPECT_FALSE(banList.addEntry(entry1));
}

TEST_F(BannedIpListTest, EntryGetDisplayName)
{
    BannedIpEntry entry("192.168.1.100", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Reason");
    EXPECT_EQ(entry.getDisplayName(), "192.168.1.100");
}

// ========== 文件操作测试 ==========

TEST_F(BannedIpListTest, SaveAndLoad)
{
    // 创建并保存
    {
        BannedIpList banList;
        banList.addEntry(
            BannedIpEntry("192.168.1.100", "2024-01-15 10:00:00 +0800", "Admin1", "forever", "DDoS attack"));
        banList.addEntry(BannedIpEntry(
            "192.168.1.101", "2024-01-15 11:00:00 +0800", "Admin2", "2099-12-31 23:59:59 +0800", "Temporary ban"));
        banList.addEntry(BannedIpEntry("10.0.0.50", "2024-01-15 12:00:00 +0800", "Admin3", "forever", "Spam"));

        auto result = banList.save(testFile_);
        EXPECT_TRUE(result.success()) << result.error().message();
    }

    // 验证文件存在
    EXPECT_TRUE(std::filesystem::exists(testFile_));

    // 加载并验证
    {
        BannedIpList banList;
        auto result = banList.load(testFile_);
        EXPECT_TRUE(result.success()) << result.error().message();

        EXPECT_EQ(banList.size(), 3);
        EXPECT_TRUE(banList.isBanned("192.168.1.100"));
        EXPECT_TRUE(banList.isBanned("192.168.1.101"));
        EXPECT_TRUE(banList.isBanned("10.0.0.50"));

        // 验证详细信息
        auto entry = banList.getEntry("192.168.1.100");
        EXPECT_TRUE(entry.has_value());
        EXPECT_EQ(entry->ip, "192.168.1.100");
        EXPECT_EQ(entry->source, "Admin1");
        EXPECT_EQ(entry->reason, "DDoS attack");
        EXPECT_EQ(entry->expires, "forever");
    }
}

TEST_F(BannedIpListTest, LoadNonExistentFile)
{
    BannedIpList banList;

    auto result = banList.load(testDir_ / "non_existent.json");
    EXPECT_TRUE(result.success()); // 应该成功，创建空列表
    EXPECT_TRUE(banList.empty());
}

TEST_F(BannedIpListTest, Reload)
{
    BannedIpList banList;

    // 初始保存
    banList.addEntry(BannedIpEntry("192.168.1.100", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Reason"));
    banList.save(testFile_);

    // 手动修改文件
    std::ofstream file(testFile_, std::ios::trunc);
    file << R"([
        {"ip": "10.0.0.1", "created": "2024-01-15 10:00:00 +0800", "source": "Admin", "expires": "forever", "reason": "Test"}
    ])";
    file.close();

    // 重新加载
    auto result = banList.reload();
    EXPECT_TRUE(result.success()) << result.error().message();

    EXPECT_EQ(banList.size(), 1);
    EXPECT_TRUE(banList.isBanned("10.0.0.1"));
    EXPECT_FALSE(banList.isBanned("192.168.1.100"));
}

TEST_F(BannedIpListTest, LoadInvalidJson)
{
    // 创建无效 JSON 文件
    std::ofstream file(testFile_);
    file << "not a valid json";
    file.close();

    BannedIpList banList;
    auto result = banList.load(testFile_);
    EXPECT_FALSE(result.success());
}

TEST_F(BannedIpListTest, LoadNonArrayJson)
{
    // 创建非数组 JSON 文件
    std::ofstream file(testFile_);
    file << R"({"ip": "192.168.1.100", "source": "Admin"})";
    file.close();

    BannedIpList banList;
    auto result = banList.load(testFile_);
    EXPECT_FALSE(result.success());
}

TEST_F(BannedIpListTest, LoadWithMissingFields)
{
    // 创建缺少字段的 JSON 文件
    std::ofstream file(testFile_);
    file << R"([
        {"ip": "192.168.1.100"},
        {"ip": "192.168.1.101", "created": "2024-01-15 10:00:00 +0800"}
    ])";
    file.close();

    BannedIpList banList;
    auto result = banList.load(testFile_);
    EXPECT_TRUE(result.success()); // 缺少可选字段应该被接受
    EXPECT_EQ(banList.size(), 2);
}

// ========== 线程安全测试 ==========

TEST_F(BannedIpListTest, ThreadSafety)
{
    BannedIpList banList;
    constexpr int numThreads = 4;
    constexpr int entriesPerThread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&banList, t, entriesPerThread]() {
            for (int i = 0; i < entriesPerThread; ++i) {
                std::string ip = "192.168." + std::to_string(t) + "." + std::to_string(i);
                banList.addEntry(BannedIpEntry(ip, "2024-01-15 10:00:00 +0800", "Admin", "forever", "Test"));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 应该恰好有 numThreads * entriesPerThread 个条目
    EXPECT_LE(banList.size(), static_cast<size_t>(numThreads * entriesPerThread));
}

// ========== 各种 IP 格式测试 ==========

TEST_F(BannedIpListTest, VariousIpFormats)
{
    BannedIpList banList;

    // IPv4 地址
    EXPECT_TRUE(banList.addEntry(BannedIpEntry("127.0.0.1", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Test")));
    EXPECT_TRUE(banList.addEntry(BannedIpEntry("0.0.0.0", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Test")));
    EXPECT_TRUE(
        banList.addEntry(BannedIpEntry("255.255.255.255", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Test")));
    EXPECT_TRUE(
        banList.addEntry(BannedIpEntry("192.168.1.1", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Test")));
    EXPECT_TRUE(banList.addEntry(BannedIpEntry("10.0.0.1", "2024-01-15 10:00:00 +0800", "Admin", "forever", "Test")));

    EXPECT_EQ(banList.size(), 5);

    // 验证各种 IP 都能正确查询
    EXPECT_TRUE(banList.isBanned("127.0.0.1"));
    EXPECT_TRUE(banList.isBanned("0.0.0.0"));
    EXPECT_TRUE(banList.isBanned("255.255.255.255"));
    EXPECT_TRUE(banList.isBanned("192.168.1.1"));
    EXPECT_TRUE(banList.isBanned("10.0.0.1"));
}

// ========== 复杂场景测试 ==========

TEST_F(BannedIpListTest, AddRemoveMultiple)
{
    BannedIpList banList;

    // 添加多个
    banList.addEntry(BannedIpEntry("192.168.1.100", "2024-01-15 10:00:00 +0800", "Admin", "forever", "R1"));
    banList.addEntry(BannedIpEntry("192.168.1.101", "2024-01-15 11:00:00 +0800", "Admin", "forever", "R2"));
    banList.addEntry(BannedIpEntry("192.168.1.102", "2024-01-15 12:00:00 +0800", "Admin", "forever", "R3"));

    EXPECT_EQ(banList.size(), 3);

    // 删除中间的
    EXPECT_TRUE(banList.removeEntry("192.168.1.101"));
    EXPECT_EQ(banList.size(), 2);
    EXPECT_FALSE(banList.isBanned("192.168.1.101"));
    EXPECT_TRUE(banList.isBanned("192.168.1.100"));
    EXPECT_TRUE(banList.isBanned("192.168.1.102"));

    // 再次添加已删除的
    EXPECT_TRUE(
        banList.addEntry(BannedIpEntry("192.168.1.101", "2024-01-15 13:00:00 +0800", "Admin", "forever", "R2-new")));
    EXPECT_EQ(banList.size(), 3);
    EXPECT_TRUE(banList.isBanned("192.168.1.101"));
}

TEST_F(BannedIpListTest, FilePathTracking)
{
    BannedIpList banList;

    banList.load(testFile_);
    EXPECT_EQ(banList.filePath(), testFile_);

    // reload 应该使用上次加载的路径
    std::ofstream file(testFile_);
    file
        << R"([{"ip": "192.168.1.100", "created": "2024-01-15 10:00:00 +0800", "source": "Admin", "expires": "forever", "reason": "Test"}])";
    file.close();

    auto result = banList.reload();
    EXPECT_TRUE(result.success());
    EXPECT_EQ(banList.size(), 1);
}

// ========== JSON 完整性测试 ==========

TEST_F(BannedIpListTest, JsonRoundTrip)
{
    BannedIpList banList;

    // 添加各种特殊字符的条目
    banList.addEntry(BannedIpEntry("192.168.1.100",
        "2024-01-15 10:00:00 +0800",
        "Admin \"Quoted\"",
        "forever",
        "Reason with \"quotes\" and \\ backslash"));
    banList.addEntry(BannedIpEntry(
        "192.168.1.101", "2024-01-15 11:00:00 +0800", "Admin\nNewLine", "forever", "Reason\nwith\nnewlines"));

    // 保存
    auto saveResult = banList.save(testFile_);
    EXPECT_TRUE(saveResult.success());

    // 加载
    BannedIpList loadedList;
    auto loadResult = loadedList.load(testFile_);
    EXPECT_TRUE(loadResult.success());

    EXPECT_EQ(loadedList.size(), 2);

    // 验证特殊字符被正确保存和加载
    auto entry1 = loadedList.getEntry("192.168.1.100");
    EXPECT_TRUE(entry1.has_value());
    EXPECT_EQ(entry1->source, "Admin \"Quoted\"");
    EXPECT_EQ(entry1->reason, "Reason with \"quotes\" and \\ backslash");

    auto entry2 = loadedList.getEntry("192.168.1.101");
    EXPECT_TRUE(entry2.has_value());
    EXPECT_EQ(entry2->source, "Admin\nNewLine");
    EXPECT_EQ(entry2->reason, "Reason\nwith\nnewlines");
}
