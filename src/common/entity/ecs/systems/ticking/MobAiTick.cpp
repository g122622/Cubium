#include "common/entity/ecs/systems/ticking/MobAiTick.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/entity/EntityManager.hpp"

namespace mc::ecs::sys {

void mobAiTick(const void* payload, OrganizerGraph::Registry& /*registry*/)
{
    // payload 指向注册时传入的 EntityManager。委托其私有 _tickMobAi()（经友元访问），
    // 复用 _tickEntities 的遍历+门控框架，对 dynamic_cast<MobEntity*> 成功的实体调
    // MobEntity::tickAiChain()（承载原 AI 链：UAF 防护 + senses/selector/navigator/controllers）。
    auto* manager = static_cast<mc::EntityManager*>(const_cast<void*>(payload));
    MC_ASSERT_RELEASE(manager != nullptr);
    manager->_tickMobAi();
}

} // namespace mc::ecs::sys
