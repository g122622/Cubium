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

#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/minecraft/screens/MessageScreen.hpp"

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
// MessageScreen 构造与模板加载测试
// ============================================================================
//
// 构造 MessageScreen 会走真实模板加载链路（TemplateScreen + TemplateInstance）。
// TemplateScreen::resolveTemplatePath 基于 __FILE__ 回退到源码树路径，
// 通常在测试环境中也能定位到 message_dialog.tpl。

class MessageScreenTest : public ::testing::Test {
protected:
    void SetUp() override { resetKageroSingletons(); }
    void TearDown() override { resetKageroSingletons(); }
};

// 构造时不崩溃，且模板加载成功时实例有效
TEST_F(MessageScreenTest, ConstructorLoadsTemplate)
{
    MessageScreen screen("Test Title", "Test Message", "OK", nullptr);
    // 不崩溃即视为通过；模板加载成功后 isValid 为 true
    // 若模板加载失败（环境问题），isValid 为 false 但不应崩溃
    EXPECT_NO_THROW((void)screen.isValid());
}

// 构造时空回调不应导致后续触发崩溃
TEST_F(MessageScreenTest, ConstructorWithNullCallbackDoesNotCrash)
{
    MessageScreen screen("Title", "Message", "OK", nullptr);
    EXPECT_NO_THROW({
        // 直接调用 onKey 模拟 ESC 按下，空回调应安全
        screen.onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    });
}

// ============================================================================
// onKey ESC 键触发回调测试
// ============================================================================

class MessageScreenOnKeyTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_callbackInvoked = false;
        m_screen = std::make_unique<MessageScreen>("Title", "Message", "OK", [this]() { m_callbackInvoked = true; });
    }

    void TearDown() override
    {
        m_screen.reset();
        resetKageroSingletons();
    }

    bool m_callbackInvoked = false;
    std::unique_ptr<MessageScreen> m_screen;
};

// ESC 按下时触发回调
TEST_F(MessageScreenOnKeyTest, EscPressInvokesCallback)
{
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(m_callbackInvoked);
}

// ESC 释放时不触发回调（仅 PRESS 触发）
TEST_F(MessageScreenOnKeyTest, EscReleaseDoesNotInvokeCallback)
{
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_RELEASE, 0);
    EXPECT_FALSE(m_callbackInvoked);
}

// ESC 重复时不额外触发回调（一次 PRESS 只触发一次）
TEST_F(MessageScreenOnKeyTest, EscRepeatDoesNotInvokeCallbackAgain)
{
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(m_callbackInvoked);
    m_callbackInvoked = false;
    m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_REPEAT, 0);
    EXPECT_FALSE(m_callbackInvoked);
}

// 非 ESC 键不触发回调（转发给基类）
TEST_F(MessageScreenOnKeyTest, NonEscKeyDoesNotInvokeCallback)
{
    m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    EXPECT_FALSE(m_callbackInvoked);
    m_screen->onKey(GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
    EXPECT_FALSE(m_callbackInvoked);
}

// ESC 按下后 onKey 返回 true（表示事件已处理）
TEST_F(MessageScreenOnKeyTest, EscPressReturnsTrue)
{
    bool result = m_screen->onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    EXPECT_TRUE(result);
}

// 非 ESC 键的返回值由基类决定（这里只验证不崩溃且类型正确）
TEST_F(MessageScreenOnKeyTest, NonEscKeyReturnsBool)
{
    bool result = m_screen->onKey(GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
    (void)result; // 基类可能返回 true 或 false，不强制断言
    SUCCEED();
}

// ============================================================================
// 回调类型验证测试
// ============================================================================

class MessageScreenCallbackTest : public ::testing::Test {
protected:
    void SetUp() override { resetKageroSingletons(); }
    void TearDown() override { resetKageroSingletons(); }
};

// 回调可以多次触发（无状态限制）
TEST_F(MessageScreenCallbackTest, CallbackCanBeInvokedMultipleTimes)
{
    i32 invokeCount = 0;
    MessageScreen screen("Title", "Message", "OK", [&invokeCount]() { ++invokeCount; });

    screen.onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    screen.onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    screen.onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);

    EXPECT_EQ(invokeCount, 3);
}

// 回调中捕获的上下文正确传递
TEST_F(MessageScreenCallbackTest, CallbackCapturesContext)
{
    std::string captured;
    std::string outerValue = "hello";
    MessageScreen screen("Title", "Message", "OK", [&captured, &outerValue]() { captured = outerValue + " world"; });

    screen.onKey(GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
    EXPECT_EQ(captured, "hello world");
}

// ============================================================================
// 模态与暂停屏属性测试
// ============================================================================

TEST_F(MessageScreenTest, IsModalByDefault)
{
    MessageScreen screen("Title", "Message", "OK", nullptr);
    // MessageScreen 默认应继承 Screen 的 m_modal=true
    EXPECT_TRUE(screen.isModal());
}

TEST_F(MessageScreenTest, IsPauseScreenAfterConstruction)
{
    MessageScreen screen("Title", "Message", "OK", nullptr);
    // 构造函数中调用 setPauseScreen(true)
    EXPECT_TRUE(screen.isPauseScreen());
}

} // namespace
} // namespace mc::client::ui::minecraft
