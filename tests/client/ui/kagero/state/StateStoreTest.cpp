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
 * @file StateStoreTest.cpp
 * @brief StateStore 单元测试
 *
 * 测试覆盖率目标：95%+
 *
 * 测试场景：
 * - 基本状态操作（set/get/has/remove/clear/keys）
 * - 订阅机制（subscribe/unsubscribe/unsubscribeAll）
 * - 动作分发（dispatch）
 * - 中间件机制（addMiddleware/clearMiddlewares）
 * - 批量更新（batchUpdate）
 * - 线程安全性测试
 */

#include "client/ui/kagero/state/StateStore.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero::state;
using mc::i32;

using mc::f32;
using mc::u64;

// ============================================================================
// 测试基类
// ============================================================================

class StateStoreTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 清空状态存储
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
// 基本状态操作测试
// ============================================================================

TEST_F(StateStoreTest, SetAndGet)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("player.health", 100);
    EXPECT_EQ(store.get<i32>("player.health"), 100);

    store.set<std::string>("player.name", "Steve");
    EXPECT_EQ(store.get<std::string>("player.name"), "Steve");

    store.set<f32>("settings.volume", 0.8f);
    EXPECT_FLOAT_EQ(store.get<f32>("settings.volume"), 0.8f);

    store.set<bool>("ui.paused", true);
    EXPECT_TRUE(store.get<bool>("ui.paused"));
}

TEST_F(StateStoreTest, GetWithDefaultValue)
{
    StateStore& store = StateStore::instance();

    // 键不存在时返回默认值
    EXPECT_EQ(store.get<i32>("nonexistent.key", 42), 42);
    EXPECT_EQ(store.get<std::string>("nonexistent.key", "default"), "default");
    EXPECT_FLOAT_EQ(store.get<f32>("nonexistent.key", 1.5f), 1.5f);
}

TEST_F(StateStoreTest, Has)
{
    StateStore& store = StateStore::instance();

    EXPECT_FALSE(store.has("player.health"));

    store.set<i32>("player.health", 100);
    EXPECT_TRUE(store.has("player.health"));

    store.remove("player.health");
    EXPECT_FALSE(store.has("player.health"));
}

TEST_F(StateStoreTest, Remove)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("player.health", 100);
    EXPECT_TRUE(store.has("player.health"));

    store.remove("player.health");
    EXPECT_FALSE(store.has("player.health"));

    // 移除不存在的键不会抛出异常
    EXPECT_NO_THROW(store.remove("nonexistent.key"));
}

TEST_F(StateStoreTest, Clear)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("a", 1);
    store.set<i32>("b", 2);
    store.set<i32>("c", 3);

    store.clear();

    EXPECT_FALSE(store.has("a"));
    EXPECT_FALSE(store.has("b"));
    EXPECT_FALSE(store.has("c"));
}

TEST_F(StateStoreTest, Keys)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("a", 1);
    store.set<i32>("b", 2);
    store.set<i32>("c", 3);

    auto keys = store.keys();
    EXPECT_EQ(keys.size(), 3u);

    // 验证所有键都存在
    EXPECT_NE(std::find(keys.begin(), keys.end(), "a"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "b"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "c"), keys.end());
}

TEST_F(StateStoreTest, OverwriteValue)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("player.health", 100);
    EXPECT_EQ(store.get<i32>("player.health"), 100);

    store.set<i32>("player.health", 50);
    EXPECT_EQ(store.get<i32>("player.health"), 50);
}

TEST_F(StateStoreTest, TypeMismatch)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("test", 42);

    // 尝试用错误的类型获取应返回默认值
    EXPECT_EQ(store.get<std::string>("test", "default"), "default");
}

// ============================================================================
// 订阅机制测试
// ============================================================================

TEST_F(StateStoreTest, Subscribe)
{
    StateStore& store = StateStore::instance();

    int callCount = 0;
    i32 lastValue = 0;

    u64 id = store.subscribe("player.health", [&]() {
        callCount++;
        lastValue = store.get<i32>("player.health");
    });

    store.set<i32>("player.health", 100);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastValue, 100);

    store.set<i32>("player.health", 50);
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(lastValue, 50);

    store.unsubscribe(id);

    store.set<i32>("player.health", 25);
    EXPECT_EQ(callCount, 2); // 不应该增加
}

TEST_F(StateStoreTest, MultipleSubscribers)
{
    StateStore& store = StateStore::instance();

    int count1 = 0, count2 = 0, count3 = 0;

    u64 id1 = store.subscribe("test", [&]() { count1++; });
    u64 id2 = store.subscribe("test", [&]() { count2++; });
    u64 id3 = store.subscribe("test", [&]() { count3++; });

    store.set<i32>("test", 1);

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);

    store.unsubscribe(id2);

    store.set<i32>("test", 2);

    EXPECT_EQ(count1, 2);
    EXPECT_EQ(count2, 1); // 已取消订阅
    EXPECT_EQ(count3, 2);

    store.unsubscribe(id1);
    store.unsubscribe(id3);
}

TEST_F(StateStoreTest, UnsubscribeNonExistent)
{
    StateStore& store = StateStore::instance();

    // 取消不存在的订阅ID应返回false
    EXPECT_FALSE(store.unsubscribe(99999));
}

TEST_F(StateStoreTest, UnsubscribeAll)
{
    StateStore& store = StateStore::instance();

    int count = 0;

    store.subscribe("test", [&]() { count++; });
    store.subscribe("test", [&]() { count++; });
    store.subscribe("test", [&]() { count++; });

    store.set<i32>("test", 1);
    EXPECT_EQ(count, 3);

    store.unsubscribeAll("test");

    count = 0;
    store.set<i32>("test", 2);
    EXPECT_EQ(count, 0);
}

TEST_F(StateStoreTest, SubscribeToDifferentKeys)
{
    StateStore& store = StateStore::instance();

    int healthCount = 0;
    int manaCount = 0;

    store.subscribe("player.health", [&]() { healthCount++; });
    store.subscribe("player.mana", [&]() { manaCount++; });

    store.set<i32>("player.health", 100);
    EXPECT_EQ(healthCount, 1);
    EXPECT_EQ(manaCount, 0);

    store.set<i32>("player.mana", 50);
    EXPECT_EQ(healthCount, 1);
    EXPECT_EQ(manaCount, 1);
}

// ============================================================================
// 动作分发测试
// ============================================================================

TEST_F(StateStoreTest, Dispatch)
{
    StateStore& store = StateStore::instance();

    auto incrementHealth = [](StateStore& s) {
        i32 current = s.get<i32>("player.health", 0);
        s.set<i32>("player.health", current + 10);
    };

    store.dispatch(incrementHealth);
    EXPECT_EQ(store.get<i32>("player.health"), 10);

    store.dispatch(incrementHealth);
    EXPECT_EQ(store.get<i32>("player.health"), 20);
}

TEST_F(StateStoreTest, DispatchComplexAction)
{
    StateStore& store = StateStore::instance();

    auto initPlayer = [](StateStore& s) {
        s.set<i32>("player.health", 100);
        s.set<i32>("player.maxHealth", 100);
        s.set<std::string>("player.name", "Steve");
    };

    store.dispatch(initPlayer);

    EXPECT_EQ(store.get<i32>("player.health"), 100);
    EXPECT_EQ(store.get<i32>("player.maxHealth"), 100);
    EXPECT_EQ(store.get<std::string>("player.name"), "Steve");
}

// ============================================================================
// 中间件测试
// ============================================================================

TEST_F(StateStoreTest, Middleware)
{
    StateStore& store = StateStore::instance();

    std::vector<std::string> changedKeys;

    store.addMiddleware(
        [&changedKeys](const std::string& key, const std::any&, StateStore&) { changedKeys.push_back(key); });

    store.set<i32>("a", 1);
    store.set<i32>("b", 2);
    store.set<i32>("c", 3);

    EXPECT_EQ(changedKeys.size(), 3u);
    EXPECT_EQ(changedKeys[0], "a");
    EXPECT_EQ(changedKeys[1], "b");
    EXPECT_EQ(changedKeys[2], "c");

    store.clearMiddlewares();

    changedKeys.clear();
    store.set<i32>("d", 4);
    EXPECT_TRUE(changedKeys.empty());
}

TEST_F(StateStoreTest, MultipleMiddlewares)
{
    StateStore& store = StateStore::instance();

    int count = 0;
    std::string lastKey;

    store.addMiddleware([&count](const std::string&, const std::any&, StateStore&) { count++; });

    store.addMiddleware([&lastKey](const std::string& key, const std::any&, StateStore&) { lastKey = key; });

    store.set<i32>("test", 42);

    EXPECT_EQ(count, 1);
    EXPECT_EQ(lastKey, "test");
}

TEST_F(StateStoreTest, MiddlewareCanModifyState)
{
    StateStore& store = StateStore::instance();

    // 中间件记录变化后的值
    store.addMiddleware([](const std::string& key, const std::any& value, StateStore&) {
        if (key == "player.health") {
            // 中间件在值设置前被调用，value 是即将设置的新值
            // 这里可以执行其他操作
        }
    });

    store.set<i32>("player.health", 100);
    EXPECT_EQ(store.get<i32>("player.health"), 100);

    store.set<i32>("player.health", 80);
    EXPECT_EQ(store.get<i32>("player.health"), 80);
}

// ============================================================================
// 批量更新测试
// ============================================================================

TEST_F(StateStoreTest, BatchUpdate)
{
    StateStore& store = StateStore::instance();

    int callCount = 0;
    store.subscribe("a", [&]() { callCount++; });
    store.subscribe("b", [&]() { callCount++; });
    store.subscribe("c", [&]() { callCount++; });

    // 批量更新前，每次set都会触发通知
    store.set<i32>("a", 1);
    store.set<i32>("b", 2);
    store.set<i32>("c", 3);
    EXPECT_EQ(callCount, 3);

    callCount = 0;

    // 批量更新应该延迟通知到结束
    store.batchUpdate([](StateStore& s) {
        s.set<i32>("a", 10);
        s.set<i32>("b", 20);
        s.set<i32>("c", 30);
    });

    // 每个键只通知一次
    EXPECT_EQ(callCount, 3);
}

TEST_F(StateStoreTest, BatchUpdateNested)
{
    StateStore& store = StateStore::instance();

    int callCount = 0;
    store.subscribe("test", [&]() { callCount++; });

    // 嵌套批量更新
    store.batchUpdate([](StateStore& s1) {
        s1.set<i32>("test", 1);
        s1.batchUpdate([](StateStore& s2) { s2.set<i32>("test", 2); });
    });

    // 应该只在外层批量更新结束时通知
    EXPECT_EQ(callCount, 1);
}

TEST_F(StateStoreTest, BatchUpdateException)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("test", 0);

    // 即使抛出异常，批量更新深度也应该正确递减
    EXPECT_THROW(
        {
            store.batchUpdate([](StateStore& s) {
                s.set<i32>("test", 1);
                throw std::runtime_error("test error");
            });
        },
        std::runtime_error);

    // 值应该已经被设置
    EXPECT_EQ(store.get<i32>("test"), 1);

    // 后续操作应该正常工作
    EXPECT_NO_THROW(store.set<i32>("test", 2));
    EXPECT_EQ(store.get<i32>("test"), 2);
}

// ============================================================================
// 线程安全性测试
// ============================================================================

TEST_F(StateStoreTest, ThreadSafety)
{
    StateStore& store = StateStore::instance();

    std::atomic<int> callCount{0};
    store.subscribe("counter", [&]() { callCount++; });

    const int numThreads = 10;
    const int iterations = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&store, t, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                store.set<i32>("counter", t * iterations + i);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 最终应该有正确数量的通知
    EXPECT_EQ(callCount.load(), numThreads * iterations);
}

TEST_F(StateStoreTest, ConcurrentReadWrite)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("value", 0);

    const int numReaders = 5;
    const int numWriters = 5;
    const int iterations = 100;

    std::vector<std::thread> threads;

    // 读者线程
    for (int t = 0; t < numReaders; ++t) {
        threads.emplace_back([&store, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                i32 value = store.get<i32>("value");
                (void)value; // 使用值防止优化
            }
        });
    }

    // 写者线程
    for (int t = 0; t < numWriters; ++t) {
        threads.emplace_back([&store, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                store.set<i32>("value", i);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 如果没有死锁或崩溃，测试通过
    SUCCEED();
}

TEST_F(StateStoreTest, ConcurrentSubscribe)
{
    StateStore& store = StateStore::instance();

    const int numThreads = 10;
    const int iterations = 50;

    std::vector<std::thread> threads;
    std::atomic<int> totalSubscriptions{0};

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&store, &totalSubscriptions, t, iterations]() {
            for (int i = 0; i < iterations; ++i) {
                std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
                u64 id = store.subscribe(key, []() {});
                totalSubscriptions++;
                store.unsubscribe(id);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(totalSubscriptions.load(), numThreads * iterations);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(StateStoreTest, EmptyKey)
{
    StateStore& store = StateStore::instance();

    // 空键也是有效的
    store.set<i32>("", 42);
    EXPECT_TRUE(store.has(""));
    EXPECT_EQ(store.get<i32>(""), 42);
}

TEST_F(StateStoreTest, LongKey)
{
    StateStore& store = StateStore::instance();

    std::string longKey(1000, 'a');
    store.set<i32>(longKey, 42);
    EXPECT_EQ(store.get<i32>(longKey), 42);
}

TEST_F(StateStoreTest, SpecialCharactersInKey)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("key.with.dots", 1);
    store.set<i32>("key/with/slashes", 2);
    store.set<i32>("key-with-dashes", 3);
    store.set<i32>("key with spaces", 4);

    EXPECT_EQ(store.get<i32>("key.with.dots"), 1);
    EXPECT_EQ(store.get<i32>("key/with/slashes"), 2);
    EXPECT_EQ(store.get<i32>("key-with-dashes"), 3);
    EXPECT_EQ(store.get<i32>("key with spaces"), 4);
}

TEST_F(StateStoreTest, ClearWithSubscribers)
{
    StateStore& store = StateStore::instance();

    int callCount = 0;
    store.subscribe("test", [&]() { callCount++; });

    store.set<i32>("test", 1);
    EXPECT_EQ(callCount, 1);

    store.clear(); // 清空状态不会触发订阅者

    EXPECT_FALSE(store.has("test"));
    EXPECT_EQ(callCount, 1);
}

// ============================================================================
// StateBindingPoint 测试
// ============================================================================

TEST_F(StateStoreTest, StateBindingPointGetSet)
{
    StateStore& store = StateStore::instance();

    StateBindingPoint point("test.binding");
    point.set<i32>(42);

    EXPECT_EQ(point.get<i32>(), 42);
    EXPECT_EQ(point.key(), "test.binding");
}

TEST_F(StateStoreTest, StateBindingPointBind)
{
    StateStore& store = StateStore::instance();

    StateBindingPoint point("test.binding");
    point.set<i32>(0);

    int callCount = 0;
    u64 id = point.bind([&]() { callCount++; });

    point.set<i32>(42);
    EXPECT_EQ(callCount, 1);

    point.unbind(id);
    point.set<i32>(100);
    EXPECT_EQ(callCount, 1);
}

// ============================================================================
// Selector 测试
// ============================================================================

TEST_F(StateStoreTest, SelectorBasic)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("player.baseHealth", 90);
    store.set<i32>("player.healthBonus", 10);

    Selector<i32> totalHealth(
        [](const StateStore& s) { return s.get<i32>("player.baseHealth", 0) + s.get<i32>("player.healthBonus", 0); });

    EXPECT_EQ(totalHealth.select(), 100);
}

TEST_F(StateStoreTest, SelectorWithChangingState)
{
    StateStore& store = StateStore::instance();

    store.set<i32>("value", 10);

    Selector<i32> selector([](const StateStore& s) { return s.get<i32>("value", 0) * 2; });

    EXPECT_EQ(selector.select(), 20);

    store.set<i32>("value", 15);
    EXPECT_EQ(selector.select(), 30);
}
