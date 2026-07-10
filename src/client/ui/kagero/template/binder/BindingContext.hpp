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

#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/ReactiveState.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include <any>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::tpl::binder {

/**
 * @brief 动态值类型
 *
 * 用于在运行时存储和传递任意类型的值。
 * 支持基本类型、数组类型和对象类型。
 */
class Value {
public:
    Value()
        : m_type(ValueType::Null)
    {}

    // 基本类型构造
    explicit Value(std::nullptr_t)
        : m_type(ValueType::Null)
    {}
    explicit Value(bool v)
        : m_type(ValueType::Bool)
        , m_boolValue(v)
    {}
    // 32 位整型构造（内部以 i64 存储，无损扩展）
    explicit Value(i32 v)
        : m_type(ValueType::Integer)
        , m_intValue(static_cast<i64>(v))
    {}
    // 64 位整型构造（原生支持，避免 i64 → i32 精度丢失）
    explicit Value(i64 v)
        : m_type(ValueType::Integer)
        , m_intValue(v)
    {}
    // 32 位无符号整型构造（内部以 i64 存储，无损）
    explicit Value(u32 v)
        : m_type(ValueType::Integer)
        , m_intValue(static_cast<i64>(v))
    {}
    // 32 位浮点构造（内部以 f64 存储，无损扩展）
    explicit Value(f32 v)
        : m_type(ValueType::Float)
        , m_floatValue(static_cast<f64>(v))
    {}
    // 64 位浮点构造（原生支持，避免 f64 → f32 精度丢失）
    explicit Value(f64 v)
        : m_type(ValueType::Float)
        , m_floatValue(v)
    {}
    explicit Value(const std::string& v)
        : m_type(ValueType::String)
        , m_stringValue(v)
    {}
    explicit Value(std::string&& v)
        : m_type(ValueType::String)
        , m_stringValue(std::move(v))
    {}
    explicit Value(const char* v)
        : m_type(ValueType::String)
        , m_stringValue(v ? v : "")
    {} // 处理字符串字面量

    // 数组类型构造
    explicit Value(const std::vector<Value>& v)
        : m_type(ValueType::Array)
        , m_arrayValue(v)
    {}
    explicit Value(std::vector<Value>&& v)
        : m_type(ValueType::Array)
        , m_arrayValue(std::move(v))
    {}

    // 从std::any构造
    static Value fromAny(const std::any& any);

    // 类型检查
    [[nodiscard]] bool isNull() const { return m_type == ValueType::Null; }
    [[nodiscard]] bool isBool() const { return m_type == ValueType::Bool; }
    [[nodiscard]] bool isInteger() const { return m_type == ValueType::Integer; }
    [[nodiscard]] bool isFloat() const { return m_type == ValueType::Float; }
    [[nodiscard]] bool isString() const { return m_type == ValueType::String; }
    [[nodiscard]] bool isArray() const { return m_type == ValueType::Array; }
    [[nodiscard]] bool isObject() const { return m_type == ValueType::Object; }
    [[nodiscard]] bool isNumber() const { return isInteger() || isFloat(); }

    // 值获取
    [[nodiscard]] bool asBool() const;
    // 返回 i32（从内部 i64 存储窄化，对超出 i32 范围的值会截断）
    [[nodiscard]] i32 asInteger() const;
    // 返回 i64（原生精度，无截断）
    [[nodiscard]] i64 asI64() const;
    // 返回 u32（从内部 i64 存储窄化，对超出 u32 范围的值会截断）
    [[nodiscard]] u32 asU32() const;
    // 返回 f32（从内部 f64 存储窄化，对超出 f32 精度的值会损失精度）
    [[nodiscard]] f32 asFloat() const;
    // 返回 f64（原生精度，无损失）
    [[nodiscard]] f64 asF64() const;
    [[nodiscard]] const std::string& asString() const;
    [[nodiscard]] std::string toString() const;

    // 数组操作
    [[nodiscard]] size_t arraySize() const;
    [[nodiscard]] Value arrayGet(size_t index) const;
    [[nodiscard]] const std::vector<Value>& asArray() const { return m_arrayValue; }
    [[nodiscard]] std::vector<Value>& asArray() { return m_arrayValue; }

    // 创建数组值
    static Value fromArray(const std::vector<Value>& values) { return Value(values); }
    static Value emptyArray() { return Value(std::vector<Value>{}); }

    // 对象操作
    [[nodiscard]] Value getProperty(const std::string& name) const;
    void setProperty(const std::string& name, const Value& value);
    [[nodiscard]] bool hasProperty(const std::string& name) const;
    [[nodiscard]] const std::unordered_map<std::string, Value>& asObject() const { return m_objectValue; }
    [[nodiscard]] std::unordered_map<std::string, Value>& asObject() { return m_objectValue; }

    // 创建对象值
    static Value fromObject(std::unordered_map<std::string, Value> properties)
    {
        Value v;
        v.m_type = ValueType::Object;
        v.m_objectValue = std::move(properties);
        return v;
    }

    // 获取数组元素
    [[nodiscard]] Value getElement(size_t index) const;

    // 类型转换
    [[nodiscard]] i32 toInteger() const;
    [[nodiscard]] i64 toI64() const;
    [[nodiscard]] u32 toU32() const;
    [[nodiscard]] f32 toFloat() const;
    [[nodiscard]] f64 toF64() const;
    [[nodiscard]] bool toBool() const;

    // 比较
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const { return !(*this == other); }

    // 类型枚举
    enum class ValueType : u8 { Null, Bool, Integer, Float, String, Array, Object };

    [[nodiscard]] ValueType type() const { return m_type; }

private:
    ValueType m_type;
    bool m_boolValue = false;
    // 整型内部以 i64 存储，可无损容纳 i32/u32/i64
    i64 m_intValue = 0;
    // 浮点型内部以 f64 存储，可无损容纳 f32/f64
    f64 m_floatValue = 0.0;
    std::string m_stringValue;
    std::vector<Value> m_arrayValue;
    std::unordered_map<std::string, Value> m_objectValue;
};

/**
 * @brief 绑定上下文
 *
 * 连接模板实例与C++状态管理系统的桥梁。
 * 支持暴露变量供模板绑定，以及暴露回调供事件调用。
 *
 * 使用示例：
 * @code
 * BindingContext ctx(stateStore, eventBus);
 *
 * // 暴露变量
 * ctx.expose("player.name", &playerName);
 * ctx.expose("player.health", &playerHealth);
 *
 * // 暴露回调
 * ctx.exposeCallback("onStartGame", [](Widget* w, const Event& e) {
 *     // 处理点击
 * });
 *
 * // 解析绑定路径
 * Value name = ctx.resolveBinding("player.name");
 * @endcode
 */
class BindingContext {
public:
    /**
     * @brief 回调类型
     */
    using Callback = std::function<void(widget::Widget*, const event::Event&)>;

    /**
     * @brief 状态变化回调
     */
    using StateChangeCallback = std::function<void(const std::string& path, const Value& newValue)>;

    /**
     * @brief 构造函数
     * @param store 状态存储
     * @param eventBus 事件总线
     */
    BindingContext(state::StateStore& store, event::EventBus& eventBus);

    /**
     * @brief 析构函数
     */
    ~BindingContext() = default;

    // 禁止拷贝
    BindingContext(const BindingContext&) = delete;
    BindingContext& operator=(const BindingContext&) = delete;

    // 允许移动
    BindingContext(BindingContext&&) noexcept = default;
    BindingContext& operator=(BindingContext&&) noexcept = default;

    // ========== 变量暴露 ==========

    /**
     * @brief 暴露C++变量供模板绑定（只读）
     *
     * @tparam T 变量类型
     * @param path 绑定路径（如 "player.name"）
     * @param ptr 变量指针
     * @param onUpdate 可选的更新回调
     */
    template <typename T>
    void expose(const std::string& path, const T* ptr, StateChangeCallback onUpdate = nullptr)
    {
        ExposedVar var;
        var.ptr = const_cast<T*>(ptr);
        var.typeId = typeid(T).hash_code();
        var.typeName = typeid(T).name();
        var.readFunc = [ptr]() -> Value { return Value(*ptr); };
        var.onUpdate = std::move(onUpdate);
        m_exposedVars[path] = std::move(var);
    }

    /**
     * @brief 暴露C++变量供模板绑定（可写）
     *
     * @tparam T 变量类型
     * @param path 绑定路径
     * @param ptr 变量指针
     * @param onUpdate 可选的更新回调
     */
    template <typename T>
    void exposeWritable(const std::string& path, T* ptr, StateChangeCallback onUpdate = nullptr)
    {
        ExposedVar var;
        var.ptr = ptr;
        var.typeId = typeid(T).hash_code();
        var.typeName = typeid(T).name();
        var.isWritable = true;
        var.readFunc = [ptr]() -> Value { return Value(*ptr); };
        var.writeFunc = [ptr](const Value& v) {
            if constexpr (std::is_same_v<T, bool>) {
                *ptr = v.toBool();
            } else if constexpr (std::is_same_v<T, i64>) {
                // i64 目标使用原生 i64 访问器，避免经 i32 中转的精度丢失
                *ptr = v.toI64();
            } else if constexpr (std::is_same_v<T, u32>) {
                // u32 目标使用 i64 访问器再窄化，避免经 i32 中转对 >2^31 值的符号问题
                *ptr = static_cast<u32>(v.toI64());
            } else if constexpr (std::is_integral_v<T>) {
                *ptr = static_cast<T>(v.toInteger());
            } else if constexpr (std::is_same_v<T, f64>) {
                // f64 目标使用原生 f64 访问器，避免经 f32 中转的精度丢失
                *ptr = v.toF64();
            } else if constexpr (std::is_floating_point_v<T>) {
                *ptr = static_cast<T>(v.toFloat());
            } else if constexpr (std::is_same_v<T, std::string>) {
                // 使用 toString() 而不是 asString()，因为后者对非字符串类型返回空
                *ptr = v.toString();
            } else {
                *ptr = static_cast<T>(v.toFloat());
            }
        };
        var.onUpdate = std::move(onUpdate);
        m_exposedVars[path] = std::move(var);
    }

    /**
     * @brief 暴露Reactive状态
     *
     * @tparam T 状态类型
     * @param path 绑定路径
     * @param reactive 响应式状态引用
     */
    template <typename T>
    void exposeReactive(const std::string& path, state::Reactive<T>& reactive)
    {
        // 暴露读取
        ExposedVar var;
        var.ptr = &reactive;
        var.typeId = typeid(T).hash_code();
        var.typeName = typeid(T).name();
        var.isWritable = true;
        var.readFunc = [&reactive]() -> Value { return Value(reactive.get()); };
        var.writeFunc = [&reactive](const Value& v) {
            if constexpr (std::is_same_v<T, bool>) {
                reactive.set(v.toBool());
            } else if constexpr (std::is_same_v<T, i64>) {
                // i64 目标使用原生 i64 访问器，避免经 i32 中转的精度丢失
                reactive.set(v.toI64());
            } else if constexpr (std::is_same_v<T, u32>) {
                // u32 目标使用 i64 访问器再窄化，避免经 i32 中转对 >2^31 值的符号问题
                reactive.set(static_cast<u32>(v.toI64()));
            } else if constexpr (std::is_integral_v<T>) {
                reactive.set(static_cast<T>(v.toInteger()));
            } else if constexpr (std::is_same_v<T, f64>) {
                // f64 目标使用原生 f64 访问器，避免经 f32 中转的精度丢失
                reactive.set(v.toF64());
            } else if constexpr (std::is_floating_point_v<T>) {
                reactive.set(static_cast<T>(v.toFloat()));
            } else if constexpr (std::is_same_v<T, std::string>) {
                reactive.set(v.asString());
            } else {
                reactive.set(static_cast<T>(v.toFloat()));
            }
        };
        m_exposedVars[path] = std::move(var);

        // 订阅变化
        reactive.observe([this, path](const T& oldValue, const T& newValue) {
            (void)oldValue;
            notifyChange(path, Value(newValue));
        });
    }

    // ========== 回调暴露 ==========

    /**
     * @brief 暴露C++回调供模板事件调用
     *
     * @param name 回调名称（如 "onStartGame"）
     * @param callback 回调函数
     */
    void exposeCallback(const std::string& name, Callback callback);

    /**
     * @brief 暴露简单回调（无参数）
     *
     * @param name 回调名称
     * @param callback 回调函数
     */
    void exposeSimpleCallback(const std::string& name, std::function<void()> callback);

    /**
     * @brief 检查回调是否存在
     */
    [[nodiscard]] bool hasCallback(const std::string& name) const;

    /**
     * @brief 调用回调
     *
     * @param name 回调名称
     * @param source 事件源Widget
     * @param event 事件对象
     * @return 是否成功调用
     */
    bool invokeCallback(const std::string& name, widget::Widget* source, const event::Event& event);

    // ========== 绑定解析 ==========

    /**
     * @brief 解析绑定路径获取值
     *
     * @param path 绑定路径（如 "player.name" 或 "$slot.item"）
     * @param loopVar 当前循环变量名（如 "slot"）
     * @param loopValue 当前循环变量值
     * @return 解析后的值
     */
    [[nodiscard]] Value resolveBinding(
        const std::string& path, const std::string& loopVar = "", const Value& loopValue = Value()) const;

    /**
     * @brief 设置绑定路径的值
     *
     * @param path 绑定路径
     * @param value 新值
     * @return 是否成功设置
     */
    bool setBinding(const std::string& path, const Value& value);

    /**
     * @brief 检查路径是否存在
     */
    [[nodiscard]] bool hasPath(const std::string& path) const;

    /**
     * @brief 检查路径是否可写
     */
    [[nodiscard]] bool isWritable(const std::string& path) const;

    // ========== 状态变更通知 ==========

    /**
     * @brief 通知状态变更
     *
     * 当C++状态变更时调用，触发所有订阅该路径的回调
     *
     * @param path 变更的路径
     * @param newValue 新值
     */
    void notifyChange(const std::string& path, const Value& newValue);

    /**
     * @brief 订阅路径变更
     *
     * @param path 路径
     * @param callback 变更回调
     * @return 订阅ID
     */
    u64 subscribe(const std::string& path, StateChangeCallback callback);

    /**
     * @brief 取消订阅
     */
    void unsubscribe(u64 id);

    // ========== 循环变量 ==========

    /**
     * @brief 设置循环变量
     *
     * 在循环渲染时设置当前循环变量的值
     *
     * @param varName 变量名（如 "slot"）
     * @param value 变量值
     */
    void setLoopVariable(const std::string& varName, const Value& value);

    /**
     * @brief 清除循环变量
     */
    void clearLoopVariable(const std::string& varName);

    /**
     * @brief 获取循环变量
     */
    [[nodiscard]] Value getLoopVariable(const std::string& varName) const;

    /**
     * @brief 检查循环变量是否存在
     */
    [[nodiscard]] bool hasLoopVariable(const std::string& varName) const;

    // ========== 集合解析 ==========

    /**
     * @brief 解析集合（数组）
     *
     * 将路径解析为值数组，用于循环渲染
     *
     * @param path 集合路径
     * @return 值数组，如果路径不存在或不是集合则返回空数组
     */
    [[nodiscard]] std::vector<Value> resolveCollection(const std::string& path) const;

    /**
     * @brief 设置集合值供索引访问
     *
     * @param name 集合名
     * @param values 值数组
     */
    void setCollectionValue(const std::string& name, const std::vector<Value>& values);

    /**
     * @brief 设置任意类型的集合
     *
     * @tparam T 元素类型
     * @param name 集合名
     * @param items 元素数组
     */
    template <typename T>
    void setCollection(const std::string& name, const std::vector<T>& items)
    {
        std::vector<Value> values;
        values.reserve(items.size());
        for (const auto& item : items) {
            values.emplace_back(Value(item));
        }
        setCollectionValue(name, values);
    }

    // ========== 工具方法 ==========

    /**
     * @brief 获取状态存储
     */
    [[nodiscard]] state::StateStore& store() { return m_store; }
    [[nodiscard]] const state::StateStore& store() const { return m_store; }

    /**
     * @brief 获取事件总线
     */
    [[nodiscard]] event::EventBus& eventBus() { return m_eventBus; }
    [[nodiscard]] const event::EventBus& eventBus() const { return m_eventBus; }

    /**
     * @brief 清除所有暴露的变量和回调
     */
    void clear();

private:
    /**
     * @brief 暴露的变量信息
     */
    struct ExposedVar {
        void* ptr = nullptr;                         ///< 变量指针
        size_t typeId = 0;                           ///< 类型ID
        const char* typeName = "";                   ///< 类型名称
        bool isWritable = false;                     ///< 是否可写
        std::function<Value()> readFunc;             ///< 读取函数
        std::function<void(const Value&)> writeFunc; ///< 写入函数
        StateChangeCallback onUpdate;                ///< 更新回调
    };

    /**
     * @brief 从路径中解析属性访问
     *
     * 支持:
     * - "path.to.value" 格式
     * - "array[index]" 格式
     */
    [[nodiscard]] Value _resolvePath(const std::string& path) const;

    /**
     * @brief 分割路径
     */
    [[nodiscard]] std::vector<std::string> _splitPath(const std::string& path) const;

    state::StateStore& m_store;
    event::EventBus& m_eventBus;

    std::unordered_map<std::string, ExposedVar> m_exposedVars;
    std::unordered_map<std::string, Callback> m_callbacks;
    std::unordered_map<std::string, std::vector<std::pair<u64, StateChangeCallback>>> m_subscribers;
    std::unordered_map<std::string, Value> m_loopVariables;

    u64 m_nextSubscriberId = 1;
};

} // namespace mc::client::ui::kagero::tpl::binder
