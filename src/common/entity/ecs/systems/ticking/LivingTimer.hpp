#pragma once

#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/LivingTimerComponent.hpp"
#include "common/entity/ecs/systems/base/EntityView.hpp"

namespace mc::ecs::sys {

/**
 * @brief LivingEntity 独立计时器 tick 系统（free function）
 *
 * 承载原 LivingEntity::tick() 内三处独立计时器推进逻辑，逐字搬迁：
 * 1. hurtResistantTime 递减（>0 则 --，原 tick 行 914-916）。
 * 2. combatTimeout 超时检查（inCombat && ticksExisted - lastDamageTimestamp >
 *    CombatTracker::COMBAT_TIMEOUT → inCombat=false + 调 sendEndCombat() 虚回调，
 *    原 tick 行 963-966）。sendEndCombat 经 EntityOwnerComponent 反查 LivingEntity* 调用。
 * 3. fallFlyTicks 递增/归零（isElytraFlying() ? ++ : =0，原 tick 行 976-980）。
 *
 * 注册到 SystemPhase::PostEntityTick 阶段（EntityTick 之后）。此阶段选择是 fallFlyTicks
 * 时序正确性的硬约束，详见下方时序分析。
 *
 * ## fallFlyTicks 跨阶段时序分析（验证 entt 阶段排序解法）
 * 原 OOP 时序（LivingEntity::tick 内）：
 *   - tick 中段：aiStep() → updateFallFlying() 读 m_fallFlyTicks + 1（行 1679）做周期触发
 *   - tick 末尾：if (isElytraFlying()) ++m_fallFlyTicks; else =0（行 977）
 * 即 updateFallFlying 用“+1”预测本帧递增后的值，递增延后到 tick 末尾补上——读用预测、
 * 写延后，二者自洽。updateFallFlying 实际读到的是“上一帧末尾递增后的值 + 1”。
 *
 * 迁移后（LivingTimerSystem 在 PostEntityTick，即 EntityTick 之后）：
 *   - EntityTick 阶段：LivingEntity::tick → aiStep → updateFallFlying 读 fallFlyTicks + 1
 *   - PostEntityTick 阶段：本 system 递增 fallFlyTicks
 * 故 updateFallFlying 读到“上一帧 PostEntityTick 递增后的值 + 1”，与原 OOP 完全等价
 * （原 OOP 也是末尾递增、下帧 aiStep 读 +1）。entt 的阶段枚举顺序（PostEntityTick 在
 * EntityTick 之后）保证了这个顺序，1-tick 延迟恰好复刻原语义，无需额外同步原语。
 *
 * 若误注册到 PostMovement（EntityTick 之前），递增会先于本帧 updateFallFlying 发生，
 * 导致 updateFallFlying 读到“本帧已递增的值 + 1”，比原 OOP 多 1，周期触发提前 1 tick，
 * 破坏鞘翅飞行装备损坏 / ELYTRA_GLIDE 事件节奏——故 PostEntityTick 是唯一正确选择。
 *
 * ## hurtResistantTime / combatTimeout 的 1-tick 延迟
 * 两者原在 tick 中段推进，迁到 PostEntityTick（tick 之后）后递减/检查延后到帧末。
 * hurtResistantTime：hurt()/isInvulnerableTo() 在 tick 间事件触发，读到的是“上次 tick
 * 递减后”的值，无论递减在 tick 中段还是末尾都等价（20 tick 容错窗口，1 tick 无意义）。
 * combatTimeout：100 tick 超时窗口，1 tick 延迟无影响。均符合“抽 system 到 PostEntityTick
 * 引入 1 tick 延迟可接受”（计划坑位11）。
 *
 * ## 未迁入本 system 的 LivingEntity 计时器（留 OOP + TODO）
 * - swing 动画三件套：本地玩家渲染路径直读 common 层 mc::Player getter，prev/cur 同帧
 *   插值时序耦合，暂留 OOP（LivingEntity::tick 行 926-937 + swing()）。
 * - swimAmount/swimAmountO：updateSwimAmount 依赖 isVisuallySwimming() 虚函数
 *   （DrownedEntity override），ECS 无多态分发，暂留 OOP（LivingEntity::tick 行 986）。
 *
 * ## 签名与依赖推导
 * 参数为 entt 原生 basic_view（经 mc::ecs::EntityView 别名绑定 EntityId）。organizer 从
 * LivingTimerComponent&（非 const，rw）推导写依赖，从 EntityOwnerComponent&（非 const，rw）
 * 推导反查句柄依赖。view 显式列出真正读写的组件以正确建图。
 */
void livingTimerTick(entt::basic_registry<EntityId>& registry,
    mc::ecs::EntityView<entt::get_t<LivingTimerComponent, EntityOwnerComponent>> view);

} // namespace mc::ecs::sys
