/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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
 */

#include "server/test/script/binding/ScriptSimulatedPlayer.hpp"

#include "common/entity/core/Entity.hpp" // Entity::attemptTeleport/changeDimension/dimension（teleport/tryTeleport）
#include "common/entity/core/LivingEntity.hpp"     // LivingEntity::getEffect（getEffect 绑定读实体效果）
#include "common/entity/effect/EffectInstance.hpp" // EffectInstance（getEffect 返回效果实例）
#include "common/entity/effect/EffectType.hpp"     // EffectType + getEffectByResourceLocation/getEffectResourceLocation
#include "common/item/core/ItemStack.hpp"          // giveItem/setItem 按值拷贝需完整类型
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"  // ScriptObjectRegistry/ClassRegistrar
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 unwrap Entity/ItemStack/Dimension
#include "common/resource/ResourceLocation.hpp"                     // ResourceLocation（getEffect typeId 资源位置）
#include "common/test/base/error/GameTestErrorType.hpp"             // GameTestErrorType::MethodNotImplemented
#include "common/util/Direction.hpp"    // Directions::fromName / mc::Direction（useItemOnBlock direction 参数）
#include "common/util/math/Vector3.hpp" // Vector3（faceLocation 参数）
#include "common/world/GlobalPos.hpp"   // GlobalPos（getSpawnPoint 返 optional<GlobalPos>）
#include "common/world/IWorld.hpp"      // IWorld::dimension()（options.dimension 跨维度判定)
#include "common/world/block/BlockPos.hpp"
#include "common/world/dimension/DimensionManager.hpp"        // DimensionId（teleport 目标维度类型）
#include "server/test/script/binding/ScriptGameTestError.hpp" // throwGameTestError（stub 用）
#include "server/test/script/context/ScriptBindingRegistry.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp"

#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {

bool _parseBlockPos(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* arg, BlockPos& out)
{
    if (!ctx.isObject(arg)) {
        static_cast<void>(ctx.throwTypeError("Block position argument must be {x,y,z} object"));
        return false;
    }
    auto x = ctx.getPropertyInt(arg, "x");
    auto y = ctx.getPropertyInt(arg, "y");
    auto z = ctx.getPropertyInt(arg, "z");
    if (!x || !y || !z) {
        static_cast<void>(ctx.throwTypeError("Block position must have numeric x,y,z"));
        return false;
    }
    out = BlockPos(static_cast<i32>(*x), static_cast<i32>(*y), static_cast<i32>(*z));
    return true;
}

// 从 JS Entity 对象 unwrap 出 mc::Entity*（@minecraft/server 注册，opaque 持 mc::Entity*）。
mc::Entity* _unwrapEntity(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* val)
{
    const u64 classId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
    return static_cast<mc::Entity*>(mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, val, classId));
}

// 从 JS ItemStack 对象 unwrap 出 mc::ItemStack*（@minecraft/server 注册，opaque 持 mc::ItemStack*）。
mc::ItemStack* _unwrapItemStack(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* val)
{
    const u64 classId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("ItemStack");
    return static_cast<mc::ItemStack*>(mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, val, classId));
}

// 统一 stub：抛对齐基岩 GameTestErrorType::MethodNotImplemented 的 GameTestError JS 实例。
// SimulatedPlayer 方法不走 GameTestResult 通道（非 Test 类断言），故绑定层直接 throwGameTestError。
void* _throwNotImplemented(mc::mod::bedrock::addon::IScriptBindingContext& ctx, std::string_view method)
{
    std::string msg = "SimulatedPlayer.";
    msg += method;
    msg += " not implemented yet (dependency system not ready)";
    return throwGameTestError(ctx, GameTestErrorType::MethodNotImplemented, msg);
}

// 把 JS direction 参数转 mc::Direction。基岩 useItemOnBlock/useItemInSlotOnBlock 的 direction 参数
// 官方文档默认值 = 1（数字，对应 Up），但 @minecraft/server.Direction 枚举值为 PascalCase 字符串
// （"Down"/"Up"/"North"/"South"/"West"/"East"）。故同时接受：
//   - 数字（0-5，与 mc::Direction 枚举序一致：Down=0..East=5）直接转；
//   - 字符串（PascalCase 或小写，经 Directions::fromName 转）。
// null/undefined 或解析失败返回默认 Up（对齐官方默认 1=Up）。
mc::Direction _directionFromApi(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* val)
{
    if (val == nullptr || ctx.isUndefined(val)) {
        return mc::Direction::Up; // 官方默认 direction=1（Up）
    }
    if (ctx.isNumber(val)) {
        auto n = ctx.toInt32(val);
        if (n && *n >= 0 && *n <= 5) {
            return static_cast<mc::Direction>(*n);
        }
        return mc::Direction::Up;
    }
    if (ctx.isString(val)) {
        auto s = ctx.toString(val);
        if (s) {
            std::string lower = *s;
            for (char& c : lower) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            auto dir = mc::Directions::fromName(lower);
            return dir.value_or(mc::Direction::Up);
        }
    }
    return mc::Direction::Up;
}

// 把 JS faceLocation 参数（Vector3 {x,y,z}，方块内 0-1 相对坐标）转 mc::Vector3。
// 官方默认 null → 方块中心 (0.5, 0.5, 0.5)。
mc::Vector3 _parseFaceLocation(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* val)
{
    if (val == nullptr || ctx.isUndefined(val) || !ctx.isObject(val)) {
        return mc::Vector3(0.5f, 0.5f, 0.5f);
    }
    auto x = ctx.getPropertyFloat(val, "x");
    auto y = ctx.getPropertyFloat(val, "y");
    auto z = ctx.getPropertyFloat(val, "z");
    return mc::Vector3(x.value_or(0.5f), y.value_or(0.5f), z.value_or(0.5f));
}

// teleport/tryTeleport 共享实现。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型），
// 故需在此重复绑定 teleport/tryTeleport（对齐 @minecraft/server Entity.teleport/tryTeleport）。
// 逻辑与 MinecraftModuleFactory.cpp 的 Entity.teleport/tryTeleport 回调一致：
//   - 解析 location {x,y,z}（必需）
//   - 解析 options.dimension（可选）：若指定且与实体当前维度不同则跨维度走 changeDimension（虚派发，
//     ServerPlayer override 调真实实现），否则同维度走 attemptTeleport（带碰撞检测）
//   - returnBoolean=false（teleport）返回 undefined；true（tryTeleport）返回 boolean
// 注：跨维度时 location 被忽略（changeDimension 不接受位置，由 Teleporter 算）。
void* _applyTeleport(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, mc::Entity* ent, i32 argc, void** args, bool returnBoolean)
{
    if (ent == nullptr || argc < 1 || !ctx.isObject(args[0])) {
        return returnBoolean ? ctx.createBoolean(false) : ctx.createUndefined();
    }
    void* locObj = args[0];
    auto xOpt = ctx.getPropertyFloat(locObj, "x");
    auto yOpt = ctx.getPropertyFloat(locObj, "y");
    auto zOpt = ctx.getPropertyFloat(locObj, "z");
    if (!xOpt || !yOpt || !zOpt) {
        if (returnBoolean) {
            return ctx.createBoolean(false);
        }
        return ctx.throwTypeError("teleport location requires {x,y,z}");
    }
    f64 x = *xOpt, y = *yOpt, z = *zOpt;

    // 解析 options.dimension（可选）：若指定且与当前维度不同则跨维度传送。
    // 解析 options.checkForBlocks（可选）：returnBoolean=false（teleport）默认 false 强制传送；
    // returnBoolean=true（tryTeleport）默认 true 检查碰撞。对齐基岩 TeleportOptions.checkForBlocks
    // 与 Entity.teleport/tryTeleport 默认语义（见 MinecraftModuleFactory.cpp Entity 绑定注释）。
    mc::DimensionId targetDim = ent->dimension();
    bool crossDim = false;
    bool checkForBlocks = returnBoolean; // teleport 默认 false，tryTeleport 默认 true
    if (argc >= 2 && ctx.isObject(args[1])) {
        void* opts = args[1];
        void* dimVal = ctx.getProperty(opts, "dimension");
        if (dimVal != nullptr && ctx.isObject(dimVal)) {
            // Dimension JS 对象 opaque 持 IWorld*，unwrap 后读 IWorld::dimension()
            const u64 dimClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Dimension");
            auto* dimWorld = static_cast<mc::IWorld*>(
                mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, dimVal, dimClassId));
            if (dimWorld != nullptr) {
                targetDim = dimWorld->dimension();
                crossDim = (targetDim != ent->dimension());
            }
        }
        ctx.releaseValue(dimVal);
        auto checkOpt = ctx.getPropertyBool(opts, "checkForBlocks");
        if (checkOpt.has_value()) {
            checkForBlocks = *checkOpt;
        }
    }

    if (crossDim) {
        // 跨维度：经虚派发 changeDimension（ServerPlayer override 调真实实现）。
        bool ok = ent->changeDimension(targetDim);
        return returnBoolean ? ctx.createBoolean(ok) : ctx.createUndefined();
    }
    if (checkForBlocks) {
        // 同维度 + 检查碰撞：attemptTeleport（findSafeTeleportPosition + 碰撞检测）。
        bool ok = ent->attemptTeleport(x, y, z, false);
        return returnBoolean ? ctx.createBoolean(ok) : ctx.createUndefined();
    }
    // 同维度 + 强制传送：直接 setPosition，跳过碰撞检测（对齐基岩 checkForBlocks=false）。
    ent->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    return returnBoolean ? ctx.createBoolean(true) : ctx.createUndefined();
}

/**
 * @brief GameMode 枚举转官方 ScriptAPI 字符串。
 *
 * 对齐基岩 @minecraft/server GameMode 枚举值（"survival"/"creative"/"adventure"/"spectator"），
 * 供 getGameMode() 返回。NotSet 映射为 "default"（官方 GameMode.default 用于恢复默认）。
 */
[[nodiscard]] const char* _gameModeToString(mc::GameMode mode)
{
    switch (mode) {
        case mc::GameMode::Survival:
            return "survival";
        case mc::GameMode::Creative:
            return "creative";
        case mc::GameMode::Adventure:
            return "adventure";
        case mc::GameMode::Spectator:
            return "spectator";
        case mc::GameMode::NotSet:
            return "default";
    }
    return "default";
}

/**
 * @brief 解析 JS effectType 参数为 EffectType。
 *
 * 对齐基岩 Entity.getEffect：接受 "minecraft:blindness" 全称或 "blindness" 简写。失败返 nullopt。
 * 复用 MinecraftModuleFactory::parseEffectType 同款逻辑（SimulatedPlayer JS 类未继承 Entity 原型，
 * 见 [[simulated-player-js-class-no-entity-inheritance]]，Entity.getEffect 不在其上，故本地重绑）。
 */
[[nodiscard]] std::optional<mc::entity::effect::EffectType> _parseEffectType(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* arg)
{
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
}

} // namespace

u64 registerSimulatedPlayerClassBinding(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx)
{
    u64 classId = ScriptObjectRegistry::allocateClassId(ctx);
    void* proto = builder.exportClass("SimulatedPlayer", classId);
    ScriptBindingRegistry::instance().registerProto(classId, proto);
    // 登记进 ScriptClassRegistry，供 test.kill 等需识别 SimulatedPlayer 形参的绑定经
    // classIdByName("SimulatedPlayer") unwrap。SimulatedPlayer JS 类独立注册不继承 Entity 原型
    // （见 [[simulated-player-js-class-no-entity-inheritance]]），opaque class_id 是本 classId 而非
    // Entity classId，test.kill 传 Entity classId 严格匹配会返 nullptr（致 "kill: argument must be
    // an Entity"）。此处登记使 test.kill 枚举 classId 时能解开 SimulatedPlayer。
    mc::mod::bedrock::addon::ScriptClassRegistry::instance().registerClass(classId, proto, "SimulatedPlayer");

    ClassRegistrar<void> reg(ctx, classId, proto);

    // --- name (readonly property) ---
    reg.readonlyProperty("name", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
        if (player == nullptr) {
            return ctx.throwTypeError("Invalid SimulatedPlayer");
        }
        // Player::username() 返构造时传入的名字（存 m_username）。
        return ctx.createString(player->username());
    });

    // --- getSpawnPoint (readonly property) ---
    // 返回玩家重生点 {x,y,z,dimensionId}（Player::getSpawnPoint 返 optional<GlobalPos>）。供 /spawnpoint
    // 命令测试读取重生点做断言。无重生点（nullopt）返回 undefined。SimulatedPlayer JS 类独立注册未继承
    // Player/Entity 原型（见 [[simulated-player-js-class-no-entity-inheritance]]），Player.getSpawnPoint
    // 不在其上，故需在此重绑（与 name/dimension 同款）。Cubium 扩展属性（官方基岩 API 无）。
    reg.readonlyProperty(
        "getSpawnPoint", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.createUndefined();
            }
            auto spawnOpt = player->getSpawnPoint();
            if (!spawnOpt.has_value()) {
                return ctx.createUndefined(); // 无重生点
            }
            const GlobalPos& gp = spawnOpt.value();
            void* obj = ctx.createObject();
            ctx.setPropertyInt(obj, "x", static_cast<i32>(gp.x()));
            ctx.setPropertyInt(obj, "y", static_cast<i32>(gp.y()));
            ctx.setPropertyInt(obj, "z", static_cast<i32>(gp.z()));
            ctx.setPropertyInt(obj, "dimensionId", static_cast<i32>(gp.getDimensionId()));
            return obj;
        });

    // --- dimension: Dimension（readonly property）---
    // 对齐基岩 @minecraft/server Entity.dimension。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型，
    // 见 [[simulated-player-js-class-no-entity-inheritance]]），Entity.dimension 不在其上，故需在此重绑。
    // 返回玩家所在维度的 Dimension JS 对象（opaque 持 IWorld*）。供命令测试读世界状态
    // （TimeCommand/WeatherCommand/GameRuleCommand 经 dimension.getTimeOfDay/isRaining/getGameRule 断言）。
    reg.readonlyProperty("dimension", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
        if (player == nullptr) {
            return ctx.createUndefined();
        }
        auto* world = player->world();
        if (world == nullptr) {
            return ctx.createUndefined();
        }
        const u64 dimClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Dimension");
        void* dimProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(dimClassId);
        if (dimProto == nullptr) {
            return ctx.createUndefined();
        }
        return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
            ctx, dimClassId, dimProto, world, false, "Dimension");
    });
    // getDimension() 方法与 dimension 属性等价（对齐基岩同时暴露属性与方法，与 Entity 类一致）。
    reg.method(
        "getDimension",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.createUndefined();
            }
            auto* world = player->world();
            if (world == nullptr) {
                return ctx.createUndefined();
            }
            const u64 dimClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Dimension");
            void* dimProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(dimClassId);
            if (dimProto == nullptr) {
                return ctx.createUndefined();
            }
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, dimClassId, dimProto, world, false, "Dimension");
        },
        0);

    // --- getGameMode(): GameMode ---
    // 对齐基岩 @minecraft/server Player.getGameMode（beta）。返回当前游戏模式字符串
    // （"survival"/"creative"/"adventure"/"spectator"）。供命令测试（/gamemode）判定模式切换生效。
    // 读 Player::m_gameMode 实体字段（/gamemode 经 GameModeCommand 实体旁路写入），非 ServerPlayerData。
    reg.method(
        "getGameMode",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            return ctx.createString(_gameModeToString(player->gameMode()));
        },
        0);

    // --- getComponent(componentId: string): Component | undefined ---
    // 对齐基岩 @minecraft/server Entity.getComponent。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型，
    // 见 [[simulated-player-js-class-no-entity-inheritance]]），Entity.getComponent 不在其上，故需在此重绑。
    // 派发通用组件（equippable/health/movement/rideable/onfire），与 MinecraftModuleFactory
    // Entity.getComponent 一致；mob 专属组件（is_charged/mark_variant 等）对 SimulatedPlayer 无意义，
    // 不派发（返 undefined，符合"组件不存在"语义）。供命令测试读玩家装备/属性（如 /enchant 后读主手附魔）。
    reg.method(
        "getComponent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createUndefined();
            }
            // 实体有效性守卫：对齐 Entity.getComponent 的 isRemoved 检查（见 MinecraftModuleFactory.cpp
            // 同款注释）。SimulatedPlayer 测试中实体销毁后句柄悬垂，graveyard 窗口内 isRemoved()=true
            // 提前返回 undefined 避免 UAF。
            if (player->isRemoved()) {
                return ctx.createUndefined();
            }
            auto compId = ctx.toString(args[0]);
            if (!compId) {
                return ctx.createUndefined();
            }
            std::string normalized = *compId;
            if (normalized.find(':') == std::string::npos) {
                normalized = "minecraft:" + normalized;
            }

            mc::Entity* ent = player; // SimulatedPlayer 经 ServerPlayer→Player→LivingEntity→Entity
            // 按类名 wrap 组件 JS 对象（opaque 持 ent，owned=false），与 Entity.getComponent 范式一致。
            auto wrapComponent = [&ctx, ent](const char* className) -> void* {
                const u64 classId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName(className);
                void* proto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(classId);
                if (proto == nullptr) {
                    return ctx.createUndefined();
                }
                return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                    ctx, classId, proto, ent, false, className, nullptr, ent->id());
            };

            if (normalized == "minecraft:rideable") {
                return wrapComponent("RideableComponent");
            }
            if (normalized == "minecraft:health" || normalized == "minecraft:movement" ||
                normalized == "minecraft:equippable") {
                // health/movement/equippable 仅 LivingEntity attach，SimulatedPlayer 是 LivingEntity 子类，
                // dynamic_cast 恒成功。对齐 Entity.getComponent 守卫。
                if (dynamic_cast<mc::LivingEntity*>(ent) == nullptr) {
                    return ctx.createUndefined();
                }
                if (normalized == "minecraft:health") {
                    return wrapComponent("HealthComponent");
                }
                if (normalized == "minecraft:movement") {
                    return wrapComponent("MovementComponent");
                }
                return wrapComponent("EquippableComponent");
            }
            if (normalized == "minecraft:onfire") {
                if (!ent->isOnFire()) {
                    return ctx.createUndefined();
                }
                return wrapComponent("OnFireComponent");
            }
            if (normalized == "minecraft:inventory") {
                // 对齐基岩 EntityInventoryComponent：仅 Player 持有背包组件。SimulatedPlayer 经
                // ServerPlayer→Player，dynamic_cast<Player*> 恒成功。组件对象的 container 只读属性返回
                // 包装 Player::inventory()（PlayerInventory : IInventory）的 Container（见
                // MinecraftModuleFactory EntityInventoryComponent 注册）。供集成测试读玩家背包物品
                // （如喂食后断言主手物品消耗）。
                if (dynamic_cast<mc::Player*>(ent) == nullptr) {
                    return ctx.createUndefined();
                }
                return wrapComponent("EntityInventoryComponent");
            }
            // TODO: 其他基岩合法 componentId（is_baby/is_tamed 等）按需补全。
            return ctx.createUndefined();
        },
        1);

    // --- getTags()/hasTag()/addTag()/removeTag() ---
    // 对齐基岩 @minecraft/server Entity 标签 API。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型，
    // 见 [[simulated-player-js-class-no-entity-inheritance]]），Entity.getTags 等不在其上，故需在此重绑。
    // 标签存储在 Entity 基类 m_tags（std::set<string>），/tag 命令直接操作之。供 TagCommand 端到端
    // 测试断言（/tag @s add foo 后 hasTag("foo")=true）。与 MinecraftModuleFactory Entity.getTags 语义一致。
    reg.method(
        "getTags",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.createArray();
            }
            const auto& tags = player->getTags();
            void* arr = ctx.createArray();
            u32 outIdx = 0;
            for (const auto& tag : tags) {
                void* str = ctx.createString(tag);
                ctx.setArrayElement(arr, outIdx, str);
                ctx.releaseValue(str);
                ++outIdx;
            }
            return arr;
        },
        0);
    reg.method(
        "hasTag",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto tag = ctx.toString(args[0]);
            return ctx.createBoolean(tag ? player->hasTag(*tag) : false);
        },
        1);
    reg.method(
        "addTag",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto tag = ctx.toString(args[0]);
            return ctx.createBoolean(tag ? player->addTag(*tag) : false);
        },
        1);
    reg.method(
        "removeTag",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr || argc < 1 || !ctx.isString(args[0])) {
                return ctx.createBoolean(false);
            }
            auto tag = ctx.toString(args[0]);
            return ctx.createBoolean(tag ? player->removeTag(*tag) : false);
        },
        1);

    // --- addEffect(effectType: string, duration: number, options?: EntityEffectOptions): void ---
    // 对齐基岩 @minecraft/server Entity.addEffect。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型，
    // 见 [[simulated-player-js-class-no-entity-inheritance]]），Entity.addEffect 不在其上，故需在此重绑。
    // 供牛奶桶饮用清除效果类测试：先 addEffect 给玩家上毒，饮用牛奶后 getEffect 验证效果被 removeAllEffects
    // 清除。与 MinecraftModuleFactory Entity.addEffect 语义一致（duration tick，options.amplifier/showParticles）。
    reg.method(
        "addEffect",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 2) {
                return ctx.createUndefined();
            }
            auto typeOpt = _parseEffectType(ctx, args[0]);
            if (!typeOpt.has_value()) {
                return ctx.createUndefined();
            }
            auto durationOpt = ctx.toInt32(args[1]);
            if (!durationOpt.has_value()) {
                return ctx.createUndefined();
            }
            i32 duration = *durationOpt;
            i32 amplifier = 0;
            bool showParticles = true; // 对齐 EffectInstance 默认 visible=true
            if (argc >= 3 && ctx.isObject(args[2])) {
                void* opts = args[2];
                auto ampOpt = ctx.getPropertyInt(opts, "amplifier");
                if (ampOpt.has_value()) {
                    amplifier = *ampOpt;
                }
                auto partOpt = ctx.getPropertyBool(opts, "showParticles");
                if (partOpt.has_value()) {
                    showParticles = *partOpt;
                }
            }
            // EffectInstance(type, duration, amplifier, ambient=false, visible=showParticles)。
            // 对齐 MinecraftModuleFactory Entity.addEffect 与 EffectCommand.cpp:187 用法。
            mc::entity::effect::EffectInstance effect(*typeOpt, duration, amplifier, false, showParticles);
            player->addEffect(std::move(effect));
            return ctx.createUndefined();
        },
        3);

    // --- getEffect(effectType: string): Effect | undefined ---
    // 对齐基岩 @minecraft/server Entity.getEffect。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型，
    // 见 [[simulated-player-js-class-no-entity-inheritance]]），Entity.getEffect 不在其上，故需在此重绑。
    // 供命令测试（/effect give/clear）判定状态效果生效：返回 { typeId, amplifier, duration } 普通对象，
    // 无该效果返回 undefined。读 LivingEntity::effectManager（实体层，/effect 经实体旁路写入处），
    // 与 MinecraftModuleFactory Entity.getEffect 返回结构一致。
    reg.method(
        "getEffect",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.createUndefined();
            }
            auto typeOpt = _parseEffectType(ctx, args[0]);
            if (!typeOpt.has_value()) {
                return ctx.createUndefined();
            }
            const mc::entity::effect::EffectInstance* inst = player->getEffect(*typeOpt);
            if (inst == nullptr) {
                return ctx.createUndefined();
            }
            void* obj = ctx.createObject();
            ctx.setPropertyString(obj, "typeId", mc::entity::effect::getEffectResourceLocation(*typeOpt).toString());
            ctx.setPropertyInt(obj, "amplifier", inst->amplifier());
            ctx.setPropertyInt(obj, "duration", inst->duration());
            return obj;
        },
        1);

    // --- level (readonly property, number) ---
    // 对齐基岩 @minecraft/server Player.level（只读，当前经验等级）。供命令测试（/xp add/set levels）
    // 判定等级变化。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型，
    // 见 [[simulated-player-js-class-no-entity-inheritance]]），Entity 侧无 level 绑定，故需在此重绑。
    // 读 Player::experienceLevel（实体层，/xp 经 ExperienceCommand 实体旁路写入处），非 ServerPlayerData。
    reg.readonlyProperty("level", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
        if (player == nullptr) {
            return ctx.throwTypeError("Invalid SimulatedPlayer");
        }
        return ctx.createInt32(player->experienceLevel());
    });

    // --- getTotalXp(): number ---
    // 对齐基岩 @minecraft/server Player.getTotalXp（总经验点数）。供命令测试（/xp add/set points）判定
    // 总经验点数变化。读 Player::totalExperience（实体层）。
    reg.method(
        "getTotalXp",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            return ctx.createInt32(player->totalExperience());
        },
        0);

    // --- xp (readonly property, number 0..1) ---
    // 对齐基岩 @minecraft/server Player.xp（只读，当前等级经验条进度 0.0-1.0）。供命令测试判定经验条进度。
    // 读 Player::experienceProgress（实体层）。注意：基岩属性名是 "xp" 非 "progress"。
    reg.readonlyProperty("xp", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
        auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
        if (player == nullptr) {
            return ctx.throwTypeError("Invalid SimulatedPlayer");
        }
        return ctx.createFloat64(player->experienceProgress());
    });

    // --- teleport(location: Vector3, teleportOptions?: TeleportOptions): void ---
    // 对齐基岩 Entity.teleport。SimulatedPlayer JS 类独立注册（未继承 Entity 类原型），
    // 故需在此绑定（逻辑见 _applyTeleport，与 Entity.teleport 一致）。跨维度走 changeDimension
    // （虚派发，ServerPlayer override 调真实实现），同维度走 attemptTeleport。
    reg.method(
        "teleport",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            return _applyTeleport(ctx, player, argc, args, /*returnBoolean=*/false);
        },
        2);

    // --- tryTeleport(location: Vector3, teleportOptions?: TeleportOptions): boolean ---
    // 对齐基岩 Entity.tryTeleport，返回是否传送成功。逻辑见 _applyTeleport。
    reg.method(
        "tryTeleport",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            return _applyTeleport(ctx, player, argc, args, /*returnBoolean=*/true);
        },
        2);

    // --- headRotation (readonly property, Vector2 {x=pitch, y=yaw}) ---
    // 对齐基岩 headRotation: Vector2（x=pitch 俯仰, y=yaw 偏航）。用 Entity::pitch() + 头部 yaw。
    // 注：基岩 Vector2.x 是 pitch、Vector2.y 是 yaw（非头部 yaw，是身体 yaw）；头部旋转见 lookAt 系列。
    reg.readonlyProperty(
        "headRotation", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            void* obj = ctx.createObject();
            ctx.setPropertyFloat(obj, "x", player->pitch());
            ctx.setPropertyFloat(obj, "y", player->yaw());
            return obj;
        });

    // --- isSprinting (read-write property, boolean) ---
    reg.property(
        "isSprinting",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            return ctx.createBoolean(player->isSprinting());
        },
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, void* value) -> void {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                static_cast<void>(ctx.throwTypeError("Invalid SimulatedPlayer"));
                return;
            }
            auto b = ctx.toBool(value);
            if (!b) {
                static_cast<void>(ctx.throwTypeError("isSprinting must be a boolean"));
                return;
            }
            player->setSprinting(*b);
        });

    // --- moveToLocation(blockPos, speed?) ---
    reg.method(
        "moveToLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1 || !ctx.isObject(args[0])) {
                return ctx.throwTypeError("moveToLocation(pos, speed?)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            f32 speed = 1.0f;
            if (argc >= 2 && ctx.isNumber(args[1])) {
                auto s = ctx.toFloat64(args[1]);
                if (s) {
                    speed = static_cast<f32>(*s);
                }
            }
            player->moveToLocation(pos, speed);
            return ctx.createUndefined();
        },
        2);

    // --- lookAtLocation(blockPos) ---
    reg.method(
        "lookAtLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1 || !ctx.isObject(args[0])) {
                return ctx.throwTypeError("lookAtLocation(pos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            player->lookAtLocation(pos);
            return ctx.createUndefined();
        },
        1);

    // --- chat(command) -> number ---
    reg.method(
        "chat",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("chat(command)");
            }
            auto cmd = ctx.toString(args[0]);
            if (!cmd) {
                return ctx.throwInternalError("Failed to read command");
            }
            i32 result = player->chat(*cmd);
            return ctx.createInt32(result);
        },
        1);

    // --- respawn() ---
    reg.method(
        "respawn",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            player->respawn();
            return ctx.createUndefined();
        },
        0);

    // --- lookAtEntity(entity, duration?) ---
    // entity 经 _unwrapEntity 取 mc::Entity*。duration（LookDuration）当前忽略（瞬时定向），TODO。
    reg.method(
        "lookAtEntity",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("lookAtEntity(entity, duration?)");
            }
            auto* target = _unwrapEntity(ctx, args[0]);
            if (target == nullptr) {
                return ctx.throwTypeError("lookAtEntity: first arg must be an Entity");
            }
            // TODO: duration（LookDuration）插值语义，当前瞬时。
            player->lookAtEntity(*target);
            return ctx.createUndefined();
        },
        2);

    // --- lookAtBlock(blockLocation, duration?) ---
    reg.method(
        "lookAtBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("lookAtBlock(blockLocation, duration?)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            // TODO: duration（LookDuration）插值语义，当前瞬时。
            player->lookAtBlock(pos);
            return ctx.createUndefined();
        },
        2);

    // --- moveToBlock(blockLocation, options?) ---
    // options（MoveToOptions）当前忽略，TODO。speed 默认 1.0。
    reg.method(
        "moveToBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("moveToBlock(blockLocation, options?)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            // TODO: 解析 MoveToOptions（maxStraightLineReach/speed 等）。
            player->moveToBlock(pos, 1.0f);
            return ctx.createUndefined();
        },
        2);

    // --- jump() -> boolean ---
    reg.method(
        "jump",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            return ctx.createBoolean(player->jump());
        },
        0);

    // --- setFoodLevel(level) -> undefined（Cubium 测试扩展）---
    // 直接设定饥饿值，供食物类测试控制进食前提（canEat 需 foodLevel<20）。见 SimulatedPlayer.hpp 注释。
    reg.method(
        "setFoodLevel",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("setFoodLevel(level)");
            }
            auto level = ctx.toInt32(args[0]);
            if (!level) {
                return ctx.throwTypeError("setFoodLevel: level must be a number");
            }
            player->setFoodLevel(*level);
            return ctx.createUndefined();
        },
        1);

    // --- disconnect() ---
    reg.method(
        "disconnect",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            player->disconnect();
            return ctx.createUndefined();
        },
        0);

    // --- giveItem(itemStack, selectSlot?) -> boolean ---
    reg.method(
        "giveItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("giveItem(itemStack, selectSlot?)");
            }
            auto* stack = _unwrapItemStack(ctx, args[0]);
            if (stack == nullptr) {
                return ctx.throwTypeError("giveItem: first arg must be an ItemStack");
            }
            bool selectSlot = false;
            if (argc >= 2) {
                auto b = ctx.toBool(args[1]);
                if (b) {
                    selectSlot = *b;
                }
            }
            return ctx.createBoolean(player->giveItem(*stack, selectSlot));
        },
        2);

    // --- setItem(itemStack, slot, selectSlot?) -> boolean ---
    reg.method(
        "setItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 2 || !ctx.isNumber(args[1])) {
                return ctx.throwTypeError("setItem(itemStack, slot, selectSlot?)");
            }
            auto* stack = _unwrapItemStack(ctx, args[0]);
            if (stack == nullptr) {
                return ctx.throwTypeError("setItem: first arg must be an ItemStack");
            }
            auto slot = ctx.toInt32(args[1]);
            if (!slot) {
                return ctx.throwTypeError("setItem: slot must be a number");
            }
            bool selectSlot = false;
            if (argc >= 3) {
                auto b = ctx.toBool(args[2]);
                if (b) {
                    selectSlot = *b;
                }
            }
            return ctx.createBoolean(player->setItem(*stack, *slot, selectSlot));
        },
        3);

    // === 寻路 stub（8 方法，被 PathNavigator 硬依赖 dynamic_cast<MobEntity*> 阻塞，Player 不是 MobEntity）===
    // TODO: PathNavigator 适配非 Mob 拥有者后做实（见 [[mobentity-navigator-pathfinder-null-global-bug]]）。
    reg.method(
        "navigateToBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "navigateToBlock"); },
        2);
    reg.method(
        "navigateToEntity",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "navigateToEntity"); },
        2);
    reg.method(
        "navigateToLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "navigateToLocation"); },
        2);
    reg.method(
        "navigateToLocations",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "navigateToLocations"); },
        2);
    reg.method(
        "move",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "move"); },
        3);
    reg.method(
        "moveRelative",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "moveRelative"); },
        3);
    reg.method(
        "stopMoving",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopMoving"); },
        0);

    // === 攻击/交互/破坏/建造 ===
    // attack() 无参版本：基岩语义为攻击当前看向的实体，依赖 raycast 命中检测，TODO 待射线体系就绪。
    reg.method(
        "attack",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "attack"); },
        0);
    // --- attackEntity(entity) ---
    // 转发 SimulatedPlayer::attack → Player::attack(target) 走完整玩家攻击伤害链
    // （playerAttack source → target.hurt → actuallyHurt → setLastHurtBy → 群体仇恨触发）。
    // 用于验证 HurtByTargetGoal alertOthers 群体仇恨、近战伤害等行为。
    reg.method(
        "attackEntity",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("attackEntity(entity)");
            }
            auto* target = _unwrapEntity(ctx, args[0]);
            if (target == nullptr) {
                return ctx.throwTypeError("attackEntity: first arg must be an Entity");
            }
            player->attack(*target);
            return ctx.createUndefined();
        },
        1);
    reg.method(
        "interact",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "interact"); },
        0);
    reg.method(
        "interactWithBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            // interactWithBlock(blockLocation, direction?) -> boolean
            // 官方基岩语义：空手右键方块（useWithoutItem 路径）。仅触发 Block.use（onBlockActivated），
            // 不派发 Item.useOn（空手无物品）。对齐 vanilla MultiPlayerGameMode#useItemOn 空手分支：
            // 空手仍调 Block.use，但不调 Item.useOn。
            //
            // 实现复用原生 useItemOnBlock(空 ItemStack, ...)：空堆路径下 useItemOnBlock 内部
            // ① Block.use 前置分支正常执行（空手右键堆肥桶 level=8 收获骨粉、空手取花盆花、空手吃蛋糕、
            // 空手熄灭点燃蜡烛蛋糕等 onBlockActivated 类行为可触发）；② Item.useOn 因 !stack.isEmpty()
            // 守卫跳过（空手不派发物品侧行为）。故传空堆等价于空手右键，且不崩。
            // 比 useItemOnBlock 优势：脚本侧无需构造占位 ItemStack，语义更贴合官方 interactWithBlock。
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("interactWithBlock(blockLocation, direction?)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            const mc::Direction face = (argc >= 2) ? _directionFromApi(ctx, args[1]) : mc::Direction::Up;
            const mc::Vector3 faceLocation = mc::Vector3(0.5f, 1.0f, 0.5f);
            // 空手：传默认构造的空 ItemStack。faceLocation.y=1.0 对齐点击方块顶面中心（多数 onBlockActivated
            // 的 hit.hitPosition().y - pos.y 判定取 0.5 阈值时，顶面命中 y≈1.0 落在「上半部」语义内，
            // 如 CandleCakeBlock 熄灭需 hitY > 0.5）。
            mc::ItemStack emptyStack;
            return ctx.createBoolean(player->useItemOnBlock(emptyStack, pos, face, faceLocation));
        },
        2);
    // --- interactWithEntity(entity) -> boolean ---
    // 对齐基岩官方 SimulatedPlayer.interactWithEntity(entity): boolean。
    // 语义：玩家用当前主手物品右键实体。转发 Player::interactOn(target, Hand::MainHand)。
    // interactOn 内部流程（Player.cpp:2771）：
    //   1) 旁观者只开 INamedContainerProvider 容器；
    //   2) target.processInitialInteract（实体侧交互，如村民交易、马匹骑乘）；
    //   3) 实体不处理时，取主手物品调 Item::itemInteractionForEntity（金苹果治愈僵尸村民、
    //      命名牌命名、小麦喂养动物、骨粉催熟幼体等）。
    // 故测试侧可 player.setItem(golden_apple, 0, true) 后 interactWithEntity(zombieVillager)
    // 触发金苹果治愈链路；空手 interactWithEntity 走 processInitialInteract（交易等）。
    // 返回值映射基岩 boolean（"Returns true if the interaction was performed"）：
    //   Success/Consume → true（交互被执行/物品被消耗），Fail/Pass → false。
    // Hand 固定 MainHand（基岩 interactWithEntity 无 hand 参数，vanilla 默认主手）。
    reg.method(
        "interactWithEntity",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("interactWithEntity(entity)");
            }
            auto* target = _unwrapEntity(ctx, args[0]);
            if (target == nullptr) {
                return ctx.throwTypeError("interactWithEntity: first arg must be an Entity");
            }
            auto result = player->interactOn(*target, mc::Hand::MainHand);
            return ctx.createBoolean(
                result == mc::ActionResultType::Success || result == mc::ActionResultType::Consume);
        },
        1);
    reg.method(
        "stopInteracting",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopInteracting"); },
        0);
    reg.method(
        "breakBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "breakBlock"); },
        2);
    reg.method(
        "stopBreakingBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopBreakingBlock"); },
        0);
    reg.method(
        "startBuild",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "startBuild"); },
        1);
    reg.method(
        "stopBuild",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopBuild"); },
        0);

    // === 飞行/滑翔/游泳 stub（依赖物理状态机/abilities 细化）===
    // TODO: fly/glide/swim 物理分支（abilities.flying / fall-flying / swim 状态）实现后做实。
    reg.method(
        "fly",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "fly"); },
        0);
    reg.method(
        "stopFlying",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopFlying"); },
        0);
    reg.method(
        "glide",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "glide"); },
        0);
    reg.method(
        "stopGliding",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopGliding"); },
        0);
    reg.method(
        "swim",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "swim"); },
        0);
    reg.method(
        "stopSwimming",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopSwimming"); },
        0);

    // === 物品使用 ===
    // useItem/useItemInSlot 走 onItemRightClick（右键空气，vanilla 不消耗）。
    // useItemOnBlock/useItemInSlotOnBlock 走 onItemUse（右键方块，消耗由 onItemUse 内部决定）。
    // 原生实现见 SimulatedPlayer::useItem/useItemInSlot/useItemOnBlock/useItemInSlotOnBlock。
    // stopUsingItem/dropSelectedItem 仍为 stub（依赖使用中状态机/掉落物体系）。

    // --- useItem(itemStack) -> boolean ---
    reg.method(
        "useItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1) {
                return ctx.throwTypeError("useItem(itemStack)");
            }
            auto* stack = _unwrapItemStack(ctx, args[0]);
            if (stack == nullptr) {
                return ctx.throwTypeError("useItem: first arg must be an ItemStack");
            }
            return ctx.createBoolean(player->useItem(*stack));
        },
        1);

    // --- useItemInSlot(slot) -> boolean ---
    reg.method(
        "useItemInSlot",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("useItemInSlot(slot)");
            }
            auto slot = ctx.toInt32(args[0]);
            if (!slot) {
                return ctx.throwTypeError("useItemInSlot: slot must be a number");
            }
            return ctx.createBoolean(player->useItemInSlot(*slot));
        },
        1);

    // --- useItemOnBlock(itemStack, blockLocation, direction?, faceLocation?) -> boolean ---
    reg.method(
        "useItemOnBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 2) {
                return ctx.throwTypeError("useItemOnBlock(itemStack, blockLocation, direction?, faceLocation?)");
            }
            auto* stack = _unwrapItemStack(ctx, args[0]);
            if (stack == nullptr) {
                return ctx.throwTypeError("useItemOnBlock: first arg must be an ItemStack");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            const mc::Direction face = (argc >= 3) ? _directionFromApi(ctx, args[2]) : mc::Direction::Up;
            const mc::Vector3 faceLocation =
                (argc >= 4) ? _parseFaceLocation(ctx, args[3]) : mc::Vector3(0.5f, 0.5f, 0.5f);
            return ctx.createBoolean(player->useItemOnBlock(*stack, pos, face, faceLocation));
        },
        4);

    // --- useItemInSlotOnBlock(slot, blockLocation, direction?, faceLocation?) -> boolean ---
    reg.method(
        "useItemInSlotOnBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            if (argc < 2 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("useItemInSlotOnBlock(slot, blockLocation, direction?, faceLocation?)");
            }
            auto slot = ctx.toInt32(args[0]);
            if (!slot) {
                return ctx.throwTypeError("useItemInSlotOnBlock: slot must be a number");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            const mc::Direction face = (argc >= 3) ? _directionFromApi(ctx, args[2]) : mc::Direction::Up;
            const mc::Vector3 faceLocation =
                (argc >= 4) ? _parseFaceLocation(ctx, args[3]) : mc::Vector3(0.5f, 0.5f, 0.5f);
            return ctx.createBoolean(player->useItemInSlotOnBlock(*slot, pos, face, faceLocation));
        },
        4);

    reg.method(
        "stopUsingItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* player = static_cast<SimulatedPlayer*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (player == nullptr) {
                return ctx.throwTypeError("Invalid SimulatedPlayer");
            }
            // 对齐基岩 SimulatedPlayer.stopUsingItem：停止使用当前物品（释放持续使用，如弓/三叉戟拉弓释放），
            // 返回停止前正在使用的物品（无则 undefined）。
            // 实现：捕获 getActiveItem（stopActiveHand 内部会清空 m_activeItem），调 stopActiveHand 触发
            // onPlayerStoppedUsing（弓耐久损耗/三叉戟投掷消耗等，已修复回写权威装备槽），返回活动物品拷贝。
            // 若不在使用物品（isUsingItem=false），stopActiveHand 早返回（LivingEntity.cpp:2138），返回 undefined。
            if (!player->isUsingItem()) {
                return ctx.createUndefined();
            }
            const mc::ItemStack activeBefore = player->getActiveItem();
            player->stopActiveHand();
            // 活动物品已空（异常情况）返回 undefined。
            if (activeBefore.isEmpty()) {
                return ctx.createUndefined();
            }
            // wrap owned 拷贝（与 Equippable.getEquipment 范式一致），JS GC 时 delete。
            const u64 isClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("ItemStack");
            void* isProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(isClassId);
            if (isProto == nullptr) {
                return ctx.createUndefined();
            }
            auto* owned = new mc::ItemStack(activeBefore);
            return ScriptObjectRegistry::wrap(ctx, isClassId, isProto, owned, true, "ItemStack");
        },
        0);
    reg.method(
        "dropSelectedItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "dropSelectedItem"); },
        0);

    // === 旋转 stub（依赖插值/身体旋转状态机）===
    // TODO: rotateBody/setBodyRotation 待玩家身体旋转插值体系实现后做实。
    reg.method(
        "rotateBody",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "rotateBody"); },
        1);
    reg.method(
        "setBodyRotation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "setBodyRotation"); },
        1);

    // 注：setSkin(options: PlayerSkinData) 属皮肤/装饰类，按任务范围排除（不绑定）。
    // flyToLocation 原生侧已有 stub 声明但未绑（与 navigateTo* 同属寻路，统一在 navigateTo* stub 群覆盖语义）。

    return classId;
}

void* wrapSimulatedPlayer(mc::mod::bedrock::addon::IScriptBindingContext& ctx, u64 classId, SimulatedPlayer* player)
{
    if (player == nullptr) {
        return ctx.createNull();
    }
    // 非拥有：实体由 ServerWorld EntityManager 拥有，测试运行期间稳定。
    void* proto = ScriptBindingRegistry::instance().proto(classId);
    return ScriptObjectRegistry::wrap(
        ctx, classId, proto, player, /*owned=*/false, "SimulatedPlayer", nullptr, player->id());
}

} // namespace mc::test
