#include "common/test/native/NativeTestRegistrationBuilder.hpp"

#include "common/test/framework/registry/GameTestRegistry.hpp"
#include "common/util/assert/AssertMacros.hpp"

namespace mc::test {

bool NativeTestRegistrationBuilder::registerTest()
{
    MC_ASSERT_RELEASE_MSG(m_body, "NativeTestRegistrationBuilder: test body is null");
    auto fn = std::make_shared<NativeGameTestFunction>(
        m_data.batchName(), m_testName, m_data.structure(), m_data, std::move(m_body));
    for (auto& tag : m_tags) {
        fn->addTag(tag);
    }
    return GameTestRegistry::instance().registerTestMethod(m_className, std::move(fn));
}

} // namespace mc::test
