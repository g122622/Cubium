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

#include "server/test/script/binding/ScriptSequence.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp" // ScriptObjectRegistry/ClassRegistrar
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "server/test/facade/GameTestHelper.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"
#include "server/test/script/context/ScriptGameTestAccessor.hpp"

#include <functional>
#include <utility>

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {

// 把 JS 回调包装为 std::function<GameTestResult()>：调 callFunction0，映射异常为 GameTestError。
// ctxPtr/jsCb 须在回调执行时仍有效（序列步骤执行时脚本上下文与 JS 函数均存活）。
std::function<GameTestResult()> _wrapJsCallback(mc::mod::bedrock::addon::IScriptBindingContext* ctxPtr, void* jsCb)
{
    return [ctxPtr, jsCb]() -> GameTestResult {
        void* undef = ctxPtr->createUndefined();
        void* ret = ctxPtr->callFunction0(jsCb, undef);
        ctxPtr->releaseValue(undef);

        GameTestResult result = pass();
        if (ctxPtr->isException(ret)) {
            void* exc = ctxPtr->getException();
            auto msg = ctxPtr->getExceptionMessage(exc);
            ctxPtr->releaseValue(exc);
            result = fail(GameTestErrorType::FailConditionsMet, std::string(msg));
        }
        ctxPtr->releaseValue(ret);
        return result;
    };
}

} // namespace

u64 registerSequenceClassBinding(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx)
{
    u64 classId = ScriptObjectRegistry::allocateClassId(ctx);
    void* proto = builder.exportClass("GameTestSequence", classId);
    ScriptBindingRegistry::instance().registerProto(classId, proto);

    ClassRegistrar<void> reg(ctx, classId, proto);

    // thenExecute(fn)
    reg.method(
        "thenExecute",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("thenExecute requires a function argument");
            }
            ctx.retainValue(args[0]);
            seq->thenExecute(_wrapJsCallback(&ctx, args[0]));
            return thisVal; // 链式返回自身
        },
        1);

    // thenExecuteAfter(delay, fn)
    reg.method(
        "thenExecuteAfter",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            if (argc < 2 || !ctx.isNumber(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("thenExecuteAfter(delay, fn)");
            }
            auto delay = ctx.toInt32(args[0]);
            if (!delay) {
                return ctx.throwTypeError("delay must be number");
            }
            ctx.retainValue(args[1]);
            seq->thenExecuteAfter(*delay, _wrapJsCallback(&ctx, args[1]));
            return thisVal;
        },
        2);

    // thenExecuteFor(tickCount, fn)
    reg.method(
        "thenExecuteFor",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            if (argc < 2 || !ctx.isNumber(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("thenExecuteFor(tickCount, fn)");
            }
            auto tc = ctx.toInt32(args[0]);
            if (!tc) {
                return ctx.throwTypeError("tickCount must be number");
            }
            ctx.retainValue(args[1]);
            seq->thenExecuteFor(*tc, _wrapJsCallback(&ctx, args[1]));
            return thisVal;
        },
        2);

    // thenIdle(delayTicks)
    reg.method(
        "thenIdle",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            if (argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.throwTypeError("thenIdle(delayTicks)");
            }
            auto d = ctx.toInt32(args[0]);
            if (!d) {
                return ctx.throwTypeError("delayTicks must be number");
            }
            seq->thenIdle(*d);
            return thisVal;
        },
        1);

    // thenSucceed()
    reg.method(
        "thenSucceed",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 /*argc*/, void** /*args*/) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            seq->thenSucceed();
            return thisVal;
        },
        0);

    // thenFail(errorText)
    reg.method(
        "thenFail",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            std::string msg = "Sequence failed";
            if (argc >= 1 && ctx.isString(args[0])) {
                auto m = ctx.toString(args[0]);
                if (m) {
                    msg = *m;
                }
            }
            seq->thenFail(GameTestError(GameTestErrorType::FailConditionsMet, std::move(msg)));
            return thisVal;
        },
        1);

    // TODO: thenWait/thenWaitAfter 依赖事件总线桥接的异步轮询语义，第一阶段未实现（throw NotImplementedError）。
    // thenTrigger（SequenceCondition）依赖 thenTrigger 消费方的桥接，同样留 TODO。

    return classId;
}

void* wrapSequence(mc::mod::bedrock::addon::IScriptBindingContext& ctx, u64 classId, GameTestSequence* seq)
{
    if (seq == nullptr) {
        return ctx.createNull();
    }
    // 非拥有：原生序列由 GameTestHelper 拥有，测试运行期间稳定；JS 对象仅持指针引用。
    // 原型句柄从 ScriptBindingRegistry 取（注册期存入）。
    void* proto = ScriptBindingRegistry::instance().proto(classId);
    return ScriptObjectRegistry::wrap(ctx, classId, proto, seq, /*owned=*/false, "GameTestSequence");
}

} // namespace mc::test
