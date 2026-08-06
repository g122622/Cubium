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

#include "server/test/script/GameTestModuleBinding.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "server/test/script/binding/ScriptRegister.hpp"
#include "server/test/script/binding/ScriptRegistrationBuilderBinding.hpp"
#include "server/test/script/binding/ScriptSequence.hpp"
#include "server/test/script/binding/ScriptSimulatedPlayer.hpp"
#include "server/test/script/binding/ScriptTestHelper.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"

#include <spdlog/spdlog.h>

namespace mc::test {

std::vector<mc::mod::bedrock::addon::ModuleVersion> GameTestModuleBinding::supportedVersions() const
{
    return {mc::mod::bedrock::addon::ModuleVersion{1, 0, 0}};
}

std::vector<mc::mod::bedrock::addon::ModuleDependency> GameTestModuleBinding::dependencies(
    const mc::mod::bedrock::addon::ModuleVersion& /*version*/) const
{
    // @minecraft/server-gametest 依赖 @minecraft/server（GameTest 测试体内常用 world/dimension）。
    // TODO: 依赖声明待 ScriptManager 依赖解析支持后细化版本范围。
    return {};
}

bool GameTestModuleBinding::registerBindings(mc::mod::bedrock::addon::IScriptContext& context)
{
    auto& ctx = context.bindingContext();

    spdlog::info("[GameTest] Registering @minecraft/server-gametest module bindings");

    // 清空上一轮注册的原型表（脚本引擎重建/测试隔离场景）。
    ScriptBindingRegistry::instance().clear();
    // 注入脚本调度器（setScheduler 在 addModuleFactory 后由注册点调），供 Test.idle 创建定时 Promise。
    ScriptBindingRegistry::instance().setScheduler(m_scheduler);

    mc::mod::bedrock::addon::NativeModuleBuilder builder(ctx, "@minecraft/server-gametest");

    // 注册顺序：依赖 classId 的后注册。
    // 1. RegistrationBuilder 类（顶层 register 返回值）
    u64 builderClassId = registerRegistrationBuilderClassBinding(builder, ctx);
    // 2. GameTestSequence 类（Test.startSequence 返回值）
    u64 sequenceClassId = registerSequenceClassBinding(builder, ctx);
    // 3. SimulatedPlayer 类（Test.spawnSimulatedPlayer 返回值）
    u64 simulatedPlayerClassId = registerSimulatedPlayerClassBinding(builder, ctx);
    // 4. Test 类（依赖 sequence + simulatedPlayer classId 做 wrap）
    u64 testClassId = registerTestClassBinding(builder, ctx, sequenceClassId, simulatedPlayerClassId);
    // 记录 Test classId 供 ScriptGameTestFunction::run 创建 Test 对象、ScriptTestHelper 取 opaque。
    ScriptBindingRegistry::instance().setTestClassId(testClassId);
    (void)builderClassId; // builderClassId 已在 registerTopLevelFunctions 内消费
    // 5. 顶层 gametest 命名空间对象（依赖 builderClassId 做 register 返回值 wrap）
    registerTopLevelFunctions(builder, ctx, builderClassId);

    // ====== 常量导出（对齐基岩 JS 模块枚举）======
    // GameMode（与 @minecraft/server 一致，便于 JS 体引用）。
    builder.exportConst("GameModeSurvival", 0);
    builder.exportConst("GameModeCreative", 1);
    builder.exportConst("GameModeAdventure", 2);
    builder.exportConst("GameModeSpectator", 3);

    // ====== 完成模块注册 ======
    if (!builder.finalize()) {
        spdlog::error("[GameTest] Failed to finalize @minecraft/server-gametest module");
        return false;
    }

    spdlog::info("[GameTest] @minecraft/server-gametest module bindings registered successfully");
    return true;
}

} // namespace mc::test
