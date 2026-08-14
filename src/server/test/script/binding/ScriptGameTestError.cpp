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

#include "server/test/script/binding/ScriptGameTestError.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp" // 跨模块 classId/proto 注册表
#include "common/test/base/error/GameTestCompletedError.hpp"
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorContext.hpp"
#include "common/test/base/tags/GameTestTags.hpp"
#include "common/world/block/BlockPos.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mc::test {

using mc::mod::bedrock::addon::ClassRegistrar;
using mc::mod::bedrock::addon::ScriptClassRegistry;
using mc::mod::bedrock::addon::ScriptObjectRegistry;

namespace {

// JS 类名常量（登记 ScriptClassRegistry 与构造实例按名查的统一键）。
constexpr const char* kGameTestErrorClassName = "GameTestError";
constexpr const char* kGameTestCompletedErrorClassName = "GameTestCompletedError";
constexpr const char* kGameTestErrorContextClassName = "GameTestErrorContext";

/// 取全局 Error 构造函数的 prototype 句柄（owned，调用方 release）。
/// 供错误类原型挂 Error.prototype 建 Error 子类继承链。失败返回 nullptr。
void* getErrorPrototype(mc::mod::bedrock::addon::IScriptBindingContext& ctx)
{
    void* global = ctx.getGlobalObject();
    void* errorCtor = ctx.getProperty(global, "Error");
    ctx.releaseValue(global);
    if (errorCtor == nullptr) {
        return nullptr;
    }
    void* errorProto = ctx.getProperty(errorCtor, "prototype");
    ctx.releaseValue(errorCtor);
    return errorProto;
}

/// 构造 BlockPos 的 JS {x,y,z} 对象（owned 句柄）。供 GameTestErrorContext 暴露坐标。
void* blockPosToJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, const BlockPos& pos)
{
    void* obj = ctx.createObject();
    ctx.setPropertyInt(obj, "x", static_cast<i32>(pos.x));
    ctx.setPropertyInt(obj, "y", static_cast<i32>(pos.y));
    ctx.setPropertyInt(obj, "z", static_cast<i32>(pos.z));
    return obj;
}

/// 构造 GameTestErrorContext JS 实例（owned 句柄）。无 context 返回 null。
void* contextToJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, const std::optional<GameTestErrorContext>& c)
{
    if (!c.has_value()) {
        return ctx.createNull();
    }
    const u64 classId = ScriptClassRegistry::instance().classIdByName(kGameTestErrorContextClassName);
    void* proto = ScriptClassRegistry::instance().proto(classId);
    if (proto == nullptr) {
        // 类未注册（不应发生）：退化为普通对象，保字段不丢。
        void* obj = ctx.createObject();
        ctx.setProperty(obj, "absolutePosition", blockPosToJs(ctx, c->absolutePosition()));
        ctx.setProperty(obj, "relativePosition", blockPosToJs(ctx, c->relativePosition()));
        ctx.setPropertyInt(obj, "tickCount", c->tickCount());
        return obj;
    }
    void* obj = ctx.createObject();
    ctx.setPrototypeOf(obj, proto);
    ctx.setProperty(obj, "absolutePosition", blockPosToJs(ctx, c->absolutePosition()));
    ctx.setProperty(obj, "relativePosition", blockPosToJs(ctx, c->relativePosition()));
    ctx.setPropertyInt(obj, "tickCount", c->tickCount());
    return obj;
}

/// 构造 string[] 的 JS 数组（owned 句柄）。
void* stringVectorToJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, const std::vector<std::string>& vec)
{
    void* arr = ctx.createArray();
    for (u32 i = 0; i < vec.size(); ++i) {
        ctx.setArrayElementString(arr, i, vec[i]);
    }
    return arr;
}

/// 构造 GameTestError JS 实例（owned 句柄）并返回。失败（类未注册）退化为普通 Error 对象。
/// 不抛出——抛出由调用方 throwValue 负责。
void* buildGameTestErrorJs(mc::mod::bedrock::addon::IScriptBindingContext& ctx, const GameTestError& err)
{
    const u64 classId = ScriptClassRegistry::instance().classIdByName(kGameTestErrorClassName);
    void* proto = ScriptClassRegistry::instance().proto(classId);
    void* obj = ctx.createObject();
    if (proto != nullptr) {
        ctx.setPrototypeOf(obj, proto);
    }
    // message 用 formattedMessage（替换占位符后的可读串），对齐基岩 Error.message 可读性。
    ctx.setPropertyString(obj, "message", err.formattedMessage());
    ctx.setPropertyString(obj, "type", gameTestErrorTypeName(err.type()));
    ctx.setProperty(obj, "context", contextToJs(ctx, err.context()));
    ctx.setProperty(obj, "params", stringVectorToJs(ctx, err.params()));
    return obj;
}

} // namespace

void registerGameTestErrorClasses(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx)
{
    // 全局 Error.prototype：错误类原型挂此建 Error 子类继承链（instanceof Error 成立）。
    void* errorProto = getErrorPrototype(ctx);

    // --- GameTestError 类（Error 子类）---
    u64 errorClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* errorProtoObj = builder.exportClass(kGameTestErrorClassName, errorClassId);
    if (errorProto != nullptr) {
        ctx.setPrototypeOf(errorProtoObj, errorProto);
        ctx.releaseValue(errorProto);
    }
    ScriptClassRegistry::instance().registerClass(errorClassId, errorProtoObj, kGameTestErrorClassName);

    ClassRegistrar<void> errorReg(ctx, errorClassId, errorProtoObj);
    // 实例字段 message/type/context/params 作为 own 属性在构造时设置（见 buildGameTestErrorJs），
    // 此处不注册原型属性——避免与实例 own 属性歧义。readonlyProperty 留空，类仅作 instanceof 锚点。

    // --- GameTestCompletedError 类（Error 子类）---
    u64 completedClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* completedProtoObj = builder.exportClass(kGameTestCompletedErrorClassName, completedClassId);
    // 复用 errorProto 已 release，重新取一份（setPrototypeOf 不消耗 proto 所有权，但 getErrorPrototype
    // 返回 owned 句柄须各自 release）。
    void* errorProto2 = getErrorPrototype(ctx);
    if (errorProto2 != nullptr) {
        ctx.setPrototypeOf(completedProtoObj, errorProto2);
        ctx.releaseValue(errorProto2);
    }
    ScriptClassRegistry::instance().registerClass(
        completedClassId, completedProtoObj, kGameTestCompletedErrorClassName);

    ClassRegistrar<void> completedReg(ctx, completedClassId, completedProtoObj);
    (void)completedReg; // 同 GameTestError：实例字段构造时设，原型仅作 instanceof 锚点。

    // --- GameTestErrorContext 类（值对象，非 Error 子类）---
    u64 contextClassId = ScriptObjectRegistry::allocateClassId(ctx);
    void* contextProtoObj = builder.exportClass(kGameTestErrorContextClassName, contextClassId);
    ScriptClassRegistry::instance().registerClass(contextClassId, contextProtoObj, kGameTestErrorContextClassName);

    ClassRegistrar<void> contextReg(ctx, contextClassId, contextProtoObj);
    (void)contextReg; // 实例字段构造时设。

    // ====== 枚举常量对象导出（对齐基岩字符串枚举语义）======

    // GameTestErrorType：值=自身 PascalCase 名（Unknown/Waiting/.../SimulatedPlayerOutOfBounds）。
    {
        void* obj = ctx.createObject();
        const auto setEntry = [&](const char* key) { ctx.setPropertyString(obj, key, key); };
        setEntry("Unknown");
        setEntry("Waiting");
        setEntry("ExhaustedAttempts");
        setEntry("AssertAtPosition");
        setEntry("MethodNotImplemented");
        setEntry("ExecutionTimeout");
        setEntry("LevelStateModificationFailed");
        setEntry("FailConditionsMet");
        setEntry("Assert");
        setEntry("SimulatedPlayerOutOfBounds");
        builder.exportValue("GameTestErrorType", obj);
        ctx.releaseValue(obj);
    }

    // GameTestCompletedErrorReason：{"Done":"Done","Cleanup":"Cleanup"}（Cleanup 小写 u 对齐官方）。
    {
        void* obj = ctx.createObject();
        ctx.setPropertyString(obj, "Done", "Done");
        ctx.setPropertyString(obj, "Cleanup", "Cleanup");
        builder.exportValue("GameTestCompletedErrorReason", obj);
        ctx.releaseValue(obj);
    }

    // Tags：五个套件标签（补全官方 SuiteDebug/SuiteNextUpdate）。
    {
        void* obj = ctx.createObject();
        ctx.setPropertyString(obj, "SuiteAll", std::string(tags::SuiteAll));
        ctx.setPropertyString(obj, "SuiteDefault", std::string(tags::SuiteDefault));
        ctx.setPropertyString(obj, "SuiteDisabled", std::string(tags::SuiteDisabled));
        ctx.setPropertyString(obj, "SuiteDebug", std::string(tags::SuiteDebug));
        ctx.setPropertyString(obj, "SuiteNextUpdate", std::string(tags::SuiteNextUpdate));
        builder.exportValue("Tags", obj);
        ctx.releaseValue(obj);
    }
}

void* throwGameTestError(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, GameTestErrorType type, std::string_view message)
{
    GameTestError err(type, std::string(message));
    void* jsErr = buildGameTestErrorJs(ctx, err);
    void* exc = ctx.throwValue(jsErr); // throwValue 内部复制，不消耗 jsErr 所有权。
    ctx.releaseValue(jsErr);
    return exc;
}

void* throwGameTestErrorFromResult(mc::mod::bedrock::addon::IScriptBindingContext& ctx, GameTestResult result)
{
    if (isPass(result)) {
        return ctx.createUndefined();
    }
    void* jsErr = buildGameTestErrorJs(ctx, *result);
    void* exc = ctx.throwValue(jsErr);
    ctx.releaseValue(jsErr);
    return exc;
}

void* throwGameTestCompletedError(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, std::string_view gameTestName, std::string_view methodName)
{
    // CompletedError 默认 reason=Done（测试已正常结束）。清理阶段调用由调用点据 isCleaningUp 判定后
    // 仍用 Done——基岩 JS 侧 GameTestCompletedError.reason 区分 Done/Cleanup，但触发点同为"已结束"，
    // 本绑定层无清理阶段独立入口，统一 Done（TODO: 清理阶段语义后续按需细化）。
    GameTestCompletedError err(GameTestCompletedErrorReason::Done, std::string(gameTestName), std::string(methodName));
    const u64 classId = ScriptClassRegistry::instance().classIdByName(kGameTestCompletedErrorClassName);
    void* proto = ScriptClassRegistry::instance().proto(classId);
    void* obj = ctx.createObject();
    if (proto != nullptr) {
        ctx.setPrototypeOf(obj, proto);
    }
    ctx.setPropertyString(obj, "reason", gameTestCompletedErrorReasonName(err.reason()));
    ctx.setPropertyString(obj, "gameTestName", err.gameTestName());
    ctx.setPropertyString(obj, "methodName", err.methodName());
    // message 对齐 Error.message 可读性。
    ctx.setPropertyString(obj, "message", err.toGameTestError().formattedMessage());
    void* exc = ctx.throwValue(obj);
    ctx.releaseValue(obj);
    return exc;
}

} // namespace mc::test
