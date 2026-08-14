#pragma once

#include "common/entity/ecs/context/EntityId.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"

#include <entt/entt.hpp>

namespace mc::ecs {

/**
 * @brief 实体上下文——ECS 实体的非拥有视图
 *
 * (EntityRegistry&, entt::basic_registry<EntityId>&, EntityId const) 三元组封装，
 * 提供组件查询/添加/删除入口。等价 entt::handle，但携带 registry 引用以支持
 * 后续弱引用/跨 registry 校验。
 *
 * Entity（OOP 句柄）内嵌一个 EntityContext 成员，通过它读写 ECS 组件数据；
 * EntityOwnerComponent 反向持有 unique_ptr<Entity>，构成双向桥接。
 *
 * 生命周期：EntityContext 不拥有实体，实体销毁后引用失效。跨帧持有需配合
 * registry.valid() 校验，或改用 EntityUniqueIDComponent 持久投影。
 *
 * 设计参考基岩版 EntityContext（mc/deps/ecs/gamerefs_entity/EntityContext.h）。
 */
class EntityContext {
public:
    EntityContext(EntityRegistry& registry, EntityId entity) noexcept
        : m_registry(registry), m_enttRegistry(registry.raw()), m_entity(entity) {}

    /** 所属 registry */
    [[nodiscard]] EntityRegistry& registry() noexcept { return m_registry; }
    [[nodiscard]] const EntityRegistry& registry() const noexcept { return m_registry; }

    /** 底层 entt registry */
    [[nodiscard]] entt::basic_registry<EntityId>& enttRegistry() noexcept { return m_enttRegistry; }
    [[nodiscard]] const entt::basic_registry<EntityId>& enttRegistry() const noexcept { return m_enttRegistry; }

    /** 实体 id */
    [[nodiscard]] EntityId entity() const noexcept { return m_entity; }

    /** 实体是否仍有效 */
    [[nodiscard]] bool valid() const noexcept { return m_enttRegistry.valid(m_entity); }

    /** 查询组件（const 重载，不存在返回 nullptr） */
    template <class T>
    [[nodiscard]] const T* tryGetComponent() const {
        return m_enttRegistry.try_get<T>(m_entity);
    }

    /** 查询组件（非 const 重载） */
    template <class T>
    [[nodiscard]] T* tryGetComponent() {
        return m_enttRegistry.try_get<T>(m_entity);
    }

    /** 是否拥有某组件 */
    template <class T>
    [[nodiscard]] bool hasComponent() const {
        return m_enttRegistry.all_of<T>(m_entity);
    }

    /** 获取组件引用（必须已存在） */
    template <class T>
    [[nodiscard]] T& getComponent() {
        return m_enttRegistry.get<T>(m_entity);
    }
    template <class T>
    [[nodiscard]] const T& getComponent() const {
        return m_enttRegistry.get<T>(m_entity);
    }

    /** 获取组件，不存在则原地构造一个 */
    template <class T, typename... Args>
    [[nodiscard]] T& getOrAddComponent(Args&&... args) {
        return m_enttRegistry.get_or_emplace<T>(m_entity, std::forward<Args>(args)...);
    }

    /** 添加/替换组件 */
    template <class T, typename... Args>
    T& addComponent(Args&&... args) {
        return m_enttRegistry.emplace_or_replace<T>(m_entity, std::forward<Args>(args)...);
    }

    /** 移除组件（不存在也安全） */
    template <class T>
    bool removeComponent() {
        return m_enttRegistry.remove<T>(m_entity);
    }

private:
    EntityRegistry& m_registry;
    entt::basic_registry<EntityId>& m_enttRegistry;
    EntityId const m_entity;
};

} // namespace mc::ecs
