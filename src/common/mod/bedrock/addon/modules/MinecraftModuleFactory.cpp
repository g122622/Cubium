#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/binding/TypeConverter.hpp"
#include "common/mod/bedrock/addon/engine/QuickJSContext.hpp"

#include <quickjs.h>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

// ============================================================================
// System类方法 - 全局system对象的方法
// ============================================================================

static JSValue systemRun(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // system.run(callback) - 下一tick执行回调
    // TODO: 集成ScriptTickListener
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "system.run requires a function argument");
    }
    return JS_NewInt32(ctx, 0); // 返回runId
}

static JSValue systemRunInterval(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // system.runInterval(callback, tickInterval)
    // TODO: 集成ScriptTickListener
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "system.runInterval requires a function argument");
    }
    return JS_NewInt32(ctx, 0);
}

static JSValue systemRunTimeout(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // system.runTimeout(callback, tickDelay)
    // TODO: 集成ScriptTickListener
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "system.runTimeout requires a function argument");
    }
    return JS_NewInt32(ctx, 0);
}

static JSValue systemClearRun(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // system.clearRun(runId)
    // TODO: 集成ScriptTickListener
    return JS_UNDEFINED;
}

static JSValue systemGetCurrentTick(JSContext* ctx, JSValueConst this_val, int, JSValueConst*)
{
    // TODO: 返回当前tick
    return JS_NewInt64(ctx, 0);
}

// ============================================================================
// World类方法
// ============================================================================

static JSValue worldGetDimension(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // TODO: 返回Dimension对象
    return JS_UNDEFINED;
}

static JSValue worldGetAllPlayers(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // TODO: 返回玩家数组
    return JS_NewArray(ctx);
}

static JSValue worldSendMessage(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    // TODO: 实现消息发送
    return JS_UNDEFINED;
}

// ============================================================================
// Dimension类
// ============================================================================

static JSValue dimensionGetId(JSContext* ctx, JSValueConst this_val, int, JSValueConst*)
{
    // TODO: 返回维度ID
    return JS_NewString(ctx, "minecraft:overworld");
}

// ============================================================================
// Entity类
// ============================================================================

static JSValue entityGetId(JSContext* ctx, JSValueConst this_val, int, JSValueConst*)
{
    // TODO: 返回实体ID
    return JS_UNDEFINED;
}

static JSValue entityGetTypeId(JSContext* ctx, JSValueConst this_val, int, JSValueConst*)
{
    // TODO: 返回实体类型ID
    return JS_UNDEFINED;
}

static JSValue entityGetDimension(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    return JS_UNDEFINED;
}

static JSValue entityGetLocation(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    return JS_UNDEFINED;
}

// ============================================================================
// Player类
// ============================================================================

static JSValue playerGetName(JSContext* ctx, JSValueConst this_val, int, JSValueConst*)
{
    // TODO: 返回玩家名
    return JS_UNDEFINED;
}

// ============================================================================
// ItemStack类
// ============================================================================

static JSValue itemStackGetTypeId(JSContext* ctx, JSValueConst this_val, int, JSValueConst*)
{
    return JS_UNDEFINED;
}

static JSValue itemStackGetAmount(JSContext* ctx, JSValueConst this_val, int, JSValueConst*)
{
    return JS_UNDEFINED;
}

static JSValue itemStackSetAmount(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    return JS_UNDEFINED;
}

// ============================================================================
// MinecraftModuleFactory
// ============================================================================

std::vector<ModuleVersion> MinecraftModuleFactory::supportedVersions() const
{
    return {ModuleVersion{2, 0, 0}, ModuleVersion{1, 17, 0}};
}

std::vector<ModuleDependency> MinecraftModuleFactory::dependencies(const ModuleVersion& version) const
{
    // @minecraft/server 没有外部依赖
    return {};
}

bool MinecraftModuleFactory::registerBindings(IScriptContext& context)
{
    auto* jsCtx = static_cast<QuickJSContext*>(&context);
    if (!jsCtx || !jsCtx->jsContext()) {
        spdlog::error("[BedrockAddon] MinecraftModuleFactory: invalid context");
        return false;
    }

    JSContext* ctx = jsCtx->jsContext();
    JSRuntime* rt = JS_GetRuntime(ctx);

    spdlog::info("[BedrockAddon] Registering @minecraft/server module bindings");

    // 创建模块构建器
    NativeModuleBuilder builder(ctx, "@minecraft/server");

    // ====== 注册类 ======

    // --- System类 ---
    JSClassID systemClassId = ScriptObjectRegistry::allocateClassId(rt);
    JSValue systemProto = builder.exportClass("System", systemClassId);

    ClassRegistrar<void> systemReg(ctx, systemClassId, systemProto);
    systemReg.method("run", systemRun)
        .method("runInterval", systemRunInterval)
        .method("runTimeout", systemRunTimeout)
        .method("clearRun", systemClearRun)
        .readonlyProperty("currentTick", systemGetCurrentTick);

    // --- World类 ---
    JSClassID worldClassId = ScriptObjectRegistry::allocateClassId(rt);
    JSValue worldProto = builder.exportClass("World", worldClassId);

    ClassRegistrar<void> worldReg(ctx, worldClassId, worldProto);
    worldReg.method("getDimension", worldGetDimension)
        .method("getAllPlayers", worldGetAllPlayers)
        .method("sendMessage", worldSendMessage);

    // --- Dimension类 ---
    JSClassID dimensionClassId = ScriptObjectRegistry::allocateClassId(rt);
    JSValue dimensionProto = builder.exportClass("Dimension", dimensionClassId);

    ClassRegistrar<void> dimensionReg(ctx, dimensionClassId, dimensionProto);
    dimensionReg.readonlyProperty("id", dimensionGetId);

    // --- Entity类 ---
    JSClassID entityClassId = ScriptObjectRegistry::allocateClassId(rt);
    JSValue entityProto = builder.exportClass("Entity", entityClassId);

    ClassRegistrar<void> entityReg(ctx, entityClassId, entityProto);
    entityReg.readonlyProperty("id", entityGetId)
        .readonlyProperty("typeId", entityGetTypeId)
        .method("getDimension", entityGetDimension)
        .method("getLocation", entityGetLocation);

    // --- Player类（继承Entity） ---
    JSClassID playerClassId = ScriptObjectRegistry::allocateClassId(rt);
    JSValue playerProto = builder.exportClass("Player", playerClassId);

    ClassRegistrar<void> playerReg(ctx, playerClassId, playerProto);
    playerReg.readonlyProperty("name", playerGetName);

    // --- Block类 ---
    JSClassID blockClassId = ScriptObjectRegistry::allocateClassId(rt);
    builder.exportClass("Block", blockClassId);

    // --- ItemStack类 ---
    JSClassID itemStackClassId = ScriptObjectRegistry::allocateClassId(rt);
    JSValue itemStackProto = builder.exportClass("ItemStack", itemStackClassId);

    ClassRegistrar<void> itemStackReg(ctx, itemStackClassId, itemStackProto);
    itemStackReg.readonlyProperty("typeId", itemStackGetTypeId)
        .property("amount", itemStackGetAmount, itemStackSetAmount);

    // ====== 注册常量 ======

    // GameMode枚举
    builder.exportConst("GameModeSurvival", 0);
    builder.exportConst("GameModeCreative", 1);
    builder.exportConst("GameModeAdventure", 2);
    builder.exportConst("GameModeSpectator", 3);

    // Dimension ID常量
    builder.exportConstString("MinecraftDimensionTypesOverworld", "minecraft:overworld");
    builder.exportConstString("MinecraftDimensionTypesNether", "minecraft:nether");
    builder.exportConstString("MinecraftDimensionTypesTheEnd", "minecraft:the_end");

    // ====== 导出全局对象 ======

    // 创建system全局对象
    JSValue systemObj = JS_NewObjectProtoClass(ctx, systemProto, systemClassId);
    auto* systemData = new ScriptObjectRegistry::ObjectData{nullptr, false, "System", nullptr};
    JS_SetOpaque(systemObj, systemData);
    builder.exportValue("system", systemObj);

    // 创建world全局对象
    JSValue worldObj = JS_NewObjectProtoClass(ctx, worldProto, worldClassId);
    auto* worldData = new ScriptObjectRegistry::ObjectData{nullptr, false, "World", nullptr};
    JS_SetOpaque(worldObj, worldData);
    builder.exportValue("world", worldObj);

    // ====== 完成模块注册 ======
    if (!builder.finalize()) {
        spdlog::error("[BedrockAddon] Failed to finalize @minecraft/server module");
        return false;
    }

    spdlog::info("[BedrockAddon] @minecraft/server module bindings registered successfully");
    return true;
}

} // namespace mc::mod::bedrock::addon
