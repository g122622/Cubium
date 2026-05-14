/**
 * @file StateObserverTest.cpp
 * @brief 状态观察者辅助类单元测试
 *
 * 测试覆盖率目标：95%+
 *
 * 测试场景：
 * - AutoObserver 自动生命周期管理
 * - MultiStateObserver 多状态观察
 * - DebouncedObserver 延迟触发
 * - StateObserverManager 观察者管理器
 */

#include "client/ui/kagero/state/StateObserver.hpp"
#include "client/ui/kagero/state/ReactiveState.hpp"
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero::state;
using mc::i32;

// ============================================================================
// AutoObserver 测试
// ============================================================================

TEST(AutoObserverTest, AutoLifecycle)
{
    Reactive<i32> value(100);

    int callCount = 0;
    i32 lastValue = 0;

    {
        AutoObserver<i32> observer(value, [&](const i32& v) {
            callCount++;
            lastValue = v;
        });

        value.set(80);
        EXPECT_EQ(callCount, 1);
        EXPECT_EQ(lastValue, 80);

        value.set(60);
        EXPECT_EQ(callCount, 2);
        EXPECT_EQ(lastValue, 60);
    }

    // 离开作用域后自动移除观察者
    value.set(40);
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(value.observerCount(), 0u);
}

TEST(AutoObserverTest, Trigger)
{
    Reactive<i32> value(100);

    int callCount = 0;
    i32 lastValue = 0;

    AutoObserver<i32> observer(value, [&](const i32& v) {
        callCount++;
        lastValue = v;
    });

    observer.trigger();
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastValue, 100); // 当前值
}

TEST(AutoObserverTest, MoveConstructor)
{
    Reactive<i32> value(100);

    int callCount = 0;

    AutoObserver<i32> observer1(value, [&](const i32&) { callCount++; });

    AutoObserver<i32> observer2 = std::move(observer1);

    value.set(50);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(value.observerCount(), 1u);
}

TEST(AutoObserverTest, MoveAssignment)
{
    Reactive<i32> value(100);

    int count1 = 0, count2 = 0;

    AutoObserver<i32> observer1(value, [&](const i32&) { count1++; });
    AutoObserver<i32> observer2(value, [&](const i32&) { count2++; });

    value.set(50);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);

    // 移动赋值后，observer1 观察的是同一个 Reactive
    // 但 observer1 的旧回调被移除，使用 observer2 的回调
    observer1 = std::move(observer2);

    value.set(25);

    // count1 不变（observer1 的旧回调被移除）
    // count2 增加一次（observer1 现在使用 observer2 的回调）
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 2);

    // value 只有一个观察者（observer1 使用 observer2 的回调）
    EXPECT_EQ(value.observerCount(), 1u);
}

TEST(AutoObserverTest, DifferentTypes)
{
    Reactive<std::string> name("Steve");

    std::string lastValue;
    AutoObserver<std::string> observer(name, [&](const std::string& v) { lastValue = v; });

    name.set("Alex");
    EXPECT_EQ(lastValue, "Alex");
}

// ============================================================================
// MultiStateObserver 测试
// ============================================================================

TEST(MultiStateObserverTest, ObserveMultiple)
{
    Reactive<i32> health(100);
    Reactive<i32> mana(50);

    int callCount = 0;

    MultiStateObserver observer;
    observer.observe(health);
    observer.observe(mana);
    observer.setCallback([&]() { callCount++; });

    health.set(80);
    EXPECT_EQ(callCount, 1);

    mana.set(40);
    EXPECT_EQ(callCount, 2);

    observer.clear();

    health.set(60);
    mana.set(30);
    EXPECT_EQ(callCount, 2);
}

TEST(MultiStateObserverTest, Clear)
{
    Reactive<i32> a(0);
    Reactive<i32> b(0);

    int callCount = 0;

    MultiStateObserver observer;
    observer.observe(a);
    observer.observe(b);
    observer.setCallback([&]() { callCount++; });

    a.set(1);
    EXPECT_EQ(callCount, 1);

    observer.clear();

    a.set(2);
    b.set(2);
    EXPECT_EQ(callCount, 1);
}

TEST(MultiStateObserverTest, SetCallbackBeforeObserve)
{
    Reactive<i32> value(0);

    int callCount = 0;

    MultiStateObserver observer;
    observer.setCallback([&]() { callCount++; });
    observer.observe(value);

    value.set(1);
    EXPECT_EQ(callCount, 1);
}

TEST(MultiStateObserverTest, NoCallback)
{
    Reactive<i32> value(0);

    MultiStateObserver observer;
    observer.observe(value);

    // 没有设置回调，不应崩溃
    EXPECT_NO_THROW(value.set(1));
}

// ============================================================================
// DebouncedObserver 测试
// ============================================================================

TEST(DebouncedObserverTest, Debounce)
{
    Reactive<std::string> searchText("");

    std::string lastSearch;
    int callCount = 0;

    DebouncedObserver<std::string> observer(
        searchText,
        [&](const std::string& text) {
            lastSearch = text;
            callCount++;
        },
        100);

    // 快速连续设置
    searchText.set("h");
    searchText.set("he");
    searchText.set("hel");
    searchText.set("hell");
    searchText.set("hello");

    // 更新后应该只有待处理的值，但还没触发
    EXPECT_EQ(callCount, 0);

    // 模拟时间流逝
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    observer.update();

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastSearch, "hello");
}

TEST(DebouncedObserverTest, Flush)
{
    Reactive<std::string> value("");

    std::string lastValue;
    int callCount = 0;

    DebouncedObserver<std::string> observer(
        value,
        [&](const std::string& text) {
            lastValue = text;
            callCount++;
        },
        1000); // 很长的延迟

    value.set("test");

    // 还没到延迟时间
    observer.update();
    EXPECT_EQ(callCount, 0);

    // 立即触发
    observer.flush();
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastValue, "test");
}

TEST(DebouncedObserverTest, MultipleUpdates)
{
    Reactive<i32> value(0);

    i32 lastValue = 0;
    int callCount = 0;

    DebouncedObserver<i32> observer(
        value,
        [&](const i32& v) {
            lastValue = v;
            callCount++;
        },
        50);

    // 第一次更新
    value.set(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    observer.update();
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastValue, 1);

    // 第二次更新
    value.set(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    observer.update();
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(lastValue, 2);
}

TEST(DebouncedObserverTest, Destructor)
{
    Reactive<i32> value(0);

    {
        DebouncedObserver<i32> observer(value, [&](const i32&) {}, 100);
        value.set(1);
        // 离开作用域时应正确移除观察者
    }

    EXPECT_EQ(value.observerCount(), 0u);
}

TEST(DebouncedObserverTest, UpdateWithoutPending)
{
    Reactive<i32> value(0);

    int callCount = 0;

    DebouncedObserver<i32> observer(value, [&](const i32&) { callCount++; }, 50);

    // 没有待处理的更新
    observer.update();
    EXPECT_EQ(callCount, 0);
}

TEST(DebouncedObserverTest, NoUpdateWithinDelay)
{
    Reactive<i32> value(0);

    int callCount = 0;

    DebouncedObserver<i32> observer(value, [&](const i32&) { callCount++; }, 100);

    value.set(1);

    // 在延迟时间内更新，不应触发
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    observer.update();
    EXPECT_EQ(callCount, 0);
}

// ============================================================================
// StateObserverManager 测试
// ============================================================================

class TestStateObserver : public IStateObserver {
public:
    void onStateChanged() override { callCount++; }

    int callCount = 0;
};

TEST(StateObserverManagerTest, AddRemoveObserver)
{
    StateObserverManager manager;
    TestStateObserver observer1;
    TestStateObserver observer2;

    manager.addObserver(&observer1);
    manager.addObserver(&observer2);

    EXPECT_EQ(manager.count(), 2u);

    manager.removeObserver(&observer1);
    EXPECT_EQ(manager.count(), 1u);

    manager.removeObserver(&observer2);
    EXPECT_EQ(manager.count(), 0u);
}

TEST(StateObserverManagerTest, NotifyAll)
{
    StateObserverManager manager;
    TestStateObserver observer1;
    TestStateObserver observer2;
    TestStateObserver observer3;

    manager.addObserver(&observer1);
    manager.addObserver(&observer2);
    manager.addObserver(&observer3);

    manager.notifyAll();

    EXPECT_EQ(observer1.callCount, 1);
    EXPECT_EQ(observer2.callCount, 1);
    EXPECT_EQ(observer3.callCount, 1);

    manager.notifyAll();

    EXPECT_EQ(observer1.callCount, 2);
    EXPECT_EQ(observer2.callCount, 2);
    EXPECT_EQ(observer3.callCount, 2);
}

TEST(StateObserverManagerTest, Clear)
{
    StateObserverManager manager;
    TestStateObserver observer1;
    TestStateObserver observer2;

    manager.addObserver(&observer1);
    manager.addObserver(&observer2);

    manager.clear();

    EXPECT_EQ(manager.count(), 0u);

    manager.notifyAll();

    EXPECT_EQ(observer1.callCount, 0);
    EXPECT_EQ(observer2.callCount, 0);
}

TEST(StateObserverManagerTest, DuplicateAdd)
{
    StateObserverManager manager;
    TestStateObserver observer;

    manager.addObserver(&observer);
    manager.addObserver(&observer);

    EXPECT_EQ(manager.count(), 1u); // 使用 unordered_set，重复添加无效
}

TEST(StateObserverManagerTest, RemoveNonExistent)
{
    StateObserverManager manager;
    TestStateObserver observer;

    // 移除不存在的观察者不应崩溃
    EXPECT_NO_THROW(manager.removeObserver(&observer));
    EXPECT_EQ(manager.count(), 0u);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST(AutoObserverTest, NullCallback)
{
    Reactive<i32> value(100);

    // 空回调不应崩溃
    AutoObserver<i32> observer(value, nullptr);

    EXPECT_NO_THROW(value.set(50));
}

TEST(MultiStateObserverTest, SameStateMultipleTimes)
{
    Reactive<i32> value(0);

    int callCount = 0;

    MultiStateObserver observer;
    observer.observe(value);
    observer.observe(value); // 同一个状态多次观察
    observer.setCallback([&]() { callCount++; });

    value.set(1);
    // 每次观察都会注册一个观察者，所以会触发多次
    // 具体行为取决于实现
    EXPECT_GT(callCount, 0);

    observer.clear();
    EXPECT_EQ(value.observerCount(), 0u);
}

TEST(DebouncedObserverTest, RapidFlush)
{
    Reactive<i32> value(0);

    int callCount = 0;

    DebouncedObserver<i32> observer(value, [&](const i32&) { callCount++; }, 100);

    value.set(1);
    observer.flush();

    value.set(2);
    observer.flush();

    EXPECT_EQ(callCount, 2);
}
