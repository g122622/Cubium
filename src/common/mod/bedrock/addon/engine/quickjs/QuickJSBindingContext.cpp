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

QuickJSBindingContext::~QuickJSBindingContext()
{
    // 释放未被 _moduleInit 消费的 pending JSValue（模块已声明但从未被 import 时残留）。
    // 正常路径下 _moduleInit 会 erase 掉对应条目；此处兜底防泄漏。
    for (auto& [module, entries] : m_pendingExports) {
        for (auto& [name, value] : entries) {
            JS_FreeValue(m_ctx, value);
        }
    }
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
    JSValue ctorVal = unwrapValue(ctor);
    // JS_SetConstructor 把 proto 挂到 ctor.prototype 并把 proto.constructor 指回 ctor，
    // 但不设 ctor 的 is_constructor 位。QuickJS 的 C 函数（JS_NewCFunctionMagic 以
    // JS_CFUNC_generic_magic 创建）is_constructor 默认 false（仅 JS_CFUNC_constructor*
    // 才置位），导致 `new ctor(...)` 在 JS_IsConstructor 检查处直接抛 "not a constructor"，
    // 根本进不了构造回调。此处显式置位，使 `new ClassName(...)` 能进入 ctor 回调
    // （回调内再决定真实构造或抛 TypeError "Use factory methods instead"）。
    JS_SetConstructorBit(m_ctx, ctorVal, true);
    JS_SetConstructor(m_ctx, ctorVal, unwrapValue(proto));
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

std::vector<std::string> QuickJSBindingContext::getPropertyNames(void* obj) const
{
    std::vector<std::string> names;
    JSValue v = unwrapValue(obj);
    if (!JS_IsObject(v)) {
        return names;
    }

    // 只取自身、可枚举、字符串键（JS_GPN_STRING_MASK 排除 Symbol，JS_GPN_ENUM_ONLY 排除非可枚举如 length）。
    JSPropertyEnum* tab = nullptr;
    uint32_t len = 0;
    if (JS_GetOwnPropertyNames(m_ctx, &tab, &len, v, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) != 0) {
        return names; // 枚举失败（如 Proxy 异常）返回空，调用方按"无属性"处理
    }

    names.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        const char* key = JS_AtomToCString(m_ctx, tab[i].atom);
        if (key != nullptr) {
            names.emplace_back(key);
            JS_FreeCString(m_ctx, key);
        }
    }
    JS_FreePropertyEnum(m_ctx, tab, len);
    return names;
}

void QuickJSBindingContext::setArrayElementInt(void* arr, u32 index, i32 value)
{
    JS_SetPropertyUint32(m_ctx, unwrapValue(arr), index, JS_NewInt32(m_ctx, value));
}

void QuickJSBindingContext::setArrayElementString(void* arr, u32 index, std::string_view value)
{
    JS_SetPropertyUint32(m_ctx, unwrapValue(arr), index, JS_NewStringLen(m_ctx, value.data(), value.size()));
}

void QuickJSBindingContext::setArrayElement(void* arr, u32 index, void* value)
{
    // 不消耗 value 所有权：DupValue 增引用给数组，原 handle 仍属调用方（须自行 release）。
    JS_SetPropertyUint32(m_ctx, unwrapValue(arr), index, JS_DupValue(m_ctx, unwrapValue(value)));
}

void QuickJSBindingContext::setPropertyFunction(void* obj, const char* key, ScriptMethodCallback callback, i32 length)
{
    // createFunction 返回 owned 函数句柄；setProperty 内部 DupValue 后挂到 obj，本句柄仍需释放。
    // 走 createFunction + setProperty 而非直接 JS_SetPropertyStr(JS_NewCFunctionMagic(...))，是为了
    // 复用 methodTrampoline（引擎无关回调 → QuickJS C 函数 trampoline），与 registerMethod 同机制，
    // 保证回调内的 this/args 句柄包装与释放语义一致。
    void* fn = createFunction(std::move(callback), key, length);
    setProperty(obj, key, fn); // setProperty 消耗 fn 所有权（Dup 一份给 obj），fn 由 setProperty 释放
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

void* QuickJSBindingContext::dupValue(void* value)
{
    if (!value) {
        return nullptr;
    }
    // 新建独立 handle，JSValue 是入参的 Dup（refcount+1）。入参 handle 所有权不变。
    // 用于回调返回值复用入参句柄（如链式方法返回 this）：trampoline 释放入参 handle 后，
    // 本句柄仍持有独立引用，避免 use-after-free / double-free。
    const auto* src = static_cast<JSValue*>(value);
    return new JSValue(JS_DupValue(m_ctx, *src));
}

// ===== 对象opaque管理 =====

void QuickJSBindingContext::setOpaque(void* obj, void* data, u64 classId)
{
    JS_SetOpaque(unwrapValue(obj), data);
}

void* QuickJSBindingContext::getOpaque(void* obj, u64 classId) const
{
    JSValue val = unwrapValue(obj);
    // classId=0 在抽象层语义为"不检查类型，取任意 opaque"（ScriptObjectRegistry::unwrap 默认值）。
    // 但 QuickJS 的 JS_GetOpaque 要求 p->class_id 精确等于传入 class_id，class_id=0 只能取
    // class_id=0 的对象，对实际 class_id!=0 的包装对象一律返回 NULL。故 classId=0 时改用
    // JS_GetAnyOpaque（不校验 class，直接取 opaque），以对齐抽象层"0=不检查"的设计意图；
    // classId!=0 时仍走精确匹配（JS_GetOpaque2 会校验并在失败时抛 InvalidClass）。
    if (classId == 0) {
        JSClassID anyClassId = 0;
        return JS_GetAnyOpaque(val, &anyClassId);
    }
    return JS_GetOpaque2(m_ctx, val, static_cast<JSClassID>(classId));
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

void* QuickJSBindingContext::throwValue(void* value)
{
    // JS_Throw 不消耗传入值的引用（内部不增加 refcount，仅把 val 记录为当前异常）。
    // unwrapValue 返回的是入参句柄底层 JSValue 的副本（不增加 refcount），若直接抛出，
    // 调用方随后 releaseValue 释放入参句柄会使抛出的异常值悬垂。故这里 Dup 一份独立引用
    // 交给 JS_Throw，入参所有权仍归调用方。
    JS_Throw(m_ctx, JS_DupValue(m_ctx, unwrapValue(value)));
    return wrapValue(JS_EXCEPTION);
}

void QuickJSBindingContext::setPrototypeOf(void* obj, void* proto)
{
    // JS_SetPrototype 不消耗 obj/proto 的引用（内部按需 Dup），调用方对二者仍持有原所有权。
    JS_SetPrototype(m_ctx, unwrapValue(obj), unwrapValue(proto));
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
    // 必须传 0：JS_NewClassID 仅当 *pclass_id==0 时才从 rt->js_class_id_alloc 分配新 id
    // （起始值=JS_CLASS_INIT_COUNT，避开内置类）。若传非 0（如 1），函数原样返回该值，
    // 不会分配——class_id=1 落在内置类区间（< JS_CLASS_INIT_COUNT），JS_SetOpaque 会因
    // "User code can't set the opaque of internal objects" 静默失败，opaque 永远 NULL，
    // 后续 JS_GetOpaque2 全部返回 nullptr，对象方法回调取不到 C++ 指针。
    JSClassID id = 0;
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
            // finalizer 在对象 GC 时触发，此时已无法知道 classId（lambda 无捕获）。
            // JS_GetOpaque(val, 0) 要求 class_id==0，但动态类 class_id 非 0 会返回 NULL，
            // 致 ObjectData 泄漏、destroy 回调不触发（如 RegistrationBuilder 的测试提交）。
            // 改用 JS_GetAnyOpaque 取任意 class 的 opaque，不依赖 classId 匹配。
            JSClassID cid = 0;
            auto* data = static_cast<ScriptObjectRegistry::ObjectData*>(JS_GetAnyOpaque(val, &cid));
            if (data) {
                if (data->owned && data->ptr && data->destroy) {
                    data->destroy(data->ptr);
                }
                delete data;
            }
        };
    }

    int ncRet = JS_NewClass(rt, jsClassId, &classDef);
    return ncRet >= 0;
}

void* QuickJSBindingContext::createClassProto(u64 classId)
{
    JSValue proto = JS_NewObject(m_ctx);
    // JS_SetClassProto 通过 set_value 把传入引用"转移"进 ctx->class_proto[classId]（GC 根数组），
    // 不做 Dup。若直接返回 wrapValue(proto)，句柄与根共享同一份引用（refcount=1），
    // 调用方按"owned 句柄"契约 releaseValue 会让 refcount 提前归零释放对象，
    // 致 class_proto[classId] 悬垂——后续 GC 遍历 gc_obj_list 访问该悬垂对象即崩。
    // 故此处 Dup 出一份独立引用供句柄持有：根 1 份 + 句柄 1 份，调用方 release 句柄时根仍保活。
    JS_SetClassProto(m_ctx, static_cast<JSClassID>(classId), proto);
    return wrapValue(JS_DupValue(m_ctx, proto));
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
    auto* module = JS_NewCModule(m_ctx, moduleName.c_str(), _moduleInit);
    if (!module) {
        spdlog::error("[BedrockAddon] Failed to create native module: {}", moduleName);
        return false;
    }
    m_module = module;
    m_moduleName = moduleName;
    m_moduleFinalized = false;
    // pending 按 JSModuleDef* 隔离：为新模块准备空条目，不清除其他模块尚未消费的 pending。
    m_pendingExports[module];
    return true;
}

bool QuickJSBindingContext::exportNativeFunction(const std::string& name, void* nativeFunc, i32 length)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    JSValue fn =
        JS_NewCFunction(m_ctx, reinterpret_cast<JSCFunction*>(nativeFunc), name.c_str(), static_cast<int>(length));
    // 值暂存：SetModuleExport 推迟到 _moduleInit（import 时 var_ref 建好后再设）。
    m_pendingExports[module].emplace_back(name, fn);
    return true;
}

bool QuickJSBindingContext::exportNativeConst(const std::string& name, i32 value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    m_pendingExports[module].emplace_back(name, JS_NewInt32(m_ctx, value));
    return true;
}

bool QuickJSBindingContext::exportNativeConstFloat(const std::string& name, f64 value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    m_pendingExports[module].emplace_back(name, JS_NewFloat64(m_ctx, value));
    return true;
}

bool QuickJSBindingContext::exportNativeConstString(const std::string& name, const std::string& value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    m_pendingExports[module].emplace_back(name, JS_NewStringLen(m_ctx, value.c_str(), value.size()));
    return true;
}

bool QuickJSBindingContext::exportNativeValue(const std::string& name, void* value)
{
    if (!m_module || m_moduleFinalized) return false;
    auto* module = static_cast<JSModuleDef*>(m_module);
    JS_AddModuleExport(m_ctx, module, name.c_str());
    // value 是 wrapValue 的 JSValue*（引用计数 1）；Dup 一份供 pending 持有，调用者仍持有原引用
    // （exportClass 在 exportNativeValue 后会 releaseValue 释放原引用，pending 的 Dup 不受影响）。
    m_pendingExports[module].emplace_back(name, JS_DupValue(m_ctx, *static_cast<JSValue*>(value)));
    return true;
}

int QuickJSBindingContext::_moduleInit(JSContext* ctx, JSModuleDef* m)
{
    auto* bindingCtx = fromJsContext(ctx);
    if (bindingCtx == nullptr) {
        return -1;
    }
    // 此时 js_create_module_function 已为每个 AddModuleExport 声明的导出建好 var_ref，
    // 可安全 SetModuleExport 填值。SetModuleExport 会接管值的引用（pending 持有的 Dup 份需释放）。
    // 按入参 m 取回该模块自己的 pending（多模块共享一个 bindingCtx，必须按 JSModuleDef* 隔离）。
    auto it = bindingCtx->m_pendingExports.find(m);
    if (it == bindingCtx->m_pendingExports.end()) {
        return 0;
    }
    for (auto& [name, value] : it->second) {
        JS_SetModuleExport(ctx, m, name.c_str(), value);
    }
    bindingCtx->m_pendingExports.erase(it);
    return 0;
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

// ===== Promise 支持（实现 IScriptBindingContext 抽象） =====

void* QuickJSBindingContext::createPromise(void** resolvingFuncsOut)
{
    // JS_NewPromiseCapability 创建 pending Promise 并把其 resolve/reject 函数写入
    // resolving_funcs[2]（JSValue 数组）。三者均为新引用，需 wrapValue 转为 void* 句柄。
    JSValue resolving[2];
    JSValue promise = JS_NewPromiseCapability(m_ctx, resolving);
    if (resolvingFuncsOut != nullptr) {
        resolvingFuncsOut[0] = wrapValue(resolving[0]);
        resolvingFuncsOut[1] = wrapValue(resolving[1]);
    } else {
        // 调用方不要 resolving 函数：立即释放避免泄漏。
        JS_FreeValue(m_ctx, resolving[0]);
        JS_FreeValue(m_ctx, resolving[1]);
    }
    return wrapValue(promise);
}

int QuickJSBindingContext::promiseState(void* promise) const
{
    if (promise == nullptr) {
        return -1; // JSPromiseStateEnum::JS_PROMISE_INVALID（非 Promise）
    }
    return static_cast<int>(JS_PromiseState(m_ctx, unwrapValue(promise)));
}

void* QuickJSBindingContext::promiseResult(void* promise) const
{
    // JS_PromiseResult 返回 settle 值的新引用（即便原 Promise 仍 pending 也返回 undefined）。
    return wrapValue(JS_PromiseResult(m_ctx, unwrapValue(promise)));
}

bool QuickJSBindingContext::isPromise(void* value) const
{
    if (value == nullptr) {
        return false;
    }
    return JS_IsPromise(unwrapValue(value)) != 0;
}

void QuickJSBindingContext::callResolvingFunc(void* resolvingFunc, void* arg)
{
    if (resolvingFunc == nullptr) {
        return;
    }
    JSValueConst argVal = unwrapValue(arg);
    JSValue ret = JS_Call(m_ctx, unwrapValue(resolvingFunc), JS_UNDEFINED, 1, &argVal);
    // resolving 函数返回值无意义（Promise 内部状态已更新），直接释放。
    JS_FreeValue(m_ctx, ret);
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
