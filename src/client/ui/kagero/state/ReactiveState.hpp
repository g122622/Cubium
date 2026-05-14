#pragma once

#include "../Types.hpp"
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::client::ui::kagero::state {

/**
 * @brief 响应式状态模板类
 *
 * 提供响应式状态管理，当状态值变化时通知所有观察者。
 *
 * 使用示例：
 * @code
 * Reactive<i32> count(0);
 * count.observe([](i32 oldValue, i32 newValue) {
 *     std::cout << "Count changed from " << oldValue << " to " << newValue << std::endl;
 * });
 * count.set(10); // 触发观察者
 * @endcode
 *
 * @tparam T 状态值类型
 */
template <typename T>
class Reactive {
public:
    using Observer = std::function<void(const T&, const T&)>;
    using ObserverId = u64;

    /**
     * @brief 默认构造函数
     */
    Reactive()
        : m_value()
    {}

    /**
     * @brief 带初始值的构造函数
     */
    explicit Reactive(T initialValue)
        : m_value(std::move(initialValue))
    {}

    /**
     * @brief 获取当前值
     */
    [[nodiscard]] const T& get() const { return m_value; }

    /**
     * @brief 获取当前值的引用
     */
    [[nodiscard]] T& getRef() { return m_value; }

    /**
     * @brief 设置新值
     *
     * 如果新值与当前值不同，将通知所有观察者
     */
    void set(const T& newValue)
    {
        if (m_value != newValue) {
            T oldValue = m_value;
            m_value = newValue;
            notify(oldValue, newValue);
        }
    }

    /**
     * @brief 设置新值（移动版本）
     */
    void set(T&& newValue)
    {
        if (m_value != newValue) {
            T oldValue = m_value;
            m_value = std::move(newValue);
            notify(oldValue, m_value);
        }
    }

    /**
     * @brief 隐式转换为值类型
     */
    operator const T&() const { return m_value; }

    /**
     * @brief 赋值操作符
     */
    Reactive& operator=(const T& newValue)
    {
        set(newValue);
        return *this;
    }

    Reactive& operator=(T&& newValue)
    {
        set(std::move(newValue));
        return *this;
    }

    /**
     * @brief 添加观察者
     *
     * @param observer 观察者函数
     * @return 观察者ID，用于移除观察者
     */
    ObserverId observe(Observer observer)
    {
        ObserverId id = m_nextObserverId++;
        m_observers.emplace_back(id, std::move(observer));
        return id;
    }

    /**
     * @brief 移除观察者
     *
     * @param id 观察者ID
     * @return 是否成功移除
     */
    bool removeObserver(ObserverId id)
    {
        auto it =
            std::find_if(m_observers.begin(), m_observers.end(), [id](const auto& pair) { return pair.first == id; });
        if (it != m_observers.end()) {
            m_observers.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief 清除所有观察者
     */
    void clearObservers() { m_observers.clear(); }

    /**
     * @brief 获取观察者数量
     */
    [[nodiscard]] size_t observerCount() const { return m_observers.size(); }

    /**
     * @brief 修改值并通知观察者（即使值相同）
     */
    void forceNotify() { notify(m_value, m_value); }

    /**
     * @brief 使用函数修改值
     *
     * @param modifier 修改函数，接收当前值的引用
     */
    template <typename Func>
    void modify(Func&& modifier)
    {
        T oldValue = m_value;
        modifier(m_value);
        if (oldValue != m_value) {
            notify(oldValue, m_value);
        }
    }

private:
    /**
     * @brief 通知所有观察者
     */
    void notify(const T& oldValue, const T& newValue)
    {
        // 复制观察者列表以避免迭代时修改
        auto observers = m_observers;
        for (const auto& pair : observers) {
            pair.second(oldValue, newValue);
        }
    }

    T m_value;
    std::vector<std::pair<ObserverId, Observer>> m_observers;
    ObserverId m_nextObserverId = 1;
};

/**
 * @brief 计算属性
 *
 * 基于其他响应式状态自动计算值
 *
 * @tparam T 计算结果类型
 */
template <typename T>
class Computed {
public:
    using ComputeFunc = std::function<T()>;

    /**
     * @brief 构造函数
     * @param compute 计算函数
     */
    explicit Computed(ComputeFunc compute)
        : m_compute(std::move(compute))
        , m_cachedValue(m_compute())
        , m_dirty(false)
    {} // 初始已计算，不需要重新计算

    /**
     * @brief 获取计算值
     */
    [[nodiscard]] const T& get()
    {
        if (m_dirty) {
            m_cachedValue = m_compute();
            m_dirty = false;
        }
        return m_cachedValue;
    }

    /**
     * @brief 标记为需要重新计算
     */
    void markDirty() { m_dirty = true; }

    /**
     * @brief 隐式转换
     */
    operator const T&() { return get(); }

private:
    ComputeFunc m_compute;
    T m_cachedValue;
    bool m_dirty = false;
};

/**
 * @brief 绑定器
 *
 * 用于将响应式状态绑定到组件属性
 */
template <typename T>
class Binding {
public:
    using Getter = std::function<T()>;
    using Setter = std::function<void(const T&)>;

    /**
     * @brief 创建单向绑定（只读）
     */
    static Binding<T> readOnly(Getter getter) { return Binding<T>(std::move(getter), nullptr); }

    /**
     * @brief 创建双向绑定
     */
    static Binding<T> twoWay(Getter getter, Setter setter) { return Binding<T>(std::move(getter), std::move(setter)); }

    /**
     * @brief 从Reactive创建双向绑定
     */
    static Binding<T> fromReactive(Reactive<T>& reactive)
    {
        return twoWay(
            [&reactive]() -> T { return reactive.get(); }, [&reactive](const T& value) { reactive.set(value); });
    }

    /**
     * @brief 创建常量绑定
     */
    static Binding<T> constant(const T& value)
    {
        return readOnly([value]() -> T { return value; });
    }

    /**
     * @brief 获取值
     */
    [[nodiscard]] T get() const { return m_getter(); }

    /**
     * @brief 设置值
     */
    void set(const T& value) const
    {
        if (m_setter) {
            m_setter(value);
        }
    }

    /**
     * @brief 检查是否可写
     */
    [[nodiscard]] bool isWritable() const { return m_setter != nullptr; }

    /**
     * @brief 隐式转换
     */
    operator T() const { return get(); }

private:
    Binding(Getter getter, Setter setter)
        : m_getter(std::move(getter))
        , m_setter(std::move(setter))
    {}

    Getter m_getter;
    Setter m_setter;
};

} // namespace mc::client::ui::kagero::state
