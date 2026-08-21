#include "common/entity/ecs/systems/ticking/LegacyTick.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/entity/EntityManager.hpp"

namespace mc::ecs::sys {

void legacyTick(const void* payload, OrganizerGraph::Registry& /*registry*/)
{
    // payload 指向注册时传入的 EntityManager。委托其私有 _tickEntities()（经友元访问），
    // 内含 playerChunks 快照 + 遍历 m_entities 调 entity->tick() + 模拟距离门控 +
    // ServerPlayer 永远 tick + per-entity trace + isRemoved() 跳过。
    auto* manager = static_cast<mc::EntityManager*>(const_cast<void*>(payload));
    MC_ASSERT_RELEASE(manager != nullptr);
    manager->_tickEntities();
}

} // namespace mc::ecs::sys
