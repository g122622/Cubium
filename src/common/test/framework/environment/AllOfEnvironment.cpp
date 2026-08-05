#include "common/test/framework/environment/AllOfEnvironment.hpp"

#include <algorithm>

namespace mc::test {

GameTestResult AllOfEnvironment::setup(BaseGameTestInstance& instance)
{
    for (auto& def : m_definitions) {
        if (!def) {
            continue;
        }
        if (GameTestResult r = def->setup(instance); !isPass(r)) {
            return r; // 任一失败即终止（对齐 Java 抛异常）
        }
    }
    return mc::test::pass();
}

GameTestResult AllOfEnvironment::teardown(BaseGameTestInstance& instance)
{
    // 逆序还原，忽略子环境 teardown 错误以保后续还原（对齐 Java 遍历 teardown 无错误传播）
    for (auto it = m_definitions.rbegin(); it != m_definitions.rend(); ++it) {
        if (*it) {
            (*it)->teardown(instance);
        }
    }
    return mc::test::pass();
}

} // namespace mc::test
