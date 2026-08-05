#include "server/test/facade/GameTestRegistrar.hpp"

namespace mc::test {

NativeTestRegistrationBuilder GameTestRegistrar::create(
    std::string className, std::string testName, NativeGameTestFunction::TestBody body)
{
    return NativeTestRegistrationBuilder(std::move(className), std::move(testName), std::move(body));
}

} // namespace mc::test
