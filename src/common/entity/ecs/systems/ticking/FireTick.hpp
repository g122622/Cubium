#pragma once

#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/FireComponent.hpp"
#include "common/entity/ecs/systems/base/EntityView.hpp"

namespace mc::ecs::sys {

/**
 * @brief 火焰 tick 系统（free function）
 *
 * 承载原 Entity::baseTick() 的 fire 链（行 751-779）：免疫→水中熄灭→每 20 tick 1 点
 * onFire 伤害→m_fire--→雨中扑灭。注册到 SystemPhase::PostEntityTick 阶段——在 EntityTick
 * 之后执行，读到本帧刚由环境感知产出的 isInWater/isInLava/isInRain 环境状态（无跨帧问题）。
 * 但 m_fire-- 递减结果下帧 baseTick 才读到，跨帧延迟 1 tick（用户已接受，单帧 50ms 玩家无感）。
 *
 * 抽取自 baseTick 的两段（逐字搬迁，行为等价）：
 *   1. if (m_fire > 0) { 免疫→clearFire; 水中→extinguishFire+setFireImmunityCooldown;
 *      else 每 20 tick && !isInLava → hurt(onFire,1.0f); m_fire-- }
 *   2. if (isInRain && isOnFire) { extinguishFire; setFireImmunityCooldown }
 *
 * 多态保留：isImmuneToFire/isInWater/isInLava/isInRain/hurt/extinguishFire/clearFire/
 * setFireImmunityCooldown 经 EntityOwnerComponent 反查 OOP Entity& 调用（isImmuneToFire
 * 等为虚函数）。m_fire 递减直接读写 FireComponent。
 *
 * baseTick 中岩浆削减 m_fallDistance（*=0.5）属 PhysicsState B 类，与 move 时序相关，
 * 留 baseTick 不迁入本系统。
 *
 * ## 签名与依赖推导
 * 参数为 entt 原生 basic_view（经 mc::ecs::EntityView 别名绑定 EntityId——entt::view 默认绑
 * entt::entity 与本项目 basic_registry<EntityId> 不匹配，见 base/EntityView.hpp）。organizer
 * 从 FireComponent&（非 const，rw）推导写依赖，从 EntityOwnerComponent&（非 const，rw）
 * 推导反查句柄依赖。
 */
void fireTick(entt::basic_registry<EntityId>& registry,
    mc::ecs::EntityView<entt::get_t<FireComponent, EntityOwnerComponent>> view);

} // namespace mc::ecs::sys
