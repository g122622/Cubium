#pragma once

#include "common/entity/ecs/components/AABBShapeComponent.hpp"
#include "common/entity/ecs/components/EntityRotationComponent.hpp"
#include "common/entity/ecs/components/PhysicsStateComponent.hpp"
#include "common/entity/ecs/components/StateVectorComponent.hpp"
#include "common/entity/ecs/components/VelocityComponent.hpp"

namespace mc::ecs {

/**
 * @brief 内建组件裸指针缓存
 *
 * 缓存 Entity 最高频访问的组件的裸指针，使 getter 直接解引用指针，避免每次都走
 * entt registry 查询（position()/onGround() 等是 tick 内最高频调用，查询开销不可接受）。
 *
 * 对齐基岩版 BuiltInActorComponents（mc/world/actor/BuiltInActorComponents.h），
 * 基岩版缓存 mStateVectorComponent/mAABBShapeComponent/mActorRotationComponent/
 * mWalkAnimationComponent 四个 not_null 指针。
 *
 * 第二批新增 physicsState 指针：PhysicsState（onGround/fallDistance/collidedH/V）在
 * move()/checkOnGround()/updateFallDistance() 及各 tick 中有 40+ 处直接访问，是每 tick
 * 多次读写的真热路径，故破例进缓存（Fire/Portal/Freeze/HurtState 低频，仍走 tryGetComponent）。
 *
 * 【指针稳定性契约】裸指针指向 entt pool 内部的组件数据。5 个组件均声明
 * `static constexpr bool in_place_delete = true`，使 entt erase 实体时走 in_place_pop
 * （原地标记 tombstone，不移动 packed array 中其他元素），保证已 emplace 组件的数据
 * 地址在 registry 生命周期内稳定。这是缓存裸指针的前提——默认 in_place_delete=false
 * （可移动类型）时 erase 走 swap_and_pop，会把末尾元素 move 到被删位置，导致缓存了
 * 末尾元素地址的其他实体裸指针悬垂（实测：全批 GameTest 共享 EntityManager 时，某实体
 * erase 触发重排，存活实体的 m_pos 读到别的实体数据，见任务 #188）。
 *
 * 注意：in_place_pop 留下的 tombstone 仅在显式 compact()/sort() 时才会被填补重排，
 * 项目全仓不对这些组件 storage 调 compact/sort（已核查），故地址在 registry 生命周期
 * 内绝对稳定。后续若引入对这些 storage 的 compact/sort，须同步改为 EntityContext 实时
 * 查询或重设计缓存失效策略。此约束须在 ecs/README 坑位记录。
 *
 * 生命周期：由 Entity 在构造时（attach 完组件后）填充，随 Entity 析构失效。
 * 不拥有所有权（纯视图），所有权归 entt registry。
 */
struct BuiltInEntityComponents {
    StateVectorComponent* stateVector = nullptr;
    VelocityComponent* velocity = nullptr;
    AABBShapeComponent* aabbShape = nullptr;
    EntityRotationComponent* rotation = nullptr;
    PhysicsStateComponent* physicsState = nullptr;
};

} // namespace mc::ecs
