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

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp" // ScriptObjectRegistry/ClassRegistrar
#include "common/world/block/BlockPos.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp"

#include <string>

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
        // ServerPlayer 名字经 getName 访问；若无则返回空串。TODO: 确认 ServerPlayer 名字 getter。
        return ctx.createString(std::string{});
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

    // TODO: lookAtEntity（需 Entity 绑定桥接）、flyToLocation/attack（原生侧 TODO stub）待补全。

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
