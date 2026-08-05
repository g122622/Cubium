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
 * 第一阶段限制（明文 TODO）：
 * - `ScriptSequence.idle`/`until` 异步断言不可用（事件总线未桥接），返回 rejected Promise。
 * - `ScriptGameTestFunction.Async` Future 轮询未实现，`registerAsync` 暂按同步语义执行（TODO）。
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
};

} // namespace mc::test
