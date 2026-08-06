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

#include "server/test/script/binding/ScriptAsyncGameTestRunResult.hpp"

#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <spdlog/spdlog.h>

namespace mc::test {

namespace {

// JSPromiseStateEnum 数值（对齐 IScriptBindingContext::promiseState 返回值）：0=Pending / 1=Fulfilled / 2=Rejected。
constexpr int kPromisePending = 0;
constexpr int kPromiseFulfilled = 1;
constexpr int kPromiseRejected = 2;

} // namespace

ScriptAsyncGameTestRunResult::ScriptAsyncGameTestRunResult(
    mc::mod::bedrock::addon::IScriptBindingContext* bindingCtx, void* promise)
    : m_bindingCtx(bindingCtx)
    , m_promise(promise)
{
    MC_ASSERT_RELEASE_MSG(m_bindingCtx != nullptr, "ScriptAsyncGameTestRunResult: binding context is null");
    MC_ASSERT_RELEASE_MSG(m_promise != nullptr, "ScriptAsyncGameTestRunResult: promise handle is null");
}

ScriptAsyncGameTestRunResult::~ScriptAsyncGameTestRunResult()
{
    if (m_bindingCtx != nullptr && m_promise != nullptr) {
        m_bindingCtx->releaseValue(m_promise);
        m_promise = nullptr;
    }
}

bool ScriptAsyncGameTestRunResult::isComplete() const
{
    // 经抽象接口查询 Promise 状态（非 Pending 即已 settle）。
    return m_bindingCtx->promiseState(m_promise) != kPromisePending;
}

GameTestResult ScriptAsyncGameTestRunResult::getError()
{
    const int state = m_bindingCtx->promiseState(m_promise);
    if (state == kPromiseFulfilled) {
        // fulfilled：JS 体执行流结束，不直接判通过——由 instance 的 succeed/超时路径接管。
        return pass();
    }
    if (state == kPromiseRejected) {
        // rejected：提取 reject reason（可能为任意 JS 值，用 toString 转字符串）映射为 GameTestError。
        void* reason = m_bindingCtx->promiseResult(m_promise);
        std::string reasonStr = "JS async test rejected";
        if (reason != nullptr) {
            auto str = m_bindingCtx->toString(reason);
            if (str.has_value() && !str->empty()) {
                reasonStr = *str;
            }
            m_bindingCtx->releaseValue(reason);
        }
        spdlog::warn("[GameTest] JS async test rejected: {}", reasonStr);
        return fail(GameTestErrorType::FailConditionsMet, std::move(reasonStr));
    }
    // 仍在 pending（不应在 isComplete()==true 后调到，幂等返回通过）。
    return pass();
}

} // namespace mc::test
