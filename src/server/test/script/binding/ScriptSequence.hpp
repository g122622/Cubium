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

class GameTestSequence;

/**
 * @brief 注册 JS `GameTestSequence` 类绑定（转发原生 `GameTestSequence`）。
 *
 * 对齐基岩 `ScriptGameTestSequence`：JS 侧 `Test.startSequence()` 返回 `GameTestSequence` 对象，链式调用
 * `thenExecute`/`thenExecuteAfter`/`thenExecuteFor`/`thenWait`/`thenWaitAfter`/`thenIdle`/`thenSucceed`/
 * `thenFail`。每个 `then*` 把 JS 回调包装为 `std::function<GameTestResult()>`（调 `callFunction0`，映射 JS
 * 异常为 `GameTestError`）转发到原生 `GameTestSequence`。
 *
 * 句柄获取：`Test.startSequence()` 经 Test 对象 opaque 携带的 `GameTestHelper*` 调 `startSequence()` 取
 * 原生 `GameTestSequence&`，包装为不透明指针存 JS 对象。原生序列由 `GameTestHelper` 拥有，生命周期覆盖测试运行。
 *
 * `thenWait`/`thenWaitAfter` 转发原生 Wait 步骤（每 tick 轮询回调直到通过），JS 回调经
 * `ScriptCallbackUtil::wrapJsCallback` 包装为 `std::function<GameTestResult()>`。
 *
 * @param builder 模块构建器。
 * @param ctx 绑定上下文。
 * @return 类 id。
 */
[[nodiscard]] u64 registerSequenceClassBinding(
    mc::mod::bedrock::addon::NativeModuleBuilder& builder, mc::mod::bedrock::addon::IScriptBindingContext& ctx);

/**
 * @brief 把原生 `GameTestSequence*` 包装为非拥有 JS 对象（`Test.startSequence` 用）。
 *
 * @param ctx 绑定上下文。
 * @param classId `registerSequenceClassBinding` 返回的类 id。
 * @param seq 原生序列指针（由 `GameTestHelper` 拥有，非拥有）。
 * @return JS 对象句柄（调用方负责 `releaseValue`）。
 */
[[nodiscard]] void* wrapSequence(
    mc::mod::bedrock::addon::IScriptBindingContext& ctx, u64 classId, mc::test::GameTestSequence* seq);

} // namespace mc::test
