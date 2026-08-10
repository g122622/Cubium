#pragma once

namespace mc {
class Entity;
} // namespace mc

namespace mc::ecs {

/**
 * @brief Entity 所有权组件——ECS 反向持有 OOP 实体对象（非拥有裸指针）
 *
 * 每个有 OOP Entity 的实体都挂一个 EntityOwnerComponent，其内持 `Entity*` 裸指针
 * （非拥有）。系统遍历若需调用 OOP 行为，签名收 EntityOwnerComponent&，从中取
 * m_entity 调虚函数。
 *
 * 【所有权说明】OOP Entity 的所有权归 EntityManager::m_entities（unique_ptr），
 * 或测试中的局部变量/容器。EntityOwnerComponent 仅持非拥有指针，供 ECS→OOP
 * 反查。Entity 构造末尾 self-attach `EntityOwnerComponent{this}`；Entity 析构时
 * 经 registry.destroy() 销毁其 entt 实体（含本组件），故指针不会悬垂。
 *
 * 这是 ECS↔OOP 双向桥接的「反向」半：Entity 内嵌 EntityContext（正向），
 * EntityOwnerComponent 持 Entity*（反向）。设计参考基岩版
 * ActorOwnerComponent（mc/entity/components/ActorOwnerComponent.h，持 `Actor&` 非拥有）。
 *
 * 【为何是裸指针而非 unique_ptr】原设计为 unique_ptr<Entity>（拥有），与
 * EntityManager::m_entities 的 unique_ptr 形成双重拥有致 double-free，故该组件
 * 此前从未被 attach，导致 FireTickSystem/PortalTickSystem 的
 * view<..., EntityOwnerComponent> 永远为空、两个系统在生产与测试中均失效。
 * 改为非拥有裸指针后由 Entity 构造 self-attach，所有权仍归 EntityManager。
 */
class EntityOwnerComponent {
public:
    explicit EntityOwnerComponent(Entity* entity) noexcept
        : m_entity(entity)
    {}

    [[nodiscard]] Entity& entity() const noexcept { return *m_entity; }
    [[nodiscard]] Entity* tryGetEntity() const noexcept { return m_entity; }

private:
    Entity* m_entity;
};

} // namespace mc::ecs
