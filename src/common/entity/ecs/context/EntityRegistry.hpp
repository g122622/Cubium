#pragma once

#include "common/entity/ecs/context/EntityId.hpp"

#include <entt/entt.hpp>
#include <string>

namespace mc::ecs {

/**
 * @brief entt registry 的薄包装
 *
 * 内嵌 entt::basic_registry<EntityId>，额外持有调试名与 registry 标识，
 * 供 EntityContext 携带 registry 引用做弱引用/跨 registry 校验。
 *
 * 每个 IWorld（服务端 ServerWorld / 客户端 ClientWorld）持有一个 EntityRegistry
 * 实例；EntityManager 内部委托它做实体的 create/destroy 与组件存储。
 *
 * 设计参考基岩版 EntityRegistry（mc/deps/ecs/gamerefs_entity/EntityRegistry.h），
 * 但首批不引入弱引用体系（过渡期沿用现有 EntityInstanceId + graveyard 机制）。
 */
class EntityRegistry {
public:
    using Registry = entt::basic_registry<EntityId>;

    /**
     * @brief 构造 registry
     * @param debugName 调试用名称（如 "server"/"client"），仅用于日志与 profiling
     */
    explicit EntityRegistry(std::string debugName);

    /**
     * @brief 获取底层 entt registry
     *
     * 系统/工厂通过此入口访问 view/group/emplace 等 entt 原生 API。
     */
    [[nodiscard]] Registry& raw() noexcept { return m_registry; }
    [[nodiscard]] const Registry& raw() const noexcept { return m_registry; }

    /** 创建一个新实体（无组件） */
    [[nodiscard]] EntityId create() { return m_registry.create(); }

    /** 销毁实体及其所有组件 */
    void destroy(EntityId entity) { m_registry.destroy(entity); }

    /** 实体是否仍有效（未销毁且版本号匹配） */
    [[nodiscard]] bool valid(EntityId entity) const noexcept { return m_registry.valid(entity); }

    /** 调试名 */
    [[nodiscard]] const std::string& debugName() const noexcept { return m_debugName; }

private:
    std::string m_debugName;
    Registry m_registry;
};

} // namespace mc::ecs
