#pragma once

#include "ConsistencyMode.hpp"
#include <cstddef>
#include <vector>
#include <rocksdb/cache.h>
#include <rocksdb/compression_type.h>
#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/statistics.h>
#include <rocksdb/table.h>

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
    // 缓存配置
    // ========================================================================

    /// 块缓存大小（字节）
    /// 默认256MB
    size_t blockCacheSize = 256 * 1024 * 1024;

    /// 行缓存大小（字节）
    /// 默认64MB
    size_t rowCacheSize = 64 * 1024 * 1024;

    // ========================================================================
    // MemTable配置
    // ========================================================================

    /// MemTable大小（字节）
    /// 默认64MB
    size_t writeBufferSize = 64 * 1024 * 1024;

    /// 最大MemTable数量
    /// 默认4个（1个活跃+3个不可变）
    int maxWriteBufferNumber = 4;

    /// 合并前最小不可变MemTable数量
    int minWriteBufferNumberToMerge = 2;

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
    bool enableWAL = true;

    /// WAL同步模式
    /// - true: 每次写入后fsync
    /// - false: 依赖操作系统刷盘
    bool walSync = true;

    /// WAL文件复用数量
    /// 减少文件分配开销
    size_t recycleLogFileNum = 10;

    /// WAL TTL（秒）
    /// 0表示无限保留
    size_t walTtlSeconds = 0;

    // ========================================================================
    // 后台线程配置
    // ========================================================================

    /// 最大后台任务数
    int maxBackgroundJobs = 4;

    /// 最大后台压缩线程数
    int maxBackgroundCompactions = 2;

    /// 最大后台刷盘线程数
    int maxBackgroundFlushes = 2;

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

        // WAL配置
        options.disableWAL = !enableWAL;
        options.sync = walSync;

        // 一致性模式
        ConsistencyConfig consistency{consistencyMode};
        if (consistencyMode == ConsistencyMode::Strongest) {
            options.sync = true;
        }

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
