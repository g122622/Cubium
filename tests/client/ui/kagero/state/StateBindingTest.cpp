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
 * @file StateBindingTest.cpp
 * @brief StateBinding, StateScope, StateContext 单元测试
 *
 * 测试覆盖率目标：95%+
 *
 * 测试场景：
 * - bind 函数测试
 * - bindReactive 双向同步测试
 * - watch/unwatch 函数测试
 * - watchAll/unwatchAll 函数测试
 * - StateScope 生命周期管理测试
 * - StateContext 组件状态管理测试
 */

#include "client/ui/kagero/state/StateBinding.hpp"
#include "client/ui/kagero/state/ReactiveState.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero::state;
using namespace mc::client::ui::kagero::state::binding;
using mc::i32;

using mc::u64;

// ============================================================================
// 测试基类
// ============================================================================

class StateBindingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        StateStore::instance().clear();
        StateStore::instance().clearMiddlewares();
    }

    void TearDown() override
    {
        StateStore::instance().clear();
        StateStore::instance().clearMiddlewares();
    }
};

// ============================================================================
// bind 函数测试
// ============================================================================

TEST_F(StateBindingTest, BindBasic)
{
    StateBindingPoint point = bind<i32>("test.key");

    point.set(42);
    EXPECT_EQ(point.get<i32>(), 42);
}

TEST_F(StateBindingTest, BindWithDefault)
{
    StateBindingPoint point = bind<i32>("nonexistent.key");

    // 不存在的键返回默认值
    EXPECT_EQ(point.get<i32>(), 0);
    EXPECT_EQ(point.get<i32>(100), 100);
}

TEST_F(StateBindingTest, BindCallback)
{
    StateBindingPoint point = bind<i32>("test.key");
    point.set(0);

    int callCount = 0;
    u64 id = point.bind([&]() { callCount++; });

    point.set(42);
    EXPECT_EQ(callCount, 1);

    point.unbind(id);
    point.set(100);
    EXPECT_EQ(callCount, 1);
}

TEST_F(StateBindingTest, BindMultipleCallbacks)
{
    StateBindingPoint point = bind<i32>("test.key");
    point.set(0);

    int count1 = 0, count2 = 0;

    u64 id1 = point.bind([&]() { count1++; });
    u64 id2 = point.bind([&]() { count2++; });

    point.set(42);

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);

    point.unbind(id1);

    point.set(100);
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 2);

    point.unbind(id2);
}

// ============================================================================
// bindReactive 双向同步测试
// ============================================================================

TEST_F(StateBindingTest, BindReactiveSyncToStore)
{
    Reactive<i32> value(42);

    StateBindingPoint point = bindReactive(value, "test.sync.to_store");

    // Reactive 变化 -> StateStore 更新
    value.set(100);
    EXPECT_EQ(StateStore::instance().get<i32>("test.sync.to_store"), 100);
}

TEST_F(StateBindingTest, BindReactiveSyncFromStore)
{
    Reactive<i32> value(42);

    StateBindingPoint point = bindReactive(value, "test.sync.from_store");

    // 验证初始值
    EXPECT_EQ(StateStore::instance().get<i32>("test.sync.from_store"), 42);

    // StateStore 变化 -> Reactive 更新
    StateStore::instance().set<i32>("test.sync.from_store", 200);
    EXPECT_EQ(value.get(), 200);
}

TEST_F(StateBindingTest, BindReactiveInitialValue)
{
    Reactive<i32> value(42);

    bindReactive(value, "test.sync.initial");

    // 初始值应该同步到 StateStore
    EXPECT_EQ(StateStore::instance().get<i32>("test.sync.initial"), 42);
}

TEST_F(StateBindingTest, BindReactivePreventLoop)
{
    Reactive<i32> value(42);

    int reactiveCallCount = 0;
    value.observe([&](const i32&, const i32&) { reactiveCallCount++; });

    bindReactive(value, "test.sync.prevent_loop");

    // 重置计数器（bindReactive 初始化时可能触发观察者）
    reactiveCallCount = 0;

    value.set(100);

    // reactiveCallCount 应该只增加一次
    // bindReactive 的防循环机制应该防止无限循环
    EXPECT_EQ(reactiveCallCount, 1);
}

// ============================================================================
// watch 函数测试
// ============================================================================

TEST_F(StateBindingTest, WatchBasic)
{
    StateStore::instance().set<i32>("test.watch", 0);

    i32 oldVal = 0, newVal = 0;
    int callCount = 0;

    u64 id = watch<i32>("test.watch", [&](const i32& old, const i32& newV) {
        oldVal = old;
        newVal = newV;
        callCount++;
    });

    StateStore::instance().set<i32>("test.watch", 42);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(oldVal, 0);
    EXPECT_EQ(newVal, 42);

    StateStore::instance().set<i32>("test.watch", 100);
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(oldVal, 42);
    EXPECT_EQ(newVal, 100);

    unwatch(id);

    StateStore::instance().set<i32>("test.watch", 200);
    EXPECT_EQ(callCount, 2);
}

TEST_F(StateBindingTest, WatchMultipleKeys)
{
    auto ids = watchAll({"key1", "key2", "key3"}, []() {});

    EXPECT_EQ(ids.size(), 3u);

    unwatchAll(ids);

    // 成功取消订阅
    SUCCEED();
}

// ============================================================================
// StateScope 测试
// ============================================================================

TEST_F(StateBindingTest, StateScopeAutoUnsubscribe)
{
    int callCount = 0;

    {
        StateScope scope;
        scope.subscribe("test.scope", [&]() { callCount++; });

        StateStore::instance().set<i32>("test.scope", 1);
        EXPECT_EQ(callCount, 1);
    }

    // 离开作用域后自动取消订阅
    StateStore::instance().set<i32>("test.scope", 2);
    EXPECT_EQ(callCount, 1);
}

TEST_F(StateBindingTest, StateScopeWatch)
{
    int callCount = 0;

    {
        StateScope scope;
        scope.watch<i32>("test.scope", [&](const i32&, const i32&) { callCount++; });

        StateStore::instance().set<i32>("test.scope", 1);
        EXPECT_EQ(callCount, 1);
    }

    StateStore::instance().set<i32>("test.scope", 2);
    EXPECT_EQ(callCount, 1);
}

TEST_F(StateBindingTest, StateScopeClear)
{
    int callCount = 0;

    StateScope scope;
    scope.subscribe("test.scope", [&]() { callCount++; });

    StateStore::instance().set<i32>("test.scope", 1);
    EXPECT_EQ(callCount, 1);

    scope.clear();

    StateStore::instance().set<i32>("test.scope", 2);
    EXPECT_EQ(callCount, 1);
}

TEST_F(StateBindingTest, StateScopeMove)
{
    int callCount = 0;

    StateScope scope1;
    scope1.subscribe("test.scope", [&]() { callCount++; });

    StateStore::instance().set<i32>("test.scope", 1);
    EXPECT_EQ(callCount, 1);

    StateScope scope2 = std::move(scope1);

    StateStore::instance().set<i32>("test.scope", 2);
    EXPECT_EQ(callCount, 2);

    EXPECT_EQ(scope1.size(), 0u);
    EXPECT_EQ(scope2.size(), 1u);
}

TEST_F(StateBindingTest, StateScopeSize)
{
    StateScope scope;

    EXPECT_EQ(scope.size(), 0u);

    scope.subscribe("key1", []() {});
    EXPECT_EQ(scope.size(), 1u);

    scope.subscribe("key2", []() {});
    EXPECT_EQ(scope.size(), 2u);

    scope.clear();
    EXPECT_EQ(scope.size(), 0u);
}

// ============================================================================
// StateContext 测试
// ============================================================================

TEST_F(StateBindingTest, StateContextGetSet)
{
    StateContext ctx;

    ctx.set<i32>("health", 100);
    EXPECT_EQ(ctx.get<i32>("health"), 100);

    ctx.set<std::string>("name", "Steve");
    EXPECT_EQ(ctx.get<std::string>("name"), "Steve");
}

TEST_F(StateBindingTest, StateContextReactive)
{
    StateContext ctx;

    Reactive<i32>& mana = ctx.reactive<i32>("mana", 50);
    EXPECT_EQ(mana.get(), 50);

    mana.set(100);
    EXPECT_EQ(mana.get(), 100);

    // 再次获取应该返回同一个对象
    Reactive<i32>& mana2 = ctx.reactive<i32>("mana", 0);
    EXPECT_EQ(mana2.get(), 100);
}

TEST_F(StateBindingTest, StateContextScope)
{
    StateContext ctx;

    int callCount = 0;
    ctx.scope().subscribe("test.ctx", [&]() { callCount++; });

    ctx.set<i32>("test.ctx", 42);
    EXPECT_EQ(callCount, 1);
}

// ============================================================================
// computed 辅助函数测试
// ============================================================================

TEST_F(StateBindingTest, ComputedHelper)
{
    StateStore::instance().set<i32>("a", 10);
    StateStore::instance().set<i32>("b", 20);

    auto sum =
        computed<i32>([]() { return StateStore::instance().get<i32>("a") + StateStore::instance().get<i32>("b"); });

    EXPECT_EQ(sum.get(), 30);

    StateStore::instance().set<i32>("a", 15);
    sum.markDirty();
    EXPECT_EQ(sum.get(), 35);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(StateBindingTest, BindWithEmptyKey)
{
    StateBindingPoint point = bind<i32>("");

    point.set(42);
    EXPECT_EQ(point.get<i32>(), 42);
}

TEST_F(StateBindingTest, MultipleBindReactive)
{
    Reactive<i32> value(42);

    bindReactive(value, "test.sync1");
    bindReactive(value, "test.sync2");

    value.set(100);

    EXPECT_EQ(StateStore::instance().get<i32>("test.sync1"), 100);
    EXPECT_EQ(StateStore::instance().get<i32>("test.sync2"), 100);
}
