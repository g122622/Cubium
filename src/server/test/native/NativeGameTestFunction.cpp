#include "server/test/native/NativeGameTestFunction.hpp"

#include "common/util/assert/AssertAll.hpp"                            // MC_UNUSED
#include "server/test/framework/function/IGameTestFunctionContext.hpp" // EmptyGameTestFunctionContext
#include "server/test/framework/function/SyncGameTestRunResult.hpp"

namespace mc::test {

std::unique_ptr<IGameTestFunctionContext> NativeGameTestFunction::createContext(IGameTestHelper& helper) const
{
    MC_UNUSED(helper);
    // 原生测试无额外上下文，用空实现（脚本测试才需持 helper 句柄）
    return std::make_unique<EmptyGameTestFunctionContext>();
}

std::unique_ptr<IGameTestFunctionRunResult> NativeGameTestFunction::run(
    IGameTestHelper& helper, IGameTestFunctionContext& context) const
{
    MC_UNUSED(context);
    GameTestResult result = m_body ? m_body(helper) : mc::test::pass();
    return std::make_unique<SyncGameTestRunResult>(std::move(result));
}

} // namespace mc::test
