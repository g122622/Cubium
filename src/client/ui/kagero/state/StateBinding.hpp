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

#include "ReactiveState.hpp"
#include "StateStore.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::state {

/**
 * @brief 状态绑定工具
 *
 * 提供状态与组件之间的绑定功能
 */
namespace binding {

/**
 * @brief 创建状态绑定
 *
 * @tparam T 状态类型
 * @param key 状态键
 * @return 绑定点
 */
template <typename T>
StateBindingPoint bind(const std::string& key)
{
    return StateBindingPoint(key);
}

/**
 * @brief 双向绑定到 Reactive 状态
 *
 * @tparam T 状态类型
 * @param reactive 响应式状态引用
 * @param key 存储键
 * @return 绑定点
 *
 * @note 双向绑定会自动同步 Reactive 和 StateStore 的值。
 *       当 Reactive 变化时，StateStore 会更新；
 *       当 StateStore 变化时，Reactive 会更新。
 *       使用值比较来避免循环更新。
 */
template <typename T>
StateBindingPoint bindReactive(Reactive<T>& reactive, const std::string& key)
{
    // 创建双向同步
    StateStore::instance().set(key, reactive.get());

    // 使用 shared_ptr 共享旧值，用于双向同步时避免循环更新
    auto lastStoreValue = std::make_shared<T>(reactive.get());

    // 从 Reactive 到 Store（按值捕获 key，确保 lambda 持有独立的 key 副本）
    reactive.observe([key, lastStoreValue](const T& /*oldValue*/, const T& newValue) {
        // 避免循环：如果新值与上次存储的值相同，跳过更新
        if (newValue != *lastStoreValue) {
            *lastStoreValue = newValue;
            StateStore::instance().set(key, newValue);
        }
    });

    // 从 Store 到 Reactive（按值捕获 key，确保 lambda 持有独立的 key 副本）
    StateStore::instance().subscribe(key, [&reactive, lastStoreValue, key]() {
        T value = StateStore::instance().get<T>(key);
        // 避免循环：如果新值与 Reactive 当前值相同，跳过更新
        if (value != reactive.get() && value != *lastStoreValue) {
            *lastStoreValue = value;
            reactive.set(value);
        }
    });

    return StateBindingPoint(key);
}

/**
 * @brief 创建计算属性
 *
 * @tparam T 结果类型
 * @tparam Func 计算函数类型
 * @param compute 计算函数
 * @return 计算属性
 */
template <typename T, typename Func>
Computed<T> computed(Func&& compute)
{
    return Computed<T>(std::forward<Func>(compute));
}

/**
 * @brief 监视状态变化（带旧值和新值）
 *
 * @tparam T 状态类型
 * @param key 状态键
 * @param callback 回调函数，接收旧值和新值
 * @return 订阅ID
 */
template <typename T>
u64 watch(const std::string& key, std::function<void(const T&, const T&)> callback)
{
    // 使用 shared_ptr 来持久化旧值
    auto oldValue = std::make_shared<T>(StateStore::instance().get<T>(key));

    return StateStore::instance().subscribe(key, [key, callback, oldValue]() {
        T newValue = StateStore::instance().get<T>(key);
        if (*oldValue != newValue) {
            callback(*oldValue, newValue);
            *oldValue = newValue;
        }
    });
}

/**
 * @brief 监视多个状态变化
 *
 * @param keys 状态键列表
 * @param callback 回调函数
 * @return 订阅ID列表
 */
inline std::vector<u64> watchAll(const std::vector<std::string>& keys, std::function<void()> callback)
{
    std::vector<u64> ids;
    ids.reserve(keys.size());
    for (const auto& key : keys) {
        ids.push_back(StateStore::instance().subscribe(key, callback));
    }
    return ids;
}

/**
 * @brief 取消监视
 */
inline void unwatch(u64 id)
{
    StateStore::instance().unsubscribe(id);
}

/**
 * @brief 取消多个监视
 */
inline void unwatchAll(const std::vector<u64>& ids)
{
    for (u64 id : ids) {
        StateStore::instance().unsubscribe(id);
    }
}

} // namespace binding

/**
 * @brief 状态作用域
 *
 * 用于管理一组相关的状态绑定，在作用域结束时自动取消所有订阅
 */
class StateScope {
public:
    StateScope() = default;

    ~StateScope() { clear(); }

    // 禁止拷贝
    StateScope(const StateScope&) = delete;
    StateScope& operator=(const StateScope&) = delete;

    // 允许移动
    StateScope(StateScope&& other) noexcept
        : m_subscriptions(std::move(other.m_subscriptions))
    {}

    StateScope& operator=(StateScope&& other) noexcept
    {
        if (this != &other) {
            clear();
            m_subscriptions = std::move(other.m_subscriptions);
        }
        return *this;
    }

    /**
     * @brief 订阅状态变化
     */
    u64 subscribe(const std::string& key, std::function<void()> callback)
    {
        u64 id = StateStore::instance().subscribe(key, std::move(callback));
        m_subscriptions.push_back(id);
        return id;
    }

    /**
     * @brief 监视状态变化
     */
    template <typename T>
    u64 watch(const std::string& key, std::function<void(const T&, const T&)> callback)
    {
        u64 id = binding::watch<T>(key, std::move(callback));
        m_subscriptions.push_back(id);
        return id;
    }

    /**
     * @brief 取消所有订阅
     */
    void clear()
    {
        for (u64 id : m_subscriptions) {
            StateStore::instance().unsubscribe(id);
        }
        m_subscriptions.clear();
    }

    /**
     * @brief 获取订阅数量
     */
    [[nodiscard]] Size size() const { return m_subscriptions.size(); }

private:
    std::vector<u64> m_subscriptions;
};

/**
 * @brief 状态上下文
 *
 * 提供组件级别的状态管理，包括响应式状态的创建和生命周期管理
 */
class StateContext {
public:
    /**
     * @brief 响应式状态包装器基类
     */
    class IReactiveHolder {
    public:
        virtual ~IReactiveHolder() = default;
    };

    /**
     * @brief 响应式状态包装器模板
     */
    template <typename T>
    class ReactiveHolder : public IReactiveHolder {
    public:
        explicit ReactiveHolder(T initialValue)
            : reactive(std::move(initialValue))
        {}

        Reactive<T> reactive;
    };

    StateContext() = default;

    /**
     * @brief 获取状态值
     */
    template <typename T>
    [[nodiscard]] T get(const std::string& key) const
    {
        return StateStore::instance().get<T>(key);
    }

    /**
     * @brief 设置状态值
     */
    template <typename T>
    void set(const std::string& key, T value)
    {
        StateStore::instance().set(key, std::move(value));
    }

    /**
     * @brief 创建或获取响应式状态
     *
     * @tparam T 状态类型
     * @param key 状态键
     * @param initialValue 初始值
     * @return 响应式状态的引用
     *
     * @note 生命周期由 StateContext 管理
     */
    template <typename T>
    Reactive<T>& reactive(const std::string& key, T initialValue)
    {
        auto it = m_reactives.find(key);
        if (it != m_reactives.end()) {
            auto* holder = dynamic_cast<ReactiveHolder<T>*>(it->second.get());
            if (holder) {
                return holder->reactive;
            }
            // 类型不匹配，重新创建
        }

        auto holder = std::make_unique<ReactiveHolder<T>>(std::move(initialValue));
        auto* ptr = holder.get();
        m_reactives[key] = std::move(holder);
        return ptr->reactive;
    }

    /**
     * @brief 获取状态作用域
     */
    StateScope& scope() { return m_scope; }

    /**
     * @brief 清除所有响应式状态
     */
    void clear()
    {
        m_reactives.clear();
        m_scope.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IReactiveHolder>> m_reactives;
    StateScope m_scope;
};

} // namespace mc::client::ui::kagero::state
