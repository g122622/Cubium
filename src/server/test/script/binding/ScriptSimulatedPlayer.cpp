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

#include "common/item/core/ItemStack.hpp"                           // giveItem/setItem 按值拷贝需完整类型
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"  // ScriptObjectRegistry/ClassRegistrar
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 unwrap Entity/ItemStack
#include "common/test/base/error/GameTestErrorType.hpp"             // GameTestErrorType::MethodNotImplemented
#include "common/world/block/BlockPos.hpp"
#include "server/test/script/binding/ScriptGameTestError.hpp" // throwGameTestError（stub 用）
#include "server/test/script/context/ScriptBindingRegistry.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp"

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

} // namespace

u64 registerSimulatedPlayerClassBinding(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx)
{
    u64 classId = ScriptObjectRegistry::allocateClassId(ctx);
    void* proto = builder.exportClass("SimulatedPlayer", classId);
    ScriptBindingRegistry::instance().registerProto(classId, proto);

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

    // === 攻击/交互/破坏/建造 stub（依赖攻击事件/射线检测/破坏链路未就绪）===
    // TODO: 各方法待对应体系（攻击事件派发/raycast/destroyBlock 战利品/建造放置）实现后做实。
    reg.method(
        "attack",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "attack"); },
        0);
    reg.method(
        "attackEntity",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "attackEntity"); },
        1);
    reg.method(
        "interact",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "interact"); },
        0);
    reg.method(
        "interactWithBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "interactWithBlock"); },
        2);
    reg.method(
        "interactWithEntity",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "interactWithEntity"); },
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

    // === 物品使用 stub（依赖物品使用派发/useOn 链路）===
    // TODO: useItem/useItemInSlot/useItemOnBlock/useItemInSlotOnBlock/stopUsingItem/dropSelectedItem 待
    // 物品使用派发（Item::use/useOn + 玩家交互管理器）实现后做实。
    reg.method(
        "useItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "useItem"); },
        1);
    reg.method(
        "useItemInSlot",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "useItemInSlot"); },
        1);
    reg.method(
        "useItemOnBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "useItemOnBlock"); },
        4);
    reg.method(
        "useItemInSlotOnBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "useItemInSlotOnBlock"); },
        4);
    reg.method(
        "stopUsingItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return _throwNotImplemented(ctx, "stopUsingItem"); },
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
    return ScriptObjectRegistry::wrap(ctx, classId, proto, player, /*owned=*/false, "SimulatedPlayer");
}

} // namespace mc::test
