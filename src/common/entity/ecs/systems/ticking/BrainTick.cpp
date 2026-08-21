#include "common/entity/ecs/systems/ticking/BrainTick.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/entity/EntityManager.hpp"

namespace mc::ecs::sys {

void brainTick(const void* payload, OrganizerGraph::Registry& /*registry*/)
{
    // payload 指向注册时传入的 EntityManager。委托其私有 _tickBrains()（经友元访问），
    // 复用 _tickEntities 的遍历+门控框架，对 dynamic_cast<VillagerEntity*> 成功的实体
    // 调 brain().tick()（当前仅 VillagerEntity 持 Brain）。
    auto* manager = static_cast<mc::EntityManager*>(const_cast<void*>(payload));
    MC_ASSERT_RELEASE(manager != nullptr);
    manager->_tickBrains();
}

} // namespace mc::ecs::sys
