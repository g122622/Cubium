// Copyright (c) 2024 Cubium Project
// SPDX-License-Identifier: MIT

#pragma once

#include "common/core/Types.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本值类型枚举
 *
 * 引擎无关的脚本值类型标识。
 */
enum class ScriptType {
    Undefined,
    Null,
    Boolean,
    Number,
    String,
    Object,
    Array,
    Function,
};

/**
 * @brief 脚本方法回调类型
 *
 * 引擎无关的方法回调签名。modules/层使用此类型注册方法，
 * 引擎实现层通过trampoline机制将其转换为引擎特定的C函数。
 *
 * @param ctx 绑定上下文，用于值操作
 * @param thisVal this对象句柄
 * @param argc 参数个数
 * @param args 参数句柄数组
 * @return 返回值句柄（调用者拥有所有权），或异常句柄
 */
using ScriptMethodCallback = std::function<void*(IScriptBindingContext&, void*, i32, void**)>;

/**
 * @brief 脚本属性getter回调类型
 *
 * @param ctx 绑定上下文
 * @param thisVal this对象句柄
 * @return 属性值句柄（调用者拥有所有权）
 */
using ScriptGetterCallback = std::function<void*(IScriptBindingContext&, void*)>;

/**
 * @brief 脚本属性setter回调类型
 *
 * @param ctx 绑定上下文
 * @param thisVal this对象句柄
 * @param value 新值句柄（setter不拥有所有权，如需持久化请retainValue）
 */
using ScriptSetterCallback = std::function<void(IScriptBindingContext&, void*, void*)>;

/**
 * @brief 脚本绑定上下文抽象接口
 *
 * 提供模块绑定所需的脚本引擎操作，不暴露具体引擎API。
 * modules/层通过此接口注册类、方法、属性、创建对象、调用回调等，
 * 无需直接依赖QuickJS或其他引擎头文件。
 *
 * 值句柄约定：
 * 所有void*句柄都是引擎特定的值引用。对于QuickJS，这是JSValue*。
 * - 创建方法返回的句柄拥有引用所有权
 * - retainValue增加引用计数（调用后调用者拥有自己的引用）
 * - releaseValue释放引用所有权
 * - getProperty返回的句柄拥有引用所有权（调用者负责release）
 * - setProperty消耗传入value的引用所有权（调用者不再需要release value）
 *
 * 生命周期：
 * - 由IScriptContext创建，每个插件上下文一个实例
 * - 引擎特定实现由engine/层提供
 */
class IScriptBindingContext {
public:
    virtual ~IScriptBindingContext() = default;

    // ===== 值创建 =====

    [[nodiscard]] virtual void* createUndefined() = 0;
    [[nodiscard]] virtual void* createNull() = 0;
    [[nodiscard]] virtual void* createBoolean(bool value) = 0;
    [[nodiscard]] virtual void* createInt32(i32 value) = 0;
    [[nodiscard]] virtual void* createInt64(i64 value) = 0;
    [[nodiscard]] virtual void* createFloat64(f64 value) = 0;
    [[nodiscard]] virtual void* createString(std::string_view value) = 0;
    [[nodiscard]] virtual void* createObject() = 0;
    [[nodiscard]] virtual void* createArray() = 0;
    [[nodiscard]] virtual void* createObjectWithProto(void* proto, u64 classId) = 0;

    /**
     * @brief 创建函数值（使用引擎无关回调）
     *
     * 创建一个JS函数对象，当被调用时执行指定的回调。
     *
     * @param callback 方法回调
     * @param name 函数名（用于调试）
     * @param length 参数个数（用于JS的length属性）
     * @return 函数值句柄（调用者拥有所有权）
     */
    [[nodiscard]] virtual void* createFunction(ScriptMethodCallback callback, const char* name, i32 length = 0) = 0;

    /**
     * @brief 设置构造函数关联
     *
     * 将构造函数与原型对象关联，使instanceof运算符正常工作。
     *
     * @param ctor 构造函数句柄
     * @param proto 原型对象句柄
     */
    virtual void setConstructor(void* ctor, void* proto) = 0;

    // ===== 值类型检查 =====

    [[nodiscard]] virtual ScriptType getType(void* value) const = 0;
    [[nodiscard]] virtual bool isUndefined(void* value) const = 0;
    [[nodiscard]] virtual bool isFunction(void* value) const = 0;
    [[nodiscard]] virtual bool isObject(void* value) const = 0;
    [[nodiscard]] virtual bool isNumber(void* value) const = 0;
    [[nodiscard]] virtual bool isString(void* value) const = 0;
    [[nodiscard]] virtual bool isException(void* value) const = 0;

    // ===== 值转换 =====

    [[nodiscard]] virtual std::optional<i32> toInt32(void* value) const = 0;
    [[nodiscard]] virtual std::optional<f64> toFloat64(void* value) const = 0;
    [[nodiscard]] virtual std::optional<bool> toBool(void* value) const = 0;
    [[nodiscard]] virtual std::optional<std::string> toString(void* value) const = 0;

    // ===== 对象属性操作 =====

    /** 设置属性（消耗value的引用所有权） */
    virtual void setProperty(void* obj, const char* key, void* value) = 0;
    virtual void setPropertyInt(void* obj, const char* key, i32 value) = 0;
    virtual void setPropertyFloat(void* obj, const char* key, f64 value) = 0;
    virtual void setPropertyBool(void* obj, const char* key, bool value) = 0;
    virtual void setPropertyString(void* obj, const char* key, std::string_view value) = 0;
    virtual void setPropertyNull(void* obj, const char* key) = 0;
    virtual void setPropertyInt64(void* obj, const char* key, i64 value) = 0;

    /** 获取属性（返回值拥有引用所有权，调用者负责release） */
    [[nodiscard]] virtual void* getProperty(void* obj, const char* key) const = 0;
    [[nodiscard]] virtual std::optional<i32> getPropertyInt(void* obj, const char* key) const = 0;
    [[nodiscard]] virtual std::optional<f64> getPropertyFloat(void* obj, const char* key) const = 0;
    [[nodiscard]] virtual std::optional<bool> getPropertyBool(void* obj, const char* key) const = 0;
    [[nodiscard]] virtual std::optional<std::string> getPropertyString(void* obj, const char* key) const = 0;

    virtual void setArrayElementInt(void* arr, u32 index, i32 value) = 0;
    virtual void setArrayElementString(void* arr, u32 index, std::string_view value) = 0;
    /**
     * @brief 设置数组元素为任意 JS 值句柄。
     *
     * 不消耗 value 所有权（内部 DupValue），调用方仍须在用完后 releaseValue(value)。
     * 供需要把 JS 对象（非 int/string）塞入数组的场景（如 Dimension.getEntities 返回 Entity[]）。
     */
    virtual void setArrayElement(void* arr, u32 index, void* value) = 0;

    // ===== 引用管理 =====

    /** 增加引用计数（用于持久化值，如回调函数）；不新建句柄，仍引用同一 handle */
    virtual void retainValue(void* value) = 0;
    /** 减少引用计数并释放 */
    virtual void releaseValue(void* value) = 0;
    /**
     * @brief 复制句柄：新建一个独立 handle，其 JSValue 是 value 的 Dup（refcount+1）。
     *
     * 用于回调返回值需复用入参句柄的场景（如链式方法返回 this）。入参 handle 的所有权仍属调用方
     * （trampoline 会释放它），返回的新 handle 所有权归调用方——二者独立，避免 double-free。
     * 与 retainValue 区别：retainValue 不新建 handle（仍同一指针），无法解决 trampoline
     * 释放入参 handle 后返回值悬垂的问题。
     */
    [[nodiscard]] virtual void* dupValue(void* value) = 0;

    // ===== 对象opaque管理 =====

    virtual void setOpaque(void* obj, void* data, u64 classId) = 0;
    [[nodiscard]] virtual void* getOpaque(void* obj, u64 classId) const = 0;

    // ===== 函数调用 =====

    [[nodiscard]] virtual void* callFunction(void* func, void* thisVal, i32 argc, void** args) = 0;
    [[nodiscard]] virtual void* callFunction0(void* func, void* thisVal) = 0;
    [[nodiscard]] virtual void* callFunction1(void* func, void* thisVal, void* arg0) = 0;

    // ===== 错误处理 =====

    [[nodiscard]] virtual void* throwTypeError(const char* message) = 0;
    [[nodiscard]] virtual void* throwInternalError(const char* message) = 0;
    [[nodiscard]] virtual void* getException() = 0;
    [[nodiscard]] virtual std::string getExceptionMessage(void* exception) const = 0;

    // ===== 类注册 =====

    [[nodiscard]] virtual u64 allocateClassId() = 0;

    /**
     * @brief 注册类定义
     *
     * @param classId 类ID（由allocateClassId()分配）
     * @param className 类名
     * @param hasFinalizer 是否需要默认的对象数据finalizer（释放ScriptObjectRegistry::ObjectData）
     * @return 注册是否成功
     */
    virtual bool registerClass(u64 classId, const char* className, bool hasFinalizer = true) = 0;

    [[nodiscard]] virtual void* createClassProto(u64 classId) = 0;

    /** 注册原生方法（nativeFunc为引擎特定的C函数指针） */
    virtual void registerNativeMethod(void* proto, const char* name, void* nativeFunc, i32 length = 0) = 0;
    /** 注册原生只读属性（nativeGetter为引擎特定的C函数指针） */
    virtual void registerNativeReadonlyProperty(void* proto, const char* name, void* nativeGetter) = 0;
    /** 注册原生读写属性 */
    virtual void registerNativeProperty(void* proto, const char* name, void* nativeGetter, void* nativeSetter) = 0;

    // ===== 高级方法/属性注册（引擎无关回调） =====

    /**
     * @brief 注册方法（使用引擎无关回调）
     *
     * 引擎实现层通过trampoline机制将ScriptMethodCallback转换为引擎特定的C函数。
     * modules/层应优先使用此方法而非registerNativeMethod。
     *
     * @param proto 原型对象句柄
     * @param name 方法名
     * @param callback 方法回调
     * @param length 参数个数（用于JS的length属性）
     */
    virtual void registerMethod(void* proto, const char* name, ScriptMethodCallback callback, i32 length = 0) = 0;

    /**
     * @brief 注册只读属性（使用引擎无关回调）
     */
    virtual void registerReadonlyProperty(void* proto, const char* name, ScriptGetterCallback getter) = 0;

    /**
     * @brief 注册读写属性（使用引擎无关回调）
     */
    virtual void registerProperty(
        void* proto, const char* name, ScriptGetterCallback getter, ScriptSetterCallback setter) = 0;

    // ===== 模块注册 =====

    virtual bool createNativeModule(const std::string& moduleName) = 0;
    virtual bool exportNativeFunction(const std::string& name, void* nativeFunc, i32 length = 0) = 0;
    virtual bool exportNativeConst(const std::string& name, i32 value) = 0;
    virtual bool exportNativeConstFloat(const std::string& name, f64 value) = 0;
    virtual bool exportNativeConstString(const std::string& name, const std::string& value) = 0;
    virtual bool exportNativeValue(const std::string& name, void* value) = 0;
    virtual bool finalizeModule() = 0;

    // ===== 全局对象 =====

    [[nodiscard]] virtual void* getGlobalObject() = 0;

    // ===== Promise 支持 =====
    //
    // 引擎无关的 JS Promise 能力。供异步脚本逻辑（如 GameTest JS `async` 测试体、`await idle(n)`）
    // 创建 Promise 并轮询其 settle 状态。各引擎后端按自身 Promise 语义实现。
    //
    // 句柄约定（沿用本接口既有约定：创建方法返回 owned 句柄，调用者负责 release）：
    // - createPromise 返回 promise（owned）+ resolvingFuncsOut[0]=resolve/[1]=reject（owned）。
    // - promiseResult 返回 settle 值的 owned 句柄（即便 pending 也返回 undefined）。
    // - resolving func 一次性：callResolvingFunc 调用后由调用方 releaseValue 释放。

    /**
     * @brief 创建 pending Promise 及其 resolve/reject 函数。
     *
     * @param resolvingFuncsOut 非 null 时，写入 [0]=resolve/[1]=reject（owned 句柄，调用者负责 release）；
     *                          null 时实现内部立即释放两函数。
     * @return Promise 句柄（owned）。
     */
    [[nodiscard]] virtual void* createPromise(void** resolvingFuncsOut) = 0;

    /**
     * @brief 查询 Promise 状态。
     * @return 0=Pending / 1=Fulfilled / 2=Rejected / -1=非 Promise 或 null（语义对齐 JSPromiseStateEnum）。
     */
    [[nodiscard]] virtual int promiseState(void* promise) const = 0;

    /**
     * @brief 取 Promise 的 settle 值（fulfilled 的值 / rejected 的 reason；pending 返回 undefined）。
     * @return owned 句柄，调用者负责 release。
     */
    [[nodiscard]] virtual void* promiseResult(void* promise) const = 0;

    /**
     * @brief 判断值是否为 Promise 对象。
     */
    [[nodiscard]] virtual bool isPromise(void* value) const = 0;

    /**
     * @brief 调用 createPromise 返回的 resolve/reject 函数，以 arg 为参数 settle Promise。
     *
     * resolving func 一次性，调用后调用者须 releaseValue 释放。
     *
     * @param resolvingFunc resolve 或 reject 函数句柄。
     * @param arg settle 参数句柄（调用方拥有所有权，本方法不释放）。
     */
    virtual void callResolvingFunc(void* resolvingFunc, void* arg) = 0;

    // ===== 上下文数据 =====

    virtual void setContextData(void* data) = 0;
    [[nodiscard]] virtual void* getContextData() const = 0;
};

} // namespace mc::mod::bedrock::addon
