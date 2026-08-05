#include "common/test/framework/batch/GameTestBatch.hpp"

#include "common/test/framework/function/BaseGameTestFunction.hpp"

namespace mc::test {

GameTestBatch::GameTestBatch(std::string name,
    std::vector<std::shared_ptr<BaseGameTestFunction>> testFunctions,
    std::function<void()> beforeBatch,
    std::function<void()> afterBatch,
    std::shared_ptr<TestEnvironmentDefinition> environment)
    : m_name(std::move(name))
    , m_testFunctions(std::move(testFunctions))
    , m_beforeBatch(std::move(beforeBatch))
    , m_afterBatch(std::move(afterBatch))
    , m_environment(std::move(environment))
{}

} // namespace mc::test
