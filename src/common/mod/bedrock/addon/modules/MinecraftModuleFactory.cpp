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

#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/mod/bedrock/addon/modules/ScriptCustomComponentBinding.hpp"
#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"

#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

void MinecraftModuleFactory::setScheduler(ScriptScheduler* scheduler)
{
    m_scheduler = scheduler;
}

void MinecraftModuleFactory::setEventSignals(const std::vector<EventSignalInfo>& signals)
{
    m_eventSignals = signals;
}

void MinecraftModuleFactory::setEventBus(ScriptEventBus* eventBus)
{
    m_eventBus = eventBus;
}

std::vector<ModuleVersion> MinecraftModuleFactory::supportedVersions() const
{
    return {ModuleVersion{2, 0, 0}, ModuleVersion{1, 17, 0}};
}

std::vector<ModuleDependency> MinecraftModuleFactory::dependencies(const ModuleVersion& version) const
{
    return {};
}

bool MinecraftModuleFactory::registerBindings(IScriptContext& context)
{
    auto& ctx = context.bindingContext();

    spdlog::info("[BedrockAddon] Registering @minecraft/server module bindings");

    // 创建模块构建器
    NativeModuleBuilder builder(ctx, "@minecraft/server");

    // ====== 注册类 ======

    // --- System类 ---
    u64 systemClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* systemProto = builder.exportClass("System", systemClassId);

    ClassRegistrar<void> systemReg(ctx, systemClassId, systemProto);

    ScriptScheduler* scheduler = m_scheduler;

    systemReg.method(
        "run",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("system.run requires a function argument");
            }
            if (!scheduler) {
                return ctx.throwInternalError("Script scheduler not available");
            }

            // 保留回调引用（持久化到调度执行时）
            void* callback = args[0];
            ctx.retainValue(callback);
            auto* ctxPtr = &ctx;

            auto runId = scheduler->run([ctxPtr, callback]() {
                void* undef = ctxPtr->createUndefined();
                void* result = ctxPtr->callFunction0(callback, undef);
                if (ctxPtr->isException(result)) {
                    void* exc = ctxPtr->getException();
                    auto msg = ctxPtr->getExceptionMessage(exc);
                    spdlog::warn("[BedrockAddon] system.run callback error: {}", msg);
                    ctxPtr->releaseValue(exc);
                }
                ctxPtr->releaseValue(result);
                ctxPtr->releaseValue(undef);
                ctxPtr->releaseValue(callback);
            });

            return ctx.createInt32(static_cast<i32>(runId));
        },
        1);

    systemReg.method(
        "runInterval",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("system.runInterval requires a function argument");
            }

            u32 tickInterval = 1;
            if (argc >= 2 && ctx.isNumber(args[1])) {
                auto interval = ctx.toInt32(args[1]);
                if (interval && *interval > 0) {
                    tickInterval = static_cast<u32>(*interval);
                }
            }

            if (!scheduler) {
                return ctx.throwInternalError("Script scheduler not available");
            }

            void* callback = args[0];
            ctx.retainValue(callback);
            auto* ctxPtr = &ctx;

            auto runId = scheduler->runInterval(
                [ctxPtr, callback]() {
                    void* undef = ctxPtr->createUndefined();
                    void* result = ctxPtr->callFunction0(callback, undef);
                    if (ctxPtr->isException(result)) {
                        void* exc = ctxPtr->getException();
                        auto msg = ctxPtr->getExceptionMessage(exc);
                        spdlog::warn("[BedrockAddon] system.runInterval callback error: {}", msg);
                        ctxPtr->releaseValue(exc);
                    }
                    ctxPtr->releaseValue(result);
                    ctxPtr->releaseValue(undef);
                },
                tickInterval);

            return ctx.createInt32(static_cast<i32>(runId));
        },
        2);

    systemReg.method(
        "runTimeout",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("system.runTimeout requires a function argument");
            }

            u32 tickDelay = 1;
            if (argc >= 2 && ctx.isNumber(args[1])) {
                auto delay = ctx.toInt32(args[1]);
                if (delay && *delay > 0) {
                    tickDelay = static_cast<u32>(*delay);
                }
            }

            if (!scheduler) {
                return ctx.throwInternalError("Script scheduler not available");
            }

            void* callback = args[0];
            ctx.retainValue(callback);
            auto* ctxPtr = &ctx;

            auto runId = scheduler->runTimeout(
                [ctxPtr, callback]() {
                    void* undef = ctxPtr->createUndefined();
                    void* result = ctxPtr->callFunction0(callback, undef);
                    if (ctxPtr->isException(result)) {
                        void* exc = ctxPtr->getException();
                        auto msg = ctxPtr->getExceptionMessage(exc);
                        spdlog::warn("[BedrockAddon] system.runTimeout callback error: {}", msg);
                        ctxPtr->releaseValue(exc);
                    }
                    ctxPtr->releaseValue(result);
                    ctxPtr->releaseValue(undef);
                    ctxPtr->releaseValue(callback);
                },
                tickDelay);

            return ctx.createInt32(static_cast<i32>(runId));
        },
        2);

    systemReg.method(
        "clearRun",
        [scheduler](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("system.clearRun requires a run ID argument");
            }
            if (!scheduler) {
                return ctx.createUndefined();
            }
            auto runId = ctx.toInt32(args[0]);
            if (!runId) {
                return ctx.createUndefined();
            }
            scheduler->clearRun(static_cast<ScriptScheduler::RunId>(*runId));
            return ctx.createUndefined();
        },
        1);

    systemReg.readonlyProperty("currentTick", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        return ctx.createInt64(static_cast<i64>(ScriptWorldAccessor::instance().currentTick()));
    });

    // --- World类 ---
    u64 worldClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* worldProto = builder.exportClass("World", worldClassId);

    ClassRegistrar<void> worldReg(ctx, worldClassId, worldProto);
    worldReg.method("getDimension", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        // TODO: 返回Dimension对象
        return ctx.createUndefined();
    });
    worldReg.method("getAllPlayers", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        auto names = ScriptWorldAccessor::instance().getAllPlayerNames();
        void* arr = ctx.createArray();
        for (u32 i = 0; i < names.size(); ++i) {
            ctx.setArrayElementString(arr, i, names[i]);
        }
        return arr;
    });
    worldReg.method("sendMessage", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        if (argc < 1 || !ctx.isString(args[0])) {
            return ctx.throwTypeError("world.sendMessage requires a string argument");
        }
        auto msg = ctx.toString(args[0]);
        if (!msg) {
            return ctx.throwInternalError("Failed to convert message to string");
        }
        ScriptWorldAccessor::instance().sendMessage(*msg);
        return ctx.createUndefined();
    });

    // --- Dimension类 ---
    u64 dimensionClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* dimensionProto = builder.exportClass("Dimension", dimensionClassId);

    ClassRegistrar<void> dimensionReg(ctx, dimensionClassId, dimensionProto);
    dimensionReg.readonlyProperty("id", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 返回维度ID
        return ctx.createString("minecraft:overworld");
    });

    // --- Entity类 ---
    u64 entityClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* entityProto = builder.exportClass("Entity", entityClassId);

    ClassRegistrar<void> entityReg(ctx, entityClassId, entityProto);
    entityReg.readonlyProperty("id", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 返回实体ID
        return ctx.createUndefined();
    });
    entityReg.readonlyProperty("typeId", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 返回实体类型ID
        return ctx.createUndefined();
    });
    entityReg.method("getDimension", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        return ctx.createUndefined();
    });
    entityReg.method("getLocation", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
        return ctx.createUndefined();
    });

    // --- Player类（继承Entity） ---
    u64 playerClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* playerProto = builder.exportClass("Player", playerClassId);

    ClassRegistrar<void> playerReg(ctx, playerClassId, playerProto);
    playerReg.readonlyProperty("name", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 返回玩家名
        return ctx.createUndefined();
    });

    // --- Block类 ---
    u64 blockClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* blockProto = builder.exportClass("Block", blockClassId);
    ctx.releaseValue(blockProto);

    // --- ItemStack类 ---
    u64 itemStackClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* itemStackProto = builder.exportClass("ItemStack", itemStackClassId);

    ClassRegistrar<void> itemStackReg(ctx, itemStackClassId, itemStackProto);
    itemStackReg.readonlyProperty(
        "typeId", [](IScriptBindingContext& ctx, void* thisVal) -> void* { return ctx.createUndefined(); });
    itemStackReg.property(
        "amount",
        [](IScriptBindingContext& ctx, void* thisVal) -> void* { return ctx.createUndefined(); },
        [](IScriptBindingContext& ctx, void* thisVal, void* value) {
            // TODO: 设置amount
        });

    // ====== 注册常量 ======

    builder.exportConst("GameModeSurvival", 0);
    builder.exportConst("GameModeCreative", 1);
    builder.exportConst("GameModeAdventure", 2);
    builder.exportConst("GameModeSpectator", 3);

    builder.exportConstString("MinecraftDimensionTypesOverworld", "minecraft:overworld");
    builder.exportConstString("MinecraftDimensionTypesNether", "minecraft:nether");
    builder.exportConstString("MinecraftDimensionTypesTheEnd", "minecraft:the_end");

    // ====== 导出全局对象 ======

    // 创建system全局对象
    void* systemObj = ScriptObjectRegistry::wrap(ctx, systemClassId, systemProto, nullptr, false, "System");
    builder.exportValue("system", systemObj);
    ctx.releaseValue(systemObj);

    // 创建world全局对象
    void* worldObj = ScriptObjectRegistry::wrap(ctx, worldClassId, worldProto, nullptr, false, "World");
    builder.exportValue("world", worldObj);

    // ====== 注册事件绑定 ======
    if (m_eventBus && !m_eventSignals.empty()) {
        registerEventBindings(ctx, worldObj, *m_eventBus, m_eventSignals);
    }

    // ====== 注册自定义组件绑定 ======
    registerCustomComponentBindings(builder);

    // ====== 完成模块注册 ======
    if (!builder.finalize()) {
        spdlog::error("[BedrockAddon] Failed to finalize @minecraft/server module");
        ctx.releaseValue(worldObj);
        return false;
    }

    ctx.releaseValue(worldObj);

    spdlog::info("[BedrockAddon] @minecraft/server module bindings registered successfully");
    return true;
}

} // namespace mc::mod::bedrock::addon
