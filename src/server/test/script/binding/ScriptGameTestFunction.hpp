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
 * 同步语义：`run()` 进入时把 helper 压入 `ScriptGameTestAccessor`，调 `callFunction0(jsCallback, undefined)`
 * 同步执行 JS 体，退出时清空 accessor。JS 体内经 `Test` 类绑定（`ScriptTestHelper`）转发到
 * `GameTestHelper` 门面。JS 体正常返回→`pass()`；JS 抛异常→`fail(FailConditionsMet, msg)`（对齐基岩把
 * JS Error 映射为 `GameTestError`）；helper 已 `succeed`/`fail`→instance 状态机接管，`run` 仍返 pass。
 *
 * 异步语义（`registerAsync`）TODO：当前按同步执行（JS 体内部 `await` 不可用），待事件总线桥接后改为
 * Promise 轮询 `IGameTestFunctionRunResult::isComplete`。
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
