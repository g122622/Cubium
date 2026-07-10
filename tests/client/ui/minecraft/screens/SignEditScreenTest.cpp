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
 * @file SignEditScreenTest.cpp
 * @brief 告示牌编辑屏幕按键交互测试
 *
 * 测试 SignEditScreen 的核心交互：
 * - ESC 取消编辑（触发关闭回调，不触发提交回调）
 * - Enter 提交编辑（触发提交回调 + 关闭回调）
 * - Tab 切换到下一行（焦点转移）
 * - Shift+Tab 切换到上一行
 * - 初始文本正确加载到输入框
 */

#include <gtest/gtest.h>

#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/minecraft/screens/SignEditScreen.hpp"

#include <GLFW/glfw3.h>

using namespace mc;
using namespace mc::client::ui::minecraft;
using namespace mc::client::ui::kagero;

namespace mc::client::ui::minecraft {
namespace {

// 清理 kagero 单例状态，避免用例间污染
void resetKageroSingletons()
{
    kagero::state::StateStore::instance().clear();
    kagero::state::StateStore::instance().clearMiddlewares();
    kagero::event::EventBus::instance().clear();
}

// ============================================================================
// SignEditScreen 构造与初始化测试
// ============================================================================

class SignEditScreenTest : public ::testing::Test {
protected:
    void SetUp() override { resetKageroSingletons(); }
    void TearDown() override { resetKageroSingletons(); }

    /// 创建一个带有空回调的 SignEditScreen
    std::unique_ptr<SignEditScreen> createScreen()
    {
        std::array<std::string, SignEditScreen::LINE_COUNT> initialLines = {"A", "B", "C", "D"};
        auto screen = std::make_unique<SignEditScreen>(
            BlockPos(10, 64, 20),
            initialLines,
            true,
            [](const BlockPos&, const std::array<std::string, SignEditScreen::LINE_COUNT>&, bool) {},
            []() {});
        screen->onOpen(); // 手动触发 onOpen 以初始化文本框
        return screen;
    }
};

TEST_F(SignEditScreenTest, ConstructDoesNotCrash)
{
    std::array<std::string, SignEditScreen::LINE_COUNT> lines = {"", "", "", ""};
    EXPECT_NO_THROW({
        SignEditScreen screen(BlockPos(0, 0, 0), lines, true, nullptr, nullptr);
        screen.onOpen();
    });
}

TEST_F(SignEditScreenTest, IsModalByDefault)
{
    auto screen = createScreen();
    EXPECT_TRUE(screen->isModal());
}

TEST_F(SignEditScreenTest, IsNotPauseScreen)
{
    // 告示牌编辑屏幕不应暂停游戏
    auto screen = createScreen();
    EXPECT_FALSE(screen->isPauseScreen());
}

TEST_F(SignEditScreenTest, OnOpenInitializesFourTextFields)
{
    auto screen = createScreen();
    // onOpen 后应有 4 个子组件（TextFieldWidget）
    // 通过检查 onKey 不崩溃来间接验证文本框已初始化
    EXPECT_NO_THROW(screen->onKey(GLFW_KEY_TAB, 0, GLFW_PRESS, 0));
}

// ============================================================================
// ESC 键取消编辑测试
// ============================================================================

class SignEditScreenEscTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_submitCalled = false;
        m_closeCalled = false;

        std::array<std::string, SignEditScreen::LINE_COUNT> lines = {"A", "B", "C", "D"};
        m_screen = std::make_unique<SignEditScreen>(
            BlockPos(10, 64, 20),
            lines,
            true,
            [this](const BlockPos&, const std::array<std::string, SignEditScreen::LINE_COUNT>&, bool) {
                m_submitCalled = true;
            },
            [this]() { m_closeCalled = true; });
        m_screen->onOpen();
    }

    void TearDown() override
    {
        m_screen.reset();
        resetKageroSingletons();
    }

    bool m_submitCalled = false;
    bool m_closeCalled = false;
    std::unique_ptr<SignEditScreen> m_screen;
};

TEST_F(SignEditScreenEscTest, EscPressInvokesCloseCallback)
{
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(m_closeCalled);
}

TEST_F(SignEditScreenEscTest, EscPressDoesNotInvokeSubmitCallback)
{
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    EXPECT_FALSE(m_submitCalled);
}

TEST_F(SignEditScreenEscTest, EscReleaseDoesNotInvokeCallback)
{
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_RELEASE, 0);
    EXPECT_FALSE(m_closeCalled);
    EXPECT_FALSE(m_submitCalled);
}

TEST_F(SignEditScreenEscTest, EscPressReturnsTrue)
{
    bool result = m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(result);
}

// ============================================================================
// Enter 键提交编辑测试
// ============================================================================

class SignEditScreenEnterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_submitCalled = false;
        m_closeCalled = false;
        m_capturedPos = BlockPos(0, 0, 0);
        m_capturedIsFront = false;

        std::array<std::string, SignEditScreen::LINE_COUNT> lines = {"A", "B", "C", "D"};
        m_screen = std::make_unique<SignEditScreen>(
            BlockPos(10, 64, 20),
            lines,
            true,
            [this](const BlockPos& pos,
                const std::array<std::string, SignEditScreen::LINE_COUNT>& lines,
                bool isFrontSide) {
                m_submitCalled = true;
                m_capturedPos = pos;
                m_capturedLines = lines;
                m_capturedIsFront = isFrontSide;
            },
            [this]() { m_closeCalled = true; });
        m_screen->onOpen();
    }

    void TearDown() override
    {
        m_screen.reset();
        resetKageroSingletons();
    }

    bool m_submitCalled = false;
    bool m_closeCalled = false;
    BlockPos m_capturedPos;
    std::array<std::string, SignEditScreen::LINE_COUNT> m_capturedLines{};
    bool m_capturedIsFront = false;
    std::unique_ptr<SignEditScreen> m_screen;
};

TEST_F(SignEditScreenEnterTest, EnterPressInvokesSubmitCallback)
{
    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(m_submitCalled);
}

TEST_F(SignEditScreenEnterTest, EnterPressInvokesCloseCallback)
{
    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(m_closeCalled);
}

TEST_F(SignEditScreenEnterTest, EnterPressPassesCorrectPosition)
{
    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    EXPECT_EQ(m_capturedPos, BlockPos(10, 64, 20));
}

TEST_F(SignEditScreenEnterTest, EnterPressPassesInitialLines)
{
    // 未编辑任何文本，提交时应返回初始文本
    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    EXPECT_EQ(m_capturedLines[0], "A");
    EXPECT_EQ(m_capturedLines[1], "B");
    EXPECT_EQ(m_capturedLines[2], "C");
    EXPECT_EQ(m_capturedLines[3], "D");
}

TEST_F(SignEditScreenEnterTest, EnterPressPassesIsFrontSide)
{
    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(m_capturedIsFront);
}

TEST_F(SignEditScreenEnterTest, EnterReleaseDoesNotInvokeCallbacks)
{
    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_RELEASE, 0);
    EXPECT_FALSE(m_submitCalled);
    EXPECT_FALSE(m_closeCalled);
}

TEST_F(SignEditScreenEnterTest, EnterPressReturnsTrue)
{
    bool result = m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(result);
}

// ============================================================================
// Tab 键切换行测试
// ============================================================================

class SignEditScreenTabTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_submitCalled = false;
        m_closeCalled = false;

        std::array<std::string, SignEditScreen::LINE_COUNT> lines = {"", "", "", ""};
        m_screen = std::make_unique<SignEditScreen>(
            BlockPos(10, 64, 20),
            lines,
            true,
            [this](const BlockPos&, const std::array<std::string, SignEditScreen::LINE_COUNT>&, bool) {
                m_submitCalled = true;
            },
            [this]() { m_closeCalled = true; });
        m_screen->onOpen();
    }

    void TearDown() override
    {
        m_screen.reset();
        resetKageroSingletons();
    }

    bool m_submitCalled = false;
    bool m_closeCalled = false;
    std::unique_ptr<SignEditScreen> m_screen;
};

TEST_F(SignEditScreenTabTest, TabPressDoesNotSubmit)
{
    m_screen->onKey(GLFW_KEY_TAB, 0, GLFW_PRESS, 0);
    EXPECT_FALSE(m_submitCalled);
    EXPECT_FALSE(m_closeCalled);
}

TEST_F(SignEditScreenTabTest, TabPressReturnsTrue)
{
    bool result = m_screen->onKey(GLFW_KEY_TAB, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(result);
}

TEST_F(SignEditScreenTabTest, TabCyclesThroughAllLinesWithoutSubmit)
{
    // 按 Tab 4 次应循环回第一行，不触发提交
    for (i32 i = 0; i < SignEditScreen::LINE_COUNT; ++i) {
        m_screen->onKey(GLFW_KEY_TAB, 0, GLFW_PRESS, 0);
    }
    EXPECT_FALSE(m_submitCalled);
    EXPECT_FALSE(m_closeCalled);
}

TEST_F(SignEditScreenTabTest, ShiftTabCyclesUpwardWithoutSubmit)
{
    // 按 Shift+Tab 4 次应循环回第一行，不触发提交
    for (i32 i = 0; i < SignEditScreen::LINE_COUNT; ++i) {
        m_screen->onKey(GLFW_KEY_TAB, 0, GLFW_PRESS, GLFW_MOD_SHIFT);
    }
    EXPECT_FALSE(m_submitCalled);
    EXPECT_FALSE(m_closeCalled);
}

// ============================================================================
// 非处理按键测试
// ============================================================================

TEST_F(SignEditScreenTabTest, KeyReleaseDoesNotTriggerActions)
{
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_RELEASE, 0);
    EXPECT_FALSE(m_closeCalled);

    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_RELEASE, 0);
    EXPECT_FALSE(m_submitCalled);
}

} // namespace
} // namespace mc::client::ui::minecraft
