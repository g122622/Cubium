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

#include "common/test/base/error/GameTestErrorType.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/function/SyncGameTestRunResult.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"
#include "server/test/facade/GameTestHelper.hpp"
#include "server/test/script/context/ScriptGameTestAccessor.hpp"
#include "server/test/script/context/ScriptGameTestFunctionContext.hpp"

#include <spdlog/spdlog.h>

namespace mc::test {

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
    // helper 由 MinecraftGameTestInstance 拥有，测试运行期间稳定；压入访问器供 JS 体回调取回。
    // 注意：IGameTestHelper 是 framework 接口，但 ScriptGameTestAccessor 存的是 facade GameTestHelper*
    // （facade 实现 IGameTestHelper，绑定层需 facade 具体类型的方法）。此处 helper 实际是 GameTestHelper
    // 实例（MinecraftGameTestHelperProvider 创建），static_cast 安全。
    auto* facadeHelper = static_cast<GameTestHelper*>(&helper);
    ScriptGameTestAccessor::instance().setCurrentHelper(facadeHelper);

    GameTestResult result = pass();

    if (m_bindingCtx == nullptr || m_jsCallback == nullptr) {
        result = fail(GameTestErrorType::LevelStateModificationFailed, "Script test function has no JS callback");
    } else {
        void* undef = m_bindingCtx->createUndefined();
        void* ret = m_bindingCtx->callFunction0(m_jsCallback, undef);
        m_bindingCtx->releaseValue(undef);

        if (m_bindingCtx->isException(ret)) {
            // JS 体抛异常 → 映射为 GameTestError(FailConditionsMet)，对齐基岩 JS Error 映射。
            void* exc = m_bindingCtx->getException();
            auto msg = m_bindingCtx->getExceptionMessage(exc);
            m_bindingCtx->releaseValue(exc);
            spdlog::warn("[GameTest] JS test body threw: {}", msg);
            result = fail(GameTestErrorType::FailConditionsMet, std::string(msg));
        }

        m_bindingCtx->releaseValue(ret);
    }

    ScriptGameTestAccessor::instance().clearCurrentHelper();
    return std::make_unique<SyncGameTestRunResult>(std::move(result));
}

} // namespace mc::test
