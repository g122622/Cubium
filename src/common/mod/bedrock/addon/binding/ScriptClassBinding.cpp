// Copyright (c) 2024 Cubium Project
// SPDX-License-Identifier: MIT

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

// ============================================================================
// ScriptObjectRegistry
// ============================================================================

void* ScriptObjectRegistry::wrap(IScriptBindingContext& ctx,
    u64 classId,
    void* proto,
    void* ptr,
    bool owned,
    const char* typeName,
    void (*destroy)(void*))
{
    if (!ptr) {
        return ctx.createNull();
    }

    void* obj = ctx.createObjectWithProto(proto, classId);
    if (ctx.isException(obj)) {
        spdlog::error("[BedrockAddon] Failed to create JS object for class: {}", typeName ? typeName : "unknown");
        return obj; // 返回异常句柄
    }

    auto* data = new ObjectData{ptr, owned, typeName, destroy};
    ctx.setOpaque(obj, data, classId);

    return obj;
}

void* ScriptObjectRegistry::unwrap(IScriptBindingContext& ctx, void* val, u64 classId)
{
    void* opaque = ctx.getOpaque(val, classId);
    if (!opaque) {
        return nullptr;
    }
    auto* data = static_cast<ObjectData*>(opaque);
    return data->ptr;
}

bool ScriptObjectRegistry::isInstanceOf(IScriptBindingContext& ctx, void* val, u64 classId)
{
    return ctx.getOpaque(val, classId) != nullptr;
}

void ScriptObjectRegistry::invalidate(IScriptBindingContext& ctx, void* val, u64 classId)
{
    void* opaque = ctx.getOpaque(val, classId);
    if (opaque) {
        auto* data = static_cast<ObjectData*>(opaque);
        data->ptr = nullptr;
        data->owned = false;
    }
}

// ============================================================================
// NativeModuleBuilder
// ============================================================================

NativeModuleBuilder::NativeModuleBuilder(IScriptBindingContext& ctx, const std::string& moduleName)
    : m_ctx(ctx)
    , m_moduleName(moduleName)
{
    if (!ctx.createNativeModule(moduleName)) {
        spdlog::error("[BedrockAddon] Failed to create native module: {}", moduleName);
    }
}

NativeModuleBuilder& NativeModuleBuilder::exportConst(const std::string& name, i32 value)
{
    if (m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export const '{}' to finalized module", name);
        return *this;
    }
    m_ctx.exportNativeConst(name, value);
    return *this;
}

NativeModuleBuilder& NativeModuleBuilder::exportConstFloat(const std::string& name, f64 value)
{
    if (m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export const '{}' to finalized module", name);
        return *this;
    }
    m_ctx.exportNativeConstFloat(name, value);
    return *this;
}

NativeModuleBuilder& NativeModuleBuilder::exportConstString(const std::string& name, const std::string& value)
{
    if (m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export const '{}' to finalized module", name);
        return *this;
    }
    m_ctx.exportNativeConstString(name, value);
    return *this;
}

NativeModuleBuilder& NativeModuleBuilder::exportValue(const std::string& name, void* value)
{
    if (m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export value '{}' to finalized module", name);
        return *this;
    }
    m_ctx.exportNativeValue(name, value);
    return *this;
}

void* NativeModuleBuilder::exportClass(const std::string& className, u64 classId)
{
    if (m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export class '{}' to finalized module", className);
        return m_ctx.createUndefined();
    }

    // 默认构造函数不可直接调用 new，抛 TypeError（对齐基岩"Use factory methods instead"语义）。
    auto stubCtor = [className](IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/) -> void* {
        return ctx.throwTypeError(
            ("Cannot directly construct " + className + ". Use factory methods instead.").c_str());
    };
    return exportClass(className, classId, std::move(stubCtor));
}

void* NativeModuleBuilder::exportClass(const std::string& className, u64 classId, ScriptMethodCallback ctorCallback)
{
    if (m_finalized) {
        spdlog::error("[BedrockAddon] Cannot export class '{}' to finalized module", className);
        return m_ctx.createUndefined();
    }

    // 注册类定义（带finalizer以释放ObjectData）
    m_ctx.registerClass(classId, className.c_str(), true);

    // 创建原型对象
    void* proto = m_ctx.createClassProto(classId);

    // 创建构造函数（由调用方提供回调，支持 new ClassName(...) 真实构造）
    void* ctor = m_ctx.createFunction(std::move(ctorCallback), className.c_str(), 0);

    // 关联构造函数和原型
    m_ctx.setConstructor(ctor, proto);

    // 将构造函数导出为模块值
    m_ctx.exportNativeValue(className, ctor);

    // 释放构造函数句柄（exportNativeValue已复制引用）
    m_ctx.releaseValue(ctor);

    return proto;
}

bool NativeModuleBuilder::finalize()
{
    if (m_finalized) {
        return false;
    }

    m_finalized = true;
    return m_ctx.finalizeModule();
}

} // namespace mc::mod::bedrock::addon
