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

#include <gtest/gtest.h>

#include "server/application/console/bridge/ConsoleCommandQueue.hpp"
#include "server/application/console/buffer/EventRingBuffer.hpp"
#include "server/application/console/buffer/LogRingBuffer.hpp"
#include "server/application/console/input/CommandCompletion.hpp"

#include <string>
#include <vector>
#include <spdlog/common.h>

using namespace mc;
using namespace mc::server::console;

// ============================================================================
// LogRingBuffer
// ============================================================================

TEST(ConsoleLogRingBufferTest, DropsOldestWhenFull)
{
    LogRingBuffer buffer(3);
    for (i32 index = 0; index < 5; ++index) {
        buffer.push(LogLine{spdlog::level::info, "line" + std::to_string(index)});
    }

    const auto lines = buffer.snapshot();
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0].text, "line2");
    EXPECT_EQ(lines[2].text, "line4");
    // 累计计数包含已被挤出的行：UI 用它判断"是否有新日志"，不能只看 size。
    EXPECT_EQ(buffer.pushedCount(), 5u);
}

TEST(ConsoleLogRingBufferTest, PushManyKeepsOrderAndCapacity)
{
    LogRingBuffer buffer(2);
    buffer.pushMany(
        {LogLine{spdlog::level::warn, "a"}, LogLine{spdlog::level::err, "b"}, LogLine{spdlog::level::info, "c"}});

    const auto lines = buffer.snapshot();
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0].text, "b");
    EXPECT_EQ(lines[0].level, spdlog::level::err);
    EXPECT_EQ(lines[1].text, "c");
}

// ============================================================================
// EventRingBuffer
// ============================================================================

TEST(ConsoleEventRingBufferTest, StripsSectionSignFormattingCodes)
{
    // "§c" 在 UTF-8 中是 0xC2 0xA7 0x63。
    const std::string formatted = "\xC2\xA7"
                                  "cSomething went wrong";
    EXPECT_EQ(stripFormattingCodes(formatted), "Something went wrong");

    // 不带样式码的文本原样返回。
    EXPECT_EQ(stripFormattingCodes("plain text"), "plain text");

    // 中文等多字节字符不受影响（其非首字节不会等于 0xC2）。
    EXPECT_EQ(stripFormattingCodes("服务器已启动"), "服务器已启动");
    EXPECT_EQ(stripFormattingCodes("\xC2\xA7"
                                   "a"
                                   "中文"),
        "中文");
}

TEST(ConsoleEventRingBufferTest, DropsOldestEventWhenFull)
{
    EventRingBuffer events(2);
    events.push(TerminalEventKind::Chat, "one");
    events.push(TerminalEventKind::Player, "two");
    events.push(TerminalEventKind::Error, "three");

    const auto snapshot = events.snapshot();
    ASSERT_EQ(snapshot.size(), 2u);
    EXPECT_EQ(snapshot[0].text, "two");
    EXPECT_EQ(snapshot[1].kind, TerminalEventKind::Error);
    EXPECT_EQ(events.pushedCount(), 3u);
}

// ============================================================================
// ConsoleCommandQueue
// ============================================================================

TEST(ConsoleCommandQueueTest, DrainsInSubmissionOrder)
{
    ConsoleCommandQueue queue;
    EXPECT_TRUE(queue.drain().empty());

    queue.submit("time set day");
    queue.submit("say hello");
    EXPECT_EQ(queue.pendingCount(), 2u);

    const auto commands = queue.drain();
    ASSERT_EQ(commands.size(), 2u);
    EXPECT_EQ(commands[0], "time set day");
    EXPECT_EQ(commands[1], "say hello");

    // drain 是"取走并清空"语义：第二次调用为空。
    EXPECT_TRUE(queue.drain().empty());
    EXPECT_EQ(queue.pendingCount(), 0u);
}

// ============================================================================
// 输入规范化与命令/聊天判定
// ============================================================================

TEST(ConsoleCommandInputTest, IsCommandInputAcceptsSlashPrefix)
{
    EXPECT_TRUE(isCommandInput("/time set day"));
    EXPECT_TRUE(isCommandInput("   /tp @p 0 64 0"));
    EXPECT_FALSE(isCommandInput("hello world"));
    EXPECT_FALSE(isCommandInput(""));
    EXPECT_FALSE(isCommandInput("   "));
}

TEST(ConsoleCommandInputTest, ToSayCommandWrapsChatText)
{
    EXPECT_EQ(toSayCommand("hello"), "say hello");
    EXPECT_EQ(toSayCommand("   leading spaces"), "say leading spaces");
    EXPECT_TRUE(toSayCommand("   ").empty());
}

TEST(ConsoleCommandInputTest, NormalizePastedTextCollapsesNewlines)
{
    EXPECT_EQ(normalizePastedText("line one\nline two"), "line one line two");
    EXPECT_EQ(normalizePastedText("crlf\r\nline"), "crlf line");
    EXPECT_EQ(normalizePastedText("tab\there"), "tab here");
    // 其它控制字符被丢弃。
    EXPECT_EQ(normalizePastedText(std::string("a\x01\x7F") + "b"), "ab");
    // 中文原样保留。
    EXPECT_EQ(normalizePastedText("你好\n世界"), "你好 世界");
}

// ============================================================================
// 补全候选状态机
// ============================================================================

namespace {

/// 构造一条建议：从 start 处替换为 text。
mc::command::Suggestion _makeSuggestion(i32 start, const std::string& text)
{
    return mc::command::Suggestion(start, text);
}

} // namespace

TEST(ConsoleCompletionStateTest, EmptySuggestionsDeactivate)
{
    CompletionState state;
    const auto count = state.reset("/ti", mc::command::Suggestions::empty());

    EXPECT_EQ(count, 0u);
    EXPECT_FALSE(state.active());
}

TEST(ConsoleCompletionStateTest, AppliesSelectedCandidateToInput)
{
    CompletionState state;
    mc::command::Suggestions suggestions({_makeSuggestion(1, "time"), _makeSuggestion(1, "title")});
    const auto count = state.reset("/ti", suggestions);

    ASSERT_EQ(count, 2u);
    ASSERT_TRUE(state.active());
    // 候选已按文本排序：time < title。
    EXPECT_EQ(state.candidates()[0].text, "/time");
    EXPECT_EQ(state.candidates()[1].text, "/title");

    std::string input = "/ti";
    i32 cursor = 3;
    EXPECT_TRUE(state.applySelected(input, cursor));
    EXPECT_EQ(input, "/time");
    EXPECT_EQ(cursor, 5);
}

TEST(ConsoleCompletionStateTest, NavigationWrapsAround)
{
    CompletionState state;
    mc::command::Suggestions suggestions(
        {_makeSuggestion(1, "alpha"), _makeSuggestion(1, "beta"), _makeSuggestion(1, "gamma")});
    state.reset("/", suggestions);

    EXPECT_EQ(state.selectedIndex(), 0);
    state.selectNext();
    EXPECT_EQ(state.selectedIndex(), 1);
    state.selectPrevious();
    state.selectPrevious();
    // 从头再往上翻会环绕到最后一项。
    EXPECT_EQ(state.selectedIndex(), 2);

    std::string input = "/";
    i32 cursor = 0;
    EXPECT_TRUE(state.applySelected(input, cursor));
    EXPECT_EQ(input, "/gamma");
}

TEST(ConsoleCompletionStateTest, DeduplicatesIdenticalCandidates)
{
    CompletionState state;
    mc::command::Suggestions suggestions(
        {_makeSuggestion(1, "time"), _makeSuggestion(1, "time"), _makeSuggestion(1, "title")});
    const auto count = state.reset("/ti", suggestions);

    EXPECT_EQ(count, 2u);
}

TEST(ConsoleCompletionStateTest, ClearResetsSelection)
{
    CompletionState state;
    state.reset("/t", mc::command::Suggestions({_makeSuggestion(1, "time"), _makeSuggestion(1, "tp")}));
    state.selectNext();
    EXPECT_EQ(state.selectedIndex(), 1);

    state.clear();
    EXPECT_FALSE(state.active());
    EXPECT_EQ(state.selectedIndex(), 0);
}
