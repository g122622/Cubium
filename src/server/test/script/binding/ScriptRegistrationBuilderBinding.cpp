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

#include "server/test/script/binding/ScriptRegistrationBuilderBinding.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp" // ScriptObjectRegistry/ClassRegistrar
#include "server/test/script/binding/ScriptRegistrationBuilder.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"

#include <string>
#include <utility>

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {

// 取当前 JS builder 对象持有的 C++ ScriptRegistrationBuilder*；失败 throw。
ScriptRegistrationBuilder* _requireBuilder(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal)
{
    auto* b = static_cast<ScriptRegistrationBuilder*>(ScriptObjectRegistry::unwrap(ctx, thisVal));
    if (b == nullptr) {
        // JS 侧已抛异常；C++ 控制流继续返回 nullptr 占位，调用方须判空并 return。
        static_cast<void>(ctx.throwTypeError("Invalid RegistrationBuilder"));
    }
    return b;
}

// 链式方法统一返回 thisVal（JS builder 对象自身）以维持链式。
void* _chainReturn(void* thisVal)
{
    return thisVal;
}

// 通用单 string 链式方法辅助。
void* _applyStringChain(mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    void* thisVal,
    i32 argc,
    void** args,
    ScriptRegistrationBuilder& (ScriptRegistrationBuilder::*method)(std::string))
{
    auto* b = _requireBuilder(ctx, thisVal);
    if (b == nullptr) {
        return nullptr;
    }
    if (argc < 1 || !ctx.isString(args[0])) {
        return ctx.throwTypeError("expected a string argument");
    }
    auto s = ctx.toString(args[0]);
    if (!s) {
        return ctx.throwInternalError("Failed to read string argument");
    }
    (b->*method)(std::move(*s));
    return _chainReturn(thisVal);
}

// 通用单 i32 链式方法辅助。
void* _applyIntChain(mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    void* thisVal,
    i32 argc,
    void** args,
    ScriptRegistrationBuilder& (ScriptRegistrationBuilder::*method)(i32) noexcept)
{
    auto* b = _requireBuilder(ctx, thisVal);
    if (b == nullptr) {
        return nullptr;
    }
    if (argc < 1 || !ctx.isNumber(args[0])) {
        return ctx.throwTypeError("expected a number argument");
    }
    auto n = ctx.toInt32(args[0]);
    if (!n) {
        return ctx.throwTypeError("expected a finite integer");
    }
    (b->*method)(*n);
    return _chainReturn(thisVal);
}

// 通用单 bool 链式方法辅助。
void* _applyBoolChain(mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    void* thisVal,
    i32 argc,
    void** args,
    ScriptRegistrationBuilder& (ScriptRegistrationBuilder::*method)(bool) noexcept)
{
    auto* b = _requireBuilder(ctx, thisVal);
    if (b == nullptr) {
        return nullptr;
    }
    if (argc < 1) {
        return ctx.throwTypeError("expected a boolean argument");
    }
    auto v = ctx.toBool(args[0]);
    if (!v) {
        return ctx.throwTypeError("expected a boolean");
    }
    (b->*method)(*v);
    return _chainReturn(thisVal);
}

} // namespace

u64 registerRegistrationBuilderClassBinding(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx)
{
    u64 classId = ScriptObjectRegistry::allocateClassId(ctx);
    void* proto = builder.exportClass("RegistrationBuilder", classId);
    ScriptBindingRegistry::instance().registerProto(classId, proto);

    ClassRegistrar<void> reg(ctx, classId, proto);

    reg.method(
        "batch",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyStringChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::batch);
        },
        1);

    reg.method(
        "maxAttempts",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyIntChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::maxAttempts);
        },
        1);

    reg.method(
        "maxTicks",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyIntChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::maxTicks);
        },
        1);

    reg.method(
        "padding",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyIntChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::padding);
        },
        1);

    reg.method(
        "required",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyBoolChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::required);
        },
        1);

    reg.method(
        "requiredSuccessfulAttempts",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyIntChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::requiredSuccessfulAttempts);
        },
        1);

    reg.method(
        "rotateTest",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyBoolChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::rotateTest);
        },
        1);

    reg.method(
        "setupTicks",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyIntChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::setupTicks);
        },
        1);

    reg.method(
        "structureName",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyStringChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::structureName);
        },
        1);

    reg.method(
        "structureLocation",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyStringChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::structureLocation);
        },
        1);

    reg.method(
        "tag",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            return _applyStringChain(ctx, thisVal, argc, args, &ScriptRegistrationBuilder::tag);
        },
        1);

    return classId;
}

} // namespace mc::test
