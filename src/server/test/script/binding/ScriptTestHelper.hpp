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

#include <string>

namespace mc::test {

/**
 * @brief 注册 JS `Test` 类绑定（转发 `GameTestHelper` 门面）。
 *
 * 对齐基岩 `ScriptGameTestHelper`：JS 侧 `Test` 对象经 opaque 携带 `GameTestHelper*`（非拥有，由
 * `ScriptGameTestFunction::run` 创建并作首参传入 JS 体）。各 Test 方法从 thisVal 经 `getOpaque(testClassId)`
 * 取 helper——不依赖单例，async 测试体 `await` 挂起后 then-handler resume 时 thisVal 仍携带正确 helper，
 * 多 async 并发安全。转发后把 `GameTestResult` 映射为 JS 行为（通过→无返回值；失败→`throw new Error(msg)`）。
 *
 * 已桥接方法子集（`assertBlockPresent`/`setBlock`/`pressButton`/`pullLever`/`pulseRedstone`/
 * `killAllEntities`/`succeed`/`fail`/`print`/`startSequence`/`spawnSimulatedPlayer`/`currentTick`）+
 * 异步方法（`idle(ticks)`→Promise<void>，经 `ScriptScheduler::runTimeout` 定时 resolve；
 * `until(fn)`→void，转发原生 `helper->until` 持续轮询），其余 ~50 方法留 TODO（按需补全）。
 *
 * @param builder 模块构建器（`exportClass` 用）。
 * @param ctx 绑定上下文（回调内从 thisVal 取 helper）。
 * @param sequenceClassId `GameTestSequence` 类 id（`startSequence` 返回值 wrap 用，须先注册序列类）。
 * @param simulatedPlayerClassId `SimulatedPlayer` 类 id（`spawnSimulatedPlayer` 返回值 wrap 用）。
 * @return `Test` 类 id。
 */
[[nodiscard]] u64 registerTestClassBinding(mc::mod::bedrock::addon::NativeModuleBuilder& builder,
    mc::mod::bedrock::addon::IScriptBindingContext& ctx,
    u64 sequenceClassId,
    u64 simulatedPlayerClassId);

} // namespace mc::test
