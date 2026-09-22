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

#pragma once

#include "ConsistencyMode.hpp"
#include <cstddef>
#include <memory>
#include <vector>
#include <rocksdb/cache.h>
#include <rocksdb/compression_type.h>
#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/statistics.h>
#include <rocksdb/table.h>
#include <rocksdb/write_buffer_manager.h>

namespace mc::world::storage {

/**
 * @brief RocksDB配置
 *
 * 包含所有RocksDB数据库配置选项。
 */
struct RocksDBConfig {
    // ========================================================================
    // 一致性配置
    // ========================================================================

    /// 一致性模式
    ConsistencyMode consistencyMode = ConsistencyMode::Strong;

    // ========================================================================
    // MemTable配置
    // ========================================================================
    //
    // 块缓存（BlockBasedTableOptions::block_cache）刻意不在此配置：RocksDB 在未显式设置时
    // 会自行创建一个 32MB 的共享 LRU 缓存，本项目依赖该默认值。本结构体此前声明过
    // blockCacheSize(256MB)/rowCacheSize(64MB) 两个字段，但从未被 createDBOptions 或
    // createColumnFamilyOptions 使用，属误导性死配置，已删除。
    // 若日后要显式调块缓存，必须让 16 个列族共享同一个 Cache 实例，否则会退化成 16 份独立缓存。

    /// 单个MemTable大小（字节）
    /// 8MB：本项目世界库的写入量很小，原值 64MB 明显偏大。该值与 maxWriteBufferNumber、
    /// 列族数三者相乘决定内存上界，故按小值设置。
    size_t writeBufferSize = 8 * 1024 * 1024;

    /// 最大MemTable数量（1 个活跃 + maxWriteBufferNumber-1 个不可变）
    int maxWriteBufferNumber = 2;

    /// 合并前最小不可变MemTable数量
    int minWriteBufferNumberToMerge = 2;

    /// 全部列族的 MemTable 内存上限（字节）
    ///
    /// 经 WriteBufferManager 施加跨列族的全局约束。没有它时，理论上界为
    /// writeBufferSize × maxWriteBufferNumber × 列族数，即便按上面调小的值也有约 256MB/库。
    size_t memtableMemoryLimit = 64 * 1024 * 1024;

    /// 允许打开的文件数上限
    ///
    /// RocksDB 默认 -1（不限制），其官方注释即提示该默认会显著占用内存。改为有界值以约束
    /// TableCache / BlobFileCache 的规模，代价是冷读需重新打开文件。
    int maxOpenFiles = 512;

    // ========================================================================
    // LSM树配置
    // ========================================================================

    /// LSM树层数
    int numLevels = 7;

    /// 目标文件大小基数（字节）
    /// 默认64MB
    size_t targetFileSizeBase = 64 * 1024 * 1024;

    /// 每层目标大小基数（字节）
    /// 默认256MB
    size_t maxBytesForLevelBase = 256 * 1024 * 1024;

    /// 每层大小倍数
    double maxBytesForLevelMultiplier = 10.0;

    // ========================================================================
    // 压缩配置
    // ========================================================================

    /// 每层压缩类型
    /// L0-L1: 无压缩（避免写放大，优先写入性能）
    /// L2+: ZSTD压缩（高压缩比，节省磁盘空间）
    std::vector<rocksdb::CompressionType> compressionPerLevel = {
        rocksdb::kNoCompression, // L0 - 频繁写入，不压缩
        rocksdb::kNoCompression, // L1 - 频繁写入，不压缩
        rocksdb::kZSTD,          // L2 - ZSTD压缩
        rocksdb::kZSTD,          // L3 - ZSTD压缩
        rocksdb::kZSTD,          // L4 - ZSTD压缩
        rocksdb::kZSTD,          // L5 - ZSTD压缩
        rocksdb::kZSTD           // L6 - ZSTD压缩
    };

    // ========================================================================
    // WAL配置
    // ========================================================================

    /// 是否启用WAL
    ///
    /// 关闭后进程崩溃即丢数据，只适合可重建的缓存型库；世界存档必须保持启用。
    /// 是否**等待 fsync** 由 consistencyMode 单独决定，不由本字段决定。
    bool enableWAL = true;

    /// WAL文件复用数量
    /// 减少文件分配开销
    size_t recycleLogFileNum = 10;

    /// WAL TTL（秒）
    /// 0表示无限保留
    size_t walTtlSeconds = 0;

    // ========================================================================
    // 后台线程配置
    // ========================================================================

    /// 最大后台任务数（RocksDB 据此自行切分压缩/刷盘线程，无需分别配置）
    int maxBackgroundJobs = 4;

    // ========================================================================
    // 统计与监控
    // ========================================================================

    /// 是否启用统计
    bool enableStatistics = true;

    /// 统计刷新间隔（毫秒）
    size_t statisticsDumpPeriodMs = 60000;

    // ========================================================================
    // Bloom过滤器配置
    // ========================================================================

    /// Bloom过滤器位数
    /// 每个key平均使用的bit数，越大误判率越低
    double bloomFilterBitsPerKey = 10.0;

    /// 是否为整体过滤器（减少内存占用）
    bool useWholeKeyBloomFilter = false;

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 创建RocksDB数据库选项
     */
    [[nodiscard]] rocksdb::DBOptions createDBOptions() const
    {
        rocksdb::DBOptions options;

        // 基本选项
        options.create_if_missing = true;
        options.create_missing_column_families = true;
        options.max_background_jobs = maxBackgroundJobs;
        options.max_open_files = maxOpenFiles;

        // 全部列族共享的 MemTable 内存上限：把「16 个列族 × 每族多个 MemTable」的最坏情况
        // 压到明确界内。第二个参数传 nullptr 表示不为 MemTable 指定计费用的块缓存。
        options.write_buffer_manager = std::make_shared<rocksdb::WriteBufferManager>(memtableMemoryLimit, nullptr);

        // WAL选项
        options.recycle_log_file_num = recycleLogFileNum;
        options.WAL_ttl_seconds = walTtlSeconds;

        // 统计
        if (enableStatistics) {
            options.statistics = rocksdb::CreateDBStatistics();
            options.stats_dump_period_sec = static_cast<unsigned int>(statisticsDumpPeriodMs / 1000);
        }

        return options;
    }

    /**
     * @brief 创建列族选项
     */
    [[nodiscard]] rocksdb::ColumnFamilyOptions createColumnFamilyOptions() const
    {
        rocksdb::ColumnFamilyOptions options;

        // MemTable
        options.write_buffer_size = writeBufferSize;
        options.max_write_buffer_number = maxWriteBufferNumber;
        options.min_write_buffer_number_to_merge = minWriteBufferNumberToMerge;

        // LSM树
        options.num_levels = numLevels;
        options.target_file_size_base = targetFileSizeBase;
        options.max_bytes_for_level_base = maxBytesForLevelBase;
        options.max_bytes_for_level_multiplier = maxBytesForLevelMultiplier;

        // 压缩
        if (compressionPerLevel.size() >= static_cast<size_t>(numLevels)) {
            options.compression_per_level = compressionPerLevel;
        }

        // 表选项（Bloom过滤器）
        rocksdb::BlockBasedTableOptions tableOptions;
        tableOptions.filter_policy.reset(rocksdb::NewBloomFilterPolicy(bloomFilterBitsPerKey, useWholeKeyBloomFilter));
        options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(tableOptions));

        return options;
    }

    /**
     * @brief 创建写入选项
     */
    [[nodiscard]] rocksdb::WriteOptions createWriteOptions() const
    {
        rocksdb::WriteOptions options;

        // WAL 是否写入由 enableWAL 决定；每次写入是否等 fsync 由一致性模式决定。
        // 这两件事必须分开表达：本函数此前把 sync 直接绑在单独一个布尔开关上，导致
        // ConsistencyMode::Eventual（服务端实际使用的模式）对写路径完全无效，每次
        // put/deleteRange 都白白付一次 fsync。
        options.disableWAL = !enableWAL;
        options.sync = consistencyModeSyncsEveryWrite(consistencyMode);

        return options;
    }

    /**
     * @brief 创建读取选项
     */
    [[nodiscard]] rocksdb::ReadOptions createReadOptions() const
    {
        rocksdb::ReadOptions options;
        // 默认读取选项
        return options;
    }
};

} // namespace mc::world::storage
