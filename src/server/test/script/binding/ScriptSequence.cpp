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
#include "server/test/script/binding/ScriptCallbackUtil.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"

#include <functional>
#include <utility>

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {
// _wrapJsCallback 已提取到 ScriptCallbackUtil.hpp::wrapJsCallback，供 ScriptTestHelper::until 与本文件复用。
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
            seq->thenExecute(wrapJsCallback(&ctx, args[0]));
            return ctx.dupValue(thisVal); // 链式返回自身（Dup 独立句柄，避免 trampoline 释放 thisHandle 后悬垂）
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
            seq->thenExecuteAfter(*delay, wrapJsCallback(&ctx, args[1]));
            return ctx.dupValue(thisVal);
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
            seq->thenExecuteFor(*tc, wrapJsCallback(&ctx, args[1]));
            return ctx.dupValue(thisVal);
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
            return ctx.dupValue(thisVal);
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
            return ctx.dupValue(thisVal);
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
            return ctx.dupValue(thisVal);
        },
        1);

    // thenWait(fn)：从当前 tick 起每 tick 轮询 fn，直到返回通过才进入下一步。
    // 转发原生 GameTestSequence::thenWait（Wait 步骤已实现轮询语义）。
    reg.method(
        "thenWait",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("thenWait requires a function argument");
            }
            seq->thenWait(wrapJsCallback(&ctx, args[0]));
            return ctx.dupValue(thisVal); // 链式返回自身
        },
        1);

    // thenWaitAfter(delay, fn)：延迟 delay tick 后开始每 tick 轮询。
    reg.method(
        "thenWaitAfter",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* seq = static_cast<GameTestSequence*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
            if (seq == nullptr) {
                return ctx.throwTypeError("Invalid GameTestSequence");
            }
            if (argc < 2 || !ctx.isNumber(args[0]) || !ctx.isFunction(args[1])) {
                return ctx.throwTypeError("thenWaitAfter(delay, fn)");
            }
            auto delay = ctx.toInt32(args[0]);
            if (!delay) {
                return ctx.throwTypeError("delay must be number");
            }
            seq->thenWaitAfter(*delay, wrapJsCallback(&ctx, args[1]));
            return ctx.dupValue(thisVal);
        },
        2);

    // TODO: thenTrigger（SequenceCondition）依赖 thenTrigger 消费方的桥接，待按需补全。

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
