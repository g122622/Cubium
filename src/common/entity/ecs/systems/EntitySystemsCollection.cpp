#include "common/entity/ecs/systems/EntitySystemsCollection.hpp"

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::ecs {

using namespace mc::trace;

void EntitySystemsCollection::registerSyncSystem(
    SystemPhase phase, SystemInfo info, SyncCallback callback, const void* payload)
{
    graph(phase).emplaceSyncPoint(callback, payload, info.name);
    recordRegistration(phase, std::move(info));
}

void EntitySystemsCollection::tick(EntityRegistry& registry)
{
    auto& rawRegistry = registry.raw();

    for (u8 phaseIdx = 0; phaseIdx < static_cast<u8>(SystemPhase::Count); ++phaseIdx) {
        const auto phase = static_cast<SystemPhase>(phaseIdx);
        const auto& g = graph(phase);
        if (g.size() == 0) {
            continue; // 空阶段跳过，避免 graph() 构建邻接表的开销
        }

        // 取拓扑序顶点列表。organizer 的 graph() 返回邻接表，顶点已按依赖拓扑序排列
        // （in-edges 为空者在前），首批顺序执行无并行。
        const auto vertices = g.graph();
        for (const auto& vertex : vertices) {
            const char* name = vertex.name();
            m_profiler.preInvoke(name);
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick,
                "EntitySystemsCollection::tick.system",
                "name",
                name != nullptr ? name : "anon");
            // prepare 注册顶点时声明的资源准备回调（free/member system 的 view 绑定等），
            // sync_point 顶点 prepare 为空。
            vertex.prepare(rawRegistry);
            // vertex.callback() 返回 organizer 类型擦除的调用入口 function_type*（形如
            // void(*)(const void*, Registry&)）。sync_point 顶点与 free/member system 顶点
            // 返回同类型，统一经 data() 取 payload 调用。
            auto cb = vertex.callback();
            MC_ASSERT_RELEASE(cb != nullptr);
            cb(vertex.data(), rawRegistry);
            m_profiler.postInvoke(name);
        }
    }
}

void EntitySystemsCollection::tickMovementCatchup(EntityRegistry& /*registry*/)
{
    // 占位空实现：应执行 category 含 UsedInClientMovementCorrections 的 system。
    // Cubium 客户端 ClientEntity 独立不跑物理，首批无此类 system。
    // TODO(阶段C/F后续): 接入客户端移动预测追帧逻辑时实现。
}

void EntitySystemsCollection::tickMovementCorrectionReplay(EntityRegistry& /*registry*/)
{
    // 占位空实现：应执行 category 含 UsedInServerPlayerMovement 的 system。
    // 首批无此类 system。
    // TODO(阶段C/F后续): 接入服务端玩家移动校正回放逻辑时实现。
}

void EntitySystemsCollection::singleTick(EntityRegistry& /*registry*/, EntityId /*entity*/)
{
    // 占位空实现：针对单实体执行 system。
    // 首批无 system 实现该路径。
    // TODO(后续批次): 按需补 free function 重载表，实现单实体 tick 语义。
}

std::size_t EntitySystemsCollection::systemCount(SystemPhase phase) const noexcept
{
    const auto idx = static_cast<u8>(phase);
    MC_ASSERT_RELEASE(idx < static_cast<u8>(SystemPhase::Count));
    return m_counts[idx];
}

std::size_t EntitySystemsCollection::totalSystemCount() const noexcept
{
    std::size_t total = 0;
    for (u8 i = 0; i < static_cast<u8>(SystemPhase::Count); ++i) {
        total += m_counts[i];
    }
    return total;
}

OrganizerGraph& EntitySystemsCollection::graph(SystemPhase phase)
{
    const auto idx = static_cast<u8>(phase);
    MC_ASSERT_RELEASE(idx < static_cast<u8>(SystemPhase::Count));
    return m_graphs[idx];
}

const OrganizerGraph& EntitySystemsCollection::graph(SystemPhase phase) const
{
    const auto idx = static_cast<u8>(phase);
    MC_ASSERT_RELEASE(idx < static_cast<u8>(SystemPhase::Count));
    return m_graphs[idx];
}

void EntitySystemsCollection::recordRegistration(SystemPhase phase, SystemInfo /*info*/)
{
    const auto idx = static_cast<u8>(phase);
    MC_ASSERT_RELEASE(idx < static_cast<u8>(SystemPhase::Count));
    ++m_counts[idx];
    // 首批 SystemInfo 仅 name 被 OrganizerGraph 用于 profiling（emplace* 已取走 name），
    // 其余字段（categories/blocking/usesEntityFactory）暂不持久持有——阶段 H 并行执行器
    // 与 category 分桶落地时再持久化。
    // TODO(阶段H): 持久化 SystemInfo 供并行执行器判 blocking 与 category 分桶。
}

} // namespace mc::ecs
