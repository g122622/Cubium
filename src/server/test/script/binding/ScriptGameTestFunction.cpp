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

#include "server/test/script/binding/ScriptGameTestFunction.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp" // ScriptObjectRegistry
#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/function/SyncGameTestRunResult.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"
#include "server/test/facade/GameTestHelper.hpp"
#include "server/test/script/binding/ScriptAsyncGameTestRunResult.hpp"
#include "server/test/script/context/ScriptBindingRegistry.hpp"
#include "server/test/script/context/ScriptGameTestFunctionContext.hpp"

#include <utility>
#include <spdlog/spdlog.h>

namespace mc::test {

using mc::mod::bedrock::addon::ScriptObjectRegistry;

ScriptGameTestFunction::ScriptGameTestFunction(std::string batchName,
    std::string testName,
    std::string structureName,
    TestData data,
    mc::mod::bedrock::addon::IScriptBindingContext* bindingCtx,
    void* jsCallback)
    : BaseGameTestFunction(std::move(batchName), std::move(testName), std::move(structureName), std::move(data))
    , m_bindingCtx(bindingCtx)
    , m_jsCallback(jsCallback)
{
    // JS 回调句柄已由调用方 retainValue，此处仅持有指针；析构时 releaseValue。
}

ScriptGameTestFunction::~ScriptGameTestFunction()
{
    if (m_bindingCtx != nullptr && m_jsCallback != nullptr) {
        m_bindingCtx->releaseValue(m_jsCallback);
    }
}

std::unique_ptr<IGameTestFunctionContext> ScriptGameTestFunction::createContext(IGameTestHelper& /*helper*/) const
{
    return std::make_unique<ScriptGameTestFunctionContext>();
}

std::unique_ptr<IGameTestFunctionRunResult> ScriptGameTestFunction::run(
    IGameTestHelper& helper, IGameTestFunctionContext& /*context*/) const
{
    // helper 由 MinecraftGameTestInstance 拥有，测试运行期间稳定。JS 侧 Test 对象经 opaque 携带
    // GameTestHelper*（非拥有），各 Test 方法从 thisVal 取回——不依赖单例，async 测试体 await 挂起后
    // then-handler resume 时 thisVal 仍携带正确 helper，多 async 并发安全。
    // 注意：IGameTestHelper 是 framework 接口，但 ScriptTestHelper 需要 facade GameTestHelper 的具体方法。
    // helper 实际是 GameTestHelper 实例（MinecraftGameTestHelperProvider 创建），static_cast 安全。
    auto* facadeHelper = static_cast<GameTestHelper*>(&helper);

    if (m_bindingCtx == nullptr || m_jsCallback == nullptr) {
        return std::make_unique<SyncGameTestRunResult>(
            fail(GameTestErrorType::LevelStateModificationFailed, "Script test function has no JS callback"));
    }

    // 创建 Test JS 对象：经 ScriptBindingRegistry 取 Test 类原型 + classId，opaque 存 facadeHelper。
    const u64 testClassId = ScriptBindingRegistry::instance().testClassId();
    void* testProto = ScriptBindingRegistry::instance().proto(testClassId);
    void* undef = m_bindingCtx->createUndefined();
    void* testObj =
        ScriptObjectRegistry::wrap(*m_bindingCtx, testClassId, testProto, facadeHelper, /*owned=*/false, "Test");

    // 调 JS 测试体，传 Test 对象作首参（对齐基岩 register(suite, name, (test)=>{...}) 签名）。
    void* ret = m_bindingCtx->callFunction1(m_jsCallback, undef, testObj);
    m_bindingCtx->releaseValue(undef);

    auto releaseTestObj = [this](void* obj) {
        if (obj != nullptr) {
            m_bindingCtx->releaseValue(obj);
        }
    };

    if (m_bindingCtx->isException(ret)) {
        // JS 体同步抛异常 → 映射为 GameTestError(FailConditionsMet)。
        void* exc = m_bindingCtx->getException();
        auto msg = m_bindingCtx->getExceptionMessage(exc);
        m_bindingCtx->releaseValue(exc);
        spdlog::warn("[GameTest] JS test body threw: {}", msg);
        m_bindingCtx->releaseValue(ret);
        releaseTestObj(testObj);
        return std::make_unique<SyncGameTestRunResult>(fail(GameTestErrorType::FailConditionsMet, std::string(msg)));
    }

    // 检测返回值是否为 Promise（async 测试体）：经抽象接口 isPromise 查询。
    const bool isPromise = m_bindingCtx->isPromise(ret);

    if (isPromise) {
        // 异步路径：AsyncRunResult 持有 Promise 句柄，由 BaseGameTestInstance 每 tick 轮询状态。
        // retainValue 给 AsyncRunResult 一份独立引用（callFunction1 返回的 ret 仍 owned）。
        m_bindingCtx->retainValue(ret);
        auto runResult = std::make_unique<ScriptAsyncGameTestRunResult>(m_bindingCtx, ret);
        m_bindingCtx->releaseValue(ret); // 释放 run 局部的 ret 引用
        releaseTestObj(testObj);
        return runResult;
    }

    // 同步路径：JS 体返回非 Promise（普通值/undefined），测试体已执行完毕。
    // 通过/失败由 helper 的 succeed/fail 状态机接管（run 返 pass，instance 据 helper 状态判定）。
    m_bindingCtx->releaseValue(ret);
    releaseTestObj(testObj);
    return std::make_unique<SyncGameTestRunResult>(pass());
}

} // namespace mc::test
