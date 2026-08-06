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
#include "common/entity/core/Entity.hpp" // mc::Entity（Entity JS 类 opaque 持此指针）
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 classId/proto 注册表
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/mod/bedrock/addon/modules/ScriptCustomComponentBinding.hpp"
#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"
#include "common/util/AxisAlignedBB.hpp" // Dimension.getEntities 构造查询包围盒
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp" // Dimension JS 类 opaque 持 IWorld*

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
    // opaque 持 mc::IWorld*（非拥有，世界由服务器管理）。GameTest 单维度场景下维度即世界；
    // test.getDimension() 与 world.getDimension() 均 wrap 同一 IWorld*。getEntities 按基岩语义
    // {type, location, volume} 查询：location 是中心、volume 是全尺寸（非半尺寸），AABB = loc ± vol/2。
    u64 dimensionClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* dimensionProto = builder.exportClass("Dimension", dimensionClassId);
    ScriptClassRegistry::instance().registerClass(dimensionClassId, dimensionProto, "Dimension");

    ClassRegistrar<void> dimensionReg(ctx, dimensionClassId, dimensionProto);
    dimensionReg.readonlyProperty("id", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // GameTest 单维度场景固定主世界；多维场景需 IWorld 暴露 dimensionId（TODO）。
        return ctx.createString("minecraft:overworld");
    });
    dimensionReg.method(
        "getEntities",
        [dimensionClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* world = static_cast<mc::IWorld*>(ScriptObjectRegistry::unwrap(ctx, thisVal, dimensionClassId));
            if (world == nullptr) {
                return ctx.createArray(); // 空数组，property getter 不抛
            }
            // 解析查询参数 {type?, location?, volume?}（均可选，对齐基岩 EntityQueryOptions）
            std::string typeFilter;
            bool hasType = false;
            if (argc >= 1 && ctx.isObject(args[0])) {
                void* opts = args[0];
                void* typeVal = ctx.getProperty(opts, "type");
                if (typeVal != nullptr && ctx.isString(typeVal)) {
                    auto t = ctx.toString(typeVal);
                    if (t) {
                        typeFilter = *t;
                        hasType = true;
                    }
                }
                ctx.releaseValue(typeVal);

                // location 中心 + volume 全尺寸 → AABB
                f32 lx = 0, ly = 0, lz = 0;
                bool hasLoc = false;
                void* locVal = ctx.getProperty(opts, "location");
                if (locVal != nullptr && ctx.isObject(locVal)) {
                    auto x = ctx.getPropertyFloat(locVal, "x");
                    auto y = ctx.getPropertyFloat(locVal, "y");
                    auto z = ctx.getPropertyFloat(locVal, "z");
                    if (x && y && z) {
                        lx = static_cast<f32>(*x);
                        ly = static_cast<f32>(*y);
                        lz = static_cast<f32>(*z);
                        hasLoc = true;
                    }
                }
                ctx.releaseValue(locVal);

                f32 vx = 0, vy = 0, vz = 0;
                bool hasVol = false;
                void* volVal = ctx.getProperty(opts, "volume");
                if (volVal != nullptr && ctx.isObject(volVal)) {
                    auto x = ctx.getPropertyFloat(volVal, "x");
                    auto y = ctx.getPropertyFloat(volVal, "y");
                    auto z = ctx.getPropertyFloat(volVal, "z");
                    if (x && y && z) {
                        vx = static_cast<f32>(*x);
                        vy = static_cast<f32>(*y);
                        vz = static_cast<f32>(*z);
                        hasVol = true;
                    }
                }
                ctx.releaseValue(volVal);

                if (hasLoc && hasVol) {
                    mc::AxisAlignedBB box(
                        lx - vx * 0.5f, ly - vy * 0.5f, lz - vz * 0.5f, lx + vx * 0.5f, ly + vy * 0.5f, lz + vz * 0.5f);
                    auto entities = world->getEntitiesInAABB(box, nullptr);
                    void* arr = ctx.createArray();
                    u32 outIdx = 0;
                    const u64 entClassId = ScriptClassRegistry::instance().classIdByName("Entity");
                    void* entProto = ScriptClassRegistry::instance().proto(entClassId);
                    for (auto* ent : entities) {
                        if (ent == nullptr) {
                            continue;
                        }
                        if (hasType && ent->getTypeId() != typeFilter) {
                            continue;
                        }
                        if (entProto != nullptr) {
                            void* jsEnt = ScriptObjectRegistry::wrap(ctx, entClassId, entProto, ent, false, "Entity");
                            ctx.setArrayElement(arr, outIdx, jsEnt); // 不消耗所有权，须手动 release
                            ctx.releaseValue(jsEnt);
                            ++outIdx;
                        }
                    }
                    return arr;
                }
            }
            // 无 location/volume：退化为全类型遍历（仅按 type 过滤），对齐基岩空 query 语义。
            // TODO: 无区域约束的全局遍历在大世界性能差，GameTest 小范围场景可接受；生产侧慎用。
            if (hasType) {
                auto entities = world->getEntitiesByType(typeFilter);
                void* arr = ctx.createArray();
                u32 outIdx = 0;
                const u64 entClassId = ScriptClassRegistry::instance().classIdByName("Entity");
                void* entProto = ScriptClassRegistry::instance().proto(entClassId);
                for (auto* ent : entities) {
                    if (ent == nullptr || entProto == nullptr) {
                        continue;
                    }
                    void* jsEnt = ScriptObjectRegistry::wrap(ctx, entClassId, entProto, ent, false, "Entity");
                    ctx.setArrayElement(arr, outIdx, jsEnt);
                    ctx.releaseValue(jsEnt);
                    ++outIdx;
                }
                return arr;
            }
            return ctx.createArray();
        },
        1);

    // --- Entity类 ---
    // opaque 持 mc::Entity*（非拥有，EntityManager 管理生命周期）。test.spawn 经 ScriptClassRegistry
    // 取本 classId/proto wrap 真实指针。getComponent("minecraft:rideable") 返回合成 RideableComponent。
    u64 entityClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* entityProto = builder.exportClass("Entity", entityClassId);
    ScriptClassRegistry::instance().registerClass(entityClassId, entityProto, "Entity");

    ClassRegistrar<void> entityReg(ctx, entityClassId, entityProto);
    entityReg.readonlyProperty("id", [entityClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
        if (ent == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt64(static_cast<i64>(ent->id()));
    });
    entityReg.readonlyProperty("typeId", [entityClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
        if (ent == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(ent->getTypeId());
    });
    entityReg.method(
        "getDimension",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr) {
                return ctx.createUndefined();
            }
            auto* world = ent->world();
            if (world == nullptr) {
                return ctx.createUndefined();
            }
            const u64 dimClassId = ScriptClassRegistry::instance().classIdByName("Dimension");
            void* dimProto = ScriptClassRegistry::instance().proto(dimClassId);
            if (dimProto == nullptr) {
                return ctx.createUndefined();
            }
            return ScriptObjectRegistry::wrap(ctx, dimClassId, dimProto, world, false, "Dimension");
        });
    entityReg.method(
        "getLocation",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr) {
                return ctx.createUndefined();
            }
            auto pos = ent->position();
            void* obj = ctx.createObject();
            ctx.setPropertyFloat(obj, "x", static_cast<f64>(pos.x));
            ctx.setPropertyFloat(obj, "y", static_cast<f64>(pos.y));
            ctx.setPropertyFloat(obj, "z", static_cast<f64>(pos.z));
            return obj;
        });
    entityReg.method(
        "getComponent",
        [entityClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            // 基岩 Entity.getComponent(componentId) 返回组件对象。项目无 C++ 实体组件体系，
            // 仅合成 "minecraft:rideable"：返回 RideableComponent JS 对象（opaque 持同一 Entity*），
            // 其 addRider(passenger) 调 passenger->startRiding(*vehicle)。其他 componentId 返回 undefined。
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createUndefined();
            }
            auto compId = ctx.toString(args[0]);
            if (!compId) {
                return ctx.createUndefined();
            }
            if (*compId != "minecraft:rideable") {
                // TODO: 其他实体组件（health/movement 等）按需补全。
                return ctx.createUndefined();
            }
            const u64 rideableClassId = ScriptClassRegistry::instance().classIdByName("RideableComponent");
            void* rideableProto = ScriptClassRegistry::instance().proto(rideableClassId);
            if (rideableProto == nullptr) {
                return ctx.createUndefined();
            }
            // RideableComponent opaque 持载具 Entity*（与 Entity 对象同指针，独立 JS 对象）。
            return ScriptObjectRegistry::wrap(ctx, rideableClassId, rideableProto, ent, false, "RideableComponent");
        },
        1);

    // --- RideableComponent类（合成，非基岩标准 API 名，仅 GameTest addRider 用）---
    // opaque 持载具 mc::Entity*。addRider(passengerEntity) 调 passenger->startRiding(*vehicle)。
    // 注意：不能调 vehicle->addPassenger(*passenger)，因其前置条件 passenger.getVehicle()==vehicle.id()
    // 未满足会断言失败；startRiding 是公共入口，内部设 m_vehicle 后再调 addPassenger。
    u64 rideableClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* rideableProto = builder.exportClass("RideableComponent", rideableClassId);
    ScriptClassRegistry::instance().registerClass(rideableClassId, rideableProto, "RideableComponent");

    ClassRegistrar<void> rideableReg(ctx, rideableClassId, rideableProto);
    rideableReg.method(
        "addRider",
        [rideableClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* vehicle = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, rideableClassId));
            if (vehicle == nullptr) {
                return ctx.throwInternalError("RideableComponent.addRider: invalid vehicle");
            }
            if (argc < 1) {
                return ctx.throwTypeError("addRider(passengerEntity)");
            }
            const u64 entClassId = ScriptClassRegistry::instance().classIdByName("Entity");
            auto* passenger = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, args[0], entClassId));
            if (passenger == nullptr) {
                return ctx.throwTypeError("addRider: argument must be an Entity");
            }
            if (!passenger->startRiding(*vehicle)) {
                return ctx.throwInternalError("addRider: passenger failed to start riding vehicle");
            }
            return ctx.createUndefined();
        },
        1);

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
