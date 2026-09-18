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
 * @file SignEntityCommandTest.cpp
 * @brief SignEntity::executeCommand 功能测试
 *
 * 测试告示牌点击事件命令执行功能：
 * - RunCommand 点击事件
 * - 各种 ClickAction 类型的行为
 * - Player::asServerPlayer() 类型转换
 */

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "entity/entities/player/Player.hpp"
#include "server/player/ServerPlayer.hpp"
#include "util/text/StringTextComponent.hpp"
#include "util/text/TextEvents.hpp"
#include "util/text/TextStyle.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/core/BlockEntityRegistry.hpp"
#include "world/blockentity/interactive/SignEntity.hpp"

using namespace mc;
using namespace mc::blockentity;
using namespace mc::text;

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
// SignEntity 文本组件测试
// ============================================================================

class SignEntityTextTest : public ::testing::Test {
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

// ========== 点击事件设置测试 ==========

TEST_F(SignEntityTextTest, SetLine_WithClickEvent_StoresCorrectly)
{
    // 设置带 RunCommand 点击事件的文本
    auto line = createTextWithClickEvent("Click me", ClickAction::RunCommand, "/help");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    // 验证文本内容
    EXPECT_EQ(signEntity_->getLineText(0), "Click me");

    // 验证点击事件
    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const Style& style = textLine->getStyle();
    const ClickEvent* clickEvent = style.getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_TRUE(clickEvent->isValid());
    EXPECT_EQ(clickEvent->getAction(), ClickAction::RunCommand);
    EXPECT_EQ(clickEvent->getValue(), "/help");
}

TEST_F(SignEntityTextTest, SetLine_OpenUrlClickEvent)
{
    auto line = createTextWithClickEvent("Open URL", ClickAction::OpenUrl, "https://example.com");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::OpenUrl);
    EXPECT_EQ(clickEvent->getValue(), "https://example.com");
}

TEST_F(SignEntityTextTest, SetLine_SuggestCommandClickEvent)
{
    auto line = createTextWithClickEvent("Suggest", ClickAction::SuggestCommand, "/gamemode");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::SuggestCommand);
}

TEST_F(SignEntityTextTest, SetLine_CopyToClipboardClickEvent)
{
    auto line = createTextWithClickEvent("Copy", ClickAction::CopyToClipboard, "copied text");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::CopyToClipboard);
}

TEST_F(SignEntityTextTest, SetLine_OpenFileClickEvent)
{
    auto line = createTextWithClickEvent("File", ClickAction::OpenFile, "/path/to/file");
    ASSERT_TRUE(signEntity_->setLine(0, std::move(line)));

    const auto* textLine = signEntity_->getLine(0);
    ASSERT_NE(textLine, nullptr);
    const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::OpenFile);
}

TEST_F(SignEntityTextTest, SetLine_EmptyClickEvent_Invalid)
{
    // 空值的点击事件是无效的
    ClickEvent emptyEvent;
    EXPECT_FALSE(emptyEvent.isValid());
}

TEST_F(SignEntityTextTest, MultipleLines_WithDifferentClickEvents)
{
    signEntity_->setLine(0, createTextWithClickEvent("Run", ClickAction::RunCommand, "/run"));
    signEntity_->setLine(1, createTextWithClickEvent("URL", ClickAction::OpenUrl, "https://test.com"));
    signEntity_->setLine(2, createTextWithClickEvent("Copy", ClickAction::CopyToClipboard, "text"));
    signEntity_->setLine(3, createTextWithClickEvent("Suggest", ClickAction::SuggestCommand, "/suggest"));

    for (int i = 0; i < 4; ++i) {
        const auto* textLine = signEntity_->getLine(i);
        ASSERT_NE(textLine, nullptr);
        const ClickEvent* clickEvent = textLine->getStyle().getClickEvent();
        ASSERT_NE(clickEvent, nullptr);
        EXPECT_TRUE(clickEvent->isValid());
    }
}

TEST_F(SignEntityTextTest, NestedComponents_ClickEvent)
{
    // 创建带子组件的文本，子组件有点击事件
    auto mainText = std::make_unique<StringTextComponent>("Main ");
    auto childText = std::make_unique<StringTextComponent>("Child");
    Style childStyle;
    childStyle.setClickEvent(ClickEvent(ClickAction::RunCommand, "/child"));
    childText->setStyle(childStyle);
    mainText->append(std::move(childText));

    ASSERT_TRUE(signEntity_->setLine(0, std::move(mainText)));

    // 验证主文本包含子组件
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

// ============================================================================
// Player::asServerPlayer 测试
// ============================================================================

class PlayerAsServerPlayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Items::initialize() 不需要，因为只测试 Player 类型转换
    }
};

TEST_F(PlayerAsServerPlayerTest, Player_ReturnsNullptr)
{
    // 普通 Player 应返回 nullptr
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    EXPECT_EQ(player.asServerPlayer(), nullptr);
}

TEST_F(PlayerAsServerPlayerTest, ServerPlayer_ReturnsThis)
{
    // ServerPlayer 应返回 this
    ServerPlayer serverPlayer(EntityInstanceId(1), "TestServerPlayer", mc::test::testEcsRegistry());
    EXPECT_EQ(serverPlayer.asServerPlayer(), &serverPlayer);

    const ServerPlayer& constServerPlayer = serverPlayer;
    EXPECT_EQ(constServerPlayer.asServerPlayer(), &constServerPlayer);
}

TEST_F(PlayerAsServerPlayerTest, ServerPlayerThroughBasePointer_Works)
{
    // 通过基类指针调用
    ServerPlayer serverPlayer(EntityInstanceId(1), "TestServerPlayer", mc::test::testEcsRegistry());
    Player* basePtr = &serverPlayer;

    EXPECT_EQ(basePtr->asServerPlayer(), &serverPlayer);
}

// ============================================================================
// SignEntity 序列化测试（带点击事件）
// ============================================================================

class SignEntitySerializationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        BlockEntityRegistry::instance().registerBuiltinTypes();
        signEntity_ = std::make_unique<SignEntity>(BlockPos(100, 64, -200));
    }

    std::unique_ptr<SignEntity> signEntity_;
};

TEST_F(SignEntitySerializationTest, SaveLoad_WithClickEvent)
{
    // 设置带点击事件的文本
    signEntity_->setLine(0, std::make_unique<StringTextComponent>("Click to run"));
    signEntity_->setLine(1, createTextWithClickEvent("Command", ClickAction::RunCommand, "/say hello"));
    signEntity_->setTextColor(14); // Red
    signEntity_->setGlowing(true);

    // 保存
    nlohmann::json data;
    signEntity_->save(data);

    // 验证保存的数据包含点击事件
    EXPECT_TRUE(data.contains("lines"));
    EXPECT_TRUE(data["lines"].is_array());
    EXPECT_EQ(data["lines"].size(), 4u);
    EXPECT_TRUE(data["lines"][1].is_object());
    EXPECT_TRUE(data["lines"][1].contains("clickEvent"));

    // 加载到新实体
    auto loaded = std::make_unique<SignEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    // 验证加载的数据
    const auto* line1 = loaded->getLine(1);
    ASSERT_NE(line1, nullptr);
    const ClickEvent* clickEvent = line1->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::RunCommand);
    EXPECT_EQ(clickEvent->getValue(), "/say hello");

    EXPECT_EQ(loaded->getTextColor(), 14);
    EXPECT_TRUE(loaded->isGlowing());
}

TEST_F(SignEntitySerializationTest, Clone_WithClickEvent)
{
    signEntity_->setLine(0, createTextWithClickEvent("Test", ClickAction::OpenUrl, "https://example.com"));
    signEntity_->setGlowing(true);

    auto cloned = signEntity_->clone();
    ASSERT_NE(cloned, nullptr);

    auto* signClone = dynamic_cast<SignEntity*>(cloned.get());
    ASSERT_NE(signClone, nullptr);

    const auto* line = signClone->getLine(0);
    ASSERT_NE(line, nullptr);
    const ClickEvent* clickEvent = line->getStyle().getClickEvent();
    ASSERT_NE(clickEvent, nullptr);
    EXPECT_EQ(clickEvent->getAction(), ClickAction::OpenUrl);
    EXPECT_TRUE(signClone->isGlowing());
}
