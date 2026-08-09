/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
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

#include "server/test/script/binding/ScriptGameTestTypes.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 classId/proto 注册表
#include "common/test/base/error/GameTestErrorType.hpp"             // GameTestErrorType::MethodNotImplemented
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/sculk/SculkSpreader.hpp"  // mc::blocks::SculkSpreader/ChargeCursor
#include "server/test/script/binding/ScriptGameTestError.hpp" // throwGameTestError（addCursorsWithOffset stub）

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptClassRegistry;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {

// JS 类名常量（登记 ScriptClassRegistry 与跨回调按名查的统一键）。
constexpr const char* kSculkSpreaderClassName = "SculkSpreader";
constexpr const char* kFenceConnectivityClassName = "FenceConnectivity";
constexpr const char* kNavigationResultClassName = "NavigationResult";

/// 解析 BlockPos 参数（JS 传 {x,y,z} 对象）。失败抛 TypeError 返回 false。
bool parseBlockPos(mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* arg, BlockPos& out)
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

void registerGameTestTypesClasses(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx)
{
    // --- SculkSpreader 类（opaque 持 mc::blocks::SculkSpreader* owned）---
    u64 sculkClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* sculkProto = builder.exportClass(kSculkSpreaderClassName, sculkClassId);
    ScriptClassRegistry::instance().registerClass(sculkClassId, sculkProto, kSculkSpreaderClassName);

    ClassRegistrar<void> sculkReg(ctx, sculkClassId, sculkProto);
    sculkReg.readonlyProperty(
        "maxCharge", [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/) -> void* {
            // MC SculkSpreader.MAX_CHARGE 常量，做实。
            return ctx.createInt32(mc::blocks::SculkSpreader::kMaxCharge);
        });
    sculkReg.method(
        "addCursors",
        [sculkClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* spreader =
                static_cast<mc::blocks::SculkSpreader*>(ScriptObjectRegistry::unwrap(ctx, thisVal, sculkClassId));
            if (spreader == nullptr || argc < 2 || !ctx.isObject(args[0]) || !ctx.isNumber(args[1])) {
                return ctx.createUndefined();
            }
            BlockPos pos;
            if (!parseBlockPos(ctx, args[0], pos)) {
                return ctx.createUndefined(); // parseBlockPos 已 throw
            }
            auto amount = ctx.toInt32(args[1]);
            if (!amount) {
                return ctx.createUndefined();
            }
            spreader->addCursors(pos, *amount);
            return ctx.createUndefined();
        },
        2);
    sculkReg.method(
        "addCursorsWithOffset",
        [](mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* /*thisVal*/, i32 /*argc*/, void** /*args*/)
            -> void* {
            // TODO: addCursorsWithOffset 依赖偏移扩散体系（facingData/方向偏移电荷注入），后续补全。
            return throwGameTestError(
                ctx, GameTestErrorType::MethodNotImplemented, "SculkSpreader.addCursorsWithOffset not implemented yet");
        },
        3);
    sculkReg.method("getNumberOfCursors",
        [sculkClassId](mc::mod::bedrock::addon::IScriptBindingContext& ctx,
            void* thisVal,
            i32 /*argc*/,
            void** /*args*/) -> void* {
            auto* spreader =
                static_cast<mc::blocks::SculkSpreader*>(ScriptObjectRegistry::unwrap(ctx, thisVal, sculkClassId));
            if (spreader == nullptr) {
                return ctx.createInt32(0);
            }
            return ctx.createInt32(static_cast<i32>(spreader->cursors().size()));
        });
    sculkReg.method("getTotalCharge",
        [sculkClassId](mc::mod::bedrock::addon::IScriptBindingContext& ctx,
            void* thisVal,
            i32 /*argc*/,
            void** /*args*/) -> void* {
            auto* spreader =
                static_cast<mc::blocks::SculkSpreader*>(ScriptObjectRegistry::unwrap(ctx, thisVal, sculkClassId));
            if (spreader == nullptr) {
                return ctx.createInt32(0);
            }
            i32 total = 0;
            for (const auto& cursor : spreader->cursors()) {
                total += cursor.charge();
            }
            return ctx.createInt32(total);
        });
    sculkReg.method(
        "getCursorPosition",
        [sculkClassId](
            mc::mod::bedrock::addon::IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            auto* spreader =
                static_cast<mc::blocks::SculkSpreader*>(ScriptObjectRegistry::unwrap(ctx, thisVal, sculkClassId));
            if (spreader == nullptr || argc < 1 || !ctx.isNumber(args[0])) {
                return ctx.createUndefined();
            }
            auto idx = ctx.toInt32(args[0]);
            const auto& cursors = spreader->cursors();
            if (!idx || *idx < 0 || *idx >= static_cast<i32>(cursors.size())) {
                return ctx.createUndefined();
            }
            const BlockPos& pos = cursors[static_cast<size_t>(*idx)].pos();
            void* obj = ctx.createObject();
            ctx.setPropertyInt(obj, "x", static_cast<i32>(pos.x));
            ctx.setPropertyInt(obj, "y", static_cast<i32>(pos.y));
            ctx.setPropertyInt(obj, "z", static_cast<i32>(pos.z));
            return obj;
        },
        1);

    // --- FenceConnectivity 类（值对象，本批仅注册原型作 instanceof 锚点）---
    // 实例由批次4 getFenceConnectivity 经 ScriptClassRegistry 查 classId/proto 构造
    // （createObject+setPrototypeOf+四 bool 属性 north/east/south/west）。
    {
        u64 fenceClassId = ScriptObjectRegistry::allocateClassId(ctx);
        void* fenceProto = builder.exportClass(kFenceConnectivityClassName, fenceClassId);
        ScriptClassRegistry::instance().registerClass(fenceClassId, fenceProto, kFenceConnectivityClassName);
        // 无方法/属性：实例字段构造时设，原型仅作 instanceof 锚点。
    }

    // --- NavigationResult 类（寻路空壳，寻路 stub 故最小实现）---
    // 实例由批次6寻路 stub 构造：isFullPath 恒 false、getPath() 空 Vector3[]。
    // TODO: 寻路（PathNavigator 适配非 MobEntity 拥有者）做实后补真实路径数据。
    {
        u64 navClassId = ScriptObjectRegistry::allocateClassId(ctx);
        void* navProto = builder.exportClass(kNavigationResultClassName, navClassId);
        ScriptClassRegistry::instance().registerClass(navClassId, navProto, kNavigationResultClassName);
        // 无方法/属性：实例字段构造时设，原型仅作 instanceof 锚点。
    }

    // --- LookDuration 枚举对象（全项目此前无，对齐基岩字符串枚举）---
    {
        void* obj = ctx.createObject();
        const auto setEntry = [&](const char* key) { ctx.setPropertyString(obj, key, key); };
        setEntry("Continuous");
        setEntry("Instant");
        setEntry("UntilMove");
        builder.exportValue("LookDuration", obj);
        ctx.releaseValue(obj);
    }
}

} // namespace mc::test
