#pragma once

#include "../../../core/Types.hpp"
#include <cstdint>

namespace mc::world::storage {

/**
 * @brief 一致性模式
 *
 * 控制RocksDB写入的持久化级别。
 */
enum class ConsistencyMode : u8 {
    /// 强一致性：关键写入使用WAL+sync，事务跨列族原子性
    /// 适合：服务器关闭、重要操作
    Strong,

    /// 最终一致性：区块写入不sync，依赖后台压缩持久化
    /// 适合：正常游戏运行
    Eventual,

    /// 最强一致性：每个区块写入都sync
    /// 适合：崩溃测试、开发调试
    Strongest
};

/**
 * @brief 一致性配置
 *
 * 根据一致性模式返回适当的RocksDB配置。
 */
struct ConsistencyConfig {
    ConsistencyMode mode = ConsistencyMode::Strong;

    /**
     * @brief 是否启用WAL同步
     *
     * - Strong: true（关键写入）
     * - Eventual: false
     * - Strongest: true（所有写入）
     */
    [[nodiscard]] bool walSync() const noexcept
    {
        return mode == ConsistencyMode::Strong || mode == ConsistencyMode::Strongest;
    }

    /**
     * @brief 是否在每次写入后sync
     *
     * - Strong: false（仅关键写入sync）
     * - Eventual: false
     * - Strongest: true
     */
    [[nodiscard]] bool syncOnWrite() const noexcept { return mode == ConsistencyMode::Strongest; }
};

} // namespace mc::world::storage
