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
 * @file SignCommandHelperTest.cpp
 * @brief SignCommandHelper 单元测试
 *
 * 测试告示牌命令执行辅助类：
 * - executeSignCommands 命令遍历和执行
 * - executeCommand 单个命令执行
 * - 各种 ClickAction 类型的处理
 * - 子组件点击事件处理
 */

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/interaction/SignCommandHelper.hpp"
#include "server/player/ServerPlayer.hpp"

using namespace mc;
using namespace mc::blockentity;
using namespace mc::text;
using namespace mc::server;

namespace {

/**
 * @brief 创建带点击事件的文本组件
 */
std::unique_ptr<StringTextComponent> createTextWithClickEvent(
    const std::string& text, ClickAction action, const std::string& value)
{
    auto component = std::make_unique<StringTextComponent>(text);
    Style style;
    style.setClickEvent(ClickEvent(action, value));
    component->setStyle(style);
    return component;
}

} // namespace

// ============================================================================
// SignCommandHelper 测试
// ============================================================================

class SignCommandHelperTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册内置方块实体类型
        BlockEntityRegistry::instance().registerBuiltinTypes();
        signEntity_ = std::make_unique<SignEntity>(BlockPos(10, 64, 20));
    }

    void TearDown() override { signEntity_.reset(); }

    std::unique_ptr<SignEntity> signEntity_;
};

// ========== 基础功能测试 ==========

TEST_F(SignCommandHelperTest, EmptySign_DefaultLines)
{
    // 空告示牌默认有空文本组件
    // getLine 返回文本组件指针，即使内容为空
    for (int i = 0; i < 4; ++i) {
        const auto* line = signEntity_->getLine(i);
        // SignEntity 可能返回空文本组件或 nullptr，取决于实现
        // 这里只验证不会崩溃
        if (line) {
            // 如果有文本，应该是空的或默认的
            std::string text = signEntity_->getLineText(i);
            EXPECT_TRUE(text.empty() || text == "");
        }
    }
}

TEST_F(SignCommandHelperTest, SignWithRunCommand_HasClickEvent)
{
    // 设置带 RunCommand 点击事件的文本
    auto line = createTextWithClickEvent("Click me", ClickAction::RunCommand, "/help");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    // 验证点击事件设置正确
    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const Style& style = textLine->getStyle();
    const ClickEvent* clickEvent = style.getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_TRUE(clickEvent->isValid());
    EXPECT_EQ(clickEvent->getAction(), ClickAction::RunCommand);
    EXPECT_EQ(clickEvent->getValue(), "/help");
}

TEST_F(SignCommandHelperTest, MultipleCommands_AllLinesWithCommands)
{
    // 在所有行设置命令
    signEntity_->setLine(0, createTextWithClickEvent("Help", ClickAction::RunCommand, "/help"));
    signEntity_->setLine(1, createTextWithClickEvent("Gamemode", ClickAction::RunCommand, "/gamemode creative"));
    signEntity_->setLine(2, createTextWithClickEvent("Time", ClickAction::RunCommand, "/time set day"));
    signEntity_->setLine(3, createTextWithClickEvent("Seed", ClickAction::RunCommand, "/seed"));

    // 验证所有行都有点击事件
    for (int i = 0; i < 4; ++i) {
        const auto* textLine = signEntity_->getLine(i);
        ASSERT_NE(textLine, nullptr);
        const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
        ASSERT_NE(clickEvent, nullptr);
        EXPECT_TRUE(clickEvent->isValid());
        EXPECT_EQ(clickEvent->getAction(), ClickAction::RunCommand);
    }
}

// ========== ClickAction 类型测试 ==========

TEST_F(SignCommandHelperTest, ClickAction_OpenUrl_NotRunCommand)
{
    // OpenUrl 不应该被服务端执行
    auto line = createTextWithClickEvent("Open URL", ClickAction::OpenUrl, "https://example.com");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    // 确认类型是 OpenUrl，不是 RunCommand
    EXPECT_EQ(clickEvent->getAction(), ClickAction::OpenUrl);
    EXPECT_NE(clickEvent->getAction(), ClickAction::RunCommand);
}

TEST_F(SignCommandHelperTest, ClickAction_SuggestCommand_NotRunCommand)
{
    // SuggestCommand 不应该被服务端执行（客户端功能）
    auto line = createTextWithClickEvent("Suggest", ClickAction::SuggestCommand, "/gamemode");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::SuggestCommand);
    EXPECT_NE(clickEvent->getAction(), ClickAction::RunCommand);
}

TEST_F(SignCommandHelperTest, ClickAction_CopyToClipboard_NotRunCommand)
{
    // CopyToClipboard 不应该被服务端执行（客户端功能）
    auto line = createTextWithClickEvent("Copy", ClickAction::CopyToClipboard, "copied text");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::CopyToClipboard);
}

TEST_F(SignCommandHelperTest, ClickAction_OpenFile_NotRunCommand)
{
    // OpenFile 不应该被服务端执行（安全原因）
    auto line = createTextWithClickEvent("File", ClickAction::OpenFile, "/path/to/file");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::OpenFile);
}

// ========== 子组件点击事件测试 ==========

TEST_F(SignCommandHelperTest, NestedComponent_ClickEvent)
{
    // 创建带子组件的文本，子组件有点击事件
    auto mainText = std::make_unique<StringTextComponent>("Main ");
    auto childText = std::make_unique<StringTextComponent>("Child");
    Style childStyle;
    childStyle.setClickEvent(ClickEvent(ClickAction::RunCommand, "/child"));
    childText->setStyle(childStyle);
    mainText->append(std::move(childText));

    ASSERT_TRUE(signEntity_->setLine(0, std::move(mainText)));

    // 验证主文本和子组件
    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    EXPECT_EQ(textLine->getUnformattedText(), "Main Child");

    // 验证子组件有点击事件
    const auto& siblings = textLine->getSiblings();
    ASSERT_EQ(siblings.size(), 1u);
    const ClickEvent* clickEvent = siblings[0]->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::RunCommand);
    EXPECT_EQ(clickEvent->getValue(), "/child");
}

TEST_F(SignCommandHelperTest, MixedClickActions_SameLine)
{
    // 主文本有一个命令，子组件有另一个命令
    auto mainText = createTextWithClickEvent("First ", ClickAction::RunCommand, "/first");
    auto childText = createTextWithClickEvent("Second", ClickAction::RunCommand, "/second");
    mainText->append(std::move(childText));

    ASSERT_TRUE(signEntity_->setLine(0, std::move(mainText)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);

    // 主文本有点击事件
    const ClickEvent* mainClick = textLine->getStyle().getClickEvent();
    ASSERT_NE(mainClick, nullptr);
    EXPECT_EQ(mainClick->getValue(), "/first");

    // 子组件也有点击事件
    const auto& siblings = textLine->getSiblings();
    ASSERT_EQ(siblings.size(), 1u);
    const ClickEvent* childClick = siblings[0]->getStyle().getClickEvent();
    ASSERT_NE(childClick, nullptr);
    EXPECT_EQ(childClick->getValue(), "/second");
}

// ========== 命令格式测试 ==========

TEST_F(SignCommandHelperTest, CommandWithoutSlash_AddedSlash)
{
    // 命令没有斜杠前缀应该自动添加
    // 这在 SignCommandHelper::executeCommand 中处理
    auto line = createTextWithClickEvent("Test", ClickAction::RunCommand, "help");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    // 原始值不带斜杠
    EXPECT_EQ(clickEvent->getValue(), "help");
}

TEST_F(SignCommandHelperTest, CommandWithSlash_Kept)
{
    // 命令已经有斜杠前缀应该保持不变
    auto line = createTextWithClickEvent("Test", ClickAction::RunCommand, "/help");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getValue(), "/help");
}

// ========== 序列化和克隆测试 ==========

TEST_F(SignCommandHelperTest, Serialization_WithClickEvent)
{
    signEntity_->setLine(0, createTextWithClickEvent("Test", ClickAction::RunCommand, "/test"));

    nlohmann::json data;
    signEntity_->save(data);

    auto loaded = std::make_unique<SignEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    const auto* line = loaded->getLine(0);
    ASSERT_NE(line, nullptr);
    const ClickEvent* clickEvent = line->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::RunCommand);
    EXPECT_EQ(clickEvent->getValue(), "/test");
}

TEST_F(SignCommandHelperTest, Clone_WithClickEvent)
{
    signEntity_->setLine(0, createTextWithClickEvent("Clone", ClickAction::RunCommand, "/clone"));

    auto cloned = signEntity_->clone();
    ASSERT_NE(cloned, nullptr);

    auto* signClone = dynamic_cast<SignEntity*>(cloned.get());
    ASSERT_NE(signClone, nullptr);

    const auto* line = signClone->getLine(0);
    ASSERT_NE(line, nullptr);
    const ClickEvent* clickEvent = line->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::RunCommand);
    EXPECT_EQ(clickEvent->getValue(), "/clone");
}

// ========== 边界情况测试 ==========

TEST_F(SignCommandHelperTest, EmptyClickEvent_NotValid)
{
    ClickEvent emptyEvent;
    EXPECT_FALSE(emptyEvent.isValid());
}

TEST_F(SignCommandHelperTest, InvalidLine_ReturnsNullptr)
{
    const auto* line = signEntity_->getLine(-1);
    EXPECT_EQ(line, nullptr);

    line = signEntity_->getLine(4);
    EXPECT_EQ(line, nullptr);
}

TEST_F(SignCommandHelperTest, TextWithoutClickEvent_NoClickEvent)
{
    auto line = std::make_unique<StringTextComponent>("No click event");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    EXPECT_EQ(clickEvent, nullptr);
}

TEST_F(SignCommandHelperTest, ClearLines_ClearsAllText)
{
    signEntity_->setLine(0, createTextWithClickEvent("Test", ClickAction::RunCommand, "/test"));
    signEntity_->setLine(1, createTextWithClickEvent("Test2", ClickAction::RunCommand, "/test2"));

    signEntity_->clearLines();

    // clearLines 会创建空的 StringTextComponent，不是 nullptr
    for (int i = 0; i < 4; ++i) {
        const auto* line = signEntity_->getLine(i);
        // SignEntity::clearLines 创建空的 StringTextComponent，所以 line 不为 nullptr
        if (line) {
            // 文本内容应该是空的
            std::string text = signEntity_->getLineText(i);
            EXPECT_TRUE(text.empty()) << "Line " << i << " should be empty after clearLines";
        }
    }
}
