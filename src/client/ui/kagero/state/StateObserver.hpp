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
#include "common/core/Types.hpp"
#include <chrono>
#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::state {

/**
 * @brief 状态观察者接口
 *
 * 定义状态变化观察者的通用接口。
 * 观察者的生命周期由外部管理，注册时传入裸指针，调用方需确保指针有效性。
 */
class IStateObserver {
public:
    virtual ~IStateObserver() = default;

    /**
     * @brief 当观察的状态变化时调用
     */
    virtual void onStateChanged() = 0;
};

/**
 * @brief 状态观察者管理器
 *
 * 管理多个状态观察者，支持批量通知。
 * 使用 unordered_set 存储，自动去重，添加/移除为 O(1)。
 */
class StateObserverManager {
public:
    /**
     * @brief 添加观察者
     */
    void addObserver(IStateObserver* observer) { m_observers.insert(observer); }

    /**
     * @brief 移除观察者
     */
    void removeObserver(IStateObserver* observer) { m_observers.erase(observer); }

    /**
     * @brief 通知所有观察者
     */
    void notifyAll()
    {
        for (auto* observer : m_observers) {
            observer->onStateChanged();
        }
    }

    /**
     * @brief 清除所有观察者
     */
    void clear() { m_observers.clear(); }

    /**
     * @brief 获取观察者数量
     */
    [[nodiscard]] Size count() const { return m_observers.size(); }

private:
    std::unordered_set<IStateObserver*> m_observers;
};

/**
 * @brief 自动状态观察者
 *
 * 自动管理响应式状态的观察，构造时注册观察者，析构时自动移除。
 * 注意：观察者生命周期不能超过被观察的 Reactive 对象。
 *
 * @tparam T 状态类型
 */
template <typename T>
class AutoObserver {
public:
    using Callback = std::function<void(const T&)>;

    /**
     * @brief 构造观察者
     * @param reactive 响应式状态
     * @param callback 变化回调
     */
    AutoObserver(Reactive<T>& reactive, Callback callback)
        : m_reactive(reactive)
        , m_callback(std::move(callback))
    {
        m_observerId = m_reactive.observe([this](const T& /*oldValue*/, const T& newValue) {
            if (m_callback) {
                m_callback(newValue);
            }
        });
    }

    /**
     * @brief 析构时自动移除观察
     */
    ~AutoObserver()
    {
        if (m_observerId != 0) {
            m_reactive.removeObserver(m_observerId);
        }
    }

    // 禁止拷贝
    AutoObserver(const AutoObserver&) = delete;
    AutoObserver& operator=(const AutoObserver&) = delete;

    // 禁止移动赋值——m_reactive 是引用无法重新绑定，赋值后 this 仍观察旧的 reactive
    AutoObserver& operator=(AutoObserver&&) = delete;

    /**
     * @brief 移动构造函数
     *
     * @note 移动后需要重新注册观察者，因为回调 lambda 捕获了 this 指针
     */
    AutoObserver(AutoObserver&& other) noexcept
        : m_reactive(other.m_reactive)
        , m_callback(std::move(other.m_callback))
        , m_observerId(0)
    {
        // 重新注册观察者，因为回调 lambda 捕获的是 this 指针
        m_observerId = m_reactive.observe([this](const T& /*oldValue*/, const T& newValue) {
            if (m_callback) {
                m_callback(newValue);
            }
        });
        // 移除旧观察者
        other.m_reactive.removeObserver(other.m_observerId);
        other.m_observerId = 0;
    }

    /**
     * @brief 手动触发回调
     */
    void trigger()
    {
        if (m_callback) {
            m_callback(m_reactive.get());
        }
    }

private:
    Reactive<T>& m_reactive;
    Callback m_callback;
    typename Reactive<T>::ObserverId m_observerId;
};

/**
 * @brief 多状态观察者
 *
 * 观察多个响应式状态，当任一状态变化时触发回调。
 * 适合需要同时监听多个状态并统一响应的场景。
 *
 * @warning 观察者生命周期不能超过任何被观察的 Reactive 对象
 */
class MultiStateObserver {
public:
    using Callback = std::function<void()>;

    MultiStateObserver() = default;
    ~MultiStateObserver() { clear(); }

    // 禁止拷贝
    MultiStateObserver(const MultiStateObserver&) = delete;
    MultiStateObserver& operator=(const MultiStateObserver&) = delete;

    /**
     * @brief 观察响应式状态
     *
     * @param reactive 响应式状态指针（调用方需确保其生命周期长于观察者）
     *
     * @note 使用指针而非引用，避免lambda捕获引用导致的悬垂风险。
     *       调用方需确保被观察的 Reactive 对象在观察者销毁前不会被销毁。
     */
    template <typename T>
    void observe(Reactive<T>* reactive)
    {
        auto id = reactive->observe([this](const T&, const T&) {
            if (m_callback) {
                m_callback();
            }
        });
        // 使用裸指针存储，析构时安全移除观察者
        m_observerEntries.push_back(
            {id, reactive, [](ObserverId id, void* ptr) { static_cast<Reactive<T>*>(ptr)->removeObserver(id); }});
    }

    /**
     * @brief 设置回调
     */
    void setCallback(Callback callback) { m_callback = std::move(callback); }

    /**
     * @brief 清除所有观察
     */
    void clear()
    {
        for (auto& entry : m_observerEntries) {
            entry.remove(entry.id, entry.reactivePtr);
        }
        m_observerEntries.clear();
    }

private:
    using ObserverId = u64;

    /**
     * @brief 观察者条目，用于析构时移除观察者
     */
    struct ObserverEntry {
        ObserverId id;
        void* reactivePtr;
        void (*remove)(ObserverId id, void* ptr);
    };

    Callback m_callback;
    std::vector<ObserverEntry> m_observerEntries;
};

/**
 * @brief 延迟状态观察者
 *
 * 延迟触发状态变化通知，避免频繁更新。
 * 需要定期调用 update() 检查延迟是否已过，适合在游戏主循环中使用。
 *
 * @tparam T 状态类型，必须可默认构造
 */
template <typename T>
class DebouncedObserver {
public:
    using Callback = std::function<void(const T&)>;

    /**
     * @brief 构造观察者
     * @param reactive 响应式状态
     * @param callback 变化回调
     * @param delayMs 延迟时间（毫秒）
     */
    DebouncedObserver(Reactive<T>& reactive, Callback callback, u32 delayMs = 100)
        : m_reactive(reactive)
        , m_callback(std::move(callback))
        , m_delayMs(delayMs)
    {
        m_observerId = m_reactive.observe([this](const T&, const T& newValue) {
            m_pendingValue = newValue;
            m_hasPending = true;
            m_lastChangeTime = std::chrono::steady_clock::now();
        });
    }

    ~DebouncedObserver() { m_reactive.removeObserver(m_observerId); }

    // 禁止拷贝
    DebouncedObserver(const DebouncedObserver&) = delete;
    DebouncedObserver& operator=(const DebouncedObserver&) = delete;

    /**
     * @brief 更新（检查是否应该触发回调）
     *
     * 每帧调用此方法来检查延迟是否已过
     */
    void update()
    {
        if (!m_hasPending) return;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastChangeTime);

        if (elapsed.count() >= m_delayMs) {
            if (m_callback) {
                m_callback(m_pendingValue);
            }
            m_hasPending = false;
        }
    }

    /**
     * @brief 立即触发回调（跳过延迟）
     */
    void flush()
    {
        if (m_hasPending && m_callback) {
            m_callback(m_pendingValue);
        }
        m_hasPending = false;
    }

private:
    Reactive<T>& m_reactive;
    Callback m_callback;
    typename Reactive<T>::ObserverId m_observerId;
    u32 m_delayMs;

    T m_pendingValue;
    bool m_hasPending = false;
    std::chrono::steady_clock::time_point m_lastChangeTime;
};

} // namespace mc::client::ui::kagero::state
