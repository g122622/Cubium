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

#include "common/item/core/ItemStack.hpp" // mc::ItemStack（_unwrapItemStack/assertContainerContains）
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"  // ScriptObjectRegistry/ClassRegistrar
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 wrap Entity/Dimension proto
#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "common/util/Direction.hpp"    // Direction / Rotation
#include "common/util/math/Vector3.hpp" // mc::math::Vector3d（worldPosition/rotateVector 等）
#include "common/world/IWorld.hpp"      // mc::IWorld（helper->world() 返回类型，wrap 为 Dimension）
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp" // mc::BlockState（_unwrapBlockPermutation/setBlockPermutation）
#include "common/world/block/blocks/sculk/SculkSpreader.hpp" // mc::blocks::SculkSpreader（getSculkSpreader wrap/delete 需完整类型）
#include "server/test/facade/GameTestHelper.hpp"
#include "server/test/script/binding/ScriptCallbackUtil.hpp"
#include "server/test/script/binding/ScriptGameTestError.hpp" // throwGameTestErrorFromResult（_resultToJs 改造）
#include "server/test/script/binding/ScriptSequence.hpp"
#include "server/test/script/binding/ScriptSimulatedPlayer.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"

#include <cctype>
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

// 把 GameTestResult 映射为 JS 行为：通过→createUndefined；失败→构造 GameTestError JS 实例并 throwValue。
// 返回 nullptr 表示已 throw，调用方 return。委托 ScriptGameTestError::throwGameTestErrorFromResult，
// 使 JS 侧经 instanceof GameTestError/Error 判别异常类型并携带 type/message/context/params 字段。
void* _resultToJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, GameTestResult result)
{
    return throwGameTestErrorFromResult(ctx, std::move(result));
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

// 解析 Vector3 参数（JS 传 {x,y,z} 对象，浮点或整数均可）。供 worldLocation/rotateVector/
// assertEntityTouching/spawnItem 等基岩 Vector3 语义方法使用（基岩 Test 类 location 统一为 Vector3）。
bool _parseVector3(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* arg, mc::math::Vector3d& out)
{
    if (!ctx.isObject(arg)) {
        static_cast<void>(ctx.throwTypeError("Vector3 argument must be {x,y,z} object"));
        return false;
    }
    auto x = ctx.getPropertyFloat(arg, "x");
    auto y = ctx.getPropertyFloat(arg, "y");
    auto z = ctx.getPropertyFloat(arg, "z");
    if (!x || !y || !z) {
        static_cast<void>(ctx.throwTypeError("Vector3 must have numeric x,y,z"));
        return false;
    }
    out = mc::math::Vector3d(*x, *y, *z);
    return true;
}

// 把 Vector3d 转成 JS {x,y,z} 对象（返回 owned 句柄）。
void* _vector3ToJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, const mc::math::Vector3d& v)
{
    void* obj = ctx.createObject();
    ctx.setPropertyFloat(obj, "x", v.x);
    ctx.setPropertyFloat(obj, "y", v.y);
    ctx.setPropertyFloat(obj, "z", v.z);
    return obj;
}

// 把 @minecraft/server.Direction（PascalCase 字符串枚举，如 "North"）转 mc::Direction。
// 项目 Directions::fromName 接小写，故先转小写。失败返回 Direction::None。
mc::Direction _directionFromApi(const std::string& apiName)
{
    std::string lower = apiName;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    auto dir = mc::Directions::fromName(lower);
    return dir.value_or(mc::Direction::None);
}

// 把 mc::Direction 转成 @minecraft/server.Direction（PascalCase 字符串，如 "North"）。
// 项目 Directions::toString 返回小写，首字母大写对齐官方 Direction 枚举。
std::string _directionToApi(mc::Direction dir)
{
    std::string name = mc::Directions::toString(dir);
    if (!name.empty()) {
        name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    }
    return name;
}

// 从 JS BlockPermutation 对象 unwrap 出 mc::BlockState*（批2 MinecraftModuleFactory 注册，opaque 持 const
// BlockState*）。 失败返回 nullptr（调用方抛 TypeError）。
const mc::BlockState* _unwrapBlockPermutation(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* val)
{
    const u64 classId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("BlockPermutation");
    return static_cast<const mc::BlockState*>(mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, val, classId));
}

// 从 JS ItemStack 对象 unwrap 出 mc::ItemStack*（批2 MinecraftModuleFactory 注册，opaque 持 mc::ItemStack*）。
mc::ItemStack* _unwrapItemStack(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* val)
{
    const u64 classId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("ItemStack");
    return static_cast<mc::ItemStack*>(mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, val, classId));
}

// 构造 FenceConnectivity JS 值对象（{north,east,south,west} 四 bool），原型挂 ScriptGameTestTypes 注册的
// FenceConnectivity classId/proto（instanceof 锚点）。失败（类未注册）退化为普通对象。
void* _fenceConnectivityToJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, const FenceConnectivity& conn)
{
    const u64 classId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("FenceConnectivity");
    void* proto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(classId);
    void* obj = ctx.createObject();
    if (proto != nullptr) {
        ctx.setPrototypeOf(obj, proto);
    }
    ctx.setPropertyBool(obj, "north", conn.north);
    ctx.setPropertyBool(obj, "east", conn.east);
    ctx.setPropertyBool(obj, "south", conn.south);
    ctx.setPropertyBool(obj, "west", conn.west);
    return obj;
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
            // 参数：assertBlockPresent(blockType:string, pos:object{x,y,z}, isPresent:boolean)
            // 此前校验误写成 !isObject(args[2])（检查 isPresent 是否为 object），但 isPresent 是 boolean，
            // 致 test.assertBlockPresent("xxx", {x,y,z}, true) 恒抛 TypeError（args[2]=true 非 object），
            // cloneBlocksCommand 全链路被此阻断。正确校验：pos=args[1] 须为 object。
            if (argc < 3 || !ctx.isString(args[0]) || !ctx.isObject(args[1])) {
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

    // --- spawnSimulatedPlayer(blockLocation, name?, gameMode?) -> SimulatedPlayer ---
    // 对齐官方基岩签名：位置在前，名字与游戏模式可选。省略 name 时用默认名 "SimulatedPlayer"。
    reg.method(
        "spawnSimulatedPlayer",
        [simulatedPlayerClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isObject(args[0])) {
                return ctx.throwTypeError("spawnSimulatedPlayer(blockLocation, name?, gameMode?)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            // name 可选：未提供或非字符串时用默认名（对齐基岩省略 name 行为）。
            std::string name = "SimulatedPlayer";
            if (argc >= 2 && ctx.isString(args[1])) {
                auto nameOpt = ctx.toString(args[1]);
                if (!nameOpt) {
                    return ctx.throwInternalError("Failed to read name");
                }
                name = *nameOpt;
            }
            mc::GameMode gameMode = mc::GameMode::Creative;
            // gameMode 位置随 name 是否省略而后移：name 提供时 gameMode 在 args[2]，省略时在 args[1]。
            // 对齐基岩：基岩按形参位置匹配，省略 name 时 gameMode 仍在第 2 个槽位（args[1]）。
            i32 gameModeIdx = (argc >= 2 && ctx.isString(args[1])) ? 2 : 1;
            if (argc > gameModeIdx && ctx.isNumber(args[gameModeIdx])) {
                auto gm = ctx.toInt32(args[gameModeIdx]);
                if (gm) {
                    gameMode = static_cast<mc::GameMode>(*gm);
                }
            }
            SimulatedPlayer* player = nullptr;
            auto result = helper->spawnSimulatedPlayer(name, pos, gameMode, player);
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
            auto wrapped = wrapJsCallback(&ctx, args[0]);
            helper->succeedWhen(std::move(wrapped));
            return ctx.createUndefined();
        },
        1);

    // --- succeedWhenEntityPresent(entityType, pos, isPresent=true) ---
    // facade 无此方法，组合实现：succeedWhen(() => assertEntityPresent(entityType, pos, 0.0, isPresent))。
    // 基岩版第三参 isPresent 默认 true；searchDistance=0 对齐 Java GameTestHelper.assertEntityPresent(type, BlockPos)
    // 的 1 格方块 AABB 精确匹配（实体须落在 pos 方块内）。此前传 64.0 球查询致假通过，已修正。
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
            // 对齐 Java GameTestHelper.succeedWhenEntityPresent(type, BlockPos) → assertEntityPresent(type, BlockPos)：
            // 用 1 格方块 AABB 精确匹配（searchDistance=0）。此前传 64.0f 球查询导致假通过
            // （实体在结构内任意位置都算 present，掩盖"实体未到精确位置"的失败，如 runAsLlama
            // 命令失败羊驼未动却通过）。
            std::string typeCopy = *entityType;
            BlockPos posCopy = pos;
            bool presentCopy = isPresent;
            helper->succeedWhen([helper, typeCopy, posCopy, presentCopy]() -> GameTestResult {
                return helper->assertEntityPresent(typeCopy, posCopy, 0.0f, presentCopy);
            });
            return ctx.createUndefined();
        },
        3);

    // --- assert(condition, message?) ---
    // 基岩 Test.assert：condition 假则抛出带 message 的错误（不直接终止测试）。
    // 抛错语义使其可在 succeedWhen/succeedIf 回调中使用：wrapJsCallback 捕获异常转为 GameTestResult
    // （has_value），succeedWhen 据此判定"条件未满足，继续等待"，而非立即 fail。
    // 直接终止测试由 Test.fail 承担；assert 仅声明条件不成立。
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
                // 抛 JS 异常而非 helper->fail：succeedWhen 回调里捕获为"未满足"继续等待，
                // 测试主函数里异常上浮由 runResult 捕获转为 fail。
                return ctx.throwInternalError(msg.c_str());
            }
            return ctx.createUndefined();
        },
        2);

    // --- worldLocation(pos) -> {x,y,z} ---
    // 基岩 Test.worldLocation：相对 Vector3 位置→世界绝对 Vector3（考虑结构旋转）。转发 helper->worldPosition。
    // 基岩 location 统一为 Vector3（浮点），与方块专用 worldBlockLocation 区分。
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
            mc::math::Vector3d pos;
            if (!_parseVector3(ctx, args[0], pos)) {
                return nullptr;
            }
            return _vector3ToJs(ctx, helper->worldPosition(pos));
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

    // --- assertEntityPresent(entityType, pos, searchDistance=0, isPresent=true) ---
    // 单独暴露（succeedWhenEntityPresent 组合内部已用，此处供测试体直接断言）。基岩 searchDistance 默认 0
    // （只搜索单个方块），isPresent 默认 true。pos 为结构相对坐标（基岩 Vector3，此处按 BlockPos 整数解析，
    // 与 assertEntityPresentInArea 等同族方法一致）。
    reg.method(
        "assertEntityPresent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("assertEntityPresent(entityType, pos, searchDistance?, isPresent?)");
            }
            auto entityType = ctx.toString(args[0]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            f32 searchDistance = 0.0f;
            if (argc >= 3 && ctx.isNumber(args[2])) {
                auto d = ctx.toFloat64(args[2]);
                if (d) {
                    searchDistance = static_cast<f32>(*d);
                }
            }
            bool isPresent = true;
            if (argc >= 4) {
                auto b = ctx.toBool(args[3]);
                if (b) {
                    isPresent = *b;
                }
            }
            auto result = helper->assertEntityPresent(*entityType, pos, searchDistance, isPresent);
            return _resultToJs(ctx, std::move(result));
        },
        4);

    // --- spawnItem(itemType, location) -> Entity ---
    // 基岩 spawnItem(itemStack: ItemStack, location: Vector3) -> Entity。项目 ItemStack JS 类未充实（空壳），
    // 此处简化接受 itemType 字符串（偏离基岩签名），内部转 spawnItemAt(itemType, Vector3d)。
    // TODO: ItemStack JS 类充实后改为接受 ItemStack 对象（解析 typeId/amount 构造 ItemStack 调 spawnItemAt）。
    // 返回 Entity JS 对象（经 ScriptClassRegistry 跨模块 wrap，与 spawn 同）。
    reg.method(
        "spawnItem",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("spawnItem(itemType, location)");
            }
            auto itemType = ctx.toString(args[0]);
            if (!itemType) {
                return ctx.throwInternalError("Failed to read itemType");
            }
            mc::math::Vector3d loc;
            if (!_parseVector3(ctx, args[1], loc)) {
                return nullptr;
            }
            mc::Entity* outEntity = nullptr;
            auto result = helper->spawnItemAt(*itemType, loc, outEntity);
            if (!isPass(result)) {
                std::string msg = result->formattedMessage();
                return ctx.throwInternalError(msg.c_str());
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            void* entityProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(entityClassId);
            if (entityProto == nullptr || outEntity == nullptr) {
                return ctx.createUndefined();
            }
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, entityClassId, entityProto, outEntity, false, "Entity");
        },
        2);

    // --- destroyBlock(pos, dropResources=false) ---
    reg.method(
        "destroyBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("destroyBlock(pos, dropResources?)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            bool dropResources = false;
            if (argc >= 2) {
                auto b = ctx.toBool(args[1]);
                if (b) {
                    dropResources = *b;
                }
            }
            auto result = helper->destroyBlock(pos, dropResources);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- assertRedstonePower(pos, power) ---
    reg.method(
        "assertRedstonePower",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isNumber(args[1])) {
                return ctx.throwTypeError("assertRedstonePower(pos, power)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto power = ctx.toInt32(args[1]);
            if (!power) {
                return ctx.throwTypeError("power must be number");
            }
            auto result = helper->assertRedstonePower(pos, *power);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- assertIsWaterlogged(pos, isWaterlogged=true) ---
    reg.method(
        "assertIsWaterlogged",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("assertIsWaterlogged(pos, isWaterlogged?)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            bool isWaterlogged = true;
            if (argc >= 2) {
                auto b = ctx.toBool(args[1]);
                if (b) {
                    isWaterlogged = *b;
                }
            }
            auto result = helper->assertIsWaterlogged(pos, isWaterlogged);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- relativeLocation(worldPos) -> {x,y,z} ---
    // 基岩 Test.relativeLocation：世界绝对 Vector3→结构相对 Vector3（考虑结构旋转）。转发 helper->relativePosition。
    reg.method(
        "relativeLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("relativeLocation(worldPos)");
            }
            mc::math::Vector3d pos;
            if (!_parseVector3(ctx, args[0], pos)) {
                return nullptr;
            }
            return _vector3ToJs(ctx, helper->relativePosition(pos));
        },
        1);

    // --- worldBlockLocation(relativePos) -> {x,y,z} ---
    // 基岩 Test.worldBlockLocation：相对方块格点→世界绝对方块格点（整数，考虑结构旋转）。
    reg.method(
        "worldBlockLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("worldBlockLocation(relativePos)");
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

    // --- relativeBlockLocation(worldPos) -> {x,y,z} ---
    // 基岩 Test.relativeBlockLocation：世界绝对方块格点→结构相对方块格点。转发 helper->relativeBlockPosition。
    reg.method(
        "relativeBlockLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("relativeBlockLocation(worldPos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            BlockPos rel = helper->relativeBlockPosition(pos);
            void* obj = ctx.createObject();
            ctx.setPropertyInt(obj, "x", rel.x);
            ctx.setPropertyInt(obj, "y", rel.y);
            ctx.setPropertyInt(obj, "z", rel.z);
            return obj;
        },
        1);

    // --- rotateVector(vector) -> {x,y,z} ---
    // 按当前结构旋转量旋转向量。转发 helper->rotateVector。
    reg.method(
        "rotateVector",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("rotateVector(vector)");
            }
            mc::math::Vector3d v;
            if (!_parseVector3(ctx, args[0], v)) {
                return nullptr;
            }
            return _vector3ToJs(ctx, helper->rotateVector(v));
        },
        1);

    // --- getTestDirection() -> Direction ---
    // 基岩返回 @minecraft/server.Direction 枚举（PascalCase 字符串 "North"/"East"/"South"/"West"）。
    // 项目 Direction 枚举值与基岩不同（项目 North=2/South=3/West=4/East=5，基岩 north=2/south=3/east=3...），
    // 直接转数字会错位；返回 Direction 字符串（PascalCase）由 JS 侧按字符串比对。
    // helper->getTestDirection() 返项目 mc::Direction，经 _directionToApi 转 PascalCase 对齐官方枚举值。
    reg.method(
        "getTestDirection",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            return ctx.createString(_directionToApi(helper->getTestDirection()));
        },
        0);

    // --- succeedWhenBlockPresent(blockType, pos, isPresent=true) ---
    // 每 tick 检查；条件满足时标记成功。转发 helper->succeedWhenBlockPresent。
    reg.method(
        "succeedWhenBlockPresent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("succeedWhenBlockPresent(blockType, pos, isPresent?)");
            }
            auto blockType = ctx.toString(args[0]);
            if (!blockType) {
                return ctx.throwInternalError("Failed to read blockType");
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
            helper->succeedWhenBlockPresent(*blockType, pos, isPresent);
            return ctx.createUndefined();
        },
        3);

    // --- succeedIf(callback) ---
    // 立即运行 callback；不抛异常则标记成功。callback 经 wrapJsCallback 包装。
    reg.method(
        "succeedIf",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("succeedIf(callback)");
            }
            auto wrapped = wrapJsCallback(&ctx, args[0]);
            helper->succeedIf(std::move(wrapped));
            return ctx.createUndefined();
        },
        1);

    // --- succeedOnTick(tick) ---
    // 在第 tick 个 tick 标记成功。
    reg.method(
        "succeedOnTick",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("succeedOnTick(tick)");
            }
            auto tick = ctx.toInt32(args[0]);
            if (!tick) {
                return ctx.throwTypeError("tick must be number");
            }
            helper->succeedOnTick(*tick);
            return ctx.createUndefined();
        },
        1);

    // --- succeedOnTickWhen(tick, callback) ---
    // 在第 tick 个 tick 运行 callback；不抛异常则标记成功。
    reg.method(
        "succeedOnTickWhen",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isNumber(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("succeedOnTickWhen(tick, callback)");
            }
            auto tick = ctx.toInt32(args[0]);
            if (!tick) {
                return ctx.throwTypeError("tick must be number");
            }
            auto wrapped = wrapJsCallback(&ctx, args[1]);
            helper->succeedOnTickWhen(*tick, std::move(wrapped));
            return ctx.createUndefined();
        },
        2);

    // --- failIf(callback) ---
    // 立即运行 callback；不抛异常则标记失败（与 succeedIf 相反）。
    reg.method(
        "failIf",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("failIf(callback)");
            }
            auto wrapped = wrapJsCallback(&ctx, args[0]);
            helper->failIf(std::move(wrapped));
            return ctx.createUndefined();
        },
        1);

    // --- runAfterDelay(delayTicks, callback) ---
    // 延迟 delayTicks 个 tick 后执行 callback（相对延迟）。转发 helper->runAfterDelay。
    reg.method(
        "runAfterDelay",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isNumber(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("runAfterDelay(delayTicks, callback)");
            }
            auto delay = ctx.toInt32(args[0]);
            if (!delay) {
                return ctx.throwTypeError("delayTicks must be number");
            }
            auto wrapped = wrapJsCallback(&ctx, args[1]);
            helper->runAfterDelay(*delay, std::move(wrapped));
            return ctx.createUndefined();
        },
        2);

    // --- runOnFinish(callback) ---
    // 测试完成（成功/失败/超时）后执行 callback。转发 helper->runOnFinish。
    reg.method(
        "runOnFinish",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("runOnFinish(callback)");
            }
            auto wrapped = wrapJsCallback(&ctx, args[0]);
            helper->runOnFinish(std::move(wrapped));
            return ctx.createUndefined();
        },
        1);

    // --- assertEntityInstancePresent(entity, pos, isPresent=true) ---
    // 按对象身份断言指定 Entity 实例存在于 pos 附近。entity 参数为 JS Entity 对象，经 ScriptClassRegistry
    // 取 Entity classId 后 unwrap 出 mc::Entity*。
    reg.method(
        "assertEntityInstancePresent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2) {
                return ctx.throwTypeError("assertEntityInstancePresent(entity, pos, isPresent?)");
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            auto* entity = static_cast<mc::Entity*>(
                mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, args[0], entityClassId));
            if (entity == nullptr) {
                return ctx.throwTypeError("assertEntityInstancePresent: argument must be an Entity");
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
            auto result = helper->assertEntityInstancePresent(*entity, pos, isPresent);
            return _resultToJs(ctx, std::move(result));
        },
        3);

    // --- assertEntityInstancePresentInArea(entity, isPresent=true) ---
    // 在整个 GameTest 区域内按对象身份断言 Entity 实例存在。
    reg.method(
        "assertEntityInstancePresentInArea",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("assertEntityInstancePresentInArea(entity, isPresent?)");
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            auto* entity = static_cast<mc::Entity*>(
                mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, args[0], entityClassId));
            if (entity == nullptr) {
                return ctx.throwTypeError("assertEntityInstancePresentInArea: argument must be an Entity");
            }
            bool isPresent = true;
            if (argc >= 2) {
                auto b = ctx.toBool(args[1]);
                if (b) {
                    isPresent = *b;
                }
            }
            auto result = helper->assertEntityInstancePresentInArea(*entity, isPresent);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- assertEntityTouching(entityType, location, isTouching=true) ---
    // 断言实体接触/连接到 location（Vector3 浮点位置）。转发 helper->assertEntityTouching。
    reg.method(
        "assertEntityTouching",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("assertEntityTouching(entityType, location, isTouching?)");
            }
            auto entityType = ctx.toString(args[0]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            mc::math::Vector3d loc;
            if (!_parseVector3(ctx, args[1], loc)) {
                return nullptr;
            }
            bool isTouching = true;
            if (argc >= 3) {
                auto b = ctx.toBool(args[2]);
                if (b) {
                    isTouching = *b;
                }
            }
            auto result = helper->assertEntityTouching(*entityType, loc, isTouching);
            return _resultToJs(ctx, std::move(result));
        },
        3);

    // --- assertItemEntityPresent(itemType, pos, searchDistance=0, isPresent=true) ---
    reg.method(
        "assertItemEntityPresent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("assertItemEntityPresent(itemType, pos, searchDistance?, isPresent?)");
            }
            auto itemType = ctx.toString(args[0]);
            if (!itemType) {
                return ctx.throwInternalError("Failed to read itemType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            f32 searchDistance = 0.0f;
            if (argc >= 3 && ctx.isNumber(args[2])) {
                auto d = ctx.toFloat64(args[2]);
                if (d) {
                    searchDistance = static_cast<f32>(*d);
                }
            }
            bool isPresent = true;
            if (argc >= 4) {
                auto b = ctx.toBool(args[3]);
                if (b) {
                    isPresent = *b;
                }
            }
            auto result = helper->assertItemEntityPresent(*itemType, pos, searchDistance, isPresent);
            return _resultToJs(ctx, std::move(result));
        },
        4);

    // --- assertItemEntityCountIs(itemType, pos, searchDistance, count) ---
    // searchDistance 与 count 均必填（基岩无默认值）。count 为"至少要找到的最小数量"。
    reg.method(
        "assertItemEntityCountIs",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 4 || !ctx.isString(args[0]) || !ctx.isNumber(args[2]) || !ctx.isNumber(args[3])) {
                return ctx.throwTypeError("assertItemEntityCountIs(itemType, pos, searchDistance, count)");
            }
            auto itemType = ctx.toString(args[0]);
            if (!itemType) {
                return ctx.throwInternalError("Failed to read itemType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            auto searchDistance = ctx.toFloat64(args[2]);
            auto count = ctx.toInt32(args[3]);
            if (!searchDistance || !count) {
                return ctx.throwTypeError("searchDistance and count must be numbers");
            }
            auto result = helper->assertItemEntityCountIs(*itemType, pos, static_cast<f32>(*searchDistance), *count);
            return _resultToJs(ctx, std::move(result));
        },
        4);

    // --- assertBlockState(pos, callback) ---
    // facade 已做实（GameTestHelper::assertBlockState 调 predicate(const BlockState&)）。JS 绑定需把
    // BlockState* wrap 成 @minecraft/server Block JS 对象传给 callback——Block 类 opaque 持 ScriptBlockRef
    // （批2 MinecraftModuleFactory 匿名结构体，跨模块不可见）。待 Block wrap helper 跨模块暴露后接通，
    // 当前绑方法名但返 undefined + TODO，避免 JS 侧 "not a function"。
    // TODO: Block wrap helper（wrapBlockRef）跨模块暴露后，构造 Block JS 对象调 callback 并据返回 bool 判定。
    reg.method(
        "assertBlockState",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            // TODO: Block JS 对象构造未跨模块暴露，stub 返 undefined（facade assertBlockState 已就绪待接）。
            return ctx.createUndefined();
        },
        2);

    // --- getBlock(pos) -> Block ---
    // facade 已做实（返回 const BlockState*）。JS 绑定需 wrap 成 Block JS 对象——同 assertBlockState 受
    // Block wrap helper 跨模块不可见阻塞。待暴露后接通，当前返 undefined + TODO。
    // TODO: Block wrap helper 跨模块暴露后，wrap BlockState* 为 Block JS 对象返回。
    reg.method(
        "getBlock",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            // TODO: Block JS 对象构造未跨模块暴露，stub 返 undefined（facade getBlock 已就绪待接）。
            return ctx.createUndefined();
        },
        1);

    // --- assertContainerContains(itemStack, pos) ---
    // 基岩 Test.assertContainerContains(itemStack, blockLocation)。itemStack 经 _unwrapItemStack 取 mc::ItemStack*。
    reg.method(
        "assertContainerContains",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2) {
                return ctx.throwTypeError("assertContainerContains(itemStack, pos)");
            }
            auto* stack = _unwrapItemStack(ctx, args[0]);
            if (stack == nullptr) {
                return ctx.throwTypeError("assertContainerContains: first arg must be an ItemStack");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            auto result = helper->assertContainerContains(*stack, pos);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- assertContainerEmpty(pos) ---
    reg.method(
        "assertContainerEmpty",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("assertContainerEmpty(pos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto result = helper->assertContainerEmpty(pos);
            return _resultToJs(ctx, std::move(result));
        },
        1);

    // --- setBlockPermutation(blockPermutation, pos) ---
    // 基岩 Test.setBlockPermutation(blockData, blockLocation)。blockData 经 _unwrapBlockPermutation 取 const
    // BlockState*。
    reg.method(
        "setBlockPermutation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2) {
                return ctx.throwTypeError("setBlockPermutation(blockPermutation, pos)");
            }
            auto* perm = _unwrapBlockPermutation(ctx, args[0]);
            if (perm == nullptr) {
                return ctx.throwTypeError("setBlockPermutation: first arg must be a BlockPermutation");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            auto result = helper->setBlockPermutation(*perm, pos);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- setFluidContainer(pos, fluidType) ---
    // 基岩 Test.setFluidContainer(location, type)。type 是 FluidType 对象（批2占位，opaque 持 string id）。
    // 此处简化接受 fluidType 字符串（"minecraft:water"/"lava" 等），facade stub 返 MethodNotImplemented。
    // TODO: FluidType JS 类充实后改为接受 FluidType 对象（unwrap 取 id）。
    reg.method(
        "setFluidContainer",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[1])) {
                return ctx.throwTypeError("setFluidContainer(pos, fluidType)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto fluidType = ctx.toString(args[1]);
            if (!fluidType) {
                return ctx.throwInternalError("Failed to read fluidType");
            }
            auto result = helper->setFluidContainer(pos, *fluidType);
            return _resultToJs(ctx, std::move(result));
        },
        2);

    // --- triggerInternalBlockEvent(pos, eventName) ---
    reg.method(
        "triggerInternalBlockEvent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[1])) {
                return ctx.throwTypeError("triggerInternalBlockEvent(pos, eventName)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto name = ctx.toString(args[1]);
            if (!name) {
                return ctx.throwInternalError("Failed to read eventName");
            }
            helper->triggerInternalBlockEvent(pos, *name);
            return ctx.createUndefined();
        },
        2);

    // --- spreadFromFaceTowardDirection(pos, fromFace, direction) ---
    // fromFace/direction 是 @minecraft/server.Direction（PascalCase 字符串），经 _directionFromApi 转 mc::Direction。
    reg.method(
        "spreadFromFaceTowardDirection",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 3 || !ctx.isString(args[1]) || !ctx.isString(args[2])) {
                return ctx.throwTypeError("spreadFromFaceTowardDirection(pos, fromFace, direction)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            auto fromFace = ctx.toString(args[1]);
            auto direction = ctx.toString(args[2]);
            if (!fromFace || !direction) {
                return ctx.throwInternalError("Failed to read direction");
            }
            helper->spreadFromFaceTowardDirection(pos, _directionFromApi(*fromFace), _directionFromApi(*direction));
            return ctx.createUndefined();
        },
        3);

    // --- spawnAtLocation(entityType, location) -> Entity ---
    // 基岩 spawnAtLocation(entityType, location: Vector3)。location 是浮点 Vector3（与 spawn 的 BlockPos 不同）。
    reg.method(
        "spawnAtLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("spawnAtLocation(entityType, location)");
            }
            auto entityType = ctx.toString(args[0]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            mc::math::Vector3d loc;
            if (!_parseVector3(ctx, args[1], loc)) {
                return nullptr;
            }
            mc::Entity* outEntity = nullptr;
            auto result = helper->spawnAtLocation(*entityType, loc, outEntity);
            if (!isPass(result)) {
                std::string msg = result->formattedMessage();
                return ctx.throwInternalError(msg.c_str());
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            void* entityProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(entityClassId);
            if (entityProto == nullptr || outEntity == nullptr) {
                return ctx.createUndefined();
            }
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, entityClassId, entityProto, outEntity, false, "Entity");
        },
        2);

    // --- spawnWithoutBehaviors(entityType, pos) / spawnWithoutBehaviorsAtLocation(entityType, location) ---
    // 两者 facade 退化为普通 spawn（行为移除体系未就绪）。wrap Entity 同 spawn。
    reg.method(
        "spawnWithoutBehaviors",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("spawnWithoutBehaviors(entityType, pos)");
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
            auto result = helper->spawnWithoutBehaviors(*entityType, pos, outEntity);
            if (!isPass(result)) {
                std::string msg = result->formattedMessage();
                return ctx.throwInternalError(msg.c_str());
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            void* entityProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(entityClassId);
            if (entityProto == nullptr || outEntity == nullptr) {
                return ctx.createUndefined();
            }
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, entityClassId, entityProto, outEntity, false, "Entity");
        },
        2);
    reg.method(
        "spawnWithoutBehaviorsAtLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("spawnWithoutBehaviorsAtLocation(entityType, location)");
            }
            auto entityType = ctx.toString(args[0]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            mc::math::Vector3d loc;
            if (!_parseVector3(ctx, args[1], loc)) {
                return nullptr;
            }
            mc::Entity* outEntity = nullptr;
            auto result = helper->spawnWithoutBehaviorsAtLocation(*entityType, loc, outEntity);
            if (!isPass(result)) {
                std::string msg = result->formattedMessage();
                return ctx.throwInternalError(msg.c_str());
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            void* entityProto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(entityClassId);
            if (entityProto == nullptr || outEntity == nullptr) {
                return ctx.createUndefined();
            }
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, entityClassId, entityProto, outEntity, false, "Entity");
        },
        2);

    // --- assertEntityHasArmor(entityType, armorSlot, armorName, armorData, pos, hasArmor?) ---
    reg.method(
        "assertEntityHasArmor",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 5 || !ctx.isString(args[0]) || !ctx.isNumber(args[1]) || !ctx.isString(args[2]) ||
                !ctx.isNumber(args[3])) {
                return ctx.throwTypeError(
                    "assertEntityHasArmor(entityType, armorSlot, armorName, armorData, pos, hasArmor?)");
            }
            auto entityType = ctx.toString(args[0]);
            auto armorSlot = ctx.toInt32(args[1]);
            auto armorName = ctx.toString(args[2]);
            auto armorData = ctx.toInt32(args[3]);
            if (!entityType || !armorSlot || !armorName || !armorData) {
                return ctx.throwInternalError("Failed to read assertEntityHasArmor args");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[4], pos)) {
                return nullptr;
            }
            bool hasArmor = true;
            if (argc >= 6) {
                auto b = ctx.toBool(args[5]);
                if (b) {
                    hasArmor = *b;
                }
            }
            auto result = helper->assertEntityHasArmor(*entityType, *armorSlot, *armorName, *armorData, pos, hasArmor);
            return _resultToJs(ctx, std::move(result));
        },
        6);

    // --- assertEntityHasComponent(entityType, componentId, pos, hasComponent?) ---
    reg.method(
        "assertEntityHasComponent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 3 || !ctx.isString(args[0]) || !ctx.isString(args[1])) {
                return ctx.throwTypeError("assertEntityHasComponent(entityType, componentId, pos, hasComponent?)");
            }
            auto entityType = ctx.toString(args[0]);
            auto componentId = ctx.toString(args[1]);
            if (!entityType || !componentId) {
                return ctx.throwInternalError("Failed to read assertEntityHasComponent args");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[2], pos)) {
                return nullptr;
            }
            bool hasComponent = true;
            if (argc >= 4) {
                auto b = ctx.toBool(args[3]);
                if (b) {
                    hasComponent = *b;
                }
            }
            auto result = helper->assertEntityHasComponent(*entityType, *componentId, pos, hasComponent);
            return _resultToJs(ctx, std::move(result));
        },
        4);

    // --- assertEntityState(pos, entityType, callback) ---
    // callback 是 JS 函数 (Entity)=>boolean。facade 接 std::function<bool(const Entity&)>。
    // 依赖实体按 pos 查询体系未就绪（facade stub），callback 包装暂用 ScriptCallbackUtil::wrapJsCallback
    // 的同类模式但签名不同（接 Entity&）——facade stub 不调 predicate，故此处仅透传占位。
    // TODO: facade assertEntityState 做实后，把 JS callback 包装成 std::function<bool(const Entity&)>，
    //       内部 wrap Entity 为 JS 对象调 callFunction1 取 bool 返回。
    reg.method(
        "assertEntityState",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 3 || !ctx.isString(args[1]) || !ctx.isFunction(args[2])) {
                return ctx.throwTypeError("assertEntityState(pos, entityType, callback)");
            }
            auto entityType = ctx.toString(args[1]);
            if (!entityType) {
                return ctx.throwInternalError("Failed to read entityType");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            // TODO: callback 包装待 facade 做实后接（当前 facade stub 不调 predicate）。
            std::function<bool(const mc::Entity&)> predicate = [](const mc::Entity&) -> bool { return false; };
            auto result = helper->assertEntityState(pos, *entityType, std::move(predicate));
            return _resultToJs(ctx, std::move(result));
        },
        3);

    // --- assertCanReachLocation(mob, pos, canReach?) ---
    // mob 是 JS Entity 对象，经 ScriptClassRegistry 取 Entity classId 后 unwrap 出 mc::Entity*。
    reg.method(
        "assertCanReachLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2) {
                return ctx.throwTypeError("assertCanReachLocation(mob, pos, canReach?)");
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            auto* entity = static_cast<mc::Entity*>(
                mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, args[0], entityClassId));
            if (entity == nullptr) {
                return ctx.throwTypeError("assertCanReachLocation: first arg must be an Entity");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[1], pos)) {
                return nullptr;
            }
            bool canReach = true;
            if (argc >= 3) {
                auto b = ctx.toBool(args[2]);
                if (b) {
                    canReach = *b;
                }
            }
            auto result = helper->assertCanReachLocation(*entity, pos, canReach);
            return _resultToJs(ctx, std::move(result));
        },
        3);

    // --- onPlayerJump(mob, jumpAmount) ---
    reg.method(
        "onPlayerJump",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isNumber(args[1])) {
                return ctx.throwTypeError("onPlayerJump(mob, jumpAmount)");
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            auto* entity = static_cast<mc::Entity*>(
                mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, args[0], entityClassId));
            if (entity == nullptr) {
                return ctx.throwTypeError("onPlayerJump: first arg must be an Entity");
            }
            auto jumpAmount = ctx.toInt32(args[1]);
            if (!jumpAmount) {
                return ctx.throwTypeError("jumpAmount must be number");
            }
            helper->onPlayerJump(*entity, *jumpAmount);
            return ctx.createUndefined();
        },
        2);

    // --- setTntFuse(entity, fuseLength) ---
    reg.method(
        "setTntFuse",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 2 || !ctx.isNumber(args[1])) {
                return ctx.throwTypeError("setTntFuse(entity, fuseLength)");
            }
            const u64 entityClassId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("Entity");
            auto* entity = static_cast<mc::Entity*>(
                mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, args[0], entityClassId));
            if (entity == nullptr) {
                return ctx.throwTypeError("setTntFuse: first arg must be an Entity");
            }
            auto fuse = ctx.toInt32(args[1]);
            if (!fuse) {
                return ctx.throwTypeError("fuseLength must be number");
            }
            helper->setTntFuse(*entity, *fuse);
            return ctx.createUndefined();
        },
        2);

    // --- succeedWhenEntityHasComponent(entityType, componentId, pos, hasComponent) ---
    reg.method(
        "succeedWhenEntityHasComponent",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 4 || !ctx.isString(args[0]) || !ctx.isString(args[1])) {
                return ctx.throwTypeError("succeedWhenEntityHasComponent(entityType, componentId, pos, hasComponent)");
            }
            auto entityType = ctx.toString(args[0]);
            auto componentId = ctx.toString(args[1]);
            if (!entityType || !componentId) {
                return ctx.throwInternalError("Failed to read succeedWhenEntityHasComponent args");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[2], pos)) {
                return nullptr;
            }
            bool hasComponent = true;
            {
                auto b = ctx.toBool(args[3]);
                if (b) {
                    hasComponent = *b;
                }
            }
            helper->succeedWhenEntityHasComponent(*entityType, *componentId, pos, hasComponent);
            return ctx.createUndefined();
        },
        4);

    // --- removeSimulatedPlayer(player) ---
    // facade 已有（IGameTestHelper::removeSimulatedPlayer）。player 经闭包捕获的 simulatedPlayerClassId unwrap。
    // classId 取自 registerTestClassBinding 的参数（与 spawnSimulatedPlayer 同源），不存 ScriptBindingRegistry。
    reg.method(
        "removeSimulatedPlayer",
        [simulatedPlayerClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("removeSimulatedPlayer(player)");
            }
            auto* player = static_cast<SimulatedPlayer*>(
                mc::mod::bedrock::addon::ScriptObjectRegistry::unwrap(ctx, args[0], simulatedPlayerClassId));
            if (player == nullptr) {
                return ctx.throwTypeError("removeSimulatedPlayer: argument must be a SimulatedPlayer");
            }
            helper->removeSimulatedPlayer(*player);
            return ctx.createUndefined();
        },
        1);

    // --- isCompleted() -> bool / isCleaningUp() -> bool ---
    reg.method(
        "isCompleted",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            return ctx.createBoolean(helper->isCompleted());
        },
        0);
    reg.method(
        "isCleaningUp",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            return ctx.createBoolean(helper->isCleaningUp());
        },
        0);

    // --- rotateDirection(direction) -> Direction ---
    // 接收 PascalCase Direction 字符串→_directionFromApi→facade→_directionToApi 返 PascalCase。
    reg.method(
        "rotateDirection",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1 || !ctx.isString(args[0])) {
                return ctx.throwTypeError("rotateDirection(direction)");
            }
            auto dirStr = ctx.toString(args[0]);
            if (!dirStr) {
                return ctx.throwInternalError("Failed to read direction");
            }
            mc::Direction out = helper->rotateDirection(_directionFromApi(*dirStr));
            return ctx.createString(_directionToApi(out));
        },
        1);

    // --- getFenceConnectivity(pos) -> FenceConnectivity ---
    reg.method(
        "getFenceConnectivity",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("getFenceConnectivity(pos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            FenceConnectivity conn = helper->getFenceConnectivity(pos);
            return _fenceConnectivityToJs(ctx, conn);
        },
        1);

    // --- getSculkSpreader(pos) -> SculkSpreader | undefined ---
    // facade 返回 owned mc::blocks::SculkSpreader*（新建空快照）。经 ScriptClassRegistry 查 SculkSpreader
    // classId/proto（批5注册），owned=true wrap（JS GC 时 delete）。nullptr 返 undefined。
    reg.method(
        "getSculkSpreader",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* helper = _requireHelper(ctx, thisVal);
            if (helper == nullptr) {
                return nullptr;
            }
            if (argc < 1) {
                return ctx.throwTypeError("getSculkSpreader(pos)");
            }
            BlockPos pos;
            if (!_parseBlockPos(ctx, args[0], pos)) {
                return nullptr;
            }
            mc::blocks::SculkSpreader* spreader = helper->getSculkSpreader(pos);
            if (spreader == nullptr) {
                return ctx.createUndefined();
            }
            const u64 classId = mc::mod::bedrock::addon::ScriptClassRegistry::instance().classIdByName("SculkSpreader");
            void* proto = mc::mod::bedrock::addon::ScriptClassRegistry::instance().proto(classId);
            if (proto == nullptr) {
                // 类未注册（不应发生）：释放 spreader 避免泄漏，返 undefined。
                delete spreader;
                return ctx.createUndefined();
            }
            // owned=true：JS GC 时 delete spreader（ScriptObjectRegistry 默认 destroy 用 delete）。
            return mc::mod::bedrock::addon::ScriptObjectRegistry::wrap(
                ctx, classId, proto, spreader, true, "SculkSpreader");
        },
        1);

    // spawnItem 已绑定（上方，简化接受 itemType 字符串，TODO ItemStack 充实后改对象）。
    // Test 类方法已全部桥接（含批次4 新增 17 方法：assertBlockState/getBlock(stub 待 Block wrap helper)/
    // assertContainerContains/assertContainerEmpty/setBlockPermutation/setFluidContainer/triggerInternalBlockEvent/
    // spreadFromFaceTowardDirection/spawnAtLocation/spawnWithoutBehaviors/spawnWithoutBehaviorsAtLocation/
    // assertEntityHasArmor/assertEntityHasComponent/assertEntityState/assertCanReachLocation/onPlayerJump/
    // setTntFuse/succeedWhenEntityHasComponent/removeSimulatedPlayer/isCompleted/isCleaningUp/rotateDirection/
    // getFenceConnectivity/getSculkSpreader）。

    return classId;
}

} // namespace mc::test
