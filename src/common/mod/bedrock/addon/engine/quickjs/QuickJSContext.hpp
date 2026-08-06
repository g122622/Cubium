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
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/core/ScriptData.hpp"
#include "common/mod/bedrock/addon/core/ScriptResult.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSModuleLoader.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <quickjs.h>

struct JSContext;
struct JSValue;
struct JSModuleDef;

namespace mc::mod::bedrock::addon {

class QuickJSRuntime;

/**
 * @brief QuickJS上下文实现
 *
 * 封装QuickJS的JSContext，提供脚本执行和模块加载功能。
 * 每个脚本插件运行在自己的上下文中，实现隔离。
 */
class QuickJSContext : public IScriptContext {
public:
    /**
     * @brief 构造QuickJS上下文
     *
     * @param runtime 所属的QuickJS运行时
     * @param config 上下文配置
     */
    QuickJSContext(QuickJSRuntime& runtime, const ContextConfig& config);
    ~QuickJSContext() override;

    // 禁止拷贝
    QuickJSContext(const QuickJSContext&) = delete;
    QuickJSContext& operator=(const QuickJSContext&) = delete;

    // 允许移动
    QuickJSContext(QuickJSContext&& other) noexcept;
    QuickJSContext& operator=(QuickJSContext&& other) noexcept;

    /**
     * @brief 初始化上下文
     *
     * 创建JSContext并注册标准内置对象。
     * @return 是否成功
     */
    bool initialize();

    // IScriptContext接口实现
    [[nodiscard]] ScriptResult evaluate(
        const std::string& source, const std::string& filename, EvalFlags flags = EvalFlags::None) override;
    [[nodiscard]] ScriptResult evaluateModule(const std::string& source, const std::string& filename) override;
    [[nodiscard]] ScriptResult callFunction(const std::string& name, const std::vector<ScriptValue>& args) override;
    [[nodiscard]] ScriptResult importModule(const std::string& moduleName) override;
    [[nodiscard]] bool isValid() const override;
    [[nodiscard]] void* nativeHandle() override;
    bool registerGlobalFunction(
        const std::string& name, std::function<ScriptValue(const std::vector<ScriptValue>&)> callback) override;
    bool setGlobalVariable(const std::string& name, const ScriptValue& value) override;
    [[nodiscard]] std::optional<ScriptValue> getGlobalVariable(const std::string& name) const override;
    [[nodiscard]] const std::string& moduleName() const override;

    /**
     * @brief 获取脚本绑定上下文
     *
     * 返回引擎无关的绑定接口，modules/层应通过此接口
     * 注册类、方法、属性、创建对象等。
     */
    [[nodiscard]] IScriptBindingContext& bindingContext() override;

    /**
     * @brief 获取原生JSContext指针
     */
    [[nodiscard]] JSContext* jsContext() const { return m_context; }

    /**
     * @brief 注册原生C函数到全局对象
     *
     * 直接将QuickJS C函数注册为全局函数，适用于高性能绑定。
     *
     * @param name 函数名
     * @param func C函数指针（签名：JSValue func(JSContext*, JSValue, int, JSValue*)）
     * @param length 参数个数
     * @return 是否注册成功
     */
    bool registerNativeGlobalFunction(const std::string& name, void* func, i32 length = 0);

    /**
     * @brief 注册原生C模块
     *
     * @param moduleName 模块名
     * @param initFunc 模块初始化函数
     * @return 是否注册成功
     */
    bool registerNativeModule(const std::string& name, std::function<int(JSContext*, JSModuleDef*)> initFunc);

    /**
     * @brief 设置模块源码提供者
     *
     * 当 JS 代码 `import "./utils"` 等相对路径模块时，QuickJS 经 moduleLoader 回调（opaque 指向本上下文的
     * m_moduleLoader）调用此处设置的 provider 获取源码。由 QuickJSEngine::createContext 在创建上下文后、
     * 执行 entry point 前注入，通常转发 IDependencyLoader::loadScript。
     *
     * @param provider 源码提供者回调（返回空字符串表示模块不存在）。
     */
    void setModuleSourceProvider(QuickJSModuleLoader::ModuleSourceProvider provider);

private:
    QuickJSRuntime& m_runtime;
    ContextConfig m_config;
    JSContext* m_context = nullptr;
    bool m_valid = false;
    std::string m_moduleName;
    std::unordered_map<std::string, std::function<ScriptValue(const std::vector<ScriptValue>&)>> m_globalFunctions;
    std::unique_ptr<class QuickJSBindingContext> m_bindingContext;
    // 模块加载器：处理相对路径 JS 模块的源码加载（@minecraft/* 原生模块由 createContext 的依赖循环预注册，
    // 不走此 loader）。unique_ptr 持有因其含 std::mutex 不可移动，而本上下文需支持移动语义。
    std::unique_ptr<QuickJSModuleLoader> m_moduleLoader;

    /**
     * @brief 将ScriptException转换为ScriptResult
     */
    [[nodiscard]] ScriptResult _exceptionToResult();

    /**
     * @brief 将任意 JSValue（如 Promise reject reason）转换为错误 ScriptResult
     *
     * 复用 _exceptionToResult 的 message/stack 提取逻辑，但不调用 JS_GetException
     * （reason 由调用方提供，不持有 context 的 current_exception）。
     */
    [[nodiscard]] ScriptResult _valueToErrorResult(JSValueConst value);

    /**
     * @brief 将JSValue转换为ScriptValue
     */
    [[nodiscard]] ScriptValue _jsValueToScriptValue(JSValue val);

    /**
     * @brief 将ScriptValue转换为JSValue
     */
    [[nodiscard]] JSValue _scriptValueToJSValue(const ScriptValue& val);
};

} // namespace mc::mod::bedrock::addon
