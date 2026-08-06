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
 *
 */

#pragma once

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/test/framework/function/IGameTestRunResult.hpp"

#include <memory>

namespace mc::test {

/**
 * @brief 脚本异步测试函数的运行结果（轮询 JS Promise 状态）。
 *
 * `ScriptGameTestFunction::run` 检测 JS 测试体返回值为 Promise 时，创建本类持有 Promise 句柄，
 * 交由 `BaseGameTestInstance` 每 tick 轮询 `isComplete()`。
 *
 * 完成判定（对齐基岩 `ScriptAsyncGameTestRunResult`，但 quickjs-ng 无 reaction API 故改轮询）：
 * - **Promise pending** → `isComplete()==false`，测试进行中（靠脚本引擎 tick 驱动 then-handler resolve）。
 * - **Promise rejected** → `isComplete()==true`，`getError()` 返回 `fail(FailConditionsMet, reason)`，
 *   instance 据此立即 fail。
 * - **Promise fulfilled** → `isComplete()==true`，`getError()` 返回 `pass()`（nullopt）。
 *   fulfilled 仅表示 JS 体执行流结束，**不**直接判通过——测试仍由 instance 的 `succeed()`/超时路径
 *   判定（对齐基岩：JS `async` 体须显式 `t.succeed()`，否则超时 fail）。
 *
 * Promise 句柄生命周期：构造时持有（owned，refcount=1），析构 releaseValue 释放。instance 在
 * `succeed()`/`fail()`/超时后 `m_runResult.reset()` 触发析构；`ScriptManager::shutdown` 前的
 * `GameTestTicker::forceStop()` 保证 instance 析构时脚本上下文仍有效。
 */
class ScriptAsyncGameTestRunResult final : public IGameTestFunctionRunResult {
public:
    /**
     * @param bindingCtx 脚本绑定上下文（抽象接口，经 promiseState/promiseResult 查询 Promise）。
     * @param promise    JS Promise 句柄（owned，调用方已 retain 一份给本对象；析构时 releaseValue）。
     */
    ScriptAsyncGameTestRunResult(mc::mod::bedrock::addon::IScriptBindingContext* bindingCtx, void* promise);
    ~ScriptAsyncGameTestRunResult() override;

    ScriptAsyncGameTestRunResult(const ScriptAsyncGameTestRunResult&) = delete;
    ScriptAsyncGameTestRunResult& operator=(const ScriptAsyncGameTestRunResult&) = delete;
    ScriptAsyncGameTestRunResult(ScriptAsyncGameTestRunResult&&) = delete;
    ScriptAsyncGameTestRunResult& operator=(ScriptAsyncGameTestRunResult&&) = delete;

    [[nodiscard]] bool isComplete() const override;
    [[nodiscard]] GameTestResult getError() override;

private:
    mc::mod::bedrock::addon::IScriptBindingContext* m_bindingCtx;
    void* m_promise;
};

} // namespace mc::test
