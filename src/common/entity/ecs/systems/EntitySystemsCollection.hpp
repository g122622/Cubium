#pragma once

#include "common/entity/ecs/context/EntityId.hpp"
#include "common/entity/ecs/systems/base/SystemCategory.hpp"
#include "common/entity/ecs/systems/base/SystemInfo.hpp"
#include "common/entity/ecs/systems/base/SystemPhase.hpp"
#include "common/entity/ecs/systems/scheduler/OrganizerGraph.hpp"
#include "common/entity/ecs/systems/scheduler/SystemProfiler.hpp"

#include <cstddef>
#include <string>

namespace mc::ecs {

class EntityRegistry;

/**
 * @brief 系统集合门面——多阶段编排 + 阶段内 organizer 依赖图执行
 *
 * 持有按 SystemPhase 分桶的 OrganizerGraph（每阶段一个）。tick() 遍历阶段，阶段内取
 * OrganizerGraph::graph() 的拓扑序逐顶点执行（首批顺序执行无并行，阶段 H 接入并行
 * 执行器）。EntityManager::tick() 委托本集合。
 *
 * ## 三类 system 注册入口（free-function 化决策）
 * 1. **registerFreeSystem<Candidate, Req...>(phase, info)**：注册 free function system，
 *    organizer 从签名自动推导 ro/rw 建依赖边。真实业务 system 用此入口（PortalTick/FireTick）。
 * 2. **registerMemberSystem<Candidate, Req...>(phase, info, instance)**：注册 member function
 *    system + 实例，自动推导依赖（用于需持有状态/委托宿主方法的 system）。
 * 3. **registerSyncSystem(phase, info, callback, payload)**：注册回调桥接壳，强制 sync_point
 *    串行。桥接壳委托宿主遍历方法（如 EntityManager 静态桥接方法调 _tickEntities），
 *    不是纯 view 遍历，无法被 organizer 推导。
 *
 * SystemInfo 在注册时显式提供（name/categories/blocking/usesEntityFactory），承载 organizer
 * 推导不出的元信息。
 *
 * ## 双 movement 路径占位
 * tickMovementCatchup / tickMovementCorrectionReplay 两专用路径首批空实现占位——
 * Cubium 客户端 ClientEntity 独立不跑物理，无消费方。category 含 Movement 的 system 本应
 * 进双路径，首批无此类 system。
 *
 * ## singleTick
 * singleTick(registry, entity) 针对单实体执行——首批无 system 实现该路径（占位空）。
 * 阶段后续按需补 free function 重载表。
 *
 * ## BuiltIn 指针稳定性契约（硬约束，见 ecs/README 坑位5）
 * tick() 执行绝不调 BuiltIn 缓存的 5 组件 storage 的 compact/sort/swap-and-pop。
 * organizer 执行器仅调 vertex 的 prepare/callback，不触及 storage 重排。
 */
class EntitySystemsCollection {
public:
    /// 回调桥接壳的函数签名（与 OrganizerGraph::Registry 一致）
    using SyncCallback = void (*)(const void*, OrganizerGraph::Registry&);

    EntitySystemsCollection() = default;
    ~EntitySystemsCollection() = default;

    EntitySystemsCollection(const EntitySystemsCollection&) = delete;
    EntitySystemsCollection& operator=(const EntitySystemsCollection&) = delete;
    EntitySystemsCollection(EntitySystemsCollection&&) noexcept = default;
    EntitySystemsCollection& operator=(EntitySystemsCollection&&) noexcept = default;

    /**
     * @brief 注册 free function system（自动推导依赖）
     *
     * @tparam Candidate free function 指针（auto 非类型模板参数，如 &sys::portalTick）
     * @tparam Req 额外资源要求/访问模式覆盖
     * @param phase 注册到哪个阶段
     * @param info system 元信息（name/categories/blocking/usesEntityFactory）
     */
    template <auto Candidate, typename... Req>
    void registerFreeSystem(SystemPhase phase, SystemInfo info)
    {
        graph(phase).emplaceFreeFunction<Candidate, Req...>(info.name);
        recordRegistration(phase, std::move(info));
    }

    /**
     * @brief 注册 member function system + 实例（自动推导依赖）
     *
     * @tparam Candidate member function 指针（如 &EntityManager::legacyTickBridge）
     * @tparam Req 额外资源要求
     * @param phase 注册到哪个阶段
     * @param info system 元信息
     * @param instance method 所属实例引用
     */
    template <auto Candidate, typename... Req, typename Type>
    void registerMemberSystem(SystemPhase phase, SystemInfo info, Type& instance)
    {
        graph(phase).emplaceMember<Candidate, Req...>(instance, info.name);
        recordRegistration(phase, std::move(info));
    }

    /**
     * @brief 注册回调桥接壳（强制 sync_point 串行，无依赖推导）
     *
     * 桥接壳委托宿主遍历方法，无法被 organizer 推导，强制串行。
     *
     * @param phase 注册到哪个阶段
     * @param info system 元信息（桥接壳通常 blocking=true）
     * @param callback 形如 void(const void* payload, Registry& reg) 的函数指针
     * @param payload 回调用户数据（通常指向宿主对象）
     */
    void registerSyncSystem(SystemPhase phase, SystemInfo info, SyncCallback callback, const void* payload);

    /**
     * @brief 执行所有阶段的 system tick
     *
     * 遍历 SystemPhase 枚举（按值递增即业务时序），阶段内取 OrganizerGraph::graph() 拓扑序
     * 逐顶点执行：prepare(registry) → callback(payload, registry)。首批顺序执行无并行。
     * 每个顶点执行前后触发 SystemProfiler 钩子（默认禁用零开销）。
     */
    void tick(EntityRegistry& registry);

    /**
     * @brief 客户端移动预测追帧专用路径（首批占位空实现）
     *
     * 应执行 category 含 UsedInClientMovementCorrections 的 system。Cubium 客户端 ClientEntity
     * 独立不跑物理，首批无此类 system，留空。
     */
    void tickMovementCatchup(EntityRegistry& registry);

    /**
     * @brief 服务端玩家移动校正回放专用路径（首批占位空实现）
     *
     * 应执行 category 含 UsedInServerPlayerMovement 的 system。首批无此类 system，留空。
     */
    void tickMovementCorrectionReplay(EntityRegistry& registry);

    /**
     * @brief 针对单实体执行 system（首批占位空实现）
     *
     * 对应单实体 tick 语义（如基岩版 IStrictTickingSystem::singleTick）。首批无 system 实现该
     * 路径，留空。后续按需补 free function 重载表。
     */
    void singleTick(EntityRegistry& registry, EntityId entity);

    /** SystemProfiler 访问器（注册钩子/开关 profiling） */
    [[nodiscard]] SystemProfiler& profiler() noexcept { return m_profiler; }
    [[nodiscard]] const SystemProfiler& profiler() const noexcept { return m_profiler; }

    /** 指定阶段已注册的 system 数（调试/断言用） */
    [[nodiscard]] std::size_t systemCount(SystemPhase phase) const noexcept;

    /** 已注册 system 总数（所有阶段） */
    [[nodiscard]] std::size_t totalSystemCount() const noexcept;

private:
    /// 取/惰性建指定阶段的 OrganizerGraph
    OrganizerGraph& graph(SystemPhase phase);
    [[nodiscard]] const OrganizerGraph& graph(SystemPhase phase) const;

    /// 记录注册（更新计数，首批 SystemInfo 仅存 name 用于 profiling，不持久持有）
    void recordRegistration(SystemPhase phase, SystemInfo info);

    /// 每阶段一个 OrganizerGraph（按 SystemPhase 枚举值索引）
    OrganizerGraph m_graphs[static_cast<u8>(SystemPhase::Count)];

    /// 每阶段已注册 system 数（systemCount/totalSystemCount 用）
    std::size_t m_counts[static_cast<u8>(SystemPhase::Count)]{};

    SystemProfiler m_profiler;
};

} // namespace mc::ecs
