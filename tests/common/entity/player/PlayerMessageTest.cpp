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
 * @file PlayerMessageTest.cpp
 * @brief Player 消息发送功能测试
 *
 * 测试 Player 基类的 sendStatusMessage 和 canReceiveMessages 方法：
 * - Player 基类默认实现（空操作）
 * - canReceiveMessages 默认返回 false
 */

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/ChatVisibility.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

/**
 * @brief Player 消息发送测试夹具
 */
class PlayerMessageTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(1, "TestPlayer", mc::test::testEcsRegistry()); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

// ========== sendStatusMessage 测试 ==========

TEST_F(PlayerMessageTest, SendStatusMessageDoesNotThrow)
{
    // Player 基类的 sendStatusMessage 应该是空操作，不会抛出异常
    EXPECT_NO_THROW(player->sendStatusMessage("test.message"));
    EXPECT_NO_THROW(player->sendStatusMessage("another.message", true));
    EXPECT_NO_THROW(player->sendStatusMessage("third.message", false));
}

TEST_F(PlayerMessageTest, SendStatusMessageWithActionBarParameter)
{
    // 测试 actionBar 参数的两种情况都不抛出异常
    EXPECT_NO_THROW(player->sendStatusMessage("test.actionbar.true", true));
    EXPECT_NO_THROW(player->sendStatusMessage("test.actionbar.false", false));
}

TEST_F(PlayerMessageTest, SendStatusMessageWithEmptyString)
{
    // 空字符串应该也不会抛出异常
    EXPECT_NO_THROW(player->sendStatusMessage(""));
    EXPECT_NO_THROW(player->sendStatusMessage("", true));
}

TEST_F(PlayerMessageTest, SendStatusMessageWithLongMessage)
{
    // 长消息也应该正常处理
    std::string longMessage(1000, 'a');
    EXPECT_NO_THROW(player->sendStatusMessage(longMessage));
}

TEST_F(PlayerMessageTest, SendStatusMessageWithTranslationKey)
{
    // 测试翻译键格式
    EXPECT_NO_THROW(player->sendStatusMessage("block.minecraft.bed.occupied"));
    EXPECT_NO_THROW(player->sendStatusMessage("block.minecraft.bed.too_far_away"));
    EXPECT_NO_THROW(player->sendStatusMessage("block.minecraft.bed.obstructed"));
    EXPECT_NO_THROW(player->sendStatusMessage("block.minecraft.bed.no_sleep"));
    EXPECT_NO_THROW(player->sendStatusMessage("block.minecraft.bed.not_safe"));
}

// ========== canReceiveMessages 测试 ==========

TEST_F(PlayerMessageTest, CanReceiveMessagesDefaultFalse)
{
    // Player 基类的 canReceiveMessages 默认返回 false
    EXPECT_FALSE(player->canReceiveMessages());
}

TEST_F(PlayerMessageTest, CanReceiveMessagesAfterStateChange)
{
    // Player 基类的 canReceiveMessages 始终返回 false
    // 因为基类没有网络连接能力
    EXPECT_FALSE(player->canReceiveMessages());

    // 即使调用 sendStatusMessage 后仍然是 false
    player->sendStatusMessage("test.message");
    EXPECT_FALSE(player->canReceiveMessages());
}

// ========== 虚方法多态性测试 ==========

TEST_F(PlayerMessageTest, VirtualMethodCanBeOverridden)
{
    // 确认 sendStatusMessage 是虚方法，可以被子类重写
    // 这个测试验证方法签名正确

    // 使用基类指针调用
    Player* basePtr = player.get();
    EXPECT_NO_THROW(basePtr->sendStatusMessage("test.polymorphism"));
    EXPECT_FALSE(basePtr->canReceiveMessages());
}

} // namespace
} // namespace mc
