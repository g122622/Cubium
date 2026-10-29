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

#include "StatusPanelComponent.hpp"

#include "CommandLineComponent.hpp"
#include "PanelDetailStore.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>

namespace mc::server::console {

namespace {

/// 把秒数格式化为 "1d 02:03:04" 形式的运行时长。
std::string _formatUptime(i64 seconds)
{
    const i64 days = seconds / 86400;
    const i64 hours = (seconds % 86400) / 3600;
    const i64 minutes = (seconds % 3600) / 60;
    const i64 secs = seconds % 60;

    std::string text;
    if (days > 0) {
        text = std::to_string(days) + "d ";
    }
    return text + fmt::format("{:02}:{:02}:{:02}", hours, minutes, secs);
}

/// 维度资源位置去掉 "minecraft:" 前缀，节省面板宽度。
std::string _shortDimension(std::string_view name)
{
    constexpr std::string_view NAMESPACE_PREFIX = "minecraft:";
    if (name.starts_with(NAMESPACE_PREFIX)) {
        return std::string(name.substr(NAMESPACE_PREFIX.size()));
    }
    return std::string(name);
}

/// 把 MiB 数值格式化为一位小数 + 单位（数值本身来自整 MiB 统计）。
std::string _formatMiB(u64 mib)
{
    return fmt::format("{:.1f} MiB", static_cast<f64>(mib));
}

ftxui::Element _sectionTitle(std::string_view title)
{
    return ftxui::text(std::string(title)) | ftxui::bold | ftxui::color(ftxui::Color::YellowLight);
}

/**
 * @brief 段落标题 + 可折叠的正文行
 *
 * 正文行数受面板高度约束：放不下时整段省略正文，只留标题与一行省略提示，保证下方的
 * 玩家列表（变化最频繁、最需要观察）不会被挤掉。
 *
 * @param title 段落标题
 * @param body 正文行（第一行是固定可见的"省略提示"之前的所有候选）
 * @param budget 本段可用行数（含标题行）
 */
ftxui::Elements _section(std::string_view title, std::vector<std::string> body, i32 budget)
{
    ftxui::Elements rows;
    const i32 needed = static_cast<i32>(body.size()) + 1;
    if (budget < needed) {
        // 放不下完整正文：整段省略，避免留下"没有内容的段落标题"占用高度。
        return rows;
    }
    rows.push_back(_sectionTitle(title));
    for (auto& line : body) {
        rows.push_back(ftxui::text(std::move(line)));
    }
    return rows;
}

/// 玩家列表最多展示的行数：超出时尾部显示 "and N more"，避免挤压其它分段。
constexpr usize MAX_PLAYER_ROWS = 10;

/// 面板各段之间的分隔线条数（四段之间各一条）。
constexpr i32 SEPARATOR_ROWS = 4;

/// `[refresh panel]` 按钮在面板内的固定位置：标题行之后居中，跨前后两行便于点击。
constexpr const char* REFRESH_LABEL = "[refresh panel]";

} // namespace

StatusPanelComponent::StatusPanelComponent(const ServerMetricsStore& metrics,
    const PanelDetailStore& details,
    CommandLineComponent& commandLine,
    ServerInfo serverInfo,
    std::chrono::steady_clock::time_point startedAt)
    : m_metrics(metrics)
    , m_details(details)
    , m_commandLine(commandLine)
    , m_serverInfo(std::move(serverInfo))
    , m_startedAt(startedAt)
{}

ftxui::Element StatusPanelComponent::OnRender()
{
    const ServerMetricsSnapshot snapshot = m_metrics.get();
    const std::vector<std::string> detailLines = m_details.get();

    const auto uptime =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_startedAt);
    const std::string uptimeText = _formatUptime(static_cast<i64>(uptime.count()));

    // ---- 服务器信息 ----
    std::vector<std::string> serverBody;
    serverBody.push_back(" ver    " + m_serverInfo.version);
    serverBody.push_back(" port   " + std::to_string(m_serverInfo.port));
    serverBody.push_back(" world  " + m_serverInfo.worldName);
    serverBody.push_back(" uptime " + uptimeText);

    // ---- 运行指标 ----
    std::vector<std::string> metricBody;
    metricBody.push_back(fmt::format(" tps          {:5.1f}", snapshot.tps));
    metricBody.push_back(fmt::format(" tick         {:5.2f} ms", snapshot.tickTimeMs));
    metricBody.push_back(fmt::format(" players      {}", snapshot.playerCount));
    metricBody.push_back(fmt::format(" sessions     {}", snapshot.sessionCount));
    metricBody.push_back(fmt::format(" chunks       {}", snapshot.loadedChunkCount));
    metricBody.push_back(fmt::format(" ticking      {}", snapshot.tickingChunkCount));
    metricBody.push_back(fmt::format(" entities     {}", snapshot.entityCount));
    metricBody.push_back(fmt::format(" block ents   {}", snapshot.blockEntityCount));

    // ---- 自定义指标（`/panel` 刷新；未刷新时给出一行提示）----
    std::vector<std::string> detailBody = detailLines;
    if (detailBody.empty()) {
        detailBody.push_back(" not refreshed yet");
    }

    // ---- 内存 ----
    std::vector<std::string> memoryBody;
    memoryBody.push_back(" working " + _formatMiB(snapshot.workingSetMiB));
    memoryBody.push_back(" commit  " + _formatMiB(snapshot.commitMiB));

    // ---- 在线玩家 ----
    std::vector<std::string> playerBody;
    if (snapshot.players.empty()) {
        playerBody.push_back(" (none)");
    } else {
        const usize shown = std::min(snapshot.players.size(), MAX_PLAYER_ROWS);
        for (usize index = 0; index < shown; ++index) {
            const auto& player = snapshot.players[index];
            playerBody.push_back(" " + player.username);
            playerBody.push_back(fmt::format("  {} {} {:.0f},{:.0f},{:.0f}",
                _shortDimension(player.dimensionName),
                fmt::format("{:.1f}hp", player.health),
                player.x,
                player.y,
                player.z));
        }
        if (snapshot.players.size() > shown) {
            playerBody.push_back(" +" + std::to_string(snapshot.players.size() - shown) + " more");
        }
    }

    // ---- 高度预算：玩家段优先保底，其余段按序分配 ----
    const i32 totalRows = std::max<i32>(m_box.y_max - m_box.y_min + 1, 0);
    i32 budget = std::max<i32>(totalRows - SEPARATOR_ROWS, 0);

    const auto costOf = [](const std::vector<std::string>& body) {
        return static_cast<i32>(body.size()) + 1;
    };
    const i32 playerBudget = std::min(costOf(playerBody), budget);
    budget -= playerBudget;

    const i32 serverBudget = std::min(costOf(serverBody), budget);
    budget -= serverBudget;
    const i32 metricBudget = std::min(costOf(metricBody), budget);
    budget -= metricBudget;
    const i32 detailBudget = std::min(costOf(detailBody), budget);
    budget -= detailBudget;
    const i32 memoryBudget = std::min(costOf(memoryBody), budget);

    ftxui::Elements rows;
    const auto append = [&rows](ftxui::Elements section) {
        if (section.empty()) {
            return;
        }
        if (!rows.empty()) {
            rows.push_back(ftxui::separator());
        }
        for (auto& row : section) {
            rows.push_back(std::move(row));
        }
    };

    append(_section("Server", std::move(serverBody), serverBudget));
    append(_section("Metrics", std::move(metricBody), metricBudget));

    {
        // 自定义指标段：标题行右侧挂 `[refresh panel]` 按钮（点击后把命令填进输入框）。
        ftxui::Elements section = _section("Panel", std::move(detailBody), detailBudget);
        if (!section.empty()) {
            section.front() = ftxui::hbox({
                std::move(section.front()),
                ftxui::filler(),
                ftxui::text(REFRESH_LABEL) | ftxui::bold | ftxui::color(ftxui::Color::CyanLight) |
                    ftxui::reflect(m_refreshBox),
            });
        }
        append(std::move(section));
    }

    append(_section("Memory", std::move(memoryBody), memoryBudget));
    append(_section("Players", std::move(playerBody), playerBudget));

    return ftxui::vbox(std::move(rows)) | ftxui::yflex | ftxui::reflect(m_box);
}

bool StatusPanelComponent::OnEvent(ftxui::Event event)
{
    if (!event.is_mouse()) {
        return false;
    }
    // v6 的 Event::mouse() 是非 const 成员，取非 const 引用来读坐标。
    const auto& mouse = const_cast<ftxui::Event&>(event).mouse();
    if (mouse.button != ftxui::Mouse::Left || mouse.motion != ftxui::Mouse::Pressed) {
        return false;
    }
    if (!m_refreshBox.Contain(mouse.x, mouse.y)) {
        return false;
    }

    // 点击按钮 = 把 `panel` 命令填进输入框（光标停行尾），焦点交还输入框：
    // 用户再按一次 Enter 即可提交。这样做的好处是"点击"没有副作用，
    // 派生的输入仍可编辑，命令也仍然经过统一的提交/历史链路。
    m_commandLine.setInput("/panel");
    m_commandLine.takeFocus();
    return true;
}

} // namespace mc::server::console
