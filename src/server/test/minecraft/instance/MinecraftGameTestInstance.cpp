#include "server/test/minecraft/instance/MinecraftGameTestInstance.hpp"

#include "common/test/framework/listener/IGameTestListener.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::test {

MinecraftGameTestInstance::MinecraftGameTestInstance(const BaseGameTestFunction& function,
    std::unique_ptr<IGameTestHelperProvider> helperProvider,
    mc::server::ServerWorld& world,
    BlockPos origin)
    : BaseGameTestInstance(function, std::move(helperProvider))
    , m_world(world)
    , m_origin(origin)
{}

bool MinecraftGameTestInstance::hasStructureBlock() const noexcept
{
    return m_structurePlaced;
}

void MinecraftGameTestInstance::clearStructure()
{
    if (m_bounds) {
        m_placer.clearArea(m_world, *m_bounds, function().data().padding());
    }
    m_bounds.reset();
    m_structurePlaced = false;
}

void MinecraftGameTestInstance::spawnStructure()
{
    MC_ASSERT_RELEASE_MSG(!m_structurePlaced, "structure already spawned");
    m_bounds = m_placer.place(m_world, m_origin, function().data());
    if (m_bounds) {
        m_structurePlaced = true;
        notifyStructureLoaded();
    } else {
        // 结构放置失败：直接 fail（携带 MethodNotImplemented 占位，TODO 细化错误码）
        fail(GameTestError{GameTestErrorType::MethodNotImplemented,
            "Failed to spawn structure '{0}' for test '{1}'",
            {function().data().structure(), function().testName()}});
    }
}

i32 MinecraftGameTestInstance::_getLevelTick() const
{
    // u64 → i32 截断：runAtTickTime 用相对 tick，正常范围远小于 i32 上限
    return static_cast<i32>(m_world.currentTick());
}

bool MinecraftGameTestInstance::_isTestReady()
{
    return m_structurePlaced;
}

} // namespace mc::test
