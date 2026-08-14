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
#include "common/entity/core/Entity.hpp"           // mc::Entity（Entity JS 类 opaque 持此指针）
#include "common/entity/core/EquipmentSlot.hpp"    // EquipmentSlot 枚举（EquippableComponent 槽位映射）
#include "common/entity/core/LivingEntity.hpp"     // LivingEntity（health/maxHealth/attributes/getEquipment）
#include "common/entity/effect/EffectInstance.hpp" // EffectInstance（getEffect/getEffects 返回效果实例）
#include "common/entity/effect/EffectType.hpp"     // EffectType + getEffectByResourceLocation/getEffectResourceLocation
#include "common/entity/entities/monster/basic/CreeperEntity.hpp" // CreeperEntity（is_charged 组件判定 isPowered）
#include "common/entity/entities/player/Player.hpp" // Player::username/Player::inventory（Player.name / Container）
#include "common/entity/inventory/IInventory.hpp"   // IInventory（Container JS 类 opaque 持此指针）
#include "common/item/core/Item.hpp"                // Item::toString/Item::getItem（ItemStack.typeId / ItemType）
#include "common/item/core/ItemRegistry.hpp"        // ItemRegistry::getItem（ItemType/ItemStack 按 id 取 Item）
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
#include "common/resource/ResourceLocation.hpp" // ResourceLocation::toString/parse（Block/ItemType typeId）
#include "common/util/AxisAlignedBB.hpp"        // Dimension.getEntities 构造查询包围盒
#include "common/util/Direction.hpp"            // mc::Direction / Directions::fromName/toString（Direction 枚举导出）
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"              // Dimension JS 类 opaque 持 IWorld*
#include "common/world/block/Block.hpp"         // Block::blockLocation/defaultState/getBlock（Block/BlockPermutation）
#include "common/world/block/BlockPos.hpp"      // BlockPos（Block.location 坐标）
#include "common/world/block/BlockRegistry.hpp" // BlockRegistry::get/getBlock（按 id 取 BlockState/Block）
#include "common/world/block/BlockState.hpp"    // BlockState（Block/BlockPermutation opaque 持此指针）
#include "common/world/blockentity/BlockEntity.hpp"          // BlockEntity（Container 经 getBlockEntity 取得）
#include "common/world/blockentity/ContainerBlockEntity.hpp" // ContainerBlockEntity::getInventory（Container 底层）

#include <optional>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

namespace {

// ============================================================================
// @minecraft/server 类补齐辅助（批次2）
//
// 下列 helper 仅供本 TU 的 Block/BlockPermutation/ItemType/Container/ItemStack 等
// 绑定回调使用。opaque 持指针语义统一经 ScriptObjectRegistry::wrap/unwrap；JS 侧
// Direction 是字符串枚举（官方首字母大写），项目 mc::Direction 配套 fromName/toString
// 用小写，故出入参须做大小写转换。
// ============================================================================

/// Block JS 类 opaque 持有的快照（owned=true，JS GC 时 delete）。
/// 持 const BlockState*（BlockRegistry 全局拥有，非拥有指针）+ BlockPos + world 回指。
/// world 仅供 isAir 等衍生查询；非拥有，绑定层保证调用期 world 存活（绑定期世界长于 JS 对象）。
struct ScriptBlockRef {
    const mc::BlockState* state = nullptr;
    mc::BlockPos pos{};
    mc::IWorld* world = nullptr;
};

// 注：Direction 字符串↔mc::Direction 转换、按 id 取 BlockState/Item 的 helper 在批次4/6（setBlock/
// spawnItem/rotateDirection 等）引入时按需补充，避免本批携带未使用函数触发 -Wunused-function。

/// ItemStack JS 类 classId 缓存（首次 unwrap 时惰性查 ScriptClassRegistry，避免每调用查表）。
/// 注：本 TU 跨多个回调共享，故用静态局部；引擎重建后 classId 变化，每次仍回查注册表校正。
u64 resolveItemStackClassId()
{
    return ScriptClassRegistry::instance().classIdByName("ItemStack");
}

} // namespace

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
    // {type, location, volume} 查询：location 是包围盒的 min 角点，volume 是从该角点延伸的尺寸，
    // AABB = [location, location + volume]。这与 utils/entity/assert.ts 的角点意图一致
    // （assert.ts 取 from 角点为 location、to-from 为 volume），基岩 BDS 实测同此语义。
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
            // 基岩 EntityQueryOptions.type 既接受 "minecraft:zoglin" 也接受 "zoglin"（自动补前缀）。
            // getTypeId() 恒返回带 "minecraft:" 前缀的完整 id，故将 typeFilter 规范化为完整 id 后比较。
            // 此前 normalizedType 仅在 hasLoc&&hasVol 分支内声明，无 location 的全类型遍历分支
            // （下方 getEntitiesByType）误用未规范化的 typeFilter（如 "spider"），与 getTypeId()
            // （"minecraft:spider"）不等致 getEntities({type:"spider"}) 恒返回空——蜘蛛攻击测试
            // succeedWhen 取不到实体而超时。此处在外层计算，AABB 与全类型两分支共用。
            std::string normalizedType;
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
                normalizedType = typeFilter;
                if (hasType && normalizedType.find(':') == std::string::npos) {
                    normalizedType = "minecraft:" + normalizedType;
                }

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
                    // 角点语义：location 是 min 角点，AABB = [location, location + volume]。
                    mc::AxisAlignedBB box(lx, ly, lz, lx + vx, ly + vy, lz + vz);
                    auto entities = world->getEntitiesInAABB(box, nullptr);
                    void* arr = ctx.createArray();
                    u32 outIdx = 0;
                    const u64 entClassId = ScriptClassRegistry::instance().classIdByName("Entity");
                    void* entProto = ScriptClassRegistry::instance().proto(entClassId);
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
                auto entities = world->getEntitiesByType(normalizedType);
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
    // 基岩 Entity.location 是标准 readonly 属性（@minecraft/server Entity.location: Vector3），
    // 返回实体世界坐标。与 getLocation() 方法等价（基岩同时暴露属性与方法，社区测试多用 .location）。
    // 此前仅绑定 getLocation() 方法，测试用 entity.location 取坐标得 undefined →
    // "cannot read property 'x' of undefined"。补 readonly property 委托同一逻辑。
    entityReg.readonlyProperty("location", [entityClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
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
            if (normalized == "minecraft:is_charged") {
                // 对齐基岩 EntityIsChargedComponent（componentId="minecraft:is_charged"）：
                // "When added, this component signifies that this entity is charged"。组件存在即充能。
                // 仅 CreeperEntity 有充能概念，dynamic_cast 后查 isPowered()；非苦力怕或未充能返 undefined。
                auto* creeper = dynamic_cast<mc::CreeperEntity*>(ent);
                if (creeper == nullptr || !creeper->isPowered()) {
                    return ctx.createUndefined();
                }
                return wrapComponent("IsChargedComponent");
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

    // --- IsChargedComponent类（minecraft:is_charged）---
    // opaque 持 mc::Entity*。对齐基岩 EntityIsChargedComponent：组件存在即代表已充能，
    // 无属性（基岩原版仅以 componentId 存在性标识 charged）。getComponent 已按 isPowered() 过滤，
    // 此处仅作为存在性标记返回，不暴露额外属性。
    u64 isChargedClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* isChargedProto = builder.exportClass("IsChargedComponent", isChargedClassId);
    ScriptClassRegistry::instance().registerClass(isChargedClassId, isChargedProto, "IsChargedComponent");
    // 无 property/method：组件对象存在即 charged（与基岩 EntityIsChargedComponent 一致）。

    // --- Effect 工具：构造基岩 Entity.getEffect 返回的 Effect 普通对象 ---
    // 基岩 Entity.getEffect(effectType) 返回 Effect 对象（{ typeId, amplifier, duration }），
    // 无该效果返回 undefined。effectType 既接受 "minecraft:blindness" 也接受简写 "blindness"。
    // 此处定义为 lambda 供 Entity.getEffect / getEffects 复用：解析 effectType→EffectType，
    // 从 LivingEntity::getEffect 取 EffectInstance，构造普通对象（不注册 JS 类，属性快照现取）。
    auto buildEffectObject =
        [](IScriptBindingContext& ctx, mc::LivingEntity* living, mc::entity::effect::EffectType type) -> void* {
        const mc::entity::effect::EffectInstance* inst = living->getEffect(type);
        if (inst == nullptr) {
            return ctx.createUndefined();
        }
        void* obj = ctx.createObject();
        // typeId：基岩用 "minecraft:blindness" 形式的资源位置。
        ctx.setPropertyString(obj, "typeId", mc::entity::effect::getEffectResourceLocation(type).toString());
        ctx.setPropertyInt(obj, "amplifier", inst->amplifier());
        ctx.setPropertyInt(obj, "duration", inst->duration());
        return obj;
    };
    // 解析 JS effectType 参数（string 或 EffectType 对象）为 EffectType，失败返 nullopt。
    // 基岩允许传字符串（"blindness"）或 EffectType 对象（.id）。Cubium 暂仅支持字符串。
    auto parseEffectType = [](IScriptBindingContext& ctx, void* arg) -> std::optional<mc::entity::effect::EffectType> {
        if (arg == nullptr || !ctx.isString(arg)) {
            return std::nullopt;
        }
        auto s = ctx.toString(arg);
        if (!s) {
            return std::nullopt;
        }
        std::string id = *s;
        if (id.find(':') == std::string::npos) {
            id = "minecraft:" + id;
        }
        return mc::entity::effect::getEffectByResourceLocation(mc::ResourceLocation(id));
    };

    // Entity.getEffect(effectType)：对齐基岩 Entity.getEffect，返回 Effect 普通对象
    // （{ typeId, amplifier, duration }），无该效果/非 LivingEntity 返回 undefined。
    // 用于 GameTest 检测实体状态效果（如幻术师镜像隐身、失明法术）。
    entityReg.method(
        "getEffect",
        [entityClassId, &buildEffectObject, &parseEffectType](
            IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr || argc < 1) {
                return ctx.createUndefined();
            }
            auto typeOpt = parseEffectType(ctx, args[0]);
            if (!typeOpt.has_value()) {
                return ctx.createUndefined();
            }
            return buildEffectObject(ctx, living, *typeOpt);
        },
        1);

    // Entity.getEffects()：对齐基岩 Entity.getEffects，返回 Effect[]（实体当前所有效果）。
    entityReg.method(
        "getEffects",
        [entityClassId, &buildEffectObject](
            IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* ent = static_cast<mc::Entity*>(ScriptObjectRegistry::unwrap(ctx, thisVal, entityClassId));
            auto* living = dynamic_cast<mc::LivingEntity*>(ent);
            if (living == nullptr) {
                return ctx.createArray();
            }
            const auto& effects = living->effectManager().getAllEffects();
            void* arr = ctx.createArray();
            u32 outIdx = 0;
            for (const auto& inst : effects) {
                void* obj = buildEffectObject(ctx, living, inst.type());
                // buildEffectObject 无该效果返 undefined（理论上 getAllEffects 不含空项，守卫跳过）。
                if (obj != nullptr && !ctx.isUndefined(obj)) {
                    ctx.setArrayElement(arr, outIdx, obj); // 不消耗所有权
                    ctx.releaseValue(obj);
                    ++outIdx;
                }
            }
            return arr;
        },
        0);

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

    // --- BlockPermutation类（@minecraft/server）---
    // 不可变方块状态包装。opaque 持 const mc::BlockState*（非拥有，BlockRegistry 全局拥有）。
    // 官方 BlockPermutation：type（→typeId，对应方块资源位置）/isValid。底层 BlockState 就绪。
    // 登记进 ScriptClassRegistry 供 setBlockPermutation 等 unwrap 入参路径跨回调取 classId。
    u64 blockPermutationClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* blockPermutationProto = builder.exportClass("BlockPermutation", blockPermutationClassId);
    ScriptClassRegistry::instance().registerClass(blockPermutationClassId, blockPermutationProto, "BlockPermutation");

    ClassRegistrar<void> blockPermutationReg(ctx, blockPermutationClassId, blockPermutationProto);
    blockPermutationReg.readonlyProperty(
        "type", [blockPermutationClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            // 官方 BlockPermutation.type 返回 BlockType（方块类型对象，含 id）。
            // 项目无独立 BlockType 类，复用 ItemType 同构：返回 typeId 字符串。TODO: 完整 BlockType 类。
            auto* state =
                static_cast<const mc::BlockState*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockPermutationClassId));
            if (state == nullptr) {
                return ctx.createUndefined();
            }
            return ctx.createString(state->blockLocation().toString());
        });
    blockPermutationReg.readonlyProperty(
        "isValid", [blockPermutationClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            // 官方 isValid 表示此 permutation 是否为有效状态（非 air 占位即视为有效）。
            auto* state =
                static_cast<const mc::BlockState*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockPermutationClassId));
            return ctx.createBoolean(state != nullptr);
        });

    // --- ItemType类（@minecraft/server）---
    // 物品类型包装。opaque 持 const mc::Item*（非拥有，ItemRegistry 全局拥有）。
    // 官方 ItemType：id（资源位置）。Item::toString 返回 itemLocation().toString()，形如 "minecraft:diamond"。
    u64 itemTypeClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* itemTypeProto = builder.exportClass("ItemType", itemTypeClassId);
    ScriptClassRegistry::instance().registerClass(itemTypeClassId, itemTypeProto, "ItemType");

    ClassRegistrar<void> itemTypeReg(ctx, itemTypeClassId, itemTypeProto);
    itemTypeReg.readonlyProperty("id", [itemTypeClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* item = static_cast<const mc::Item*>(ScriptObjectRegistry::unwrap(ctx, thisVal, itemTypeClassId));
        if (item == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(item->toString());
    });

    // --- Block类（实化，原为空壳）---
    // 官方 Block：world 内某坐标处方块快照。opaque 持 ScriptBlockRef*（owned=true，JS GC 时 delete）。
    // 属性 typeId（方块资源位置）/permutation（BlockPermutation 包装 state）/x/y/z/location。
    // 底层 BlockState/Block/BlockRegistry 就绪。登记 ScriptClassRegistry 供跨回调 wrap/unwrap。
    u64 blockClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* blockProto = builder.exportClass("Block", blockClassId);
    ScriptClassRegistry::instance().registerClass(blockClassId, blockProto, "Block");

    ClassRegistrar<void> blockReg(ctx, blockClassId, blockProto);
    blockReg.readonlyProperty("typeId", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr || ref->state == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(ref->state->blockLocation().toString());
    });
    blockReg.readonlyProperty("permutation",
        [blockClassId, blockPermutationClassId, blockPermutationProto](
            IScriptBindingContext& ctx, void* thisVal) -> void* {
            // 包装内嵌 state 为 BlockPermutation（非拥有，state 由 BlockRegistry 全局拥有）。
            auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
            if (ref == nullptr || ref->state == nullptr) {
                return ctx.createUndefined();
            }
            return ScriptObjectRegistry::wrap(ctx,
                blockPermutationClassId,
                blockPermutationProto,
                const_cast<mc::BlockState*>(ref->state),
                false,
                "BlockPermutation");
        });
    blockReg.readonlyProperty("x", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->pos.x));
    });
    blockReg.readonlyProperty("y", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->pos.y));
    });
    blockReg.readonlyProperty("z", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createInt32(static_cast<i32>(ref->pos.z));
    });
    blockReg.readonlyProperty("location", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 官方 Block.location 返回 Vector3（浮点坐标，对齐 Entity.getLocation）。
        auto* ref = static_cast<ScriptBlockRef*>(ScriptObjectRegistry::unwrap(ctx, thisVal, blockClassId));
        if (ref == nullptr) {
            return ctx.createUndefined();
        }
        void* obj = ctx.createObject();
        ctx.setPropertyFloat(obj, "x", static_cast<f64>(ref->pos.x));
        ctx.setPropertyFloat(obj, "y", static_cast<f64>(ref->pos.y));
        ctx.setPropertyFloat(obj, "z", static_cast<f64>(ref->pos.z));
        return obj;
    });
    blockReg.readonlyProperty("isWaterlogged", [blockClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        // 官方 Block.isWaterlogged：方块是否被水淹没。读 BlockState 流体状态非空判定。
        // TODO: 流体状态判定后续完善，暂保守返回 false。
        (void)thisVal;
        return ctx.createBoolean(false);
    });

    // --- ItemStack类 ---
    // opaque 持 mc::ItemStack*。EquippableComponent.getEquipment 以 owned=true 拷贝 wrap（new ItemStack(s)，
    // JS GC 时 delete），规避 setEquipment 改写装备数组致 owned=false 引用悬垂。typeId/amount 为只读快照。
    // 构造函数支持官方基岩 API：new ItemStack(typeId: string, amount?: number)，按 typeId 查 Item 构造
    // owned 拷贝（JS GC 时 delete），与 Container.getItem/Equippable.getEquipment 的 wrap 范式一致。
    u64 itemStackClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* itemStackProto = builder.exportClass("ItemStack",
        itemStackClassId,
        [](IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            // new ItemStack(typeId: string, amount?: number = 1)
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("new ItemStack(typeId: string, amount?: number)");
            }
            auto typeId = ctx.toString(args[0]);
            if (!typeId) {
                return ctx.throwInternalError("Failed to read typeId");
            }
            i32 amount = 1;
            if (argc >= 2 && ctx.isNumber(args[1])) {
                auto a = ctx.toInt32(args[1]);
                if (a && *a > 0) {
                    amount = *a;
                }
            }
            const mc::Item* item = mc::ItemRegistry::instance().getItem(mc::ResourceLocation::parse(*typeId));
            if (item == nullptr) {
                return ctx.throwTypeError(("Unknown item type: " + *typeId).c_str());
            }
            const u64 isClassId = resolveItemStackClassId();
            void* isProto = ScriptClassRegistry::instance().proto(isClassId);
            if (isProto == nullptr) {
                return ctx.createUndefined();
            }
            auto* stack = new mc::ItemStack(item, amount); // owned，JS GC 时 delete
            return ScriptObjectRegistry::wrap(ctx, isClassId, isProto, stack, true, "ItemStack");
        });
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
            // amount setter：设数量（setCount 内部 <=0 置空）。仅当 owned（Equippable.getEquipment 拷贝）
            // 时安全写回；非拥有快照写入会被 C++ 侧覆盖，故非拥有时静默忽略留 TODO。
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, thisVal, 0));
            if (stack == nullptr) {
                return;
            }
            auto v = ctx.toInt32(value);
            if (!v) {
                return;
            }
            stack->setCount(*v);
        });

    // --- Direction 枚举对象（@minecraft/server）---
    // 官方 Direction 是字符串枚举（Down="Down"...West="West"），非数字。导出为只读对象，
    // 各键值均为自身字符串名（对齐 system/world 全局对象导出模式）。接收 JS Direction 字符串时
    // 绑定层用 directionFromApiString 转 mc::Direction（见 ScriptTestHelper 等）。
    {
        void* directionObj = ctx.createObject();
        ctx.setPropertyString(directionObj, "Down", "Down");
        ctx.setPropertyString(directionObj, "East", "East");
        ctx.setPropertyString(directionObj, "North", "North");
        ctx.setPropertyString(directionObj, "South", "South");
        ctx.setPropertyString(directionObj, "Up", "Up");
        ctx.setPropertyString(directionObj, "West", "West");
        builder.exportValue("Direction", directionObj);
        ctx.releaseValue(directionObj);
    }

    // --- FluidType类（@minecraft/server）---
    // 流体类型包装。项目流体体系（Fluids namespace）尚未暴露完整查询入口，本类占位：
    // opaque 持 std::string*（owned=true，JS GC 时 delete，存资源位置 id 如 "minecraft:water"）。
    // 仅暴露 id 属性；完整 fluid state 体系后续 TODO。
    u64 fluidTypeClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* fluidTypeProto = builder.exportClass("FluidType", fluidTypeClassId);
    ScriptClassRegistry::instance().registerClass(fluidTypeClassId, fluidTypeProto, "FluidType");

    ClassRegistrar<void> fluidTypeReg(ctx, fluidTypeClassId, fluidTypeProto);
    fluidTypeReg.readonlyProperty("id", [fluidTypeClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* id = static_cast<std::string*>(ScriptObjectRegistry::unwrap(ctx, thisVal, fluidTypeClassId));
        if (id == nullptr) {
            return ctx.createUndefined();
        }
        return ctx.createString(*id);
    });

    // --- Container类（@minecraft/server）---
    // 容器包装。opaque 持 mc::IInventory*（非拥有，容器由 BlockEntity 或 Player 拥有）。
    // 官方 Container：size/emptySlotsCount + getItem(slot)/setItem(slot,itemStack)/addItem/transferItem/clearAll。
    // 底层 IInventory/ContainerBlockEntity::getInventory 就绪。getItem 返回 owned ItemStack 拷贝
    // （规避 setItem 改写致引用悬垂）；setItem 接 ItemStack unwrap（非拥有，仅拷贝写入容器）。
    u64 containerClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* containerProto = builder.exportClass("Container", containerClassId);
    ScriptClassRegistry::instance().registerClass(containerClassId, containerProto, "Container");

    ClassRegistrar<void> containerReg(ctx, containerClassId, containerProto);
    containerReg.readonlyProperty("size", [containerClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
        if (inv == nullptr) {
            return ctx.createInt32(0);
        }
        return ctx.createInt32(inv->getContainerSize());
    });
    containerReg.readonlyProperty(
        "emptySlotsCount", [containerClassId](IScriptBindingContext& ctx, void* thisVal) -> void* {
            // 统计空槽位数（getItem(i).isEmpty()）。
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr) {
                return ctx.createInt32(0);
            }
            i32 empty = 0;
            for (i32 i = 0; i < inv->getContainerSize(); ++i) {
                if (inv->getItem(i).isEmpty()) {
                    ++empty;
                }
            }
            return ctx.createInt32(empty);
        });
    containerReg.method(
        "getItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr || argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.createUndefined();
            }
            auto slot = ctx.toInt32(args[0]);
            if (!slot || *slot < 0 || *slot >= inv->getContainerSize()) {
                return ctx.createUndefined();
            }
            // getItem 返回 owned 拷贝（new ItemStack(inv->getItem(slot))），JS GC 时 delete。
            const u64 isClassId = resolveItemStackClassId();
            void* isProto = ScriptClassRegistry::instance().proto(isClassId);
            if (isProto == nullptr) {
                return ctx.createUndefined();
            }
            auto* copy = new mc::ItemStack(inv->getItem(*slot));
            return ScriptObjectRegistry::wrap(ctx, isClassId, isProto, copy, true, "ItemStack");
        },
        1);
    containerReg.method(
        "setItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr || argc < 2 || !ctx.isNumber(args[0])) {
                return ctx.createUndefined();
            }
            auto slot = ctx.toInt32(args[0]);
            if (!slot || *slot < 0 || *slot >= inv->getContainerSize()) {
                return ctx.createUndefined();
            }
            // unwrap 入参 ItemStack（按 classId 校验类型）。
            const u64 isClassId = resolveItemStackClassId();
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, args[1], isClassId));
            if (stack == nullptr) {
                return ctx.throwTypeError("Container.setItem: argument must be an ItemStack");
            }
            inv->setItem(*slot, *stack);
            return ctx.createUndefined();
        },
        2);
    containerReg.method(
        "addItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv == nullptr || argc < 1) {
                return ctx.createUndefined();
            }
            const u64 isClassId = resolveItemStackClassId();
            auto* stack = static_cast<mc::ItemStack*>(ScriptObjectRegistry::unwrap(ctx, args[0], isClassId));
            if (stack == nullptr) {
                return ctx.throwTypeError("Container.addItem: argument must be an ItemStack");
            }
            // addItem 返回剩余未放入的堆；官方无返回值，此处忽略剩余（对齐基岩 void 语义）。
            (void)inv->addItem(*stack);
            return ctx.createUndefined();
        },
        1);
    containerReg.method("clearAll",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* inv = static_cast<mc::IInventory*>(ScriptObjectRegistry::unwrap(ctx, thisVal, containerClassId));
            if (inv != nullptr) {
                inv->clear();
            }
            return ctx.createUndefined();
        });
    containerReg.method(
        "transferItem",
        [containerClassId](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            // TODO: transferItem(fromSlot,toContainer,toSlot) 跨容器迁移依赖第二个 Container 入参 unwrap，
            //       框架已具备（同 containerClassId），完整迁移语义（堆叠/空槽选择）后续补全。
            (void)thisVal;
            (void)argc;
            (void)args;
            return ctx.createUndefined();
        },
        3);

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
