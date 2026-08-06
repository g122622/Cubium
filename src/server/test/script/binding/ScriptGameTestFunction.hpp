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

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/test/base/data/TestData.hpp"
#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/function/IGameTestFunctionContext.hpp"
#include "common/test/framework/function/IGameTestRunResult.hpp"

#include <memory>
#include <string>

namespace mc::test {

/**
 * @brief 脚本测试函数（`BaseGameTestFunction` 子类，持 JS 回调）。
 *
 * 对齐基岩 `ScriptGameTestFunction`：把 JS `register(testClassName, testName, fn)` 的 `fn` 包装为
 * `BaseGameTestFunction`，与原生 `NativeGameTestFunction` 汇入同一 `GameTestRegistry`。
 *
 * `run()` 创建 Test JS 对象（opaque 携带 `GameTestHelper*`，非拥有），经 `callFunction1` 作首参传给 JS 体
 * （对齐基岩 `register(suite, name, (test)=>{...})` 签名）。JS 体各 Test 方法从 thisVal 取 helper，不依赖
 * 单例——async 测试体 `await` 挂起后 then-handler resume 时 thisVal 仍携带正确 helper，多 async 并发安全。
 *
 * 同步语义：JS 体返回非 Promise（普通值/undefined）→ `SyncGameTestRunResult(pass())`，通过/失败由 helper 的
 * succeed/fail 状态机接管。JS 体同步抛异常 → `SyncGameTestRunResult(fail(FailConditionsMet, msg))`。
 *
 * 异步语义（`registerAsync` 或 `register` 中 JS 体返回 Promise）：检测 `isPromise(ret)`→返回
 * `ScriptAsyncGameTestRunResult`（持有 Promise 句柄），由 `BaseGameTestInstance` 每 tick 轮询 Promise 状态
 * （rejected→fail，fulfilled→交由 succeed/超时接管）。`registerAsync` 与 `register` 统一走 Promise 检测，
 * 二者皆允许 JS 体返回 Promise 或普通值。
 */
class ScriptGameTestFunction final : public BaseGameTestFunction {
public:
    /**
     * @brief 构造脚本测试函数。
     *
     * @param batchName      批次/套件名（JS `register` 首参）。
     * @param testName       测试名（JS `register` 次参）。
     * @param structureName  结构名（默认 `gametest:empty_3x3`，由 `RegistrationBuilder.structureName` 设）。
     * @param data           注册期元数据（`TestData`，由 builder 填充）。
     * @param bindingCtx     脚本绑定上下文（`IScriptBindingContext*`，非拥有，脚本引擎生命周期内稳定）。
     * @param jsCallback     JS 回调句柄（`void*`，`run` 时 `callFunction0` 调用；析构时 `releaseValue`）。
     */
    ScriptGameTestFunction(std::string batchName,
        std::string testName,
        std::string structureName,
        TestData data,
        mc::mod::bedrock::addon::IScriptBindingContext* bindingCtx,
        void* jsCallback);

    ~ScriptGameTestFunction() override;

    ScriptGameTestFunction(const ScriptGameTestFunction&) = delete;
    ScriptGameTestFunction& operator=(const ScriptGameTestFunction&) = delete;
    ScriptGameTestFunction(ScriptGameTestFunction&&) = delete;
    ScriptGameTestFunction& operator=(ScriptGameTestFunction&&) = delete;

    [[nodiscard]] std::unique_ptr<IGameTestFunctionContext> createContext(IGameTestHelper& helper) const override;
    [[nodiscard]] std::unique_ptr<IGameTestFunctionRunResult> run(
        IGameTestHelper& helper, IGameTestFunctionContext& context) const override;

private:
    mc::mod::bedrock::addon::IScriptBindingContext* m_bindingCtx;
    void* m_jsCallback;
};

} // namespace mc::test
