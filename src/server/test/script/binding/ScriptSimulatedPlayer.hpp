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

#pragma once

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"

namespace mc::test {

class SimulatedPlayer;

/**
 * @brief 注册 JS `SimulatedPlayer` 类绑定（转发原生 `SimulatedPlayer`）。
 *
 * 对齐基岩 `ScriptSimulatedPlayer`：JS 侧 `Test.spawnSimulatedPlayer(name, location)` 返回
 * `SimulatedPlayer` 对象，方法 `moveToLocation`/`lookAtLocation`/`lookAtEntity`/`chat`/`respawn` 转发到
 * 原生 `SimulatedPlayer`。`flyToLocation`/`attack` 为 TODO stub（原生侧未实现）。
 *
 * 句柄：JS 对象经 `ScriptObjectRegistry::wrap` 持 `SimulatedPlayer*`（非拥有，原生实体由 `ServerWorld`
 * EntityManager 拥有）。`ScriptObjectRegistry::unwrap` 取回指针。`Test.spawnSimulatedPlayer` 绑定调
 * `GameTestHelper::spawnSimulatedPlayer` 获 `SimulatedPlayer*` 后 wrap 返回 JS 对象。
 *
 * @param builder 模块构建器。
 * @param ctx 绑定上下文。
 * @return 类 id。
 */
[[nodiscard]] u64 registerSimulatedPlayerClassBinding(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx);

/**
 * @brief 把原生 `SimulatedPlayer*` 包装为 JS 对象（`Test.spawnSimulatedPlayer` 用）。
 *
 * @param ctx 绑定上下文。
 * @param classId `registerSimulatedPlayerClassBinding` 返回的类 id。
 * @param player 原生指针（非拥有）。
 * @return JS 对象句柄（调用方负责 `releaseValue`）。
 */
[[nodiscard]] void* wrapSimulatedPlayer(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, u64 classId, SimulatedPlayer* player);

} // namespace mc::test
