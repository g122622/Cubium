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

#include "common/core/Types.hpp"

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

namespace mc::server::console {

/**
 * @brief 右侧面板展示的单个在线玩家
 */
struct PlayerSummary {
    std::string username;      ///< 玩家名
    std::string dimensionName; ///< 所在维度名（如 "minecraft:overworld"）
    f64 x = 0.0;               ///< 坐标 X
    f64 y = 0.0;               ///< 坐标 Y
    f64 z = 0.0;               ///< 坐标 Z
    f32 health = 0.0f;         ///< 当前生命值
};

/**
 * @brief 右侧面板展示的服务器静态信息（启动时确定，不随运行变化）
 */
struct ServerInfo {
    std::string version;   ///< 版本号（如 "0.1.0"）
    std::string worldName; ///< 世界名（server options 中的 worldName）
    u16 port = 0;          ///< 监听端口
};

/**
 * @brief tick 线程采集、UI 线程读取的服务器指标快照
 *
 * 世界/玩家状态只能在 tick 线程访问，UI 线程不能直接读取，故由 post-tick 回调定期
 * 生成一份值语义快照交换给 UI 侧（互斥保护，读取时拷贝一份，写入频率 ≤ 1/秒）。
 */
struct ServerMetricsSnapshot {
    f64 tps = 0.0;                      ///< 最近统计出的 tick 速率
    f64 tickTimeMs = 0.0;               ///< 最近一次 tick 执行耗时（EMA 平滑）
    usize playerCount = 0;              ///< 在线玩家数
    usize sessionCount = 0;             ///< 远程会话数（TCP 连接，含尚未登录的）
    i64 chunkHolderCount = 0;           ///< 全部维度的区块票据数
    i64 loadedChunkCount = 0;           ///< 全部维度已加载区块数
    i64 tickingChunkCount = 0;          ///< 全部维度处于"方块可 tick"范围的区块数
    i64 entityCount = 0;                ///< 全部维度实体数
    usize blockEntityCount = 0;         ///< 全部维度已加载区块中的方块实体数
    u64 workingSetMiB = 0;              ///< 进程工作集（MiB）
    u64 commitMiB = 0;                  ///< 进程提交量（MiB）
    std::vector<PlayerSummary> players; ///< 在线玩家列表（按玩家名排序）
};

/**
 * @brief 指标快照的线程安全交换容器
 */
class ServerMetricsStore {
public:
    /**
     * @brief 写入最新快照（tick 线程调用）
     * @param snapshot 新快照（整体替换）
     */
    void update(ServerMetricsSnapshot snapshot);

    /**
     * @brief 读取最新快照副本（UI 线程调用）
     */
    [[nodiscard]] ServerMetricsSnapshot get() const;

private:
    mutable std::mutex m_mutex;
    ServerMetricsSnapshot m_snapshot;
};

} // namespace mc::server::console
