#pragma once

#include <memory>

namespace mc {
class Entity;
} // namespace mc

namespace mc::ecs {

/**
 * @brief Entity 所有权组件——ECS 反向持有 OOP 实体对象
 *
 * 每个有 OOP Entity 的实体都挂一个 EntityOwnerComponent，其内 unique_ptr<Entity>
 * 拥有该 Entity 对象。系统遍历若需调用 OOP 行为，签名收 EntityOwnerComponent&，
 * 从中取 m_entity 调虚函数。
 *
 * 这是 ECS↔OOP 双向桥接的「反向」半：Entity 内嵌 EntityContext（正向），
 * EntityOwnerComponent 持 unique_ptr<Entity>（反向）。设计参考基岩版
 * ActorOwnerComponent（mc/entity/components/ActorOwnerComponent.h）。
 */
class EntityOwnerComponent {
public:
    explicit EntityOwnerComponent(std::unique_ptr<Entity> entity) : m_entity(std::move(entity)) {}

    [[nodiscard]] Entity& entity() const noexcept { return *m_entity; }
    [[nodiscard]] Entity* tryGetEntity() const noexcept { return m_entity.get(); }

private:
    std::unique_ptr<Entity> m_entity;
};

} // namespace mc::ecs
