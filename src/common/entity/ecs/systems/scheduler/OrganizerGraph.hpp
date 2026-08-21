#pragma once

#include "common/entity/ecs/context/EntityId.hpp"

#include <entt/entt.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace mc::ecs {

/**
 * @brief entt::organizer 的阶段内任务图封装
 *
 * 用 entt::organizer 从 system 遍历函数签名**自动推导 ro/rw 资源依赖**并建图，阶段内可
 * 按拓扑序执行无冲突顶点（阶段 H 接入并行执行器）。
 *
 * ## system 形态（free-function 化决策）
 * organizer 的自动依赖推导只对编译期 free function / member function 有效，且参数须是
 * entt 原生类型（view<>/group<>/basic_registry&）。故本封装提供三类注册入口：
 *
 * 1. **emplaceFreeFunction<Candidate, Req...>()**：注册 free function system。organizer
 *    从签名自动推导 ro（const T&）/ rw（T&）。真实业务 system 用此入口——view 中须显式
 *    列出真正读写的组件以正确建图，OOP 虚函数调用（经 EntityOwnerComponent 反查）作为
 *    副作用不参与推导。
 * 2. **emplaceMember<Candidate, Req...>(instance)**：注册 member function system + 实例，
 *    同样自动推导依赖（用于需持有状态的 system）。
 * 3. **emplaceSyncPoint(callback, payload, name)**：注册回调桥接壳，强制 sync_point=true
 *    （永远串行）。桥接壳委托 EntityManager 遍历方法（含模拟距离门控/ServerPlayer 短路），
 *    不是纯 view 遍历，无法被 organizer 推导，故强制串行（桥接壳本就该串行）。
 *
 * ## Registry 类型
 * organizer 的 Registry 模板参数用 entt::basic_registry<EntityId>（entt 原生），非项目自定义
 * EntityRegistry。调用方经 EntityRegistry::raw() 取底层 registry 传给 graph/tick。
 *
 * ## BuiltIn 指针稳定性契约（硬约束，见 ecs/README 坑位5）
 * organizer 构图/执行**绝不**对 BuiltIn 缓存的 5 组件 storage 调 compact/sort/swap-and-pop，
 * 否则 m_builtIn 裸指针悬垂。organizer 本身不调这些操作，但若阶段 H 并行执行器对任务排序
 * 也不得触及。本类注释此硬约束提醒后续维护者。
 */
class OrganizerGraph {
public:
    /// entt 原生 registry 类型（organizer 的 Registry 模板参数）
    using Registry = entt::basic_registry<EntityId>;
    /// organizer 类型（显式指定 Registry 为 basic_registry<EntityId>，非默认 entt::entity）
    using Organizer = entt::basic_organizer<Registry>;
    /// organizer 顶点类型（任务图节点）
    using Vertex = typename Organizer::vertex;

    OrganizerGraph() = default;

    /**
     * @brief 注册 free function system（自动推导依赖）
     *
     * @tparam Candidate free function 指针
     * @tparam Req 额外资源要求/访问模式覆盖（透传给 organizer 的 Req 参数）
     *
     * 真实业务 system 用此入口。函数签名参数须为 entt 原生 view<>/group<>/basic_registry&，
     * organizer 从 const 性推导 ro/rw。
     */
    template <auto Candidate, typename... Req>
    void emplaceFreeFunction(const char* name = nullptr)
    {
        m_organizer.emplace<Candidate, Req...>(name);
    }

    /**
     * @brief 注册 member function system + 实例（自动推导依赖）
     *
     * @tparam Candidate member function 指针（&Class::method）
     * @tparam Req 额外资源要求
     * @param instance method 所属实例引用
     */
    template <auto Candidate, typename... Req, typename Type>
    void emplaceMember(Type& instance, const char* name = nullptr)
    {
        m_organizer.emplace<Candidate, Req...>(instance, name);
    }

    /**
     * @brief 注册回调桥接壳（强制 sync_point 串行，无依赖推导）
     *
     * 桥接壳委托 EntityManager 遍历方法，无法被 organizer 推导，强制串行。
     *
     * @param callback 形如 void(const void* payload, Registry& reg) 的函数指针
     * @param payload 回调用户数据（通常指向 EntityManager 或 system 状态）
     * @param name system 名（profiling）
     */
    void emplaceSyncPoint(void (*callback)(const void*, Registry&), const void* payload, const char* name);

    /**
     * @brief 生成任务图邻接表
     *
     * 返回 organizer 的 vertex 列表，每个 vertex 含 callback/payload/prepare/in-edges/out-edges/name。
     * 调度器按拓扑序（in-edges 为空的顶点优先）执行；阶段 H 并行执行器按无冲突顶点并行。
     */
    [[nodiscard]] std::vector<Vertex> graph() const;

    /**
     * @brief 导出任务图为 Graphviz dot 格式（调试时序用）
     *
     * 遍历 vertex 的 out-edges 输出 `lhs->rhs`。用 Graphviz 渲染可可视化阶段内依赖关系，
     * 对排查 ECS 时序 bug 极有价值。
     */
    [[nodiscard]] std::string dumpDot() const;

    /** 当前注册的顶点数 */
    [[nodiscard]] std::size_t size() const noexcept;

    /** 清空所有注册的任务 */
    void clear();

private:
    Organizer m_organizer;
};

} // namespace mc::ecs
