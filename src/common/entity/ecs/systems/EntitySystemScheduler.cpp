#include "common/entity/ecs/systems/EntitySystemScheduler.hpp"

#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::ecs {

using namespace mc::trace;

void EntitySystemScheduler::registerSystem(SystemPhase phase, std::shared_ptr<ITickingSystem> system)
{
    MC_ASSERT_RELEASE(system != nullptr);
    const auto idx = static_cast<u8>(phase);
    MC_ASSERT_RELEASE(idx < static_cast<u8>(SystemPhase::Count));
    m_systems[idx].push_back(std::move(system));
}

void EntitySystemScheduler::tick(EntityRegistry& registry)
{
    for (u8 phase = 0; phase < static_cast<u8>(SystemPhase::Count); ++phase) {
        for (auto& system : m_systems[phase]) {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Server.Tick, "EntitySystemScheduler::tick.system", "name", system->name());
            system->tick(registry);
        }
    }
}

} // namespace mc::ecs
