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

#include "server/world/storage/db/RocksDBDatabase.hpp"
#include "common/TempDirHelper.hpp"
#include "server/world/storage/db/ColumnFamilies.hpp"
#include "server/world/storage/db/ConsistencyMode.hpp"
#include "server/world/storage/db/RocksDBConfig.hpp"
#include <filesystem>
#include <string>
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

// ============================================================================
// 批次内 DeleteRange 与 Put 的顺序语义
// ============================================================================

/**
 * 回归测试：同一个 WriteBatch 内"先 DeleteRange 整段删除、再 Put 新行"必须得到
 * "旧行消失、新行存活"的结果。
 *
 * EntityStorageManager::replaceEntitiesInChunks 完全建立在这个语义之上：它把每个区块的
 * 整段删除与该区块存活实体的写入放进同一个批次。若 RocksDB 让先插入的 RangeTombstone
 * 覆盖后插入的 Put，那批内刚写进去的实体会被自己那条删除指令抹掉，表现为关服/卸载后
 * 区块里的实体全部消失——而且不会有任何报错。
 */
TEST_F(RocksDBDatabaseTest, WriteBatchDeleteRangeThenPutKeepsNewEntries)
{
    auto result = RocksDBDatabase::open(getDbPath("batch_range_delete"));
    ASSERT_TRUE(result.success()) << result.error().message();
    auto db = std::move(result.value());

    auto* cf = db->getCF(cf::ENTITIES_OVERWORLD);
    ASSERT_NE(cf, nullptr);

    const std::vector<u8> staleA{'0', ':', '0', ':', 'a'};
    const std::vector<u8> staleB{'0', ':', '0', ':', 'b'};
    const std::string freshKey = "0:0:c";

    // 预置两行"存档尸体"，模拟上一次会话遗留在该区块前缀下的记录
    ASSERT_TRUE(db->put(cf::ENTITIES_OVERWORLD, staleA, {9}).success());
    ASSERT_TRUE(db->put(cf::ENTITIES_OVERWORLD, staleB, {9}).success());
    ASSERT_TRUE(db->exists(cf::ENTITIES_OVERWORLD, staleA));
    ASSERT_TRUE(db->exists(cf::ENTITIES_OVERWORLD, staleB));

    const std::string prefix = "0:0:";
    const std::string endKey = prefix + static_cast<char>(0xFF);

    rocksdb::WriteBatch batch;
    batch.DeleteRange(cf, rocksdb::Slice(prefix.data(), prefix.size()), rocksdb::Slice(endKey.data(), endKey.size()));
    batch.Put(cf, rocksdb::Slice(freshKey.data(), freshKey.size()), rocksdb::Slice("value-c"));
    ASSERT_TRUE(db->writeBatch(batch, true).success());

    EXPECT_FALSE(db->exists(cf::ENTITIES_OVERWORLD, staleA)) << "批内的 RangeTombstone 未清除旧行";
    EXPECT_FALSE(db->exists(cf::ENTITIES_OVERWORLD, staleB)) << "批内的 RangeTombstone 未清除旧行";

    auto freshResult = db->get(cf::ENTITIES_OVERWORLD, rocksdb::Slice(freshKey.data(), freshKey.size()));
    ASSERT_TRUE(freshResult.success()) << "同一批内后写入的行被 RangeTombstone 一并抹掉了";
    EXPECT_EQ(freshResult.value(), (std::vector<u8>{'v', 'a', 'l', 'u', 'e', '-', 'c'}));

    db->close();
}

// ============================================================================
// 一致性模式对默认写入选项的映射
// ============================================================================

/**
 * 回归测试：ConsistencyMode 必须真正决定默认写入是否等待 WAL fsync。
 *
 * 历史缺陷：createWriteOptions() 构造了一个 ConsistencyConfig 却从未使用它，options.sync
 * 实际取自 RocksDBConfig::walSync（默认 true），于是服务端显式配置的
 * ConsistencyMode::Eventual 被完全架空——每次 put/deleteRange 都仍要等一次 fsync，
 * 关服 2048 个 section + 1089 次范围删除因此多花了约 10 秒。
 */
TEST_F(RocksDBDatabaseTest, ConsistencyModeControlsDefaultWriteSync)
{
    EXPECT_FALSE(consistencyModeSyncsEveryWrite(ConsistencyMode::Eventual));
    EXPECT_FALSE(consistencyModeSyncsEveryWrite(ConsistencyMode::Strong));
    EXPECT_TRUE(consistencyModeSyncsEveryWrite(ConsistencyMode::Strongest));

    for (const ConsistencyMode mode :
        {ConsistencyMode::Eventual, ConsistencyMode::Strong, ConsistencyMode::Strongest}) {
        RocksDBConfig config;
        config.consistencyMode = mode;

        const rocksdb::WriteOptions options = config.createWriteOptions();
        EXPECT_EQ(options.sync, consistencyModeSyncsEveryWrite(mode)) << "consistencyMode=" << static_cast<i32>(mode);

        // sync 只决定是否等待 fsync；WAL 本身始终写入，因此进程崩溃后仍可由 WAL 回放恢复
        EXPECT_FALSE(options.disableWAL) << "consistencyMode=" << static_cast<i32>(mode);
    }

    // 只有显式关闭 WAL 才会禁用 WAL 写入，与一致性模式无关
    RocksDBConfig walDisabled;
    walDisabled.enableWAL = false;
    EXPECT_TRUE(walDisabled.createWriteOptions().disableWAL);
}

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
