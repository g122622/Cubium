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
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"            // mc::Entity（Entity JS 类 opaque 持此指针）
#include "common/entity/core/EquipmentSlot.hpp"     // EquipmentSlot 枚举（EquippableComponent 槽位映射）
#include "common/entity/core/LivingEntity.hpp"      // LivingEntity（health/maxHealth/attributes/getEquipment）
#include "common/entity/entities/player/Player.hpp" // Player::username（Player.name）
#include "common/item/core/Item.hpp"                // Item::toString（ItemStack.typeId getter）
#include "common/item/core/ItemStack.hpp"           // ItemStack（Equippable.getEquipment 返回值）
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

#include <optional>
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
                    // 基岩 EntityQueryOptions.type 既接受 "minecraft:zoglin" 也接受 "zoglin"（自动补前缀）。
                    // getTypeId() 恒返回带 "minecraft:" 前缀的完整 id，故将 typeFilter 规范化为完整 id 后比较。
                    std::string normalizedType = typeFilter;
                    if (hasType && normalizedType.find(':') == std::string::npos) {
                        normalizedType = "minecraft:" + normalizedType;
                    }
                    for (auto* ent : entities) {
                        if (ent == nullptr) {
                            continue;
                        }
                        if (hasType && ent->getTypeId() != normalizedType) {
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
    // 取本 classId/proto wrap 真实指针。getComponent 派发 rideable/health/movement/equippable/onfire 组件。
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
    entityReg.method("getDimension",
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
    entityReg.method("getLocation",
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
            // 基岩 Entity.getComponent(componentId) 返回组件对象，组件不存在返 undefined。
            // componentId 既接受 "minecraft:health" 也接受 "health"，不含 ':' 时补 minecraft: 前缀。
            // 派发：rideable（OOP 合成）/ health/movement/equippable（须 LivingEntity）/ onfire（须 isOnFire）。
            // 各组件 JS 类 opaque 持同一 mc::Entity*（owned=false），getter 内 dynamic_cast/tryGetComponent 现取数据。
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            if (ent == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createUndefined();
            }
            auto compId = ctx.toString(args[0]);
            if (!compId) {
                return ctx.createUndefined();
            }
            // normalize 前缀（对齐 Dimension.getEntities typeFilter 规范化语义）。
            std::string normalized = *compId;
            if (normalized.find(':') == std::string::npos) {
                normalized = "minecraft:" + normalized;
            }

            // 按类名 wrap 组件 JS 对象（opaque 持 ent，owned=false）。类未注册时返 undefined。
            auto wrapComponent = [&ctx, ent](const char* className) -> void* {
                const u64 classId = ScriptClassRegistry::instance().classIdByName(className);
                void* proto = ScriptClassRegistry::instance().proto(classId);
                if (proto == nullptr) {
                    return ctx.createUndefined();
                }
                return ScriptObjectRegistry::wrap(ctx, classId, proto, ent, false, className);
            };

            if (normalized == "minecraft:rideable") {
                return wrapComponent("RideableComponent");
            }
            if (normalized == "minecraft:health" || normalized == "minecraft:movement" ||
                normalized == "minecraft:equippable") {
                // health/movement/equippable 仅 LivingEntity attach，非 LivingEntity 返 undefined。
                if (dynamic_cast<mc::LivingEntity*>(ent) == nullptr) {
                    return ctx.createUndefined();
                }
                if (normalized == "minecraft:health") return wrapComponent("HealthComponent");
                if (normalized == "minecraft:movement") return wrapComponent("MovementComponent");
                return wrapComponent("EquippableComponent");
            }
            if (normalized == "minecraft:onfire") {
                // 对齐基岩 OnFireComponent："When present on an entity, this entity is on fire"。
                if (!ent->isOnFire()) {
                    return ctx.createUndefined();
                }
                return wrapComponent("OnFireComponent");
            }
            // TODO: 其他基岩合法 componentId（is_baby/is_tamed/lava_movement 等标记/属性族）按需补全。
            return ctx.createUndefined();
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

    // --- OnFireComponent类（minecraft:onfire）---
    // opaque 持 mc::Entity*。FireComponent 在 Entity 层 attach，无 LivingEntity 约束。
    // 对齐基岩 EntityOnFireComponent：仅 readonly onFireTicksRemaining（设火走 Entity.setOnFire，不在此暴露）。
    u64 onFireClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* onFireProto = builder.exportClass("OnFireComponent", onFireClassId);
    ScriptClassRegistry::instance().registerClass(onFireClassId, onFireProto, "OnFireComponent");

    ClassRegistrar<void> onFireReg(ctx, onFireClassId, onFireProto);
    onFireReg.readonlyProperty(
        "onFireTicksRemaining", [onFireClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, onFireClassId));
            if (ent == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createInt32(ent->getRemainingFireTicks());
        });

    // --- HealthComponent类（minecraft:health，Attribute 族）---
    // opaque 持 mc::Entity*。HealthComponent 仅 LivingEntity attach，getter 内 dynamic_cast<LivingEntity*>，
    // 失败返 undefined（对齐基岩"组件不存在则 getComponent 返 undefined"）。currentValue/effectiveMax 走
    // LivingEntity::health()/maxHealth()（HealthComponent 真相源 + 属性系统）；setCurrentValue/resetToMaxValue
    // 走 setHealth（带 DataParameter 同步副作用）。effectiveMin/defaultValue 保守硬编码留 TODO。
    u64 healthClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* healthProto = builder.exportClass("HealthComponent", healthClassId);
    ScriptClassRegistry::instance().registerClass(healthClassId, healthProto, "HealthComponent");

    ClassRegistrar<void> healthReg(ctx, healthClassId, healthProto);
    healthReg.readonlyProperty("currentValue", [healthClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createFloat64(static_cast<f64>(living->health()));
    });
    healthReg.readonlyProperty("effectiveMax", [healthClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createFloat64(static_cast<f64>(living->maxHealth()));
    });
    healthReg.readonlyProperty("effectiveMin", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 读 AttributeInstance::attribute().minValue()，属性边界后续完善。
        return ctx.createFloat64(0.0);
    });
    healthReg.readonlyProperty("defaultValue", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: HealthComponent 默认 m_health{20.0f}，按实体类型差异后续完善。
        return ctx.createFloat64(20.0);
    });
    healthReg.method(
        "setCurrentValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr) {
                return ctx.createBoolean(false);
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("health.setCurrentValue requires a number argument");
            }
            auto value = ctx.toFloat64(args[0]);
            if (!value) {
                return ctx.createBoolean(false);
            }
            living->setHealth(static_cast<f32>(*value)); // 内部 clamp 到 [0, maxHealth]
            return ctx.createBoolean(true);
        },
        1);
    healthReg.method("resetToMaxValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->setHealth(living->maxHealth());
            }
            return ctx.createUndefined();
        });
    healthReg.method("resetToMinValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            // TODO: 按 AttributeInstance::attribute().minValue() 设置，暂用 1.0（属性 min=1.0）。
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->setHealth(1.0f);
            }
            return ctx.createUndefined();
        });
    healthReg.method("resetToDefaultValue",
        [healthClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            // TODO: 按实体类型默认 health 设置，暂用 20.0。
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, healthClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->setHealth(20.0f);
            }
            return ctx.createUndefined();
        });

    // --- MovementComponent类（minecraft:movement，Attribute 族）---
    // opaque 持 mc::Entity*。移动速度走 AttributeComponent 持有的 AttributeMap（Attributes::MOVEMENT_SPEED）。
    // dynamic_cast<LivingEntity*> 后经 attributes() 读写。effectiveMax/Min/defaultValue 走
    // AttributeInstance::attribute().maxValue()/minValue()/defaultValue()，属性实例缺失时 fallback。
    u64 movementClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* movementProto = builder.exportClass("MovementComponent", movementClassId);
    ScriptClassRegistry::instance().registerClass(movementClassId, movementProto, "MovementComponent");

    ClassRegistrar<void> movementReg(ctx, movementClassId, movementProto);
    movementReg.readonlyProperty("currentValue", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createFloat64(living->attributes().getValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED, 0.0));
    });
    movementReg.readonlyProperty("effectiveMax", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
        // 属性实例缺失时 fallback 1024.0（vanilla MOVEMENT_SPEED 上界）。
        return ctx.createFloat64(inst != nullptr ? inst->attribute().maxValue() : 1024.0);
    });
    movementReg.readonlyProperty("effectiveMin", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
        return ctx.createFloat64(inst != nullptr ? inst->attribute().minValue() : 0.0);
    });
    movementReg.readonlyProperty("defaultValue", [movementClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
        auto* living = dynamic_cast<mc::LivingEntity*>(ent);
        if (living == nullptr) {
            return ctx.createUndefined();
        }
        const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
        // TODO: 各实体默认移动速度不同，inst 缺失时 fallback 0.25（vanilla 通用值）。
        return ctx.createFloat64(inst != nullptr ? inst->attribute().defaultValue() : 0.25);
    });
    movementReg.method(
        "setCurrentValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr) {
                return ctx.createBoolean(false);
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("movement.setCurrentValue requires a number argument");
            }
            auto value = ctx.toFloat64(args[0]);
            if (!value) {
                return ctx.createBoolean(false);
            }
            living->attributes().setBaseValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED, *value);
            return ctx.createBoolean(true);
        },
        1);
    movementReg.method("resetToDefaultValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                living->attributes().resetBaseValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
            }
            return ctx.createUndefined();
        });
    movementReg.method("resetToMaxValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
                if (inst != nullptr) {
                    living->attributes().setBaseValue(
                        mc::entity::attribute::Attributes::MOVEMENT_SPEED, inst->attribute().maxValue());
                }
            }
            return ctx.createUndefined();
        });
    movementReg.method("resetToMinValue",
        [movementClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, movementClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living != nullptr) {
                const auto* inst = living->attributes().getInstance(mc::entity::attribute::Attributes::MOVEMENT_SPEED);
                if (inst != nullptr) {
                    living->attributes().setBaseValue(
                        mc::entity::attribute::Attributes::MOVEMENT_SPEED, inst->attribute().minValue());
                }
            }
            return ctx.createUndefined();
        });

    // --- EquippableComponent类（minecraft:equippable）---
    // opaque 持 mc::Entity*。dynamic_cast<LivingEntity*> 后经 getEquipment/setEquipment（虚派发，Player 重写
    // 走 PlayerInventory）。基岩 EquipmentSlot 字符串值 "Head"/"Chest"/"Legs"/"Feet"/"Mainhand"/"Offhand"/"Body"
    // 映射项目 EquipmentSlot 枚举（注意 Mainhand/Offhand 的 h/a 小写，无 Saddle）。
    // getEquipment 返回 owned 拷贝 ItemStack（规避 setEquipment 改写数组致引用悬垂）。
    // setEquipment 仅支持清空（undefined/null），传 ItemStack 对象因 JS 类无 unwrap 路径抛 TypeError 留 TODO。
    u64 equippableClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* equippableProto = builder.exportClass("EquippableComponent", equippableClassId);
    ScriptClassRegistry::instance().registerClass(equippableClassId, equippableProto, "EquippableComponent");

    ClassRegistrar<void> equippableReg(ctx, equippableClassId, equippableProto);
    equippableReg.readonlyProperty("totalArmor", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 读 attributes().getValue(Attributes::ARMOR)，属性未必对所有 LivingEntity 注册，暂返 0。
        return ctx.createFloat64(0.0);
    });
    equippableReg.readonlyProperty("totalToughness", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // TODO: 读 attributes().getValue(Attributes::ARMOR_TOUGHNESS)，暂返 0。
        return ctx.createFloat64(0.0);
    });
    equippableReg.method(
        "getEquipment",
        [equippableClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, equippableClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createUndefined();
            }
            auto slotStr = ctx.toString(args[0]);
            if (!slotStr) {
                return ctx.createUndefined();
            }
            // 基岩 EquipmentSlot 字符串 → 项目枚举（精确匹配，未知返 undefined）。
            std::optional<mc::EquipmentSlot> slot = std::nullopt;
            const std::string& s = *slotStr;
            if (s == "Head")
                slot = mc::EquipmentSlot::Head;
            else if (s == "Chest")
                slot = mc::EquipmentSlot::Chest;
            else if (s == "Legs")
                slot = mc::EquipmentSlot::Legs;
            else if (s == "Feet")
                slot = mc::EquipmentSlot::Feet;
            else if (s == "Mainhand")
                slot = mc::EquipmentSlot::MainHand;
            else if (s == "Offhand")
                slot = mc::EquipmentSlot::OffHand;
            else if (s == "Body")
                slot = mc::EquipmentSlot::Body;
            if (!slot.has_value()) {
                return ctx.createUndefined();
            }
            const mc::ItemStack& stack = living->getEquipment(*slot);
            if (stack.isEmpty()) {
                return ctx.createUndefined();
            }
            // owned 拷贝：JS GC 时 delete，规避 setEquipment 改写装备数组致 owned=false 引用悬垂。
            const u64 itemStackClassId = ScriptClassRegistry::instance().classIdByName("ItemStack");
            void* itemStackProto = ScriptClassRegistry::instance().proto(itemStackClassId);
            if (itemStackProto == nullptr) {
                return ctx.createUndefined();
            }
            auto* owned = new mc::ItemStack(stack); // 拷贝（含 NBT 深拷贝）
            return ScriptObjectRegistry::wrap(
                ctx, itemStackClassId, itemStackProto, owned, true, "ItemStack", [](void* p) {
                    delete static_cast<mc::ItemStack*>(p);
                });
        },
        1);
    equippableReg.method(
        "setEquipment",
        [equippableClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, equippableClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr || argc < 2 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto slotStr = ctx.toString(args[0]);
            if (!slotStr) {
                return ctx.createBoolean(false);
            }
            std::optional<mc::EquipmentSlot> slot = std::nullopt;
            const std::string& s = *slotStr;
            if (s == "Head")
                slot = mc::EquipmentSlot::Head;
            else if (s == "Chest")
                slot = mc::EquipmentSlot::Chest;
            else if (s == "Legs")
                slot = mc::EquipmentSlot::Legs;
            else if (s == "Feet")
                slot = mc::EquipmentSlot::Feet;
            else if (s == "Mainhand")
                slot = mc::EquipmentSlot::MainHand;
            else if (s == "Offhand")
                slot = mc::EquipmentSlot::OffHand;
            else if (s == "Body")
                slot = mc::EquipmentSlot::Body;
            if (!slot.has_value()) {
                return ctx.createBoolean(false);
            }
            // 仅支持清空槽位（undefined/null）。ItemStack JS 类无 unwrap 路径，传对象抛 TypeError 留 TODO。
            void* itemArg = args[1];
            if (ctx.isUndefined(itemArg) || ctx.getType(itemArg) == ScriptType::Null) {
                living->setEquipment(*slot, mc::ItemStack::EMPTY);
                return ctx.createBoolean(true);
            }
            return ctx.throwTypeError("EquippableComponent.setEquipment: ItemStack argument not yet supported");
        },
        2);

    // --- Player类（继承Entity） ---
    u64 playerClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* playerProto = builder.exportClass("Player", playerClassId);

    ClassRegistrar<void> playerReg(ctx, playerClassId, playerProto);
    playerReg.readonlyProperty("name", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // Player JS 类 opaque 持 mc::Entity*（实际为 Player*）。name 走 Player::username（非 nameTag/customName）。
        auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
        auto* player = dynamic_cast<mc::Player*>(ent);
        if (player == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(player->username());
    });

    // --- Block类 ---
    u64 blockClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* blockProto = builder.exportClass("Block", blockClassId);
    ctx.releaseValue(blockProto);

    // --- ItemStack类 ---
    // opaque 持 mc::ItemStack*。EquippableComponent.getEquipment 以 owned=true 拷贝 wrap（new ItemStack(s)，
    // JS GC 时 delete），规避 setEquipment 改写装备数组致 owned=false 引用悬垂。typeId/amount 为只读快照。
    u64 itemStackClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* itemStackProto = builder.exportClass("ItemStack", itemStackClassId);
    ScriptClassRegistry::instance().registerClass(itemStackClassId, itemStackProto, "ItemStack");

    ClassRegistrar<void> itemStackReg(ctx, itemStackClassId, itemStackProto);
    itemStackReg.readonlyProperty("typeId", [](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
        if (stack == nullptr) {
            return ctx.createUndefined();
        }
        const mc::Item* item = stack->getItem();
        if (item == nullptr) {
            return ctx.createUndefined();
        }
        // Item::toString 返回 itemLocation().toString()，形如 "minecraft:diamond_sword"。
        return ctx.createString(item->toString());
    });
    itemStackReg.property(
        "amount",
        [](IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createInt32(stack->getCount());
        },
        [](IScriptBindingContext& ctx, void* thisVal, void* value) {
            // TODO: ItemStack.amount setter 待 ItemStack JS 类补全 unwrap/构造路径后实现。
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
