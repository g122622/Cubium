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

#pragma once

#include "common/mod/bedrock/addon/binding/IModuleBindingFactory.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {
class ScriptScheduler;
}

namespace mc::test {

/**
 * @brief `@minecraft/server-gametest` 模块绑定工厂。
 *
 * 对齐基岩版官方 JS 模块 `@minecraft/server-gametest`：把 C++ 门面（`GameTestHelper`/`SimulatedPlayer`/
 * `GameTestSequence`）与注册机制（`GameTestRegistry`）暴露给 JS 行为包。模块顶层导出 `register`/
 * `registerAsync`/`setBeforeBatchCallback`/`setAfterBatchCallback`/`spawnSimulatedPlayer`，类导出 `Test`/
 * `RegistrationBuilder`/`SimulatedPlayer`/`GameTestSequence`。
 *
 * 依赖方向：本工厂在 `src/server/test/`（server 侧）编入 `minecraft-server` exe，可同时见服务端类型与
 * `mc_bedrock_addon` 绑定基础设施；`mc_bedrock_addon` 本身**不**依赖 `mc_test`/server（保持 common 纯净）。
 * 工厂经 `ScriptManager::registerModuleFactory` 公共钩子由服务端层注册。
 *
 * 与原生测试的汇聚：JS `register(suite,name,fn)` 经 `ScriptGameTestFunction`（`BaseGameTestFunction` 子类，
 * 持 JS 回调）提交到内部 `GameTestRegistry`，与原生 `MC_REGISTER_GAME_TEST` 同一注册表。
 *
 * 异步桥接（事件总线）：`ScriptGameTestFunction::run` 检测 JS 体返回 Promise 时返回
 * `ScriptAsyncGameTestRunResult`，由 `BaseGameTestInstance` 每 tick 轮询 Promise 状态（rejected→fail，
 * fulfilled→交由 succeed/超时接管）。`Test.idle(t)` 经 `ScriptScheduler::runTimeout` 创建定时 resolve
 * 的 Promise；`Test.until(fn)`/`Sequence.thenWait(fn)` 转发原生轮询/等待。`registerAsync` 与 `register`
 * 统一走 Promise 检测路径，二者皆允许 JS 体返回 Promise 或普通值。
 */
class GameTestModuleBinding : public mc::mod::bedrock::addon::IModuleBindingFactory {
public:
    GameTestModuleBinding() = default;
    ~GameTestModuleBinding() override = default;

    [[nodiscard]] std::string name() const override { return "@minecraft/server-gametest"; }
    [[nodiscard]] std::string uuid() const override { return "b9c4d8e1-2f3a-4b5c-9d6e-7f8a9b0c1d2e"; }
    [[nodiscard]] std::vector<mc::mod::bedrock::addon::ModuleVersion> supportedVersions() const override;
    [[nodiscard]] std::vector<mc::mod::bedrock::addon::ModuleDependency> dependencies(
        const mc::mod::bedrock::addon::ModuleVersion& version) const override;
    bool registerBindings(mc::mod::bedrock::addon::IScriptContext& context) override;

    /**
     * @brief 注入脚本调度器（注册前调）。
     *
     * 仿 `MinecraftModuleFactory::setScheduler` 先例：`ScriptManager` 拥有 `ScriptScheduler`，
     * 但 `registerBindings(IScriptContext&)` 签名拿不到 `ScriptManager`，故经此 setter 注入。
     * `registerBindings` 时转存入 `ScriptBindingRegistry`，供 `ScriptTestHelper::idle` 创建
     * 定时 resolve 的 Promise（实现 JS `await test.idle(n)`）。须在 `addModuleFactory` 后、
     * 插件加载前调用。
     */
    void setScheduler(mc::mod::bedrock::addon::ScriptScheduler* scheduler) noexcept { m_scheduler = scheduler; }

private:
    mc::mod::bedrock::addon::ScriptScheduler* m_scheduler = nullptr;
};

} // namespace mc::test
