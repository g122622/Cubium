#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"

#include <cstring>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

// ============================================================================
// ScriptObjectRegistry
// ============================================================================

namespace {

/**
 * @brief GC finalizer for script objects
 *
 * 当JS对象被垃圾回收时调用，释放关联的C++对象（如果owned=true）。
 */
void scriptObjectFinalizer(JSRuntime* rt, JSValueConst val)
{
    // 从opaque数据中获取ObjectData
    // 由于finalizer中没有JSContext，我们直接从JSValue获取opaque
    // classId为0表示获取任何class的opaque
    auto* data = static_cast<ScriptObjectRegistry::ObjectData*>(JS_GetOpaque(val, 0));
    if (!data) {
        return;
    }

    // 如果对象由JS管理生命周期，则销毁C++对象
    if (data->owned && data->ptr) {
        if (data->destroy) {
            data->destroy(data->ptr);
        } else {
            // 没有自定义销毁回调，使用默认的delete
            // 注意：这里无法知道具体的C++类型，所以必须提供destroy回调
            spdlog::warn("[BedrockAddon] Script object finalizer: owned object without destroy callback (type: {})",
                data->typeName ? data->typeName : "unknown");
        }
    }

    delete data;
}

} // anonymous namespace

JSValue ScriptObjectRegistry::wrap(JSContext* ctx,
    JSClassID classId,
    JSValue proto,
    void* ptr,
    bool owned,
    const char* typeName,
    void (*destroy)(void*))
{
    if (!ptr) {
        return JS_NULL;
    }

    JSValue obj = JS_NewObjectProtoClass(ctx, proto, classId);
    if (JS_IsException(obj)) {
        spdlog::error("[BedrockAddon] Failed to create JS object for class: {}", typeName ? typeName : "unknown");
        return JS_EXCEPTION;
    }

    auto* data = new ObjectData{ptr, owned, typeName, destroy};
    JS_SetOpaque(obj, data);

    return obj;
}

void* ScriptObjectRegistry::unwrap(JSContext* ctx, JSValue val, JSClassID classId)
{
    ObjectData* data = nullptr;
    if (classId != JS_INVALID_CLASS_ID && classId != 0) {
        data = static_cast<ObjectData*>(JS_GetOpaque2(ctx, val, classId));
    } else {
        // classId为0时使用JS_GetOpaque（不检查类类型）
        data = static_cast<ObjectData*>(JS_GetOpaque(val, JS_INVALID_CLASS_ID));
    }
    if (!data) {
        return nullptr;
    }
    return data->ptr;
}

bool ScriptObjectRegistry::isInstanceOf(JSContext* ctx, JSValue val, JSClassID classId)
{
    return JS_GetOpaque2(ctx, val, classId) != nullptr;
}

void ScriptObjectRegistry::invalidate(JSContext* ctx, JSValue val, JSClassID classId)
{
    ObjectData* data = static_cast<ObjectData*>(JS_GetOpaque2(ctx, val, classId));
    if (data) {
        data->ptr = nullptr;
        data->owned = false;
    }
}

// ============================================================================
// NativeModuleBuilder
// ============================================================================

NativeModuleBuilder::NativeModuleBuilder(JSContext* ctx, const std::string& moduleName)
    : m_ctx(ctx)
    , m_moduleName(moduleName)
{
    // 创建C模块，使用空初始化函数
    // 导出项通过JS_SetModuleExport注册
    m_module = JS_NewCModule(ctx, moduleName.c_str(), [](JSContext*, JSModuleDef*) -> int { return 0; });

    if (!m_module) {
        spdlog::error("[BedrockAddon] Failed to create native module: {}", moduleName);
    }
}

NativeModuleBuilder& NativeModuleBuilder::exportFunction(const std::string& name, NativeFunction* func, int length)
{
    if (!m_module || m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export function '{}' to finalized/invalid module", name);
        return *this;
    }

    // 先声明导出名，再设置导出值
    JS_AddModuleExport(m_ctx, m_module, name.c_str());
    JSValue fn = JS_NewCFunction(m_ctx, func, name.c_str(), length);
    JS_SetModuleExport(m_ctx, m_module, name.c_str(), fn);
    return *this;
}

NativeModuleBuilder& NativeModuleBuilder::exportConst(const std::string& name, i32 value)
{
    if (!m_module || m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export const '{}' to finalized/invalid module", name);
        return *this;
    }

    JS_AddModuleExport(m_ctx, m_module, name.c_str());
    JS_SetModuleExport(m_ctx, m_module, name.c_str(), JS_NewInt32(m_ctx, value));
    return *this;
}

NativeModuleBuilder& NativeModuleBuilder::exportConstFloat(const std::string& name, f64 value)
{
    if (!m_module || m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export const '{}' to finalized/invalid module", name);
        return *this;
    }

    JS_AddModuleExport(m_ctx, m_module, name.c_str());
    JS_SetModuleExport(m_ctx, m_module, name.c_str(), JS_NewFloat64(m_ctx, value));
    return *this;
}

NativeModuleBuilder& NativeModuleBuilder::exportConstString(const std::string& name, const std::string& value)
{
    if (!m_module || m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export const '{}' to finalized/invalid module", name);
        return *this;
    }

    JS_AddModuleExport(m_ctx, m_module, name.c_str());
    JS_SetModuleExport(m_ctx, m_module, name.c_str(), JS_NewStringLen(m_ctx, value.c_str(), value.size()));
    return *this;
}

NativeModuleBuilder& NativeModuleBuilder::exportValue(const std::string& name, JSValue value)
{
    if (!m_module || m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export value '{}' to finalized/invalid module", name);
        return *this;
    }

    JS_AddModuleExport(m_ctx, m_module, name.c_str());
    JS_SetModuleExport(m_ctx, m_module, name.c_str(), value);
    return *this;
}

JSValue NativeModuleBuilder::exportClass(const std::string& className, JSClassID classId, JSClassFinalizer* finalizer)
{
    if (!m_module || m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export class '{}' to finalized/invalid module", className);
        return JS_UNDEFINED;
    }

    JSRuntime* rt = JS_GetRuntime(m_ctx);

    // 检查类是否已注册
    if (!JS_IsRegisteredClass(rt, classId)) {
        JSClassDef classDef = {};
        classDef.class_name = className.c_str();
        classDef.finalizer = finalizer ? finalizer : scriptObjectFinalizer;

        if (JS_NewClass(rt, classId, &classDef) < 0) {
            spdlog::error("[BedrockAddon] Failed to register JS class: {}", className);
            return JS_UNDEFINED;
        }
    }

    // 创建原型对象
    JSValue proto = JS_NewObject(m_ctx);
    JS_SetClassProto(m_ctx, classId, proto);

    // 创建构造函数（不可直接调用new，需要通过工厂方法创建对象）
    JSValue ctor = JS_NewCFunction2(
        m_ctx,
        [](JSContext* ctx, JSValueConst, int, JSValueConst*) -> JSValue {
            return JS_ThrowTypeError(ctx, "Cannot directly construct this class. Use factory methods instead.");
        },
        className.c_str(),
        0,
        JS_CFUNC_constructor,
        0);

    JS_SetConstructor(m_ctx, ctor, proto);

    // 声明并导出构造函数
    JS_AddModuleExport(m_ctx, m_module, className.c_str());
    JS_SetModuleExport(m_ctx, m_module, className.c_str(), ctor);

    return proto;
}

bool NativeModuleBuilder::finalize()
{
    if (!m_module || m_finalized) {
        return false;
    }

    m_finalized = true;
    spdlog::info("[BedrockAddon] Native module finalized: {}", m_moduleName);
    return true;
}

} // namespace mc::mod::bedrock::addon
