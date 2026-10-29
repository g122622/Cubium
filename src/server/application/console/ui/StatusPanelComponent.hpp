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
#include "server/application/console/metrics/ServerMetricsSnapshot.hpp"

#include <chrono>
#include <string>
#include <vector>
#include <ftxui/component/component_base.hpp>
#include <ftxui/screen/box.hpp>

namespace mc::server::console {

class CommandLineComponent;
class PanelDetailStore;

/**
 * @brief 右侧信息面板
 *
 * 分五段常驻展示：服务器信息（版本/端口/世界/运行时长）、运行指标（TPS/tick 耗时/
 * 玩家/区块/实体）、自定义指标（`/panel` 刷新的世界状态摘要）、内存占用、在线玩家列表。
 * 指标来自 tick 线程定期写入的快照，本组件只读，不触碰世界。
 *
 * 每段行数受面板高度约束：行数多于可用高度时整段省略，只保留段标题与一行省略提示，
 * 确保下方的玩家列表（变化最频繁、最需要观察）始终可见。
 */
class StatusPanelComponent final : public ftxui::ComponentBase {
public:
    /**
     * @brief 构造面板
     * @param metrics 指标快照存储（须比本对象存活更久）
     * @param details 自定义指标行存储（须比本对象存活更久）
     * @param commandLine 输入框组件（面板内的 `[refresh panel]` 按钮点击后聚焦它）
     * @param serverInfo 服务器静态信息（版本、端口、世界名）
     * @param startedAt 服务端启动时刻，用于计算运行时长
     */
    StatusPanelComponent(const ServerMetricsStore& metrics,
        const PanelDetailStore& details,
        CommandLineComponent& commandLine,
        ServerInfo serverInfo,
        std::chrono::steady_clock::time_point startedAt);

    ftxui::Element OnRender() override;

    /**
     * @brief 处理鼠标点击（面板内按钮）
     *
     * @param event 待处理事件
     * @return 是否已消费
     */
    bool OnEvent(ftxui::Event event) override;

private:
    const ServerMetricsStore& m_metrics;
    const PanelDetailStore& m_details;
    CommandLineComponent& m_commandLine;
    const ServerInfo m_serverInfo;
    const std::chrono::steady_clock::time_point m_startedAt;

    /// `[refresh panel]` 按钮的包围盒（渲染时回填），用于命中判定。
    ftxui::Box m_refreshBox;
    /// 面板整体包围盒（渲染时由 reflect 回填），用于按可用高度折叠段落。
    ftxui::Box m_box;
};

} // namespace mc::server::console
