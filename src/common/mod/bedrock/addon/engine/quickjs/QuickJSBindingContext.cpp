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

#include "common/mod/bedrock/addon/engine/quickjs/QuickJSBindingContext.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"

#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <quickjs.h>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

// ============================================================================
// JSValue句柄管理（内部辅助函数）
//
// JSValue是QuickJS的128位tagged union，不能直接存储为void*。
// 我们使用堆分配的JSValue来作为void*句柄。
//
// 所有权约定：
// - 创建方法（createXxx）返回的句柄拥有JSValue的所有权
// - retainValue增加引用计数
// - releaseValue释放引用计数并删除句柄
// - unwrapValue返回JSValue的副本，不转移所有权
// ============================================================================

namespace {

JSValue unwrapValue(void* handle)
{
    if (!handle) return JS_UNDEFINED;
    return *static_cast<JSValue*>(handle);
}

void* wrapValue(JSValue val)
{
    return new JSValue(val);
}

std::vector<JSValueConst> unwrapArgs(void** args, i32 argc)
{
    std::vector<JSValueConst> result;
    result.reserve(argc);
    for (i32 i = 0; i < argc; ++i) {
        result.push_back(unwrapValue(args[i]));
    }
    return result;
}

} // anonymous namespace

// ============================================================================
// QuickJSBindingContext 实现
// ============================================================================

QuickJSBindingContext::QuickJSBindingContext(JSContext* ctx)
    : m_ctx(ctx)
{
    // 将绑定上下文指针存储在JSContext opaque中，供trampoline函数访问
    JS_SetContextOpaque(ctx, this);
}

// ===== 值创建 =====

void* QuickJSBindingContext::createUndefined()
{
    return wrapValue(JS_UNDEFINED);
}

void* QuickJSBindingContext::createNull()
{
    return wrapValue(JS_NULL);
}

void* QuickJSBindingContext::createBoolean(bool value)
{
    return wrapValue(JS_NewBool(m_ctx, value ? 1 : 0));
}

void* QuickJSBindingContext::createInt32(i32 value)
{
    return wrapValue(JS_NewInt32(m_ctx, value));
}

void* QuickJSBindingContext::createInt64(i64 value)
{
    return wrapValue(JS_NewInt64(m_ctx, value));
}

void* QuickJSBindingContext::createFloat64(f64 value)
{
    return wrapValue(JS_NewFloat64(m_ctx, value));
}

void* QuickJSBindingContext::createString(std::string_view value)
{
    return wrapValue(JS_NewStringLen(m_ctx, value.data(), value.size()));
}

void* QuickJSBindingContext::createObject()
{
    return wrapValue(JS_NewObject(m_ctx));
}

void* QuickJSBindingContext::createArray()
{
    return wrapValue(JS_NewArray(m_ctx));
}

void* QuickJSBindingContext::createObjectWithProto(void* proto, u64 classId)
{
    JSValue protoVal = unwrapValue(proto);
    JSValue obj = JS_NewObjectProtoClass(m_ctx, protoVal, static_cast<JSClassID>(classId));
    return wrapValue(obj);
}

void* QuickJSBindingContext::createFunction(ScriptMethodCallback callback, const char* name, i32 length)
{
    int magic = static_cast<int>(m_methodCallbacks.size());
    m_methodCallbacks.push_back(std::move(callback));
    JSValue fn =
        JS_NewCFunctionMagic(m_ctx, methodTrampoline, name, static_cast<int>(length), JS_CFUNC_generic_magic, magic);
    return wrapValue(fn);
}

void QuickJSBindingContext::setConstructor(void* ctor, void* proto)
{
    JS_SetConstructor(m_ctx, unwrapValue(ctor), unwrapValue(proto));
}

// ===== 值类型检查 =====

ScriptType QuickJSBindingContext::getType(void* value) const
{
    JSValue v = unwrapValue(value);
    if (JS_IsUndefined(v)) return ScriptType::Undefined;
    if (JS_IsNull(v)) return ScriptType::Null;
    if (JS_IsBool(v)) return ScriptType::Boolean;
    if (JS_IsNumber(v)) return ScriptType::Number;
    if (JS_IsString(v)) return ScriptType::String;
    if (JS_IsFunction(m_ctx, v)) return ScriptType::Function;
    if (JS_IsObject(v)) return ScriptType::Object;
    return ScriptType::Undefined;
}

bool QuickJSBindingContext::isUndefined(void* value) const
{
    return JS_IsUndefined(unwrapValue(value));
}

bool QuickJSBindingContext::isFunction(void* value) const
{
    return JS_IsFunction(m_ctx, unwrapValue(value));
}

bool QuickJSBindingContext::isObject(void* value) const
{
    return JS_IsObject(unwrapValue(value));
}

bool QuickJSBindingContext::isNumber(void* value) const
{
    return JS_IsNumber(unwrapValue(value));
}

bool QuickJSBindingContext::isString(void* value) const
{
    return JS_IsString(unwrapValue(value));
}

bool QuickJSBindingContext::isException(void* value) const
{
    return JS_IsException(unwrapValue(value));
}

// ===== 值转换 =====

std::optional<i32> QuickJSBindingContext::toInt32(void* value) const
{
    JSValue v = unwrapValue(value);
    if (!JS_IsNumber(v)) return std::nullopt;
    i32 result;
    if (JS_ToInt32(m_ctx, &result, v) != 0) return std::nullopt;
    return result;
}

std::optional<f64> QuickJSBindingContext::toFloat64(void* value) const
{
    JSValue v = unwrapValue(value);
    if (!JS_IsNumber(v)) return std::nullopt;
    f64 result;
    if (JS_ToFloat64(m_ctx, &result, v) != 0) return std::nullopt;
    return result;
}

std::optional<bool> QuickJSBindingContext::toBool(void* value) const
{
    JSValue v = unwrapValue(value);
    if (!JS_IsBool(v)) return std::nullopt;
    return JS_ToBool(m_ctx, v) != 0;
}

std::optional<std::string> QuickJSBindingContext::toString(void* value) const
{
    JSValue v = unwrapValue(value);
    if (!JS_IsString(v)) return std::nullopt;
    const char* str = JS_ToCString(m_ctx, v);
    if (!str) return std::nullopt;
    std::string result(str);
    JS_FreeCString(m_ctx, str);
    return result;
}

// ===== 对象属性操作 =====

void QuickJSBindingContext::setProperty(void* obj, const char* key, void* value)
{
    JS_DupValue(m_ctx, unwrapValue(value));
    JS_SetPropertyStr(m_ctx, unwrapValue(obj), key, unwrapValue(value));
}

void QuickJSBindingContext::setPropertyInt(void* obj, const char* key, i32 value)
{
    JS_SetPropertyStr(m_ctx, unwrapValue(obj), key, JS_NewInt32(m_ctx, value));
}

void QuickJSBindingContext::setPropertyFloat(void* obj, const char* key, f64 value)
{
    JS_SetPropertyStr(m_ctx, unwrapValue(obj), key, JS_NewFloat64(m_ctx, value));
}

void QuickJSBindingContext::setPropertyBool(void* obj, const char* key, bool value)
{
    JS_SetPropertyStr(m_ctx, unwrapValue(obj), key, JS_NewBool(m_ctx, value ? 1 : 0));
}

void QuickJSBindingContext::setPropertyString(void* obj, const char* key, std::string_view value)
{
    JS_SetPropertyStr(m_ctx, unwrapValue(obj), key, JS_NewStringLen(m_ctx, value.data(), value.size()));
}

void QuickJSBindingContext::setPropertyNull(void* obj, const char* key)
{
    JS_SetPropertyStr(m_ctx, unwrapValue(obj), key, JS_NULL);
}

void* QuickJSBindingContext::getProperty(void* obj, const char* key) const
{
    JSValue val = JS_GetPropertyStr(m_ctx, unwrapValue(obj), key);
    return wrapValue(val);
}

std::optional<i32> QuickJSBindingContext::getPropertyInt(void* obj, const char* key) const
{
    JSValue val = JS_GetPropertyStr(m_ctx, unwrapValue(obj), key);
    if (JS_IsUndefined(val) || !JS_IsNumber(val)) {
        JS_FreeValue(m_ctx, val);
        return std::nullopt;
    }
    i32 result;
    if (JS_ToInt32(m_ctx, &result, val) != 0) {
        JS_FreeValue(m_ctx, val);
        return std::nullopt;
    }
    JS_FreeValue(m_ctx, val);
    return result;
}

std::optional<f64> QuickJSBindingContext::getPropertyFloat(void* obj, const char* key) const
{
    JSValue val = JS_GetPropertyStr(m_ctx, unwrapValue(obj), key);
    if (JS_IsUndefined(val) || !JS_IsNumber(val)) {
        JS_FreeValue(m_ctx, val);
        return std::nullopt;
    }
    f64 result;
    if (JS_ToFloat64(m_ctx, &result, val) != 0) {
        JS_FreeValue(m_ctx, val);
        return std::nullopt;
    }
    JS_FreeValue(m_ctx, val);
    return result;
}

std::optional<bool> QuickJSBindingContext::getPropertyBool(void* obj, const char* key) const
{
    JSValue val = JS_GetPropertyStr(m_ctx, unwrapValue(obj), key);
    if (JS_IsUndefined(val) || !JS_IsBool(val)) {
        JS_FreeValue(m_ctx, val);
        return std::nullopt;
    }
    bool result = JS_ToBool(m_ctx, val) != 0;
    JS_FreeValue(m_ctx, val);
    return result;
}

std::optional<std::string> QuickJSBindingContext::getPropertyString(void* obj, const char* key) const
{
    JSValue val = JS_GetPropertyStr(m_ctx, unwrapValue(obj), key);
    if (JS_IsUndefined(val) || !JS_IsString(val)) {
        JS_FreeValue(m_ctx, val);
        return std::nullopt;
    }
    const char* str = JS_ToCString(m_ctx, val);
    if (!str) {
        JS_FreeValue(m_ctx, val);
        return std::nullopt;
    }
    std::string result(str);
    JS_FreeCString(m_ctx, str);
    JS_FreeValue(m_ctx, val);
    return result;
}

void QuickJSBindingContext::setArrayElementInt(void* arr, u32 index, i32 value)
{
    JS_SetPropertyUint32(m_ctx, unwrapValue(arr), index, JS_NewInt32(m_ctx, value));
}

void QuickJSBindingContext::setArrayElementString(void* arr, u32 index, std::string_view value)
{
    JS_SetPropertyUint32(m_ctx, unwrapValue(arr), index, JS_NewStringLen(m_ctx, value.data(), value.size()));
}

// ===== 引用管理 =====

void QuickJSBindingContext::retainValue(void* value)
{
    if (value) {
        JS_DupValue(m_ctx, unwrapValue(value));
    }
}

void QuickJSBindingContext::releaseValue(void* value)
{
    if (value) {
        auto* handle = static_cast<JSValue*>(value);
        JS_FreeValue(m_ctx, *handle);
        delete handle;
    }
}

// ===== 对象opaque管理 =====

void QuickJSBindingContext::setOpaque(void* obj, void* data, u64 classId)
{
    JS_SetOpaque(unwrapValue(obj), data);
}

void* QuickJSBindingContext::getOpaque(void* obj, u64 classId) const
{
    return JS_GetOpaque2(m_ctx, unwrapValue(obj), static_cast<JSClassID>(classId));
}

// ===== 函数调用 =====

void* QuickJSBindingContext::callFunction(void* func, void* thisVal, i32 argc, void** args)
{
    auto jsArgs = unwrapArgs(args, argc);
    JSValue result = JS_Call(m_ctx, unwrapValue(func), unwrapValue(thisVal), argc, jsArgs.data());
    return wrapValue(result);
}

void* QuickJSBindingContext::callFunction0(void* func, void* thisVal)
{
    JSValue result = JS_Call(m_ctx, unwrapValue(func), unwrapValue(thisVal), 0, nullptr);
    return wrapValue(result);
}

void* QuickJSBindingContext::callFunction1(void* func, void* thisVal, void* arg0)
{
    JSValueConst arg = unwrapValue(arg0);
    JSValue result = JS_Call(m_ctx, unwrapValue(func), unwrapValue(thisVal), 1, &arg);
    return wrapValue(result);
}

// ===== 错误处理 =====

void* QuickJSBindingContext::throwTypeError(const char* message)
{
    JS_ThrowTypeError(m_ctx, "%s", message);
    return wrapValue(JS_EXCEPTION);
}

void* QuickJSBindingContext::throwInternalError(const char* message)
{
    JS_ThrowInternalError(m_ctx, "%s", message);
    return wrapValue(JS_EXCEPTION);
}

void* QuickJSBindingContext::getException()
{
    return wrapValue(JS_GetException(m_ctx));
}

std::string QuickJSBindingContext::getExceptionMessage(void* exception) const
{
    const char* msg = JS_ToCString(m_ctx, unwrapValue(exception));
    std::string result = msg ? msg : "unknown";
    JS_FreeCString(m_ctx, msg);
    return result;
}

// ===== 类注册 =====

u64 QuickJSBindingContext::allocateClassId()
{
    JSRuntime* rt = JS_GetRuntime(m_ctx);
    JSClassID id = 1;
    return static_cast<u64>(JS_NewClassID(rt, &id));
}

bool QuickJSBindingContext::registerClass(u64 classId, const char* className, bool hasFinalizer)
{
    JSRuntime* rt = JS_GetRuntime(m_ctx);
    auto jsClassId = static_cast<JSClassID>(classId);

    if (JS_IsRegisteredClass(rt, jsClassId)) {
        return true;
    }

    JSClassDef classDef = {};
    classDef.class_name = className;

    if (hasFinalizer) {
        classDef.finalizer = [](JSRuntime* rt, JSValue val) {
            auto* data = static_cast<ScriptObjectRegistry::ObjectData*>(JS_GetOpaque(val, 0));
            if (data) {
                if (data->owned && data->ptr && data->destroy) {
                    data->destroy(data->ptr);
                }
                delete data;
            }
        };
    }

    return JS_NewClass(rt, jsClassId, &classDef) >= 0;
}

void* QuickJSBindingContext::createClassProto(u64 classId)
{
    JSValue proto = JS_NewObject(m_ctx);
    JS_SetClassProto(m_ctx, static_cast<JSClassID>(classId), proto);
    return wrapValue(proto);
}

void QuickJSBindingContext::registerNativeMethod(void* proto, const char* name, void* nativeFunc, i32 length)
{
    JSValue fn = JS_NewCFunction(m_ctx, reinterpret_cast<JSCFunction*>(nativeFunc), name, static_cast<int>(length));
    JS_SetPropertyStr(m_ctx, unwrapValue(proto), name, fn);
}

void QuickJSBindingContext::registerNativeReadonlyProperty(void* proto, const char* name, void* nativeGetter)
{
    JSAtom atom = JS_NewAtomLen(m_ctx, name, strlen(name));
    JSValue getterFn = JS_NewCFunction(m_ctx, reinterpret_cast<JSCFunction*>(nativeGetter), name, 0);
    JS_DefinePropertyGetSet(m_ctx, unwrapValue(proto), atom, getterFn, JS_UNDEFINED, 0);
    JS_FreeAtom(m_ctx, atom);
}

void QuickJSBindingContext::registerNativeProperty(
    void* proto, const char* name, void* nativeGetter, void* nativeSetter)
{
    JSAtom atom = JS_NewAtomLen(m_ctx, name, strlen(name));
    std::string getterName = std::string("get ") + name;
    std::string setterName = std::string("set ") + name;
    JSValue getterFn = JS_NewCFunction(m_ctx, reinterpret_cast<JSCFunction*>(nativeGetter), getterName.c_str(), 0);
    JSValue setterFn = JS_NewCFunction(m_ctx, reinterpret_cast<JSCFunction*>(nativeSetter), setterName.c_str(), 1);
    JS_DefinePropertyGetSet(m_ctx, unwrapValue(proto), atom, getterFn, setterFn, 0);
    JS_FreeAtom(m_ctx, atom);
}

// ===== 模块注册 =====

bool QuickJSBindingContext::createNativeModule(const std::string& moduleName)
{
    auto* module = JS_NewCModule(m_ctx, moduleName.c_str(), [](JSContext*, JSModuleDef*) -> int { return 0; });
    if (!module) {
        spdlog::error("[BedrockAddon] Failed to create native module: {}", moduleName);
        return false;
    }
    m_module = module;
    m_moduleName = moduleName;
    m_moduleFinalized = false;
    return true;
}

bool QuickJSBindingContext::exportNativeFunction(const std::string& name, void* nativeFunc, i32 length)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    JSValue fn =
        JS_NewCFunction(m_ctx, reinterpret_cast<JSCFunction*>(nativeFunc), name.c_str(), static_cast<int>(length));
    JS_SetModuleExport(m_ctx, module, name.c_str(), fn);
    return true;
}

bool QuickJSBindingContext::exportNativeConst(const std::string& name, i32 value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    JS_SetModuleExport(m_ctx, module, name.c_str(), JS_NewInt32(m_ctx, value));
    return true;
}

bool QuickJSBindingContext::exportNativeConstFloat(const std::string& name, f64 value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    JS_SetModuleExport(m_ctx, module, name.c_str(), JS_NewFloat64(m_ctx, value));
    return true;
}

bool QuickJSBindingContext::exportNativeConstString(const std::string& name, const std::string& value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    JS_SetModuleExport(m_ctx, module, name.c_str(), JS_NewStringLen(m_ctx, value.c_str(), value.size()));
    return true;
}

bool QuickJSBindingContext::exportNativeValue(const std::string& name, void* value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    JS_SetModuleExport(m_ctx, module, name.c_str(), unwrapValue(value));
    return true;
}

bool QuickJSBindingContext::finalizeModule()
{
    if (!m_module || m_moduleFinalized) return false;
    m_moduleFinalized = true;
    spdlog::info("[BedrockAddon] Native module finalized: {}", m_moduleName);
    return true;
}

// ===== 全局对象 =====

void* QuickJSBindingContext::getGlobalObject()
{
    return wrapValue(JS_GetGlobalObject(m_ctx));
}

// ===== 上下文数据 =====

void QuickJSBindingContext::setContextData(void* data)
{
    m_contextData = data;
}

void* QuickJSBindingContext::getContextData() const
{
    return m_contextData;
}

// ===== 高级方法/属性注册（trampoline机制） =====

QuickJSBindingContext* QuickJSBindingContext::fromJsContext(JSContext* ctx)
{
    // 从JSContext opaque获取绑定上下文指针
    // 注意：setContextData存储的是用户数据，绑定上下文指针存储在独立的opaque槽中
    // 我们使用JS_GetContextOpaque获取之前存储的QuickJSBindingContext*
    return static_cast<QuickJSBindingContext*>(JS_GetContextOpaque(ctx));
}

JSValue QuickJSBindingContext::methodTrampoline(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic)
{
    auto* bindingCtx = fromJsContext(ctx);
    if (!bindingCtx) {
        return JS_ThrowInternalError(ctx, "No binding context available");
    }

    if (magic < 0 || static_cast<size_t>(magic) >= bindingCtx->m_methodCallbacks.size()) {
        return JS_ThrowInternalError(ctx, "Invalid method callback index");
    }

    auto& callback = bindingCtx->m_methodCallbacks[magic];

    // 包装this和args为void*句柄
    void* thisHandle = new JSValue(JS_DupValue(ctx, this_val));
    std::vector<void*> argHandles;
    argHandles.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        argHandles.push_back(new JSValue(JS_DupValue(ctx, argv[i])));
    }

    void* result = callback(*bindingCtx, thisHandle, argc, argHandles.data());

    // 释放arg句柄
    for (auto* h : argHandles) {
        JS_FreeValue(ctx, *static_cast<JSValue*>(h));
        delete static_cast<JSValue*>(h);
    }
    // 释放this句柄
    JS_FreeValue(ctx, *static_cast<JSValue*>(thisHandle));
    delete static_cast<JSValue*>(thisHandle);

    // 解包结果
    if (!result) {
        return JS_UNDEFINED;
    }
    JSValue ret = *static_cast<JSValue*>(result);
    delete static_cast<JSValue*>(result);
    return ret;
}

JSValue QuickJSBindingContext::getterTrampoline(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic)
{
    auto* bindingCtx = fromJsContext(ctx);
    if (!bindingCtx) {
        return JS_ThrowInternalError(ctx, "No binding context available");
    }

    if (static_cast<size_t>(magic) >= bindingCtx->m_getterCallbacks.size()) {
        return JS_ThrowInternalError(ctx, "Invalid getter callback index");
    }

    auto& callback = bindingCtx->m_getterCallbacks[magic];

    void* thisHandle = new JSValue(JS_DupValue(ctx, this_val));
    void* result = callback(*bindingCtx, thisHandle);
    JS_FreeValue(ctx, *static_cast<JSValue*>(thisHandle));
    delete static_cast<JSValue*>(thisHandle);

    if (!result) {
        return JS_UNDEFINED;
    }
    JSValue ret = *static_cast<JSValue*>(result);
    delete static_cast<JSValue*>(result);
    return ret;
}

JSValue QuickJSBindingContext::setterTrampoline(
    JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic)
{
    auto* bindingCtx = fromJsContext(ctx);
    if (!bindingCtx) {
        return JS_ThrowInternalError(ctx, "No binding context available");
    }

    if (static_cast<size_t>(magic) >= bindingCtx->m_setterCallbacks.size()) {
        return JS_ThrowInternalError(ctx, "Invalid setter callback index");
    }

    auto& callback = bindingCtx->m_setterCallbacks[magic];

    void* thisHandle = new JSValue(JS_DupValue(ctx, this_val));
    void* valueHandle = (argc >= 1) ? new JSValue(JS_DupValue(ctx, argv[0])) : new JSValue(JS_UNDEFINED);

    callback(*bindingCtx, thisHandle, valueHandle);

    JS_FreeValue(ctx, *static_cast<JSValue*>(thisHandle));
    delete static_cast<JSValue*>(thisHandle);
    // setter的value句柄不拥有所有权（按约定），但仍需释放DupValue
    JS_FreeValue(ctx, *static_cast<JSValue*>(valueHandle));
    delete static_cast<JSValue*>(valueHandle);

    return JS_UNDEFINED;
}

void QuickJSBindingContext::registerMethod(void* proto, const char* name, ScriptMethodCallback callback, i32 length)
{
    int magic = static_cast<int>(m_methodCallbacks.size());
    m_methodCallbacks.push_back(std::move(callback));

    JSValue fn =
        JS_NewCFunctionMagic(m_ctx, methodTrampoline, name, static_cast<int>(length), JS_CFUNC_generic_magic, magic);
    JS_SetPropertyStr(m_ctx, unwrapValue(proto), name, fn);
}

void QuickJSBindingContext::registerReadonlyProperty(void* proto, const char* name, ScriptGetterCallback getter)
{
    int magic = static_cast<int>(m_getterCallbacks.size());
    m_getterCallbacks.push_back(std::move(getter));

    JSAtom atom = JS_NewAtomLen(m_ctx, name, strlen(name));
    JSValue getterFn = JS_NewCFunctionMagic(m_ctx, getterTrampoline, name, 0, JS_CFUNC_generic_magic, magic);
    JS_DefinePropertyGetSet(m_ctx, unwrapValue(proto), atom, getterFn, JS_UNDEFINED, 0);
    JS_FreeAtom(m_ctx, atom);
}

void QuickJSBindingContext::registerProperty(
    void* proto, const char* name, ScriptGetterCallback getter, ScriptSetterCallback setter)
{
    int getterMagic = static_cast<int>(m_getterCallbacks.size());
    m_getterCallbacks.push_back(std::move(getter));

    int setterMagic = static_cast<int>(m_setterCallbacks.size());
    m_setterCallbacks.push_back(std::move(setter));

    JSAtom atom = JS_NewAtomLen(m_ctx, name, strlen(name));
    std::string getterName = std::string("get ") + name;
    std::string setterName = std::string("set ") + name;
    JSValue getterFn =
        JS_NewCFunctionMagic(m_ctx, getterTrampoline, getterName.c_str(), 0, JS_CFUNC_generic_magic, getterMagic);
    JSValue setterFn =
        JS_NewCFunctionMagic(m_ctx, setterTrampoline, setterName.c_str(), 1, JS_CFUNC_generic_magic, setterMagic);
    JS_DefinePropertyGetSet(m_ctx, unwrapValue(proto), atom, getterFn, setterFn, 0);
    JS_FreeAtom(m_ctx, atom);
}

void QuickJSBindingContext::setPropertyInt64(void* obj, const char* key, i64 value)
{
    JS_SetPropertyStr(m_ctx, unwrapValue(obj), key, JS_NewInt64(m_ctx, value));
}

} // namespace mc::mod::bedrock::addon
