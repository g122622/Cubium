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

#include "common/mod/bedrock/addon/engine/quickjs/QuickJSContext.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/core/ScriptResult.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSBindingContext.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSModuleLoader.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSRuntime.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <quickjs.h>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

/**
 * @brief 全局函数回调注册表
 *
 * QuickJS的JS_NewCFunction不支持用户数据指针，
 * 因此使用全局映射表将(上下文+函数名)映射到C++回调。
 * 每个QuickJSContext有独立的映射条目，线程安全由context隔离保证。
 */
static std::mutex s_callbackMutex;
static std::unordered_map<const QuickJSContext*,
    std::unordered_map<std::string, std::function<ScriptValue(const std::vector<ScriptValue>&)>>>
    s_globalCallbackRegistry;

QuickJSContext::QuickJSContext(QuickJSRuntime& runtime, const ContextConfig& config)
    : m_runtime(runtime)
    , m_config(config)
    , m_moduleLoader(std::make_unique<QuickJSModuleLoader>())
{}

QuickJSContext::~QuickJSContext()
{
    // 清理全局回调注册表
    {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        s_globalCallbackRegistry.erase(this);
    }

    if (m_context) {
        JS_FreeContext(m_context);
        m_context = nullptr;
        m_valid = false;
        spdlog::info("[BedrockAddon] QuickJS context destroyed");
    }
}

QuickJSContext::QuickJSContext(QuickJSContext&& other) noexcept
    : m_runtime(other.m_runtime)
    , m_config(std::move(other.m_config))
    , m_context(other.m_context)
    , m_valid(other.m_valid)
    , m_moduleName(std::move(other.m_moduleName))
    , m_globalFunctions(std::move(other.m_globalFunctions))
    , m_bindingContext(std::move(other.m_bindingContext))
    , m_moduleLoader(std::move(other.m_moduleLoader))
{
    // 更新全局回调注册表中的指针
    {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        auto it = s_globalCallbackRegistry.find(&other);
        if (it != s_globalCallbackRegistry.end()) {
            s_globalCallbackRegistry[this] = std::move(it->second);
            s_globalCallbackRegistry.erase(it);
        }
    }

    other.m_context = nullptr;
    other.m_valid = false;
}

QuickJSContext& QuickJSContext::operator=(QuickJSContext&& other) noexcept
{
    if (this != &other) {
        // 清理当前资源
        {
            std::lock_guard<std::mutex> lock(s_callbackMutex);
            s_globalCallbackRegistry.erase(this);
        }
        if (m_context) {
            JS_FreeContext(m_context);
        }

        // 移动资源
        m_config = std::move(other.m_config);
        m_context = other.m_context;
        m_valid = other.m_valid;
        m_moduleName = std::move(other.m_moduleName);
        m_globalFunctions = std::move(other.m_globalFunctions);
        m_bindingContext = std::move(other.m_bindingContext);
        m_moduleLoader = std::move(other.m_moduleLoader);

        // 更新全局回调注册表中的指针
        {
            std::lock_guard<std::mutex> lock(s_callbackMutex);
            auto it = s_globalCallbackRegistry.find(&other);
            if (it != s_globalCallbackRegistry.end()) {
                s_globalCallbackRegistry[this] = std::move(it->second);
                s_globalCallbackRegistry.erase(it);
            }
        }

        other.m_context = nullptr;
        other.m_valid = false;
    }
    return *this;
}

bool QuickJSContext::initialize()
{
    JSRuntime* rt = m_runtime.nativeRuntime();
    if (!rt) {
        spdlog::error("[BedrockAddon] Cannot create context: runtime is null");
        return false;
    }

    m_context = JS_NewContext(rt);
    if (!m_context) {
        spdlog::error("[BedrockAddon] Failed to create QuickJS context");
        return false;
    }

    // 注册模块加载器：opaque 指向本上下文持有的 m_moduleLoader（相对路径 JS 模块经此加载源码）。
    // 注意 JS_SetModuleLoaderFunc 是 runtime 级注册，多 context 共享同一 runtime 时后注册者覆盖前者；
    // 当前每插件独立 runtime 故无此问题。@minecraft/* 原生模块由 createContext 的依赖循环预注册，不走此回调。
    JS_SetModuleLoaderFunc(
        rt, QuickJSModuleLoader::moduleNormalize, QuickJSModuleLoader::moduleLoader, m_moduleLoader.get());

    m_valid = true;
    spdlog::info("[BedrockAddon] QuickJS context initialized");
    return true;
}

ScriptResult QuickJSContext::evaluate(const std::string& source, const std::string& filename, EvalFlags flags)
{
    if (!m_valid || !m_context) {
        return ScriptResult::error("Context is not valid");
    }

    i32 evalFlags = JS_EVAL_TYPE_GLOBAL;
    if (flags & EvalFlags::Strict) {
        evalFlags |= JS_EVAL_FLAG_STRICT;
    }
    if (flags & EvalFlags::Module) {
        evalFlags = JS_EVAL_TYPE_MODULE;
    }

    // QuickJS 栈溢出检查：rt->stack_limit = rt->stack_top - rt->stack_size。
    // stack_top 在 JS_NewRuntime 时记录（调用点 sp），stack_size 默认 4MB。
    // Windows 默认主线程栈仅 1MB，stack_top（~1.4MB 低地址）< stack_size（4MB）致
    // stack_top - stack_size 无符号下溢成巨大值，sp<stack_limit 恒真，误报栈溢出。
    // 根因修复在链接期把 exe 主线程栈提升到 16MB（见 src/{server,client}/CMakeLists.txt /STACK），
    // 此处不再调 JS_UpdateStackTop（在深栈刷新会让 stack_top 变小，反而加剧下溢）。
    JSValue val = JS_Eval(m_context, source.c_str(), source.size(), filename.c_str(), evalFlags);

    if (JS_IsException(val)) {
        JS_FreeValue(m_context, val);
        return _exceptionToResult();
    }

    // quickjs-ng 模块求值（JS_EVAL_TYPE_MODULE）返回模块的 Promise（js_evaluate_module 内部
    // JS_NewPromiseCapability），模块体的同步异常被 reject 进 Promise 而非同步抛出，JS_Eval 不返回
    // EXCEPTION。故模块模式下须查 Promise 状态：Rejected 取 reason 报错（否则模块体异常被静默吞掉，
    // 入口"成功"但 register 等从未执行）；Pending 则推进 pending jobs 至 settle（顶层 await 场景）。
    if (evalFlags == JS_EVAL_TYPE_MODULE && JS_IsPromise(val)) {
        // 推进 pending jobs 直至 Promise settle 或无更多 job。同步模块体立即 settle，循环 0 次。
        // TODO: 顶层 await 的长时异步模块需设上限避免无限等待，当前行为包入口均为同步。
        JSPromiseStateEnum state = JS_PromiseState(m_context, val);
        JSRuntime* rt = JS_GetRuntime(m_context);
        while (state == JS_PROMISE_PENDING && JS_IsJobPending(rt)) {
            JSContext* pctx = nullptr;
            int jobRet = JS_ExecutePendingJob(rt, &pctx);
            if (jobRet <= 0) {
                break; // 无更多 job 或 job 自身抛错
            }
            state = JS_PromiseState(m_context, val);
        }

        if (state == JS_PROMISE_REJECTED) {
            JSValue reason = JS_PromiseResult(m_context, val);
            ScriptResult err = _valueToErrorResult(reason);
            JS_FreeValue(m_context, reason);
            JS_FreeValue(m_context, val);
            return err;
        }
        // Fulfilled 或仍 Pending（异步模块未完成）：视为成功，返回 undefined。
        // 异步模块的最终结果由后续 pending job 驱动（GameTest register 在模块体同步阶段已完成）。
    }

    ScriptValue result = _jsValueToScriptValue(val);
    JS_FreeValue(m_context, val);
    return ScriptResult::ok(std::move(result));
}

ScriptResult QuickJSContext::evaluateModule(const std::string& source, const std::string& filename)
{
    return evaluate(source, filename, EvalFlags::Module);
}

ScriptResult QuickJSContext::callFunction(const std::string& name, const std::vector<ScriptValue>& args)
{
    if (!m_valid || !m_context) {
        return ScriptResult::error("Context is not valid");
    }

    // 获取全局对象
    JSValue global = JS_GetGlobalObject(m_context);

    // 查找函数
    JSValue func = JS_GetPropertyStr(m_context, global, name.c_str());
    JS_FreeValue(m_context, global);

    if (JS_IsUndefined(func) || JS_IsException(func)) {
        JS_FreeValue(m_context, func);
        return ScriptResult::error("Function not found: " + name);
    }

    if (!JS_IsFunction(m_context, func)) {
        JS_FreeValue(m_context, func);
        return ScriptResult::error("Not a function: " + name);
    }

    // 转换参数
    std::vector<JSValue> jsArgs;
    jsArgs.reserve(args.size());
    for (const auto& arg : args) {
        jsArgs.push_back(_scriptValueToJSValue(arg));
    }

    // 调用函数
    JSValue result = JS_Call(m_context, func, JS_UNDEFINED, static_cast<i32>(jsArgs.size()), jsArgs.data());

    // 释放参数
    for (auto& arg : jsArgs) {
        JS_FreeValue(m_context, arg);
    }
    JS_FreeValue(m_context, func);

    if (JS_IsException(result)) {
        JS_FreeValue(m_context, result);
        return _exceptionToResult();
    }

    ScriptValue scriptResult = _jsValueToScriptValue(result);
    JS_FreeValue(m_context, result);
    return ScriptResult::ok(std::move(scriptResult));
}

ScriptResult QuickJSContext::importModule(const std::string& moduleName)
{
    if (!m_valid || !m_context) {
        return ScriptResult::error("Context is not valid");
    }

    // 使用JS_Eval加载模块
    std::string importCode = "import * as m from '" + moduleName + "'; m;";
    return evaluate(importCode, "<import>", EvalFlags::Module);
}

bool QuickJSContext::isValid() const
{
    return m_valid;
}

void* QuickJSContext::nativeHandle()
{
    return m_context;
}

IScriptBindingContext& QuickJSContext::bindingContext()
{
    if (!m_bindingContext) {
        m_bindingContext = std::make_unique<QuickJSBindingContext>(m_context);
    }
    return *m_bindingContext;
}

namespace {
/**
 * @brief 将JS参数转换为ScriptValue向量
 *
 * 用于全局函数回调分发器中。
 */
std::vector<ScriptValue> convertArgsToScriptValues(JSContext* ctx, i32 argc, JSValueConst* argv)
{
    std::vector<ScriptValue> scriptArgs;
    scriptArgs.reserve(argc);
    for (i32 i = 0; i < argc; ++i) {
        if (JS_IsUndefined(argv[i]) || JS_IsUninitialized(argv[i])) {
            scriptArgs.emplace_back();
        } else if (JS_IsNull(argv[i])) {
            scriptArgs.emplace_back(nullptr);
        } else if (JS_IsBool(argv[i])) {
            scriptArgs.emplace_back(JS_ToBool(ctx, argv[i]) != 0);
        } else if (JS_IsNumber(argv[i])) {
            f64 num;
            if (JS_ToFloat64(ctx, &num, argv[i]) == 0) {
                scriptArgs.emplace_back(num);
            } else {
                scriptArgs.emplace_back(0.0);
            }
        } else if (JS_IsString(argv[i])) {
            const char* str = JS_ToCString(ctx, argv[i]);
            scriptArgs.emplace_back(std::string(str ? str : ""));
            JS_FreeCString(ctx, str);
        } else {
            // TODO: 对象、数组、函数等复杂类型作为参数传递暂不支持，需要扩展ScriptValue类型系统
            scriptArgs.emplace_back();
        }
    }
    return scriptArgs;
}

/**
 * @brief 将ScriptValue转换为JSValue
 */
JSValue convertScriptValueToJSValue(JSContext* ctx, const ScriptValue& result)
{
    switch (result.type()) {
        case ScriptValueType::Undefined:
            return JS_UNDEFINED;
        case ScriptValueType::Null:
            return JS_NULL;
        case ScriptValueType::Boolean:
            return JS_NewBool(ctx, result.asBoolean() ? 1 : 0);
        case ScriptValueType::Number:
            return JS_NewFloat64(ctx, result.asNumber());
        case ScriptValueType::String:
            return JS_NewStringLen(ctx, result.asString().c_str(), result.asString().size());
        case ScriptValueType::Object:
        case ScriptValueType::Array:
        case ScriptValueType::Function:
            // TODO: 对象、数组、函数等复杂类型暂不支持，需要扩展ScriptValue类型系统
            return JS_UNDEFINED;
    }
    return JS_UNDEFINED;
}
} // namespace

bool QuickJSContext::registerGlobalFunction(
    const std::string& name, std::function<ScriptValue(const std::vector<ScriptValue>&)> callback)
{
    if (!m_valid || !m_context) {
        return false;
    }

    // 存储回调到注册表
    {
        std::lock_guard<std::mutex> lock(s_callbackMutex);
        s_globalCallbackRegistry[this][name] = std::move(callback);
    }

    // 使用JS_NewCClosure传递回调数据（context指针+函数名）
    // JSCClosure签名：JSValue(JSContext*, JSValueConst, i32 argc, JSValueConst*, i32 magic, void* opaque)
    struct GlobalCallbackData {
        const QuickJSContext* context;
        std::string name;
    };

    auto* cbData = new GlobalCallbackData{this, name};

    JSValue closure = JS_NewCClosure(
        m_context,
        [](JSContext* ctx, JSValueConst, i32 argc, JSValueConst* argv, i32, void* opaque) -> JSValue {
            auto* data = static_cast<GlobalCallbackData*>(opaque);

            // 在注册表中查找回调
            std::function<ScriptValue(const std::vector<ScriptValue>&)> callback;
            {
                std::lock_guard<std::mutex> lock(s_callbackMutex);
                auto ctxIt = s_globalCallbackRegistry.find(data->context);
                if (ctxIt == s_globalCallbackRegistry.end()) {
                    return JS_ThrowReferenceError(ctx, "Context not found for function: %s", data->name.c_str());
                }
                auto cbIt = ctxIt->second.find(data->name);
                if (cbIt == ctxIt->second.end()) {
                    return JS_ThrowReferenceError(ctx, "Function not found: %s", data->name.c_str());
                }
                callback = cbIt->second;
            }

            // 转换参数并调用
            auto scriptArgs = convertArgsToScriptValues(ctx, argc, argv);
            ScriptValue result = callback(scriptArgs);
            return convertScriptValueToJSValue(ctx, result);
        },
        name.c_str(),
        [](void* opaque) {
            // finalizer: 释放回调数据
            delete static_cast<GlobalCallbackData*>(opaque);
        },
        0,
        0,
        cbData);

    if (JS_IsException(closure)) {
        delete cbData;
        spdlog::error("[BedrockAddon] Failed to register global function: {}", name);
        return false;
    }

    // 设置为全局属性
    JSValue global = JS_GetGlobalObject(m_context);
    JS_SetPropertyStr(m_context, global, name.c_str(), closure);
    JS_FreeValue(m_context, global);

    spdlog::info("[BedrockAddon] Registered global function: {}", name);
    return true;
}

bool QuickJSContext::registerNativeGlobalFunction(const std::string& name, void* func, i32 length)
{
    if (!m_valid || !m_context) {
        return false;
    }

    JSValue global = JS_GetGlobalObject(m_context);
    JSValue fn = JS_NewCFunction(m_context, reinterpret_cast<JSCFunction*>(func), name.c_str(), length);
    JS_SetPropertyStr(m_context, global, name.c_str(), fn);
    JS_FreeValue(m_context, global);

    return true;
}

bool QuickJSContext::registerNativeModule(
    const std::string& name, std::function<int(JSContext*, JSModuleDef*)> initFunc)
{
    if (!m_valid || !m_context) {
        return false;
    }

    JSModuleDef* m = JS_NewCModule(m_context, name.c_str(), [](JSContext* ctx, JSModuleDef*) -> int { return 0; });
    if (!m) {
        spdlog::error("[BedrockAddon] Failed to create native module in context: {}", name);
        return false;
    }

    if (initFunc(m_context, m) < 0) {
        spdlog::error("[BedrockAddon] Failed to initialize native module in context: {}", name);
        return false;
    }

    spdlog::info("[BedrockAddon] Registered native module in context: {}", name);
    return true;
}

void QuickJSContext::setModuleSourceProvider(QuickJSModuleLoader::ModuleSourceProvider provider)
{
    if (m_moduleLoader) {
        m_moduleLoader->setModuleSourceProvider(std::move(provider));
    }
}

bool QuickJSContext::setGlobalVariable(const std::string& name, const ScriptValue& value)
{
    if (!m_valid || !m_context) {
        return false;
    }

    JSValue global = JS_GetGlobalObject(m_context);
    JSValue jsVal = _scriptValueToJSValue(value);
    i32 ret = JS_SetPropertyStr(m_context, global, name.c_str(), jsVal);
    JS_FreeValue(m_context, global);

    return ret >= 0;
}

std::optional<ScriptValue> QuickJSContext::getGlobalVariable(const std::string& name) const
{
    if (!m_valid || !m_context) {
        return std::nullopt;
    }

    JSValue global = JS_GetGlobalObject(m_context);
    JSValue val = JS_GetPropertyStr(m_context, global, name.c_str());
    JS_FreeValue(m_context, global);

    if (JS_IsUndefined(val) || JS_IsException(val)) {
        JS_FreeValue(m_context, val);
        return std::nullopt;
    }

    ScriptValue result = const_cast<QuickJSContext*>(this)->_jsValueToScriptValue(val);
    JS_FreeValue(m_context, val);
    return result;
}

const std::string& QuickJSContext::moduleName() const
{
    return m_moduleName;
}

ScriptResult QuickJSContext::_exceptionToResult()
{
    if (!m_context) {
        return ScriptResult::error("Unknown error: context is null");
    }

    JSValue exception = JS_GetException(m_context);
    ScriptResult result = _valueToErrorResult(exception);
    JS_FreeValue(m_context, exception);
    return result;
}

ScriptResult QuickJSContext::_valueToErrorResult(JSValueConst value)
{
    if (!m_context) {
        return ScriptResult::error("Unknown error: context is null");
    }
    // 即便不是标准 Error 对象（如栈溢出可能抛 RangeError），也尝试取 message/stack。
    std::string message = "Unknown error";
    if (JS_IsError(value) || JS_IsObject(value)) {
        const char* msg = JS_ToCString(m_context, value);
        if (msg && msg[0] != '\0') {
            message = msg;
        }
        JS_FreeCString(m_context, msg);

        // 获取堆栈跟踪
        JSValue stack = JS_GetPropertyStr(m_context, value, "stack");
        if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
            const char* stackStr = JS_ToCString(m_context, stack);
            if (stackStr && stackStr[0] != '\0') {
                message += "\nStack:\n";
                message += stackStr;
            }
            JS_FreeCString(m_context, stackStr);
        }
        JS_FreeValue(m_context, stack);
    } else {
        // 非 object（如 null/undefined）：输出 tag 以辅助诊断
        message += " (tag=" + std::to_string(JS_VALUE_GET_TAG(value)) + ")";
    }
    return ScriptResult::error(std::move(message));
}

ScriptValue QuickJSContext::_jsValueToScriptValue(JSValue val)
{
    if (!m_context) {
        return ScriptValue();
    }

    if (JS_IsUndefined(val) || JS_IsUninitialized(val)) {
        return ScriptValue();
    }
    if (JS_IsNull(val)) {
        return ScriptValue(nullptr);
    }
    if (JS_IsBool(val)) {
        return ScriptValue(JS_ToBool(m_context, val) != 0);
    }
    if (JS_IsNumber(val)) {
        f64 num;
        if (JS_ToFloat64(m_context, &num, val) == 0) {
            return ScriptValue(num);
        }
        return ScriptValue(0.0);
    }
    if (JS_IsString(val)) {
        const char* str = JS_ToCString(m_context, val);
        std::string result = str ? str : "";
        JS_FreeCString(m_context, str);
        return ScriptValue(std::move(result));
    }
    // 对象、数组、函数等暂时作为undefined返回
    // TODO: 绑定层通过ScriptClassBinding和ClassRegistrar直接操作JSValue，需要完善此处的转换逻辑
    return ScriptValue();
}

JSValue QuickJSContext::_scriptValueToJSValue(const ScriptValue& val)
{
    if (!m_context) {
        return JS_UNDEFINED;
    }

    switch (val.type()) {
        case ScriptValueType::Undefined:
            return JS_UNDEFINED;
        case ScriptValueType::Null:
            return JS_NULL;
        case ScriptValueType::Boolean:
            return JS_NewBool(m_context, val.asBoolean() ? 1 : 0);
        case ScriptValueType::Number:
            return JS_NewFloat64(m_context, val.asNumber());
        case ScriptValueType::String:
            return JS_NewStringLen(m_context, val.asString().c_str(), val.asString().size());
        case ScriptValueType::Object:
        case ScriptValueType::Array:
        case ScriptValueType::Function:
            // 绑定层通过ScriptObjectRegistry直接管理JSValue
            return JS_UNDEFINED;
    }
    return JS_UNDEFINED;
}

} // namespace mc::mod::bedrock::addon
