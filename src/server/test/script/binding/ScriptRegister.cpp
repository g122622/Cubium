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
#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "server/test/script/binding/ScriptRegistrationBuilder.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"

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
    ctx.retainValue(args[2]);
    auto* cppBuilder = new ScriptRegistrationBuilder(*className, *testName, &ctx, args[2]);

    void* proto = ScriptBindingRegistry::instance().proto(registrationBuilderClassId);
    // owned=true + 自定义 destroy：GC 时 _destroyRegistrationBuilder 提交测试后 delete。
    return ScriptObjectRegistry::wrap(ctx,
        registrationBuilderClassId,
        proto,
        cppBuilder,
        /*owned=*/true,
        "RegistrationBuilder",
        _destroyRegistrationBuilder);
}

} // namespace

void registerTopLevelFunctions(mc::mod::bedrock::addon::NativeModuleBuilder& builder,
    mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    u64 registrationBuilderClassId)
{
    // 注册 `GameTest` 命名空间对象（对齐 @minecraft/server 的 system/world 全局对象模式）。
    // TODO: 官方 API 是模块顶层自由函数 `register(...)`，本骨架因 exportNativeFunction 不支持捕获状态
    //       的 std::function 而暂用命名空间对象 `gametest.register(...)`，待绑定基础设施扩展后迁回。
    u64 namespaceClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* namespaceProto = builder.exportClass("GameTest", namespaceClassId);

    ClassRegistrar<void> nsReg(ctx, namespaceClassId, namespaceProto);

    // --- register(testClassName, testName, fn) -> RegistrationBuilder ---
    nsReg.method(
        "register",
        [registrationBuilderClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            return _doRegister(ctx, argc, args, registrationBuilderClassId);
        },
        3);

    // --- registerAsync(testClassName, testName, fn) -> RegistrationBuilder ---
    // TODO: 异步语义（Promise 轮询）未实现，当前按同步注册（与 register 等价）。
    nsReg.method(
        "registerAsync",
        [registrationBuilderClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 argc, void** args) -> void* {
            return _doRegister(ctx, argc, args, registrationBuilderClassId);
        },
        3);

    // --- setBeforeBatchCallback(batchName, fn) ---
    // TODO: 批次回调经 GameTestRegistry.registerBeforeBatchFunction 注册，需保留 JS 回调并在批次开始触发。
    //       第一阶段为骨架，暂不接线（throw InternalError 由调用方感知）。
    nsReg.method(
        "setBeforeBatchCallback",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return ctx.throwInternalError("setBeforeBatchCallback not implemented yet"); },
        2);

    // --- setAfterBatchCallback(batchName, fn) ---
    // TODO: 同 setBeforeBatchCallback。
    nsReg.method(
        "setAfterBatchCallback",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return ctx.throwInternalError("setAfterBatchCallback not implemented yet"); },
        2);

    // --- spawnSimulatedPlayer(name, location) -> SimulatedPlayer ---
    // TODO: 顶层 spawn 需当前测试上下文（与 Test.spawnSimulatedPlayer 不同，无 Test 对象），待接线。
    nsReg.method(
        "spawnSimulatedPlayer",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* { return ctx.throwInternalError("spawnSimulatedPlayer not implemented yet"); },
        2);

    // 导出 GameTest 命名空间对象实例为模块顶层名 `gametest`。
    void* namespaceObj = ScriptObjectRegistry::wrap(ctx, namespaceClassId, namespaceProto, nullptr, false, "GameTest");
    builder.exportValue("gametest", namespaceObj);
    ctx.releaseValue(namespaceObj);
}

} // namespace mc::test
