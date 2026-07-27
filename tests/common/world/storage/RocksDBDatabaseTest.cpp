/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "world/storage/db/RocksDBDatabase.hpp"
#include "common/TempDirHelper.hpp"
#include "world/storage/db/ColumnFamilies.hpp"
#include <filesystem>
#include <vector>
#include <gtest/gtest.h>

namespace mc::world::storage {
namespace {

// ============================================================================
// 测试基类：每个测试创建独立的临时数据库目录
// ============================================================================

class RocksDBDatabaseTest : public ::testing::Test {
protected:
    std::filesystem::path m_testDir;

    void SetUp() override
    {
        // PID + 纳秒 + 计数器组合，跨进程唯一；每个 TEST_F 独立子目录，避免并行抢锁
        m_testDir = mc::test::makeUniqueTestDir("mc_rocksdb_test");
    }

    void TearDown() override { mc::test::removeTestDir(m_testDir); }

    /// 获取本次测试的数据库路径（每个测试用例独立路径）
    std::filesystem::path getDbPath(const std::string& name) const { return m_testDir / name; }
};

// ============================================================================
// 打开 / 关闭
// ============================================================================

TEST_F(RocksDBDatabaseTest, OpenNewDatabase)
{
    auto result = RocksDBDatabase::open(getDbPath("new_db"));
    ASSERT_TRUE(result.success()) << result.error().message();
    auto db = std::move(result.value());
    ASSERT_NE(db, nullptr);
    EXPECT_TRUE(db->isOpen());
    EXPECT_GT(db->listColumnFamilies().size(), 0u);
}

TEST_F(RocksDBDatabaseTest, OpenExistingDatabase)
{
    auto path = getDbPath("existing_db");

    // 首次创建
    {
        auto result = RocksDBDatabase::open(path);
        ASSERT_TRUE(result.success()) << result.error().message();
        auto db = std::move(result.value());
        ASSERT_NE(db, nullptr);

        // 写入测试数据
        auto putResult = db->put(cf::META, {1, 2, 3}, {4, 5, 6});
        ASSERT_TRUE(putResult.success());

        // 关闭数据库
        db->close();
    }

    // 重新打开
    {
        auto result = RocksDBDatabase::open(path);
        ASSERT_TRUE(result.success()) << result.error().message();
        auto db = std::move(result.value());
        ASSERT_NE(db, nullptr);
        EXPECT_TRUE(db->isOpen());

        // 验证数据仍然存在
        auto getResult = db->get(cf::META, {1, 2, 3});
        ASSERT_TRUE(getResult.success());
        EXPECT_EQ(getResult.value(), (std::vector<u8>{4, 5, 6}));
    }
}

TEST_F(RocksDBDatabaseTest, OpenReadOnlyDatabase)
{
    auto path = getDbPath("readonly_db");

    // 先写入数据
    {
        auto result = RocksDBDatabase::open(path);
        ASSERT_TRUE(result.success()) << result.error().message();
        auto db = std::move(result.value());
        ASSERT_TRUE(db->put(cf::META, {10, 20}, {30, 40}).success());
    }

    // 以只读模式打开
    {
        auto result = RocksDBDatabase::openReadOnly(path);
        ASSERT_TRUE(result.success()) << result.error().message();
        auto db = std::move(result.value());
        EXPECT_TRUE(db->isOpen());

        // 读取成功
        auto getResult = db->get(cf::META, {10, 20});
        ASSERT_TRUE(getResult.success());
        EXPECT_EQ(getResult.value(), (std::vector<u8>{30, 40}));

        // 写入应失败（只读模式）
        auto putResult = db->put(cf::META, {1}, {2});
        EXPECT_FALSE(putResult.success());
    }
}

TEST_F(RocksDBDatabaseTest, OpenNonexistentReadOnly)
{
    auto result = RocksDBDatabase::openReadOnly(getDbPath("nonexistent_db"));
    EXPECT_FALSE(result.success());
}

TEST_F(RocksDBDatabaseTest, CloseDatabase)
{
    auto result = RocksDBDatabase::open(getDbPath("close_db"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());
    ASSERT_NE(db, nullptr);

    db->close();
    EXPECT_FALSE(db->isOpen());
}

TEST_F(RocksDBDatabaseTest, DoubleClose)
{
    auto result = RocksDBDatabase::open(getDbPath("double_close_db"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    db->close();
    EXPECT_FALSE(db->isOpen());

    // 二次关闭不应崩溃
    db->close();
    EXPECT_FALSE(db->isOpen());
}

// ============================================================================
// 列族管理
// ============================================================================

TEST_F(RocksDBDatabaseTest, AllColumnFamiliesCreatedOnNewDatabase)
{
    auto result = RocksDBDatabase::open(getDbPath("cf_all"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    auto cfList = db->listColumnFamilies();
    for (const auto& cfName : cf::ALL_COLUMN_FAMILIES) {
        EXPECT_TRUE(db->hasColumnFamily(cfName)) << "Missing column family: " << cfName;
    }

    // 验证列族数量不少于定义的常量
    EXPECT_GE(cfList.size(), cf::ALL_COLUMN_FAMILIES.size());
}

TEST_F(RocksDBDatabaseTest, ColumnFamilyGetReturnsValidHandle)
{
    auto result = RocksDBDatabase::open(getDbPath("cf_get"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    // 验证 getCF 返回非空句柄
    for (const auto& cfName : cf::ALL_COLUMN_FAMILIES) {
        auto* handle = db->getCF(cfName);
        EXPECT_NE(handle, nullptr) << "getCF returned null for: " << cfName;
    }
}

TEST_F(RocksDBDatabaseTest, ColumnFamilyGetReturnsNullForNonexistent)
{
    auto result = RocksDBDatabase::open(getDbPath("cf_nonexistent"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    auto* handle = db->getCF("nonexistent_column_family");
    EXPECT_EQ(handle, nullptr);
}

TEST_F(RocksDBDatabaseTest, HasColumnFamily)
{
    auto result = RocksDBDatabase::open(getDbPath("cf_has"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    EXPECT_TRUE(db->hasColumnFamily(cf::META));
    EXPECT_TRUE(db->hasColumnFamily(cf::SECTIONS_OVERWORLD));
    EXPECT_TRUE(db->hasColumnFamily(cf::PLAYERS));
    EXPECT_FALSE(db->hasColumnFamily("nonexistent"));
}

TEST_F(RocksDBDatabaseTest, DimensionAwareColumnFamilies)
{
    auto result = RocksDBDatabase::open(getDbPath("cf_dimension"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    // 主世界
    EXPECT_TRUE(db->hasColumnFamily(cf::getSectionCF(0)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getEntityCF(0)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getBlockEntityCF(0)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getPoiCF(0)));

    // 下界
    EXPECT_TRUE(db->hasColumnFamily(cf::getSectionCF(-1)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getEntityCF(-1)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getBlockEntityCF(-1)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getPoiCF(-1)));

    // 末地
    EXPECT_TRUE(db->hasColumnFamily(cf::getSectionCF(1)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getEntityCF(1)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getBlockEntityCF(1)));
    EXPECT_TRUE(db->hasColumnFamily(cf::getPoiCF(1)));
}

// ============================================================================
// 基础读写操作
// ============================================================================

TEST_F(RocksDBDatabaseTest, PutAndGet)
{
    auto result = RocksDBDatabase::open(getDbPath("put_get"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    std::vector<u8> key = {1, 2, 3, 4};
    std::vector<u8> value = {5, 6, 7, 8};

    auto putResult = db->put(cf::META, key, value);
    ASSERT_TRUE(putResult.success());

    auto getResult = db->get(cf::META, key);
    ASSERT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), value);
}

TEST_F(RocksDBDatabaseTest, GetNonexistentKey)
{
    auto result = RocksDBDatabase::open(getDbPath("get_nonexistent"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    auto getResult = db->get(cf::META, {99, 99, 99});
    EXPECT_FALSE(getResult.success());
}

TEST_F(RocksDBDatabaseTest, Delete)
{
    auto result = RocksDBDatabase::open(getDbPath("delete"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    std::vector<u8> key = {10, 20, 30};
    ASSERT_TRUE(db->put(cf::META, key, {40, 50}).success());

    auto delResult = db->del(cf::META, key);
    ASSERT_TRUE(delResult.success());

    auto getResult = db->get(cf::META, key);
    EXPECT_FALSE(getResult.success());
}

TEST_F(RocksDBDatabaseTest, Exists)
{
    auto result = RocksDBDatabase::open(getDbPath("exists"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    std::vector<u8> key = {100, 200};
    EXPECT_FALSE(db->exists(cf::META, key));

    ASSERT_TRUE(db->put(cf::META, key, {1}).success());
    EXPECT_TRUE(db->exists(cf::META, key));
}

TEST_F(RocksDBDatabaseTest, PutToNonexistentColumnFamilyFails)
{
    auto result = RocksDBDatabase::open(getDbPath("put_bad_cf"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    auto putResult = db->put("nonexistent_cf", {1}, {2});
    EXPECT_FALSE(putResult.success());
}

TEST_F(RocksDBDatabaseTest, GetFromNonexistentColumnFamilyFails)
{
    auto result = RocksDBDatabase::open(getDbPath("get_bad_cf"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    auto getResult = db->get("nonexistent_cf", {1});
    EXPECT_FALSE(getResult.success());
}

// ============================================================================
// 多列族操作
// ============================================================================

TEST_F(RocksDBDatabaseTest, WriteToDifferentColumnFamilies)
{
    auto result = RocksDBDatabase::open(getDbPath("multi_cf"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    std::vector<u8> key = {1, 2, 3};
    std::vector<u8> sectionsValue = {10, 20};
    std::vector<u8> entitiesValue = {30, 40};
    std::vector<u8> playersValue = {50, 60};

    ASSERT_TRUE(db->put(cf::SECTIONS_OVERWORLD, key, sectionsValue).success());
    ASSERT_TRUE(db->put(cf::ENTITIES_OVERWORLD, key, entitiesValue).success());
    ASSERT_TRUE(db->put(cf::PLAYERS, key, playersValue).success());

    // 相同 key 在不同列族中存储不同值
    auto r1 = db->get(cf::SECTIONS_OVERWORLD, key);
    ASSERT_TRUE(r1.success());
    EXPECT_EQ(r1.value(), sectionsValue);

    auto r2 = db->get(cf::ENTITIES_OVERWORLD, key);
    ASSERT_TRUE(r2.success());
    EXPECT_EQ(r2.value(), entitiesValue);

    auto r3 = db->get(cf::PLAYERS, key);
    ASSERT_TRUE(r3.success());
    EXPECT_EQ(r3.value(), playersValue);
}

// ============================================================================
// 批量操作
// ============================================================================

TEST_F(RocksDBDatabaseTest, WriteBatch)
{
    auto result = RocksDBDatabase::open(getDbPath("batch"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    rocksdb::WriteBatch batch;
    auto* cfMeta = db->getCF(cf::META);
    ASSERT_NE(cfMeta, nullptr);

    batch.Put(cfMeta, rocksdb::Slice("key1"), rocksdb::Slice("value1"));
    batch.Put(cfMeta, rocksdb::Slice("key2"), rocksdb::Slice("value2"));
    batch.Put(cfMeta, rocksdb::Slice("key3"), rocksdb::Slice("value3"));

    auto writeResult = db->writeBatch(batch);
    ASSERT_TRUE(writeResult.success());

    // 验证所有写入
    auto r1 = db->get(cf::META, std::vector<u8>{'k', 'e', 'y', '1'});
    ASSERT_TRUE(r1.success());
    EXPECT_EQ(r1.value(), (std::vector<u8>{'v', 'a', 'l', 'u', 'e', '1'}));
}

// ============================================================================
// 迭代器
// ============================================================================

TEST_F(RocksDBDatabaseTest, Iterator)
{
    auto result = RocksDBDatabase::open(getDbPath("iterator"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    // 写入多个键
    for (int i = 0; i < 5; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        ASSERT_TRUE(
            db->put(cf::META, std::vector<u8>(key.begin(), key.end()), std::vector<u8>(value.begin(), value.end()))
                .success());
    }

    // 使用迭代器遍历
    auto iter = db->newIterator(cf::META);
    ASSERT_NE(iter, nullptr);

    int count = 0;
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        count++;
    }
    EXPECT_EQ(count, 5);
}

// ============================================================================
// 快照
// ============================================================================

TEST_F(RocksDBDatabaseTest, Snapshot)
{
    auto result = RocksDBDatabase::open(getDbPath("snapshot"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    // 写入数据并创建快照
    ASSERT_TRUE(db->put(cf::META, {1}, {10}).success());
    const rocksdb::Snapshot* snapshot = db->createSnapshot();
    ASSERT_NE(snapshot, nullptr);

    // 在快照后修改数据
    ASSERT_TRUE(db->put(cf::META, {1}, {20}).success());

    // 新读取应返回更新后的值
    auto getResult = db->get(cf::META, {1});
    ASSERT_TRUE(getResult.success());
    EXPECT_EQ(getResult.value(), (std::vector<u8>{20}));

    // 释放快照
    db->releaseSnapshot(snapshot);
}

// ============================================================================
// 压缩和刷新
// ============================================================================

TEST_F(RocksDBDatabaseTest, CompactAndFlush)
{
    auto result = RocksDBDatabase::open(getDbPath("compact_flush"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    ASSERT_TRUE(db->put(cf::META, {1}, {2}).success());

    // 刷新单个列族
    auto flushResult = db->flush(cf::META);
    ASSERT_TRUE(flushResult.success());

    // 刷新所有列族
    flushResult = db->flush();
    ASSERT_TRUE(flushResult.success());

    // 压缩
    auto compactResult = db->compact(cf::META);
    ASSERT_TRUE(compactResult.success());
}

// ============================================================================
// 重新打开后列族保持
// ============================================================================

TEST_F(RocksDBDatabaseTest, ColumnFamiliesPreservedAfterReopen)
{
    auto path = getDbPath("cf_preserve");

    // 写入数据到多个列族
    {
        auto result = RocksDBDatabase::open(path);
        ASSERT_TRUE(result.success());
        auto db = std::move(result.value());

        ASSERT_TRUE(db->put(cf::SECTIONS_OVERWORLD, {1}, {10}).success());
        ASSERT_TRUE(db->put(cf::ENTITIES_OVERWORLD, {2}, {20}).success());
        ASSERT_TRUE(db->put(cf::PLAYERS, {3}, {30}).success());
    }

    // 重新打开后验证列族和数据
    {
        auto result = RocksDBDatabase::open(path);
        ASSERT_TRUE(result.success());
        auto db = std::move(result.value());

        // 列族仍然存在
        EXPECT_TRUE(db->hasColumnFamily(cf::SECTIONS_OVERWORLD));
        EXPECT_TRUE(db->hasColumnFamily(cf::ENTITIES_OVERWORLD));
        EXPECT_TRUE(db->hasColumnFamily(cf::PLAYERS));

        // 数据仍然存在
        auto r1 = db->get(cf::SECTIONS_OVERWORLD, {1});
        ASSERT_TRUE(r1.success());
        EXPECT_EQ(r1.value(), (std::vector<u8>{10}));

        auto r2 = db->get(cf::ENTITIES_OVERWORLD, {2});
        ASSERT_TRUE(r2.success());
        EXPECT_EQ(r2.value(), (std::vector<u8>{20}));

        auto r3 = db->get(cf::PLAYERS, {3});
        ASSERT_TRUE(r3.success());
        EXPECT_EQ(r3.value(), (std::vector<u8>{30}));
    }
}

// ============================================================================
// 统计信息和属性
// ============================================================================

TEST_F(RocksDBDatabaseTest, Statistics)
{
    auto result = RocksDBDatabase::open(getDbPath("stats"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    // 统计信息可能为空字符串（取决于配置），但不应崩溃
    std::string stats = db->getStatistics();
    (void)stats;
}

TEST_F(RocksDBDatabaseTest, Property)
{
    auto result = RocksDBDatabase::open(getDbPath("property"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());

    // 获取基本属性
    std::string basicStats = db->getProperty(cf::META, "rocksdb.stats");
    (void)basicStats;
}

// ============================================================================
// ClosedDatabase 操作
// ============================================================================

TEST_F(RocksDBDatabaseTest, OperationsOnClosedDatabase)
{
    auto result = RocksDBDatabase::open(getDbPath("closed_ops"));
    ASSERT_TRUE(result.success());
    auto db = std::move(result.value());
    db->close();

    // 关闭后的操作应返回错误
    EXPECT_FALSE(db->put(cf::META, {1}, {2}).success());
    EXPECT_FALSE(db->get(cf::META, {1}).success());
    EXPECT_FALSE(db->del(cf::META, {1}).success());
    EXPECT_FALSE(db->exists(cf::META, {1}));
    EXPECT_EQ(db->newIterator(cf::META), nullptr);
    EXPECT_FALSE(db->compact(cf::META).success());
    EXPECT_FALSE(db->flush(cf::META).success());
}

} // namespace
} // namespace mc::world::storage
