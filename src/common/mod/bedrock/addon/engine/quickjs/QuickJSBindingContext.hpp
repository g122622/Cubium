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

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <quickjs.h>

namespace mc::mod::bedrock::addon {

/**
 * @brief QuickJS实现的脚本绑定上下文
 *
 * 将IScriptBindingContext的抽象接口映射到QuickJS C API。
 * 此文件位于engine/层，是唯一允许直接使用QuickJS头文件的地方之一。
 *
 * void*句柄语义：
 * - 所有通过接口传递的void*句柄都是JSValue*（堆分配，引用计数为1）
 * - retainValue()增加引用计数（JS_DupValue）
 * - releaseValue()减少引用计数并释放（JS_FreeValue + delete）
 * - 创建方法返回的句柄初始引用计数为1
 */
class QuickJSBindingContext : public IScriptBindingContext {
public:
    explicit QuickJSBindingContext(JSContext* ctx);

    // ===== 值创建 =====

    [[nodiscard]] void* createUndefined() override;
    [[nodiscard]] void* createNull() override;
    [[nodiscard]] void* createBoolean(bool value) override;
    [[nodiscard]] void* createInt32(i32 value) override;
    [[nodiscard]] void* createInt64(i64 value) override;
    [[nodiscard]] void* createFloat64(f64 value) override;
    [[nodiscard]] void* createString(std::string_view value) override;
    [[nodiscard]] void* createObject() override;
    [[nodiscard]] void* createArray() override;
    [[nodiscard]] void* createObjectWithProto(void* proto, u64 classId) override;
    [[nodiscard]] void* createFunction(ScriptMethodCallback callback, const char* name, i32 length = 0) override;
    virtual void setConstructor(void* ctor, void* proto) override;

    // ===== 值类型检查 =====

    [[nodiscard]] ScriptType getType(void* value) const override;
    [[nodiscard]] bool isUndefined(void* value) const override;
    [[nodiscard]] bool isFunction(void* value) const override;
    [[nodiscard]] bool isObject(void* value) const override;
    [[nodiscard]] bool isNumber(void* value) const override;
    [[nodiscard]] bool isString(void* value) const override;
    [[nodiscard]] bool isException(void* value) const override;

    // ===== 值转换 =====

    [[nodiscard]] std::optional<i32> toInt32(void* value) const override;
    [[nodiscard]] std::optional<f64> toFloat64(void* value) const override;
    [[nodiscard]] std::optional<bool> toBool(void* value) const override;
    [[nodiscard]] std::optional<std::string> toString(void* value) const override;

    // ===== 对象属性操作 =====

    void setProperty(void* obj, const char* key, void* value) override;
    void setPropertyInt(void* obj, const char* key, i32 value) override;
    void setPropertyFloat(void* obj, const char* key, f64 value) override;
    void setPropertyBool(void* obj, const char* key, bool value) override;
    void setPropertyString(void* obj, const char* key, std::string_view value) override;
    void setPropertyNull(void* obj, const char* key) override;
    void setPropertyInt64(void* obj, const char* key, i64 value) override;

    [[nodiscard]] void* getProperty(void* obj, const char* key) const override;
    [[nodiscard]] std::optional<i32> getPropertyInt(void* obj, const char* key) const override;
    [[nodiscard]] std::optional<f64> getPropertyFloat(void* obj, const char* key) const override;
    [[nodiscard]] std::optional<bool> getPropertyBool(void* obj, const char* key) const override;
    [[nodiscard]] std::optional<std::string> getPropertyString(void* obj, const char* key) const override;

    void setArrayElementInt(void* arr, u32 index, i32 value) override;
    void setArrayElementString(void* arr, u32 index, std::string_view value) override;
    void setArrayElement(void* arr, u32 index, void* value) override;

    // ===== 引用管理 =====

    void retainValue(void* value) override;
    void releaseValue(void* value) override;
    [[nodiscard]] void* dupValue(void* value) override;

    // ===== 对象opaque管理 =====

    void setOpaque(void* obj, void* data, u64 classId) override;
    [[nodiscard]] void* getOpaque(void* obj, u64 classId) const override;

    // ===== 函数调用 =====

    [[nodiscard]] void* callFunction(void* func, void* thisVal, i32 argc, void** args) override;
    [[nodiscard]] void* callFunction0(void* func, void* thisVal) override;
    [[nodiscard]] void* callFunction1(void* func, void* thisVal, void* arg0) override;

    // ===== 错误处理 =====

    [[nodiscard]] void* throwTypeError(const char* message) override;
    [[nodiscard]] void* throwInternalError(const char* message) override;
    [[nodiscard]] void* throwValue(void* value) override;
    [[nodiscard]] void* getException() override;
    [[nodiscard]] std::string getExceptionMessage(void* exception) const override;

    /**
     * @brief 设置对象原型（[[Prototype]]），供建立继承链（如自定义 Error 子类）。
     *
     * 实现：JS_SetPrototype（不消耗 obj/proto 引用，内部自行 Dup）。
     */
    void setPrototypeOf(void* obj, void* proto) override;

    // ===== 类注册 =====

    [[nodiscard]] u64 allocateClassId() override;
    bool registerClass(u64 classId, const char* className, bool hasFinalizer = true) override;
    [[nodiscard]] void* createClassProto(u64 classId) override;
    void registerNativeMethod(void* proto, const char* name, void* nativeFunc, i32 length = 0) override;
    void registerNativeReadonlyProperty(void* proto, const char* name, void* nativeGetter) override;
    void registerNativeProperty(void* proto, const char* name, void* nativeGetter, void* nativeSetter) override;

    // ===== 高级方法/属性注册（引擎无关回调） =====

    void registerMethod(void* proto, const char* name, ScriptMethodCallback callback, i32 length = 0) override;
    void registerReadonlyProperty(void* proto, const char* name, ScriptGetterCallback getter) override;
    void registerProperty(
        void* proto, const char* name, ScriptGetterCallback getter, ScriptSetterCallback setter) override;

    // ===== 模块注册 =====

    bool createNativeModule(const std::string& moduleName) override;
    bool exportNativeFunction(const std::string& name, void* nativeFunc, i32 length = 0) override;
    bool exportNativeConst(const std::string& name, i32 value) override;
    bool exportNativeConstFloat(const std::string& name, f64 value) override;
    bool exportNativeConstString(const std::string& name, const std::string& value) override;
    bool exportNativeValue(const std::string& name, void* value) override;
    bool finalizeModule() override;

    // ===== 全局对象 =====

    [[nodiscard]] void* getGlobalObject() override;

    // ===== 上下文数据 =====

    void setContextData(void* data) override;
    [[nodiscard]] void* getContextData() const override;

    // ===== Promise 支持（实现 IScriptBindingContext 抽象） =====
    //
    // quickjs-ng 未提供 JS_AddPromiseReaction，仅提供 JS_NewPromiseCapability /
    // JS_PromiseState / JS_PromiseResult / JS_IsPromise。故异步脚本逻辑（如 GameTest JS
    // `async` 测试体）采用"轮询 promiseState"完成判定（见 ScriptAsyncGameTestRunResult）。

    /**
     * @brief 创建一个 pending Promise，取出其 resolve/reject 函数对。
     *
     * @param resolvingFuncsOut 长度 2 的数组：[0]=resolve、[1]=reject（皆 owned 句柄，
     *        调用方负责 releaseValue；resolving 函数一次性，调用后须释放）。
     * @return Promise 句柄（owned，调用方负责 releaseValue）。
     */
    [[nodiscard]] void* createPromise(void** resolvingFuncsOut) override;

    /**
     * @brief 查询 Promise 状态。
     *
     * @return JSPromiseStateEnum：-1 非 Promise / 0 Pending / 1 Fulfilled / 2 Rejected。
     */
    [[nodiscard]] int promiseState(void* promise) const override;

    /**
     * @brief 取 Promise settle 后的结果值（fulfilled 的值或 rejected 的 reason）。
     *
     * 仅在 state != Pending 时有意义。返回 owned 句柄（调用方负责 releaseValue）。
     */
    [[nodiscard]] void* promiseResult(void* promise) const override;

    /**
     * @brief 判断值是否为 Promise 对象。
     */
    [[nodiscard]] bool isPromise(void* value) const override;

    /**
     * @brief 调用 Promise 的 resolving 函数（resolve 或 reject）。
     *
     * @param resolvingFunc createPromise 取出的 resolve/reject 句柄。
     * @param arg 入参句柄（非拥有，本方法内部 Dup 一份供 JS_Call 使用）。
     *
     * resolving 函数一次性，调用后调用方仍须 releaseValue 释放句柄。
     */
    void callResolvingFunc(void* resolvingFunc, void* arg) override;

    /**
     * @brief 获取底层JSContext指针
     *
     * 仅限QuickJS后端内部使用，modules/层不应调用此方法。
     */
    [[nodiscard]] JSContext* jsContext() const noexcept { return m_ctx; }

private:
    JSContext* m_ctx;
    void* m_module = nullptr; ///< 当前正在构建的JSModuleDef*
    std::string m_moduleName;
    bool m_moduleFinalized = false;
    void* m_contextData = nullptr;

    /// 待设值的模块导出条目（name → JSValue）。
    /// QuickJS 要求 JS_AddModuleExport 在 import 前（声明），JS_SetModuleExport 在 init 回调内（var_ref 建好后）。
    /// 故 export* 只调 AddModuleExport 声明并把值暂存于此，由 _moduleInit 在模块被 import 时统一 SetModuleExport。
    /// 暂存的 JSValue 经 JS_DupValue 持有，_moduleInit 消费或析构时释放。
    std::vector<std::pair<std::string, JSValue>> m_pendingExports;

    /**
     * @brief C 模块 init 回调：模块被 import 时由 QuickJS 调用。
     *
     * 此时 js_create_module_function 已为每个 AddModuleExport 声明的导出建好 var_ref，
     * 故在此遍历 m_pendingExports 调 JS_SetModuleExport 填值安全。
     */
    static int _moduleInit(JSContext* ctx, JSModuleDef* m);

    // ===== 回调trampoline存储 =====
    std::vector<ScriptMethodCallback> m_methodCallbacks;
    std::vector<ScriptGetterCallback> m_getterCallbacks;
    std::vector<ScriptSetterCallback> m_setterCallbacks;

    static JSValue methodTrampoline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic);
    static JSValue getterTrampoline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic);
    static JSValue setterTrampoline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic);

    /**
     * @brief 从JSContext opaque中获取QuickJSBindingContext指针
     *
     * trampoline函数通过此方法获取绑定上下文。
     */
    static QuickJSBindingContext* fromJsContext(JSContext* ctx);
};

} // namespace mc::mod::bedrock::addon
