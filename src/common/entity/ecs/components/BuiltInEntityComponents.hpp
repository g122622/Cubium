#pragma once

#include "common/entity/ecs/components/AABBShapeComponent.hpp"
#include "common/entity/ecs/components/EntityRotationComponent.hpp"
#include "common/entity/ecs/components/StateVectorComponent.hpp"
#include "common/entity/ecs/components/VelocityComponent.hpp"

namespace mc::ecs {

/**
 * @brief 内建组件裸指针缓存
 *
 * 缓存 Entity 最高频访问的 4 个组件（StateVector / Velocity / AABBShape /
 * EntityRotation）的裸指针，使 position()/velocity() 等 getter 直接解引用指针，
 * 避免每次都走 entt registry 查询（position() 是 tick 内最高频调用，查询开销不可接受）。
 *
 * 对齐基岩版 BuiltInActorComponents（mc/world/actor/BuiltInActorComponents.h），
 * 基岩版缓存 mStateVectorComponent/mAABBShapeComponent/mActorRotationComponent/
 * mWalkAnimationComponent 四个 not_null 指针。
 *
 * 【指针稳定性契约】裸指针指向 entt pool 内部的组件数据。entt 的 sparse_set 实现
 * 保证：组件 emplace 后，只要不被 remove，其数据地址在 pool 生命周期内稳定（packed
 * array 分页存储，不会因其他实体的增删而移动已存在组件）。因此本结构缓存指针的
 * 前提是：首批4组件一旦 attach 永不移除。后续批次若引入组件的动态移除，须改用
 * EntityContext 查询或重设计缓存失效策略。此约束须在 ecs/README 坑位记录。
 *
 * 生命周期：由 Entity 在构造时（attach 完4组件后）填充，随 Entity 析构失效。
 * 不拥有所有权（纯视图），所有权归 entt registry。
 */
struct BuiltInEntityComponents {
    StateVectorComponent* stateVector = nullptr;
    VelocityComponent* velocity = nullptr;
    AABBShapeComponent* aabbShape = nullptr;
    EntityRotationComponent* rotation = nullptr;
};

} // namespace mc::ecs
