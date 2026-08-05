#include "server/test/minecraft/helper/MinecraftGameTestHelperProvider.hpp"

#include "server/test/facade/GameTestHelper.hpp" // GameTestHelper（1F facade 具体类）
#include "server/test/minecraft/instance/MinecraftGameTestInstance.hpp"
#include "common/util/assert/AssertMacros.hpp"

namespace mc::test {

MinecraftGameTestHelperProvider::MinecraftGameTestHelperProvider(mc::server::ServerWorld& world)
    : m_world(world)
{}

std::unique_ptr<IGameTestHelper> MinecraftGameTestHelperProvider::createGameTestHelper(BaseGameTestInstance& instance)
{
    // 向下转型取 MinecraftGameTestInstance 的 origin/bounds，构造绑 ServerWorld 的 GameTestHelper。
    // 传入 instance 引用：GameTestHelper 需回指 instance 以注册 runAtTickTime/succeedIf/failIf 等回调
    // 并读取 currentTick/maxTicks/rotation（对齐基岩 BaseGameTestHelper(BaseGameTestInstance&) 单阶段构造）。
    auto& mcInstance = static_cast<MinecraftGameTestInstance&>(instance);
    return std::make_unique<GameTestHelper>(mcInstance.world(), mcInstance.origin(), mcInstance.bounds(), instance);
}

std::unique_ptr<IGameTestHelperProvider> MinecraftGameTestHelperProvider::clone() const
{
    return std::make_unique<MinecraftGameTestHelperProvider>(m_world);
}

} // namespace mc::test
