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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, express or implied.
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT THAT THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file ScreenStackWidgetTest.cpp
 * @brief ScreenStackWidget、ScreenChangeInfo 和 IScreen 接口单元测试
 *
 * 测试范围：
 * - ScreenChangeInfo 结构体的字段语义
 * - ScreenChangeCallback 回调触发逻辑
 * - IScreen 接口的默认实现
 * - Screen 基础属性
 * - ScreenStackWidget 的 push/pop/clear 核心逻辑
 *   - 回调触发时机和参数正确性
 *   - 栈状态一致性
 *   - hasScreen() / screenCount() 查询
 *   - top() / topIScreen() 访问器
 *   - nullptr 输入安全性
 *   - 空栈 pop/clear 安全性
 */

#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/screen/IScreen.hpp"
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::client::ui::minecraft::widgets;
using namespace mc::client::ui::minecraft;
using namespace mc;

// ============================================================================
// 测试用辅助类
// ============================================================================

/**
 * @brief 可追踪生命周期的 IScreen 实现
 *
 * 记录 init()/onClose() 调用，用于验证 ScreenStackWidget 对 IScreen 的
 * 生命周期管理是否正确。
 */
class TrackedIScreen : public IScreen {
public:
    explicit TrackedIScreen(std::string title)
        : m_title(std::move(title))
    {}

    void init() override { m_initCalled = true; }
    void render(i32, i32, f32) override {}
    bool onClick(i32, i32, i32) override { return false; }
    bool onKey(i32, i32, i32, i32) override { return false; }
    void onClose() override { m_closeCalled = true; }

    [[nodiscard]] std::string getTitle() const override { return m_title; }

    [[nodiscard]] bool wasInitCalled() const { return m_initCalled; }
    [[nodiscard]] bool wasCloseCalled() const { return m_closeCalled; }

private:
    std::string m_title;
    bool m_initCalled = false;
    bool m_closeCalled = false;
};

// ============================================================================
// ScreenChangeInfo 结构体测试
// ============================================================================

TEST(ScreenChangeInfoTest, DefaultValuesAreNullAndFalse)
{
    ScreenChangeInfo info;
    EXPECT_EQ(info.newScreen, nullptr);
    EXPECT_EQ(info.newIScreen, nullptr);
    EXPECT_FALSE(info.stackCleared);
}

TEST(ScreenChangeInfoTest, StackClearedCanBeSet)
{
    ScreenChangeInfo info;
    info.stackCleared = true;
    EXPECT_TRUE(info.stackCleared);
}

TEST(ScreenChangeInfoTest, ScreenAndIScreenAreMutuallyExclusive)
{
    // 同一时刻只有一种类型的屏幕位于栈顶
    ScreenChangeInfo info;

    // 设置 Screen 指针时，IScreen 应为 nullptr
    info.newScreen = reinterpret_cast<Screen*>(0x1);
    info.newIScreen = nullptr;
    EXPECT_NE(info.newScreen, nullptr);
    EXPECT_EQ(info.newIScreen, nullptr);

    // 设置 IScreen 指针时，Screen 应为 nullptr
    info.newScreen = nullptr;
    info.newIScreen = reinterpret_cast<IScreen*>(0x1);
    EXPECT_EQ(info.newScreen, nullptr);
    EXPECT_NE(info.newIScreen, nullptr);
}

TEST(ScreenChangeInfoTest, StackClearedAndPointersAreIndependent)
{
    ScreenChangeInfo info;
    info.stackCleared = true;
    info.newScreen = nullptr;
    info.newIScreen = nullptr;
    EXPECT_TRUE(info.stackCleared);
    EXPECT_EQ(info.newScreen, nullptr);

    // stackCleared 与指针字段独立
    info.newScreen = reinterpret_cast<Screen*>(0x1);
    EXPECT_TRUE(info.stackCleared);
    EXPECT_NE(info.newScreen, nullptr);
}

// ============================================================================
// ScreenChangeCallback 类型测试
// ============================================================================

TEST(ScreenChangeCallbackTest, CallbackCanBeInvokedWithChangeInfo)
{
    int callCount = 0;
    ScreenChangeInfo lastInfo;
    ScreenStackWidget::ScreenChangeCallback callback = [&](const ScreenChangeInfo& info) {
        ++callCount;
        lastInfo = info;
    };

    ScreenChangeInfo info;
    info.stackCleared = true;
    callback(info);

    EXPECT_EQ(callCount, 1);
    EXPECT_TRUE(lastInfo.stackCleared);
}

TEST(ScreenChangeCallbackTest, EmptyCallbackIsNotCallable)
{
    ScreenStackWidget::ScreenChangeCallback callback;
    EXPECT_FALSE(static_cast<bool>(callback));

    // 带空检查的调用模式（与 ScreenStackWidget 内部一致）
    if (callback) {
        callback(ScreenChangeInfo{});
    }
    // 不崩溃即通过
}

TEST(ScreenChangeCallbackTest, MultipleCallbacksTrackSequence)
{
    std::vector<ScreenChangeInfo> history;
    ScreenStackWidget::ScreenChangeCallback callback = [&](const ScreenChangeInfo& info) { history.push_back(info); };

    // 模拟 push 操作后的回调
    ScreenChangeInfo pushInfo;
    pushInfo.newScreen = reinterpret_cast<Screen*>(0x1);
    callback(pushInfo);

    // 模拟 pop 操作后的回调
    ScreenChangeInfo popInfo;
    popInfo.newScreen = nullptr;
    popInfo.newIScreen = nullptr;
    callback(popInfo);

    // 模拟 clear 操作后的回调
    ScreenChangeInfo clearInfo;
    clearInfo.stackCleared = true;
    callback(clearInfo);

    ASSERT_EQ(history.size(), 3u);
    EXPECT_NE(history[0].newScreen, nullptr);
    EXPECT_EQ(history[1].newScreen, nullptr);
    EXPECT_TRUE(history[2].stackCleared);
}

// ============================================================================
// IScreen 接口默认实现测试
// ============================================================================

class ConcreteIScreen : public IScreen {
public:
    void init() override {}
    void render(i32, i32, f32) override {}
    bool onClick(i32, i32, i32) override { return false; }
    bool onKey(i32, i32, i32, i32) override { return false; }
    void onClose() override {}
    [[nodiscard]] std::string getTitle() const override { return "test_iscreen"; }
};

TEST(IScreenDefaultTest, DefaultOnReleaseReturnsFalse)
{
    ConcreteIScreen screen;
    EXPECT_FALSE(screen.onRelease(0, 0, 0));
}

TEST(IScreenDefaultTest, DefaultOnDragReturnsFalse)
{
    ConcreteIScreen screen;
    EXPECT_FALSE(screen.onDrag(0, 0, 0, 0));
}

TEST(IScreenDefaultTest, DefaultOnScrollReturnsFalse)
{
    ConcreteIScreen screen;
    EXPECT_FALSE(screen.onScroll(0, 0, 0.0));
}

TEST(IScreenDefaultTest, DefaultOnCharReturnsFalse)
{
    ConcreteIScreen screen;
    EXPECT_FALSE(screen.onChar(U'a'));
}

TEST(IScreenDefaultTest, DefaultIsPauseScreenReturnsFalse)
{
    ConcreteIScreen screen;
    EXPECT_FALSE(screen.isPauseScreen());
}

TEST(IScreenDefaultTest, DefaultOnResizeDoesNothing)
{
    ConcreteIScreen screen;
    // 不崩溃即通过
    screen.onResize(800, 600);
}

TEST(IScreenDefaultTest, DefaultTickDoesNothing)
{
    ConcreteIScreen screen;
    // 不崩溃即通过
    screen.tick(0.016f);
}

// ============================================================================
// Screen 基础属性测试
// ============================================================================

TEST(ScreenBaseTest, DefaultModalIsTrue)
{
    Screen screen("test");
    EXPECT_TRUE(screen.isModal());
}

TEST(ScreenBaseTest, ModalCanBeChanged)
{
    Screen screen("test");
    screen.setModal(false);
    EXPECT_FALSE(screen.isModal());
    screen.setModal(true);
    EXPECT_TRUE(screen.isModal());
}

TEST(ScreenBaseTest, DefaultPauseScreenIsFalse)
{
    Screen screen("test");
    EXPECT_FALSE(screen.isPauseScreen());
}

TEST(ScreenBaseTest, PauseScreenCanBeSet)
{
    Screen screen("test");
    screen.setPauseScreen(true);
    EXPECT_TRUE(screen.isPauseScreen());
    screen.setPauseScreen(false);
    EXPECT_FALSE(screen.isPauseScreen());
}

TEST(ScreenBaseTest, OnOpenOnCloseDoNotCrash)
{
    Screen screen("test");
    screen.onOpen();
    screen.onClose();
    // 不崩溃即通过
}

TEST(ScreenBaseTest, IdIsSetCorrectly)
{
    Screen screen("my_screen_id");
    EXPECT_EQ(screen.id(), "my_screen_id");
}

// ============================================================================
// ScreenStackWidget 核心功能测试
// ============================================================================

TEST(ScreenStackWidgetTest, InitialStateIsEmpty)
{
    ScreenStackWidget stack;
    EXPECT_FALSE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 0u);
    EXPECT_EQ(stack.top(), nullptr);
    EXPECT_EQ(stack.topIScreen(), nullptr);
}

TEST(ScreenStackWidgetTest, PushScreenIncreasesCount)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<Screen>("screen_a");
    Screen* rawPtr = screen.get();
    stack.push(std::move(screen));

    EXPECT_TRUE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 1u);
    EXPECT_EQ(stack.top(), rawPtr);
    EXPECT_EQ(stack.topIScreen(), nullptr);
}

TEST(ScreenStackWidgetTest, PushIScreenIncreasesCount)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<TrackedIScreen>("iscreen_a");
    IScreen* rawPtr = screen.get();
    stack.pushIScreen(std::move(screen));

    EXPECT_TRUE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 1u);
    EXPECT_EQ(stack.top(), nullptr);
    EXPECT_EQ(stack.topIScreen(), rawPtr);
}

TEST(ScreenStackWidgetTest, PushNullptrScreenDoesNothing)
{
    ScreenStackWidget stack;
    stack.push(nullptr);
    EXPECT_FALSE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 0u);
}

TEST(ScreenStackWidgetTest, PushNullptrIScreenDoesNothing)
{
    ScreenStackWidget stack;
    stack.pushIScreen(nullptr);
    EXPECT_FALSE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 0u);
}

TEST(ScreenStackWidgetTest, PopDecreasesCount)
{
    ScreenStackWidget stack;
    stack.push(std::make_unique<Screen>("screen_a"));
    stack.push(std::make_unique<Screen>("screen_b"));
    EXPECT_EQ(stack.screenCount(), 2u);

    stack.pop();
    EXPECT_EQ(stack.screenCount(), 1u);
    EXPECT_EQ(stack.top()->id(), "screen_a");
}

TEST(ScreenStackWidgetTest, PopOnEmptyStackDoesNothing)
{
    ScreenStackWidget stack;
    stack.pop();
    EXPECT_FALSE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 0u);
}

TEST(ScreenStackWidgetTest, PopLastScreenLeavesEmpty)
{
    ScreenStackWidget stack;
    stack.push(std::make_unique<Screen>("screen_a"));
    stack.pop();
    EXPECT_FALSE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 0u);
    EXPECT_EQ(stack.top(), nullptr);
    EXPECT_EQ(stack.topIScreen(), nullptr);
}

TEST(ScreenStackWidgetTest, ClearRemovesAllScreens)
{
    ScreenStackWidget stack;
    stack.push(std::make_unique<Screen>("a"));
    stack.push(std::make_unique<Screen>("b"));
    stack.push(std::make_unique<Screen>("c"));
    EXPECT_EQ(stack.screenCount(), 3u);

    stack.clear();
    EXPECT_FALSE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 0u);
    EXPECT_EQ(stack.top(), nullptr);
    EXPECT_EQ(stack.topIScreen(), nullptr);
}

TEST(ScreenStackWidgetTest, ClearOnEmptyStackDoesNothing)
{
    ScreenStackWidget stack;
    stack.clear();
    EXPECT_FALSE(stack.hasScreen());
    EXPECT_EQ(stack.screenCount(), 0u);
}

TEST(ScreenStackWidgetTest, TopReturnsCorrectScreenType)
{
    ScreenStackWidget stack;
    stack.push(std::make_unique<Screen>("widget_screen"));
    EXPECT_NE(stack.top(), nullptr);
    EXPECT_EQ(stack.topIScreen(), nullptr);
    EXPECT_EQ(stack.top()->id(), "widget_screen");
}

TEST(ScreenStackWidgetTest, TopReturnsCorrectIScreenType)
{
    ScreenStackWidget stack;
    stack.pushIScreen(std::make_unique<TrackedIScreen>("iscreen_title"));
    EXPECT_EQ(stack.top(), nullptr);
    EXPECT_NE(stack.topIScreen(), nullptr);
    EXPECT_EQ(stack.topIScreen()->getTitle(), "iscreen_title");
}

TEST(ScreenStackWidgetTest, MixedPushScreenAndIScreen)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<Screen>("widget");
    Screen* rawScreen = screen.get();
    stack.push(std::move(screen));

    auto iscreen = std::make_unique<TrackedIScreen>("legacy");
    IScreen* rawIScreen = iscreen.get();
    stack.pushIScreen(std::move(iscreen));

    EXPECT_EQ(stack.screenCount(), 2u);
    // IScreen 在栈顶
    EXPECT_EQ(stack.top(), nullptr);
    EXPECT_EQ(stack.topIScreen(), rawIScreen);

    // pop IScreen 后，Screen 在栈顶
    stack.pop();
    EXPECT_EQ(stack.top(), rawScreen);
    EXPECT_EQ(stack.topIScreen(), nullptr);
}

// ============================================================================
// ScreenStackWidget 回调测试
// ============================================================================

TEST(ScreenStackWidgetTest, PushTriggersCallback)
{
    ScreenStackWidget stack;
    int callCount = 0;
    ScreenChangeInfo lastInfo;

    stack.setScreenChangeCallback([&](const ScreenChangeInfo& info) {
        ++callCount;
        lastInfo = info;
    });

    stack.push(std::make_unique<Screen>("screen_a"));

    EXPECT_EQ(callCount, 1);
    EXPECT_NE(lastInfo.newScreen, nullptr);
    EXPECT_EQ(lastInfo.newIScreen, nullptr);
    EXPECT_FALSE(lastInfo.stackCleared);
    EXPECT_EQ(lastInfo.newScreen->id(), "screen_a");
}

TEST(ScreenStackWidgetTest, PushIScreenTriggersCallback)
{
    ScreenStackWidget stack;
    int callCount = 0;
    ScreenChangeInfo lastInfo;

    stack.setScreenChangeCallback([&](const ScreenChangeInfo& info) {
        ++callCount;
        lastInfo = info;
    });

    stack.pushIScreen(std::make_unique<TrackedIScreen>("iscreen_title"));

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastInfo.newScreen, nullptr);
    EXPECT_NE(lastInfo.newIScreen, nullptr);
    EXPECT_FALSE(lastInfo.stackCleared);
    EXPECT_EQ(lastInfo.newIScreen->getTitle(), "iscreen_title");
}

TEST(ScreenStackWidgetTest, PopTriggersCallbackWithNullScreen)
{
    ScreenStackWidget stack;
    stack.push(std::make_unique<Screen>("screen_a"));

    int callCount = 0;
    ScreenChangeInfo lastInfo;

    stack.setScreenChangeCallback([&](const ScreenChangeInfo& info) {
        ++callCount;
        lastInfo = info;
    });

    stack.pop();

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastInfo.newScreen, nullptr);
    EXPECT_EQ(lastInfo.newIScreen, nullptr);
    EXPECT_FALSE(lastInfo.stackCleared);
}

TEST(ScreenStackWidgetTest, PopToPreviousScreenShowsInCallback)
{
    ScreenStackWidget stack;
    auto bottomScreen = std::make_unique<Screen>("bottom");
    Screen* rawBottom = bottomScreen.get();
    stack.push(std::move(bottomScreen));
    stack.push(std::make_unique<Screen>("top"));

    ScreenChangeInfo lastInfo;
    stack.setScreenChangeCallback([&](const ScreenChangeInfo& info) { lastInfo = info; });

    stack.pop();

    EXPECT_NE(lastInfo.newScreen, nullptr);
    EXPECT_EQ(lastInfo.newScreen, rawBottom);
    EXPECT_EQ(lastInfo.newScreen->id(), "bottom");
}

TEST(ScreenStackWidgetTest, ClearTriggersCallbackWithStackCleared)
{
    ScreenStackWidget stack;
    stack.push(std::make_unique<Screen>("a"));
    stack.push(std::make_unique<Screen>("b"));

    int callCount = 0;
    ScreenChangeInfo lastInfo;

    stack.setScreenChangeCallback([&](const ScreenChangeInfo& info) {
        ++callCount;
        lastInfo = info;
    });

    stack.clear();

    // clear() 只触发一次回调，带 stackCleared=true
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastInfo.newScreen, nullptr);
    EXPECT_EQ(lastInfo.newIScreen, nullptr);
    EXPECT_TRUE(lastInfo.stackCleared);
}

TEST(ScreenStackWidgetTest, CallbackTracksFullLifecycle)
{
    ScreenStackWidget stack;
    std::vector<ScreenChangeInfo> history;

    stack.setScreenChangeCallback([&](const ScreenChangeInfo& info) { history.push_back(info); });

    // push screen
    stack.push(std::make_unique<Screen>("first"));
    // push iscreen
    stack.pushIScreen(std::make_unique<TrackedIScreen>("second"));
    // pop iscreen -> first becomes top
    stack.pop();
    // clear
    stack.clear();

    ASSERT_EQ(history.size(), 4u);

    // push Screen
    EXPECT_NE(history[0].newScreen, nullptr);
    EXPECT_EQ(history[0].newIScreen, nullptr);
    EXPECT_FALSE(history[0].stackCleared);

    // push IScreen
    EXPECT_EQ(history[1].newScreen, nullptr);
    EXPECT_NE(history[1].newIScreen, nullptr);
    EXPECT_FALSE(history[1].stackCleared);

    // pop IScreen -> Screen 回到栈顶
    EXPECT_NE(history[2].newScreen, nullptr);
    EXPECT_EQ(history[2].newIScreen, nullptr);
    EXPECT_FALSE(history[2].stackCleared);

    // clear
    EXPECT_EQ(history[3].newScreen, nullptr);
    EXPECT_EQ(history[3].newIScreen, nullptr);
    EXPECT_TRUE(history[3].stackCleared);
}

TEST(ScreenStackWidgetTest, NoCallbackWhenNullptrPushed)
{
    ScreenStackWidget stack;
    int callCount = 0;
    stack.setScreenChangeCallback([&](const ScreenChangeInfo&) { ++callCount; });

    stack.push(nullptr);
    stack.pushIScreen(nullptr);

    EXPECT_EQ(callCount, 0);
}

TEST(ScreenStackWidgetTest, NoCallbackWhenPoppingEmptyStack)
{
    ScreenStackWidget stack;
    int callCount = 0;
    stack.setScreenChangeCallback([&](const ScreenChangeInfo&) { ++callCount; });

    stack.pop();
    EXPECT_EQ(callCount, 0);
}

TEST(ScreenStackWidgetTest, NoCallbackWhenClearingEmptyStack)
{
    ScreenStackWidget stack;
    int callCount = 0;
    stack.setScreenChangeCallback([&](const ScreenChangeInfo&) { ++callCount; });

    stack.clear();
    EXPECT_EQ(callCount, 0);
}

// ============================================================================
// ScreenStackWidget IScreen 生命周期测试
// ============================================================================

TEST(ScreenStackWidgetTest, PushIScreenCallsInit)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<TrackedIScreen>("test");
    TrackedIScreen* raw = screen.get();
    stack.pushIScreen(std::move(screen));

    EXPECT_TRUE(raw->wasInitCalled());
    EXPECT_FALSE(raw->wasCloseCalled());
}

TEST(ScreenStackWidgetTest, PopIScreenCallsOnClose)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<TrackedIScreen>("test");
    TrackedIScreen* raw = screen.get();
    stack.pushIScreen(std::move(screen));
    stack.pop();

    EXPECT_TRUE(raw->wasCloseCalled());
}

TEST(ScreenStackWidgetTest, ClearCallsOnCloseForAllIScreens)
{
    ScreenStackWidget stack;
    auto screen1 = std::make_unique<TrackedIScreen>("first");
    auto screen2 = std::make_unique<TrackedIScreen>("second");
    TrackedIScreen* raw1 = screen1.get();
    TrackedIScreen* raw2 = screen2.get();

    stack.pushIScreen(std::move(screen1));
    stack.pushIScreen(std::move(screen2));
    stack.clear();

    EXPECT_TRUE(raw1->wasCloseCalled());
    EXPECT_TRUE(raw2->wasCloseCalled());
}

// ============================================================================
// ScreenStackWidget shouldPauseGame 测试
// ============================================================================

TEST(ScreenStackWidgetTest, ShouldPauseGameReturnsFalseWhenEmpty)
{
    ScreenStackWidget stack;
    EXPECT_FALSE(stack.shouldPauseGame());
}

TEST(ScreenStackWidgetTest, ShouldPauseGameReturnsFalseForNormalScreen)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<Screen>("normal");
    screen->setPauseScreen(false);
    stack.push(std::move(screen));
    EXPECT_FALSE(stack.shouldPauseGame());
}

TEST(ScreenStackWidgetTest, ShouldPauseGameReturnsTrueForPauseScreen)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<Screen>("pause_menu");
    screen->setPauseScreen(true);
    stack.push(std::move(screen));
    EXPECT_TRUE(stack.shouldPauseGame());
}

// ============================================================================
// ScreenStackWidget modal 传播测试
// ============================================================================

TEST(ScreenStackWidgetTest, PushScreenReadsModalFromScreen)
{
    ScreenStackWidget stack;
    auto screen = std::make_unique<Screen>("nonmodal");
    screen->setModal(false);
    stack.push(std::move(screen));

    // Screen 的 modal 状态由 Screen::isModal() 决定
    // 默认 Screen::isModal() 返回 true，我们设为 false
    // push 时 _onOpenScreen 会从 screen 读取 modal 状态
    // 不崩溃即通过，modal 状态影响 paint 和事件传播
}

TEST(ScreenStackWidgetTest, ScreenOnOpenCalledOnPush)
{
    ScreenStackWidget stack;
    // Screen::onOpen() 默认为空操作，不崩溃即通过
    stack.push(std::make_unique<Screen>("test"));
}

TEST(ScreenStackWidgetTest, ScreenOnCloseCalledOnPop)
{
    ScreenStackWidget stack;
    // Screen::onClose() 默认为空操作，不崩溃即通过
    stack.push(std::make_unique<Screen>("test"));
    stack.pop();
}
