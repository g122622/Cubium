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

#include "server/test/script/binding/ScriptRegister.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp" // ScriptObjectRegistry/ClassRegistrar
#include "common/test/base/error/GameTestErrorType.hpp"            // GameTestErrorType::MethodNotImplemented
#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "server/test/script/binding/ScriptGameTestError.hpp" // throwGameTestError（spawnSimulatedPlayer stub）
#include "server/test/script/binding/ScriptRegistrationBuilder.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"

#include <functional> // std::function（批次回调 lambda 转 C++ 回调）
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {

// 默认结构名（JS 作者未设 structureName 时用）。
constexpr const char* _kDefaultStructure = "gametest:empty_3x3";

// RegistrationBuilder JS 对象的销毁回调：GC 时先提交测试到注册表，再 delete C++ builder。
// 对齐 JS 文档：`gametest.register(...).structureName(...).maxTicks(...)` 链式末尾无显式提交，由 GC 触发。
void _destroyRegistrationBuilder(void* ptr)
{
    auto* b = static_cast<ScriptRegistrationBuilder*>(ptr);
    if (b == nullptr) {
        return;
    }
    if (!b->registerTest(_kDefaultStructure)) {
        // 同名已存在不视为致命（JS 可能重复注册），记 warn。
        spdlog::warn("[GameTest] Script test registration rejected (duplicate name)");
    }
    delete b;
}

// 共用的 register/registerAsync 实现。
void* _doRegister(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, i32 argc, void** args, u64 registrationBuilderClassId)
{
    if (argc < 3 || !ctx.isString(args[0]) || !ctx.isString(args[1]) || !ctx.isFunction(args[2])) {
        return ctx.throwTypeError("register(testClassName, testName, fn)");
    }
    auto className = ctx.toString(args[0]);
    auto testName = ctx.toString(args[1]);
    if (!className || !testName) {
        return ctx.throwInternalError("Failed to read test class/name");
    }
    // 保留 JS 回调，生命周期跟随 ScriptGameTestFunction（其析构 releaseValue）。
    // 须用 dupValue（新建独立 JSValue handle）而非 retainValue（仅 refcount+1，不新建 handle）：
    // trampoline 在 _doRegister 返回后会 delete arg handle 内存，retainValue 持的 void* 遂成悬垂指针，
    // 后续 isFunction/callFunction1 解引用 UB（表现为 "TypeError: not a function"）。
    // dupValue 新建独立 handle，trampoline 释放 arg handle 不影响本句柄。
    void* jsCallback = ctx.dupValue(args[2]);
    auto* cppBuilder = new ScriptRegistrationBuilder(*className, *testName, &ctx, jsCallback);

    void* proto = ScriptBindingRegistry::instance().proto(registrationBuilderClassId);
    // owned=true + 自定义 destroy：GC 时 _destroyRegistrationBuilder 提交测试后 delete。
    void* wrapped = ScriptObjectRegistry::wrap(ctx,
        registrationBuilderClassId,
        proto,
        cppBuilder,
        /*owned=*/true,
        "RegistrationBuilder",
        _destroyRegistrationBuilder);
    return wrapped;
}

} // namespace

void registerTopLevelFunctions(mc::mod::bedrock::addon::NativeModuleBuilder& builder,
    mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    u64 registrationBuilderClassId)
{
    // 对齐基岩官方 @minecraft/server-gametest API：register/registerAsync 等为模块顶层命名导出，
    // JS 侧 `import { register } from "@minecraft/server-gametest"` 或 `import * as GameTest` 后
    // `GameTest.register(...)` 均可。此前用 `gametest` 命名空间对象包裹与官方不符，且使
    // `import * as GameTest; GameTest.register` 取到 undefined（register 挂在 gametest 对象上而非模块顶层）。
    // 改用 createFunction（支持 std::function 捕获 registrationBuilderClassId）+ exportValue 顶层导出。

    // --- register(testClassName, testName, fn) -> RegistrationBuilder ---
    void* registerFn = ctx.createFunction(
        [registrationBuilderClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            return _doRegister(ctx, argc, args, registrationBuilderClassId);
        },
        "register",
        3);
    builder.exportValue("register", registerFn);
    ctx.releaseValue(registerFn);

    // --- registerAsync(testClassName, testName, fn) -> RegistrationBuilder ---
    // registerAsync 与 register 统一走 _doRegister：JS 体返回 Promise 时由 ScriptGameTestFunction::run
    // 检测 isPromise 并返回 ScriptAsyncGameTestRunResult（轮询 Promise 状态）。二者皆允许 JS 体返回
    // Promise 或普通值，区别仅在文档语义（registerAsync 明示作者意图用 async/await）。
    void* registerAsyncFn = ctx.createFunction(
        [registrationBuilderClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            return _doRegister(ctx, argc, args, registrationBuilderClassId);
        },
        "registerAsync",
        3);
    builder.exportValue("registerAsync", registerAsyncFn);
    ctx.releaseValue(registerAsyncFn);

    // --- setBeforeBatchCallback(batchName, fn) ---
    // 批次前置回调：把 JS 函数注册到 GameTestRegistry，由 BaseGameTestBatchRunner 在批次开始时触发。
    // 触发链已通（GameTestServer 取 getBeforeBatchFunction 注入 GameTestBatch，runner 调 batch.beforeBatch()()），
    // 本处只补 JS→C++ 桥接。
    void* setBeforeBatchFn = ctx.createFunction(
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            if (argc < 2 || !ctx.isString(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("setBeforeBatchCallback(batchName, fn)");
            }
            auto batchName = ctx.toString(args[0]);
            if (!batchName) {
                return ctx.throwInternalError("Failed to read batch name");
            }
            // dupValue 保留独立 JS 句柄（trampoline 在回调返回后会释放 args[1] handle，dupValue 不受影响）。
            void* fnHandle = ctx.dupValue(args[1]);
            // 构造 C++ 回调：捕获 ctx 指针 + fn 句柄，触发时 callFunction0 调 JS 函数。
            // JS 异常记 warn 不中断批次（对齐 ScriptGameTestFunction::run 的异常处理语义）。
            auto* ctxPtr = &ctx;
            std::function<void()> cb = [ctxPtr, fnHandle]() {
                void* undef = ctxPtr->createUndefined();
                void* ret = ctxPtr->callFunction0(fnHandle, undef);
                ctxPtr->releaseValue(undef);
                if (ctxPtr->isException(ret)) {
                    void* exc = ctxPtr->getException();
                    auto msg = ctxPtr->getExceptionMessage(exc);
                    ctxPtr->releaseValue(exc);
                    spdlog::warn("[GameTest] beforeBatch callback threw: {}", msg);
                }
                ctxPtr->releaseValue(ret); // callFunction0 返回 owned 句柄，须 release
            };
            GameTestRegistry::instance().registerBeforeBatchFunction(*batchName, std::move(cb));
            return ctx.createUndefined();
        },
        "setBeforeBatchCallback",
        2);
    builder.exportValue("setBeforeBatchCallback", setBeforeBatchFn);
    ctx.releaseValue(setBeforeBatchFn);

    // --- setAfterBatchCallback(batchName, fn) ---
    // 同 setBeforeBatchCallback，注册到 afterBatch（批次结束时触发）。
    void* setAfterBatchFn = ctx.createFunction(
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            if (argc < 2 || !ctx.isString(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("setAfterBatchCallback(batchName, fn)");
            }
            auto batchName = ctx.toString(args[0]);
            if (!batchName) {
                return ctx.throwInternalError("Failed to read batch name");
            }
            void* fnHandle = ctx.dupValue(args[1]);
            auto* ctxPtr = &ctx;
            std::function<void()> cb = [ctxPtr, fnHandle]() {
                void* undef = ctxPtr->createUndefined();
                void* ret = ctxPtr->callFunction0(fnHandle, undef);
                ctxPtr->releaseValue(undef);
                if (ctxPtr->isException(ret)) {
                    void* exc = ctxPtr->getException();
                    auto msg = ctxPtr->getExceptionMessage(exc);
                    ctxPtr->releaseValue(exc);
                    spdlog::warn("[GameTest] afterBatch callback threw: {}", msg);
                }
                ctxPtr->releaseValue(ret);
            };
            GameTestRegistry::instance().registerAfterBatchFunction(*batchName, std::move(cb));
            return ctx.createUndefined();
        },
        "setAfterBatchCallback",
        2);
    builder.exportValue("setAfterBatchCallback", setAfterBatchFn);
    ctx.releaseValue(setAfterBatchFn);
    // TODO: fn 句柄（dupValue 的 owned handle）未显式 release——底层 JS 对象由 JS_FreeRuntime 引擎销毁时全清，
    //       但 handle 的 C++ 内存（new JSValue）极小泄漏（每次注册 16 字节；行为包极少用此 API）。
    //       主动 release 会面临 GameTestRegistry 单例析构（进程退出）时 ctx 已死的崩溃风险，故暂不释放；
    //       待批次回调生命周期管理完善（按 environment 分组 + 接入 releaseAllScriptResources 显式释放链）后补全。
    // TODO: 当前只有 "default" 一个批次（GameTestServer 未做按 environment 分组），其他 batchName 回调注册但不触发。

    // --- spawnSimulatedPlayer(name, location) -> SimulatedPlayer ---
    // 顶层 spawn 无 Test 上下文（与 Test.spawnSimulatedPlayer 不同，无 Test 对象定位结构原点），
    // 项目 GameTest 框架的 SimulatedPlayer::spawn 需 GameTestHelper（结构相对坐标→世界绝对），
    // 顶层无此上下文故 stub。对齐基岩 throwGameTestError(MethodNotImplemented)（非 throwInternalError）。
    // TODO: 顶层 spawn 体系（全局结构原点/世界坐标直接传入）实现后接通。
    void* spawnSimulatedPlayerFn = ctx.createFunction(
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* {
            return throwGameTestError(ctx,
                GameTestErrorType::MethodNotImplemented,
                "spawnSimulatedPlayer not implemented yet (no top-level test context for structure origin)");
        },
        "spawnSimulatedPlayer",
        2);
    builder.exportValue("spawnSimulatedPlayer", spawnSimulatedPlayerFn);
    ctx.releaseValue(spawnSimulatedPlayerFn);
}

} // namespace mc::test
