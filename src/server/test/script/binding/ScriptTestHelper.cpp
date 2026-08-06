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

#include "server/test/script/binding/ScriptTestHelper.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"  // ScriptObjectRegistry/ClassRegistrar
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 wrap Entity/Dimension proto
#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "common/world/IWorld.hpp" // mc::IWorld（helper->world() 返回类型，wrap 为 Dimension）
#include "common/world/block/BlockPos.hpp"
#include "server/test/facade/GameTestHelper.hpp"
#include "server/test/script/binding/ScriptCallbackUtil.hpp"
#include "server/test/script/binding/ScriptSequence.hpp"
#include "server/test/script/binding/ScriptSimulatedPlayer.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"

#include <string>
#include <utility>

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {

// 文件局部：从 JS Test 对象（thisVal）的 opaque 取 helper；失败抛 JS TypeError。
// Test 对象由 ScriptGameTestFunction::run 创建，opaque 存 GameTestHelper*（非拥有）。
// 返回 nullptr 时调用方已 throw，应立即 return。
GameTestHelper* _requireHelper(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal)
{
    const u64 testClassId = ScriptBindingRegistry::instance().testClassId();
    auto* helper = static_cast<GameTestHelper*>(ScriptObjectRegistry::unwrap(ctx, thisVal, testClassId));
    if (helper == nullptr) {
        // JS 侧已抛异常；C++ 控制流返回 nullptr 占位，调用方须判空并 return。
        static_cast<void>(ctx.throwTypeError("Test method called on invalid Test object"));
    }
    return helper;
}

// 把 GameTestResult 映射为 JS 行为：通过→createUndefined；失败→throwInternalError。
// 返回 nullptr 表示已 throw，调用方 return。
void* _resultToJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, GameTestResult result)
{
    if (isPass(result)) {
        return ctx.createUndefined();
    }
    std::string msg = result->formattedMessage();
    return ctx.throwInternalError(msg.c_str());
}

// 解析 BlockPos 参数（JS 传 {x,y,z} 对象形式）。
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

u64 registerTestClassBinding(mc::mod::bedrock::addon::NativeModuleBuilder& builder,
    mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    u64 sequenceClassId,
    u64 simulatedPlayerClassId)
{
    u64 classId = ScriptObjectRegistry::allocateClassId(ctx);
    void* proto = builder.exportClass("Test", classId);
    ScriptBindingRegistry::instance().registerProto(classId, proto);

    ClassRegistrar<void> reg(ctx, classId, proto);

    // --- assertBlockPresent(blockType, blockPos, isPresent) ---
    reg.method(
        "assertBlockPresent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 3 || !ctx.isString(args[0]) || !ctx.isObject(args[2])) {
                return ctx.throwTypeError("assertBlockPresent(blockType, pos, isPresent)");
            }
            auto blockType = ctx.toString(args[0]);
            if (!blockType) {
                return ctx.throwInternalError("Failed to read blockType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            auto isPresent = ctx.toBool(args[2]);
            if (!isPresent) {
                return ctx.throwTypeError("isPresent must be boolean");
            }
            auto result = helper->assertBlockPresent(*blockType, pos, *isPresent);
            return _resultToJs(ctx, std::move(result));
        },
        3);

    // --- setBlock(blockType, blockPos, updateFlags=3) ---
    reg.method(
        "setBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("setBlock(blockType, pos, updateFlags?)");
            }
            auto blockType = ctx.toString(args[0]);
            if (!blockType) {
                return ctx.throwInternalError("Failed to read blockType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            i32 flags = 3;
            if (argc >= 3 && ctx.isNumber(args[2])) {
                auto f = ctx.toInt32(args[2]);
                if (f) {
                    flags = *f;
                }
            }
            auto result = helper->setBlock(*blockType, pos, flags);
            return _resultToJs(ctx, std::move(result));
        },
        3);

    // --- pressButton(pos) ---
    reg.method(
        "pressButton",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("pressButton(pos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto result = helper->pressButton(pos);
            return _resultToJs(ctx, std::move(result));
        },
        1);

    // --- pullLever(pos) ---
    reg.method(
        "pullLever",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("pullLever(pos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto result = helper->pullLever(pos);
            return _resultToJs(ctx, std::move(result));
        },
        1);

    // --- pulseRedstone(pos, duration) ---
    reg.method(
        "pulseRedstone",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isNumber(args[1])) {
                return ctx.throwTypeError("pulseRedstone(pos, duration)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto dur = ctx.toInt32(args[1]);
            if (!dur) {
                return ctx.throwTypeError("duration must be number");
            }
            auto result = helper->pulseRedstone(pos, *dur);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- killAllEntities() ---
    reg.method(
        "killAllEntities",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            auto result = helper->killAllEntities();
            return _resultToJs(ctx, std::move(result));
        },
        0);

    // --- succeed() ---
    reg.method(
        "succeed",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            helper->succeed();
            return ctx.createUndefined();
        },
        0);

    // --- fail(errorText) ---
    reg.method(
        "fail",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            std::string msg = "Test failed";
            if (argc >= 1 && ctx.isString(args[0])) {
                auto m = ctx.toString(args[0]);
                if (m) {
                    msg = *m;
                }
            }
            helper->fail(GameTestError(GameTestErrorType::FailConditionsMet, std::move(msg)));
            return ctx.createUndefined();
        },
        1);

    // --- print(text) ---
    reg.method(
        "print",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            std::string text;
            if (argc >= 1 && ctx.isString(args[0])) {
                auto t = ctx.toString(args[0]);
                if (t) {
                    text = *t;
                }
            }
            helper->print(text);
            return ctx.createUndefined();
        },
        1);

    // --- startSequence() -> GameTestSequence ---
    reg.method(
        "startSequence",
        [sequenceClassId](mc::mod::bedrock::addon::IScriptBindingContext& ctx,
            void* thisVal,
            i32 /*argc*/,
            void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            GameTestSequence& seq = helper->startSequence();
            return wrapSequence(ctx, sequenceClassId, &seq);
        },
        0);

    // --- spawnSimulatedPlayer(name, blockPos, gameMode?) -> SimulatedPlayer ---
    reg.method(
        "spawnSimulatedPlayer",
        [simulatedPlayerClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0]) || !ctx.isObject(args[1])) {
                return ctx.throwTypeError("spawnSimulatedPlayer(name, pos, gameMode?)");
            }
            auto name = ctx.toString(args[0]);
            if (!name) {
                return ctx.throwInternalError("Failed to read name");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            mc::GameMode gameMode = mc::GameMode::Creative;
            if (argc >= 3 && ctx.isNumber(args[2])) {
                auto gm = ctx.toInt32(args[2]);
                if (gm) {
                    gameMode = static_cast<mc::GameMode>(*gm);
                }
            }
            SimulatedPlayer* player = nullptr;
            auto result = helper->spawnSimulatedPlayer(*name, pos, gameMode, player);
            if (!isPass(result)) {
                std::string msg = result->formattedMessage();
                return ctx.throwInternalError(msg.c_str());
            }
            if (player == nullptr) {
                return ctx.throwInternalError("spawnSimulatedPlayer returned null");
            }
            return wrapSimulatedPlayer(ctx, simulatedPlayerClassId, player);
        },
        3);

    // --- idle(ticks) -> Promise<void> ---
    // JS Promise 语义专属：await test.idle(n) 暂停 n tick。经 ScriptScheduler::runTimeout 在 n tick 后
    // resolve Promise。scheduler 由 GameTestModuleBinding::setScheduler 注入 ScriptBindingRegistry。
    reg.method(
        "idle",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (_requireHelper(ctx, thisVal) == nullptr) {
                return nullptr; // 仅校验 Test 对象有效性（idle 不直接用 helper）
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("idle(ticks)");
            }
            auto ticks = ctx.toInt32(args[0]);
            if (!ticks || *ticks < 0) {
                return ctx.throwTypeError("ticks must be non-negative number");
            }
            auto* scheduler = ScriptBindingRegistry::instance().scheduler();
            if (scheduler == nullptr) {
                return ctx.throwInternalError("idle requires scheduler");
            }
            // createPromise 返回 promise（owned）+ resolvingFuncs[0]=resolve/[1]=reject（owned）。
            void* resolving[2] = {nullptr, nullptr};
            void* promise = ctx.createPromise(resolving);
            void* resolveFn = resolving[0];
            void* rejectFn = resolving[1];
            // reject 未使用，立即释放避免泄漏。
            ctx.releaseValue(rejectFn);

            // runTimeout 回调在 n tick 后执行：调 resolve(undefined) 触发 Promise resolve，
            // 然后 releaseValue(resolveFn)（resolving func 一次性，调用后释放）。
            // 全程经抽象接口 IScriptBindingContext，不依赖具体引擎后端。
            mc::mod::bedrock::addon::IScriptBindingContext* ctxPtr = &ctx;
            scheduler->runTimeout(
                [ctxPtr, resolveFn]() {
                    void* undef = ctxPtr->createUndefined();
                    ctxPtr->callResolvingFunc(resolveFn, undef);
                    ctxPtr->releaseValue(undef);
                    ctxPtr->releaseValue(resolveFn);
                },
                static_cast<u32>(*ticks));
            return promise; // owned，返给 JS 引擎
        },
        1);

    // --- until(condition) -> void ---
    // 基岩 JS Test.until(condition) 是 void（注册持续轮询，不阻塞 JS 体）。转发原生 helper->until，
    // 把 JS 函数 wrap 成 std::function<GameTestResult()>（复用 ScriptCallbackUtil::wrapJsCallback）。
    reg.method(
        "until",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("until(condition)");
            }
            ctx.retainValue(args[0]); // wrapJsCallback 持久化引用，回调执行时须存活
            auto wrapped = wrapJsCallback(&ctx, args[0]);
            helper->until(std::move(wrapped), nullptr);
            return ctx.createUndefined();
        },
        1);

    // --- currentTick (readonly property) ---
    reg.readonlyProperty(
        "currentTick", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal) -> void* {
            const u64 testClassId = ScriptBindingRegistry::instance().testClassId();
            auto* helper = static_cast<GameTestHelper*>(ScriptObjectRegistry::unwrap(ctx, thisVal, testClassId));
            if (helper == nullptr) {
                return ctx.createInt32(0); // property getter 不抛异常，失败返回 0
            }
            return ctx.createInt32(helper->currentTick());
        });

    // --- assertEntityPresentInArea(entityType, isPresent) ---
    // 行为包 MobBehaviorTests/StarterTests 用：断言结构包围盒内是否存在的指定类型实体。
    reg.method(
        "assertEntityPresentInArea",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("assertEntityPresentInArea(entityType, isPresent)");
            }
            auto entityType = ctx.toString(args[0]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            auto isPresent = ctx.toBool(args[1]);
            if (!isPresent) {
                return ctx.throwTypeError("isPresent must be boolean");
            }
            auto result = helper->assertEntityPresentInArea(*entityType, *isPresent);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- spawn(entityType, pos) -> Entity ---
    // 转发 helper->spawnEntity 真正生成实体，并 wrap 成 @minecraft/server Entity JS 对象返回。
    // Entity classId/proto 经 ScriptClassRegistry 跨模块取（@minecraft/server 模块注册）。
    // 实体由 EntityManager 拥有，此处非拥有（owned=false），生命周期与测试实例一致。
    reg.method(
        "spawn",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("spawn(entityType, pos)");
            }
            auto entityType = ctx.toString(args[0]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            mc::Entity* outEntity = nullptr;
            auto result = helper->spawnEntity(*entityType, pos, outEntity);
            if (!isPass(result)) {
                std::string msg = result->formattedMessage();
                return ctx.throwInternalError(msg.c_str());
            }
            // wrap outEntity 为 @minecraft/server Entity JS 对象：经 ScriptClassRegistry 取
            // @minecraft/server 模块注册的 Entity classId/proto（跨模块 wrap）。
            // 实体由 EntityManager 拥有，此处非拥有（owned=false），生命周期与测试实例一致。
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            void* entityProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(entityClassId);
            if (entityProto == nullptr || outEntity == nullptr) {
                // 引擎未就绪或 spawn 异常返回 undefined（不崩，对齐基岩容错）。
                return ctx.createUndefined();
            }
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, entityClassId, entityProto, outEntity, false, "Entity");
        },
        2);

    // --- runAtTickTime(tick, fn) ---
    // 注册"在第 tick tick 执行一次 fn"的调度回调（对齐基岩 Test.runAtTickTime）。
    // fn 经 wrapJsCallback 包装成 std::function<GameTestResult()>，调用方须 retainValue 持久化 JS 句柄。
    reg.method(
        "runAtTickTime",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isNumber(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("runAtTickTime(tick, fn)");
            }
            auto tick = ctx.toInt32(args[0]);
            if (!tick) {
                return ctx.throwTypeError("tick must be number");
            }
            ctx.retainValue(args[1]); // wrapJsCallback 持久化引用，回调执行时须存活
            auto wrapped = wrapJsCallback(&ctx, args[1]);
            helper->runAtTickTime(*tick, std::move(wrapped));
            return ctx.createUndefined();
        },
        2);

    // --- succeedWhen(fn) ---
    // 注册"持续轮询 fn 直到返回 pass 即 succeed"的完成条件（对齐基岩 Test.succeedWhen）。
    reg.method(
        "succeedWhen",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("succeedWhen(fn)");
            }
            ctx.retainValue(args[0]); // wrapJsCallback 持久化引用，回调执行时须存活
            auto wrapped = wrapJsCallback(&ctx, args[0]);
            helper->succeedWhen(std::move(wrapped));
            return ctx.createUndefined();
        },
        1);

    // --- succeedWhenEntityPresent(entityType, pos, isPresent=true) ---
    // facade 无此方法，组合实现：succeedWhen(() => assertEntityPresent(entityType, pos, searchDistance, isPresent))。
    // 基岩版第三参 isPresent 默认 true；searchDistance 取结构包围盒对角线长度（与 area 查询语义一致，足够覆盖）。
    reg.method(
        "succeedWhenEntityPresent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("succeedWhenEntityPresent(entityType, pos, isPresent?)");
            }
            auto entityType = ctx.toString(args[0]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            bool isPresent = true;
            if (argc >= 3) {
                auto b = ctx.toBool(args[2]);
                if (b) {
                    isPresent = *b;
                }
            }
            // 用较大 searchDistance（64）覆盖结构范围，对齐基岩 area 内实体存在性判定语义。
            std::string typeCopy = *entityType;
            BlockPos posCopy = pos;
            bool presentCopy = isPresent;
            helper->succeedWhen([helper, typeCopy, posCopy, presentCopy]() -> GameTestResult {
                return helper->assertEntityPresent(typeCopy, posCopy, 64.0f, presentCopy);
            });
            return ctx.createUndefined();
        },
        3);

    // --- assert(condition, message?) ---
    // 基岩 Test.assert：condition 假则 fail(message)。纯 JS 语义，转发 helper->fail。
    reg.method(
        "assert",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("assert(condition, message?)");
            }
            auto cond = ctx.toBool(args[0]);
            if (!cond) {
                return ctx.throwTypeError("assert condition must be boolean");
            }
            if (!*cond) {
                std::string msg = "Assertion failed";
                if (argc >= 2 && ctx.isString(args[1])) {
                    auto m = ctx.toString(args[1]);
                    if (m) {
                        msg = *m;
                    }
                }
                helper->fail(GameTestError(GameTestErrorType::FailConditionsMet, std::move(msg)));
            }
            return ctx.createUndefined();
        },
        2);

    // --- worldLocation(pos) -> {x,y,z} ---
    // 基岩 Test.worldLocation：相对坐标→世界绝对坐标。转发 helper->worldBlockPosition。
    reg.method(
        "worldLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("worldLocation(pos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            BlockPos world = helper->worldBlockPosition(pos);
            void* obj = ctx.createObject();
            ctx.setPropertyInt(obj, "x", world.x);
            ctx.setPropertyInt(obj, "y", world.y);
            ctx.setPropertyInt(obj, "z", world.z);
            return obj;
        },
        1);

    // --- setBlockType(blockType, pos) ---
    // 基岩版 setBlockType 是 setBlock 的历史别名（无 updateFlags 参数，等价 setBlock(blockType, pos, 3)）。
    reg.method(
        "setBlockType",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("setBlockType(blockType, pos)");
            }
            auto blockType = ctx.toString(args[0]);
            if (!blockType) {
                return ctx.throwInternalError("Failed to read blockType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            auto result = helper->setBlock(*blockType, pos, 3);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- getDimension() -> Dimension ---
    // 返回测试所在维度（helper->world() 即 ServerWorld&，按 mc::IWorld 接口暴露）。
    // wrap 成 @minecraft/server Dimension JS 对象：经 ScriptClassRegistry 跨模块取 Dimension
    // classId/proto。IWorld 由 ServerWorld 持有，此处非拥有（owned=false）。
    reg.method(
        "getDimension",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            mc::IWorld* world = &helper->world();
            const u64 dimClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Dimension");
            void* dimProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(dimClassId);
            if (dimProto == nullptr) {
                // @minecraft/server Dimension 类未注册（模块未加载），容错返回 undefined。
                return ctx.createUndefined();
            }
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, dimClassId, dimProto, world, false, "Dimension");
        },
        0);

    // TODO: 仍未桥接的方法（assertEntityPresent（已用于 succeedWhenEntityPresent 组合，未单独暴露 JS）/
    // spawnItem/assertBlockState/destroyBlock/assertRedstonePower/assertIsWaterlogged/worldPosition/
    // relativePosition/rotateVector/getTestDirection/succeedWhenBlockPresent/succeedIf/succeedOnTick/
    // succeedOnTickWhen/failIf/runAfterDelay/runOnFinish/getBlock/relativeBlockLocation/worldBlockLocation/
    // assertEntityInstancePresent/assertEntityInstancePresentInArea/assertEntityTouching/
    // assertItemEntityPresent/assertItemEntityCountIs/spawnItemAt 等）行为包 0 使用，待按需补全。
    // 另：spawn 返回值 Entity JS 对象、getDimension 返回 Dimension JS 对象已随阶段 3 补全。

    return classId;
}

} // namespace mc::test
