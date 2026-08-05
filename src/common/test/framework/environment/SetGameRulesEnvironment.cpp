#include "common/test/framework/environment/SetGameRulesEnvironment.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

GameTestResult SetGameRulesEnvironment::setup(BaseGameTestInstance& instance)
{
    // framework 层引擎无关，无法操作 ServerWorld/GameRules。由 minecraft 绑定层
    // MinecraftEnvironmentApplier::applySetup 经 dynamic_cast<SetGameRulesEnvironment> 接管。
    // TODO: GameRules 用类型化键（BooleanGameRuleKey/IntegerGameRuleKey）而非字符串名，
    // applier 须建立规则名→键映射表后调 getGameRules().setBoolean/setInt，当前 applier 记 warn 跳过。
    // 此 setup() 不再被 batch runner 调用，保留为接口占位。
    MC_UNUSED(instance);
    return mc::test::pass();
}

} // namespace mc::test
