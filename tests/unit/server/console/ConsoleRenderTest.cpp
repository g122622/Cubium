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

/**
 * @file ConsoleRenderTest.cpp
 * @brief 交互式终端渲染层纯逻辑测试（折行、宽度、面板统计文本）
 *
 * 这些函数决定了日志区如何折行、面板数字如何显示，出错会直接体现为"日志被截断"
 * 或"数字错位"，且不需要终端即可验证，故单独覆盖。
 */

#include <gtest/gtest.h>

#include "server/application/console/render/LineWrapper.hpp"
#include "server/application/console/render/PanelStats.hpp"

#include <string>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::server::console;

namespace {

/// 把折行结果拼回一行（仅用于验证"不丢字符"）。
std::string joinLines(const std::vector<std::string>& lines)
{
    std::string joined;
    for (const auto& line : lines) {
        joined += line;
    }
    return joined;
}

} // namespace

// ============================================================================
// displayWidth
// ============================================================================

TEST(ConsoleLineWrapperTest, DisplayWidthCountsWideCharactersAsTwo)
{
    EXPECT_EQ(displayWidth(""), 0u);
    EXPECT_EQ(displayWidth("log"), 3u);
    // 中文全角：每个字 2 列。
    EXPECT_EQ(displayWidth("你好"), 4u);
    EXPECT_EQ(displayWidth("hi 你"), 5u);
}

TEST(ConsoleLineWrapperTest, DisplayWidthToleratesInvalidUtf8)
{
    // 截断的多字节序列不能抛异常、不能越界：按 1 列处理。
    const std::string broken = "\xE4\xBD";
    EXPECT_EQ(displayWidth(broken), 2u);
}

// ============================================================================
// wrapByDisplayWidth
// ============================================================================

TEST(ConsoleLineWrapperTest, WrapKeepsShortLineIntact)
{
    const auto lines = wrapByDisplayWidth("short line", 40);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "short line");
}

TEST(ConsoleLineWrapperTest, WrapBreaksAtSpacesWithoutLosingText)
{
    const std::string text = "alpha beta gamma delta";
    const auto lines = wrapByDisplayWidth(text, 12);

    ASSERT_GE(lines.size(), 2u);
    for (const auto& line : lines) {
        EXPECT_LE(displayWidth(line), 12u) << "line exceeded width: " << line;
    }
    EXPECT_EQ(joinLines(lines), text);
}

TEST(ConsoleLineWrapperTest, WrapHardSplitsUnspacedText)
{
    // 中文没有空格可断：必须按字符硬切，且每个显示行不超过给定宽度。
    const std::string text = "加载区块时出现问题请检查数据包";
    const auto lines = wrapByDisplayWidth(text, 8);

    ASSERT_GT(lines.size(), 1u);
    for (const auto& line : lines) {
        EXPECT_LE(displayWidth(line), 8u) << "line exceeded width: " << line;
    }
    EXPECT_EQ(joinLines(lines), text);
}

TEST(ConsoleLineWrapperTest, WrapSplitsCharacterWiderThanLine)
{
    // 终端极窄（1 列）时宽字符单独成行，不能死循环也不能丢字符。
    const auto lines = wrapByDisplayWidth("你好", 1);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "你");
    EXPECT_EQ(lines[1], "好");

    // 混合宽度：ASCII 与宽字符混排时，宽字符单独成行，ASCII 不被连带切散成多余空行。
    const auto mixed = wrapByDisplayWidth("a你b", 1);
    ASSERT_EQ(mixed.size(), 3u);
    EXPECT_EQ(mixed[0], "a");
    EXPECT_EQ(mixed[1], "你");
    EXPECT_EQ(mixed[2], "b");
}

TEST(ConsoleLineWrapperTest, WrapEmptyTextProducesOneEmptyLine)
{
    const auto lines = wrapByDisplayWidth("", 10);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_TRUE(lines[0].empty());
}

// ============================================================================
// expandTabs
// ============================================================================

TEST(ConsoleLineWrapperTest, ExpandTabsAlignsToTabStops)
{
    EXPECT_EQ(expandTabs("ab\tc", 4), "ab  c");
    EXPECT_EQ(expandTabs("\tx", 4), "    x");
    // 制表位按显示列对齐：中文字占两列。
    EXPECT_EQ(expandTabs("你\tx", 4), "你  x");
}

// ============================================================================
// PanelStats：数字与排行文本
// ============================================================================

TEST(ConsolePanelStatsTest, FormatThousandsInsertsSeparators)
{
    EXPECT_EQ(formatThousands(0), "0");
    EXPECT_EQ(formatThousands(999), "999");
    EXPECT_EQ(formatThousands(1000), "1,000");
    EXPECT_EQ(formatThousands(1234567), "1,234,567");
    EXPECT_EQ(formatThousands(-1234), "-1,234");
}

TEST(ConsolePanelStatsTest, StripNamespaceRemovesMinecraftPrefix)
{
    EXPECT_EQ(stripNamespace("minecraft:overworld"), "overworld");
    EXPECT_EQ(stripNamespace("custom:thing"), "custom:thing");
}

TEST(ConsolePanelStatsTest, RankedCountsSortsDescendingAndCapsEntries)
{
    std::vector<std::pair<std::string, i64>> counts{{"stone", 10}, {"dirt", 30}, {"grass_block", 60}};
    const auto lines = formatRankedCounts(counts, 2, 100);

    ASSERT_EQ(lines.size(), 3u); // 2 项 + "+1 more"
    EXPECT_NE(lines[0].find("grass_block"), std::string::npos);
    EXPECT_NE(lines[1].find("dirt"), std::string::npos);
    EXPECT_NE(lines[2].find("+1 more"), std::string::npos);
    // 百分比按总数归一化，保留一位小数与单位上下文。
    EXPECT_NE(lines[0].find("(60.0%)"), std::string::npos);
}

TEST(ConsolePanelStatsTest, RankedCountsHandlesEmptyInput)
{
    const auto lines = formatRankedCounts({}, 5, 0);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_NE(lines[0].find("none"), std::string::npos);
}

TEST(ConsolePanelStatsTest, ChunkStatusLinesReportPercentages)
{
    ChunkStatusStats stats;
    stats.loadedChunks = 200;
    stats.tickingChunks = 50;
    stats.postProcessedChunks = 150;
    stats.loadedEntities = 1234;

    const auto lines = formatChunkStatusLines(stats);
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_NE(lines[0].find("200"), std::string::npos);
    EXPECT_NE(lines[1].find("(25.0%)"), std::string::npos);
    EXPECT_NE(lines[2].find("(75.0%)"), std::string::npos);
    EXPECT_NE(lines[3].find("1,234"), std::string::npos);
}
