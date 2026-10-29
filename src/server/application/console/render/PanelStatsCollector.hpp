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
#include "server/application/console/render/PanelStats.hpp"

#include <string>
#include <vector>

namespace mc::server {
class MinecraftServer;
class ServerWorld;
} // namespace mc::server

namespace mc::server::console {

/**
 * @brief 右侧面板自定义段的统计参数
 */
struct PanelStatsOptions {
    /// 方块/群系 Top-N 的条目上限
    usize topEntries = 5;
    /// 单个维度最多统计的区块数（防止加载区块过多时命令卡顿）
    usize maxChunksScanned = 4096;
};

/**
 * @brief 采集右侧面板自定义段的内容
 *
 * 全部世界访问都发生在这里，**必须在 tick 线程调用**（由 `/panel` 命令在命令队列中
 * 执行）：面板的周期指标（TPS/内存等）走的是独立的轻量快照，而方块/群系直方图需要
 * 逐个扫描已加载区块，只在用户要求刷新时才付这份成本。
 */
class PanelStatsCollector {
public:
    /**
     * @brief 构造采集器
     * @param server 服务器实例（不得为空，且须在本次调用期间保持存活）
     * @param options 统计参数
     */
    PanelStatsCollector(MinecraftServer& server, PanelStatsOptions options);

    /**
     * @brief 采集全部维度并生成面板行
     *
     * 每个维度输出一段：区块状态、方块 Top-N、群系 Top-N。
     *
     * @return 面板行（可直接交给 PanelDetailStore::update）
     */
    [[nodiscard]] std::vector<std::string> collect();

private:
    /// 单个维度的扫描结果
    struct ScanResult {
        ChunkStatusStats status;
        std::vector<std::pair<std::string, i64>> blockCounts;
        std::vector<std::pair<std::string, i64>> biomeCounts;
        usize scannedChunks = 0;
        usize totalChunks = 0;
    };

    /**
     * @brief 扫描单个维度的全部已加载区块
     * @param world 维度世界
     */
    [[nodiscard]] ScanResult _scanWorld(ServerWorld& world);

    MinecraftServer& m_server;
    PanelStatsOptions m_options;
};

} // namespace mc::server::console
