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

#pragma once

#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <any>
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::state {

/**
 * @brief 状态存储
 *
 * 全局状态管理器，支持：
 * - 键值对存储任意类型值
 * - 订阅状态变化
 * - 动作分发（类似Redux）
 * - 中间件机制
 * - 批量更新
 *
 * 使用示例：
 * @code
 * StateStore& store = StateStore::instance();
 *
 * // 设置状态
 * store.set("player.health", 100);
 * store.set("player.maxHealth", 100);
 *
 * // 获取状态
 * i32 health = store.get<i32>("player.health");
 *
 * // 订阅变化
 * u64 id = store.subscribe("player.health", []() {
 *     std::cout << "Health changed!" << std::endl;
 * });
 *
 * // 取消订阅
 * store.unsubscribe(id);
 * @endcode
 */
class StateStore {
public:
    using Subscriber = std::function<void()>;
    using SubscriberId = u64;
    using Action = std::function<void(StateStore&)>;
    using Middleware = std::function<void(const std::string&, const std::any&, StateStore&)>;

    /**
     * @brief 获取单例实例
     */
    static StateStore& instance()
    {
        static StateStore instance;
        return instance;
    }

    // 禁止拷贝和移动
    StateStore(const StateStore&) = delete;
    StateStore& operator=(const StateStore&) = delete;

    // ==================== 状态访问 ====================

    /**
     * @brief 获取状态值
     *
     * @tparam T 值类型
     * @param key 状态键
     * @param defaultValue 默认值（如果键不存在）
     * @return 状态值
     */
    template <typename T>
    [[nodiscard]] T get(const std::string& key, const T& defaultValue = T{}) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_state.find(key);
        if (it != m_state.end()) {
            try {
                return std::any_cast<T>(it->second);
            }
            catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief 检查键是否存在
     */
    [[nodiscard]] bool has(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state.find(key) != m_state.end();
    }

    /**
     * @brief 获取状态的std::any引用，用于类型转换
     * @param key 状态键
     * @return 状态值的const引用，键不存在时抛出异常
     */
    [[nodiscard]] const std::any& getAny(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_state.find(key);
        if (it == m_state.end()) {
            static const std::any empty;
            return empty;
        }
        return it->second;
    }

    /**
     * @brief 设置状态值
     *
     * @tparam T 值类型
     * @param key 状态键
     * @param value 新值
     */
    template <typename T>
    void set(const std::string& key, T value)
    {
        std::vector<Subscriber> subscribersToNotify;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // 检查值是否真正改变，避免不必要的通知
            auto it = m_state.find(key);
            if (it != m_state.end()) {
                try {
                    if (std::any_cast<T>(&it->second) != nullptr && *std::any_cast<T>(&it->second) == value) {
                        return; // 值未改变，跳过
                    }
                }
                catch (...) {
                    // 类型不匹配，继续设置新值
                }
            }

            // 调用中间件（仅在非批量模式下）
            if (m_batchDepth == 0) {
                for (const auto& middleware : m_middlewares) {
                    middleware(key, value, *this);
                }
            }

            // 设置值
            m_state[key] = std::any(std::move(value));

            if (m_batchDepth > 0) {
                // 批量模式：记录变更，延迟通知
                if (std::find(m_pendingKeys.begin(), m_pendingKeys.end(), key) == m_pendingKeys.end()) {
                    m_pendingKeys.push_back(key);
                }
            } else {
                // 非批量模式：立即收集订阅者
                auto it = m_subscribers.find(key);
                if (it != m_subscribers.end()) {
                    for (const auto& pair : it->second) {
                        subscribersToNotify.push_back(pair.second);
                    }
                }
            }
        }

        // 在锁外通知订阅者（仅非批量模式）
        for (const auto& subscriber : subscribersToNotify) {
            subscriber();
        }
    }

    /**
     * @brief 删除状态值
     */
    void remove(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.erase(key);
    }

    /**
     * @brief 清空所有状态
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state.clear();
    }

    /**
     * @brief 获取所有键
     */
    [[nodiscard]] std::vector<std::string> keys() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> result;
        result.reserve(m_state.size());
        for (const auto& pair : m_state) {
            result.push_back(pair.first);
        }
        return result;
    }

    // ==================== 订阅 ====================

    /**
     * @brief 订阅状态变化
     *
     * @param key 状态键
     * @param subscriber 订阅者函数
     * @return 订阅者ID，用于取消订阅
     */
    SubscriberId subscribe(const std::string& key, Subscriber subscriber)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        SubscriberId id = m_nextSubscriberId++;
        m_subscribers[key].emplace_back(id, std::move(subscriber));
        m_subscriberToKey[id] = key;
        return id;
    }

    /**
     * @brief 取消订阅
     *
     * @param id 订阅者ID
     * @return 是否成功取消
     */
    bool unsubscribe(SubscriberId id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto keyIt = m_subscriberToKey.find(id);
        if (keyIt == m_subscriberToKey.end()) {
            return false;
        }

        const std::string& key = keyIt->second;
        auto& subscribers = m_subscribers[key];
        auto it =
            std::find_if(subscribers.begin(), subscribers.end(), [id](const auto& pair) { return pair.first == id; });

        if (it != subscribers.end()) {
            subscribers.erase(it);
            m_subscriberToKey.erase(keyIt);
            return true;
        }

        return false;
    }

    /**
     * @brief 取消某键的所有订阅
     */
    void unsubscribeAll(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_subscribers.find(key);
        if (it != m_subscribers.end()) {
            for (const auto& pair : it->second) {
                m_subscriberToKey.erase(pair.first);
            }
            m_subscribers.erase(it);
        }
    }

    // ==================== 动作 ====================

    /**
     * @brief 派发动作
     *
     * @param action 动作函数
     */
    void dispatch(Action action) { action(*this); }

    /**
     * @brief 添加中间件
     *
     * 中间件在每次状态变化时被调用（仅非批量模式）
     */
    void addMiddleware(Middleware middleware)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_middlewares.push_back(std::move(middleware));
    }

    /**
     * @brief 清除所有中间件
     */
    void clearMiddlewares()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_middlewares.clear();
    }

    // ==================== 批量操作 ====================

    /**
     * @brief 批量更新
     *
     * 在批量更新期间不通知订阅者，更新完成后一次性通知。
     * 支持嵌套调用，只有最外层的批量更新结束才会通知。
     *
     * @note 批量更新期间中间件不会被调用
     *
     * @param func 批量更新函数
     */
    template <typename Func>
    void batchUpdate(Func&& func)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_batchDepth++;
        }

        try {
            func(*this);
        }
        catch (...) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_batchDepth--;
            }
            throw;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_batchDepth--;
            if (m_batchDepth == 0) {
                // 交换出待通知的键，避免在通知期间持有锁
                std::vector<std::string> keysToNotify = std::move(m_pendingKeys);
                m_pendingKeys.clear();

                // 收集所有订阅者
                std::vector<Subscriber> subscribersToNotify;
                for (const auto& key : keysToNotify) {
                    auto it = m_subscribers.find(key);
                    if (it != m_subscribers.end()) {
                        for (const auto& pair : it->second) {
                            subscribersToNotify.push_back(pair.second);
                        }
                    }
                }

                // 释放锁后通知订阅者
                // 注意：这里锁已经释放，因为离开了作用域
                for (const auto& subscriber : subscribersToNotify) {
                    subscriber();
                }
            }
        }
    }

private:
    StateStore() = default;

    std::unordered_map<std::string, std::any> m_state;
    std::unordered_map<std::string, std::vector<std::pair<SubscriberId, Subscriber>>> m_subscribers;
    std::unordered_map<SubscriberId, std::string> m_subscriberToKey;
    std::vector<Middleware> m_middlewares;
    std::vector<std::string> m_pendingKeys;
    std::atomic<SubscriberId> m_nextSubscriberId{1};
    i32 m_batchDepth = 0;
    mutable std::mutex m_mutex;
};

/**
 * @brief 状态选择器
 *
 * 用于从状态存储中选择和转换数据
 *
 * @tparam T 选择结果类型
 */
template <typename T>
class Selector {
public:
    using SelectFunc = std::function<T(const StateStore&)>;

    /**
     * @brief 构造函数
     * @param select 选择函数
     */
    explicit Selector(SelectFunc select)
        : m_select(std::move(select))
    {}

    /**
     * @brief 构造函数（带订阅键）
     * @param select 选择函数
     * @param key 用于订阅的键
     */
    Selector(SelectFunc select, std::string key)
        : m_select(std::move(select))
        , m_key(std::move(key))
    {}

    /**
     * @brief 执行选择
     */
    [[nodiscard]] T select() const { return m_select(StateStore::instance()); }

    /**
     * @brief 设置订阅键
     */
    void setKey(const std::string& key) { m_key = key; }

    /**
     * @brief 订阅选择结果变化
     *
     * @param subscriber 订阅者函数
     * @return 订阅ID
     *
     * @note 必须先设置订阅键才能使用此方法
     */
    u64 subscribe(std::function<void(const T&)> subscriber)
    {
        MC_ASSERT_DEBUG_MSG(!m_key.empty(), "StateSelector::subscribe() called without setting a key");
        if (m_key.empty()) {
            return 0; // 无效订阅
        }
        return StateStore::instance().subscribe(m_key, [this, subscriber]() {
            T newValue = select();
            subscriber(newValue);
        });
    }

private:
    SelectFunc m_select;
    std::string m_key; // 用于订阅的键
};

/**
 * @brief 状态绑定点
 *
 * 连接响应式状态和组件
 */
class StateBindingPoint {
public:
    using UpdateCallback = std::function<void()>;

    /**
     * @brief 创建绑定点
     * @param storeKey 状态存储键
     */
    explicit StateBindingPoint(std::string storeKey)
        : m_storeKey(std::move(storeKey))
    {}

    /**
     * @brief 获取值
     */
    template <typename T>
    [[nodiscard]] T get() const
    {
        return StateStore::instance().get<T>(m_storeKey);
    }

    /**
     * @brief 获取值（带默认值）
     */
    template <typename T>
    [[nodiscard]] T get(const T& defaultValue) const
    {
        return StateStore::instance().get<T>(m_storeKey, defaultValue);
    }

    /**
     * @brief 设置值
     */
    template <typename T>
    void set(const T& value)
    {
        StateStore::instance().set(m_storeKey, value);
    }

    /**
     * @brief 绑定更新回调
     */
    u64 bind(UpdateCallback callback) { return StateStore::instance().subscribe(m_storeKey, std::move(callback)); }

    /**
     * @brief 解除绑定
     */
    void unbind(u64 id) { StateStore::instance().unsubscribe(id); }

    /**
     * @brief 获取存储键
     */
    [[nodiscard]] const std::string& key() const { return m_storeKey; }

private:
    std::string m_storeKey;
};

} // namespace mc::client::ui::kagero::state
