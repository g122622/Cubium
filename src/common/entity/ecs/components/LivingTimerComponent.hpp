#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief LivingEntity 独立计时器组件
 *
 * 承载原 LivingEntity 的四个独立递减/递增计时器字段，这些字段在 LivingEntity::tick() 内
 * 是纯本地无同步、无强时序内聚的机械递减（除 fallFlyTicks 与 updateFallFlying 有跨阶段
 * 读取关系，见下），适合抽成 ECS system 每帧遍历处理。
 *
 * 由 ecs::sys::livingTimerTick（SystemPhase::PostEntityTick 阶段，在 EntityTick 之后）
 * 每帧推进。注册到 PostEntityTick 而非 PostMovement（EntityTick 之前）的关键原因见
 * fallFlyTicks 字段注释——fallFlyTicks 的递增必须晚于本帧 aiStep/updateFallFlying 的读取，
 * 以复刻原 OOP“末尾递增、下帧读 +1”语义。
 *
 * 字段语义（与原 LivingEntity 成员逐字一致）：
 * - hurtResistantTime：受伤无敌帧计时器（0..20）。LivingEntity::tick 每帧递减；
 *   hurt() 重置为 MAX_HURT_RESISTANT_TIME(20)；isInvulnerableTo()/hurt() 读它判定无敌。
 * - inCombat：是否处于战斗状态。actuallyHurt 受击时置 true；tick 超时检查置 false 并调
 *   sendEndCombat() 虚回调（当前无子类 override，空操作，system 内经 OOP 句柄回调）。
 * - lastDamageTimestamp：最后受击时间戳（ticksExisted）。与 inCombat 配合做超时判定；
 *   参与 NBT 序列化（HURT_BY_TIMESTAMP），重载后恢复。
 * - fallFlyTicks：鞘翅飞行已持续 tick 数。isElytraFlying() 时每帧 ++，否则归零。
 *   updateFallFlying()（aiStep 内，EntityTick 阶段）读 fallFlyTicks+1 做周期触发
 *   （每 10 tick 装备损坏 / ELYTRA_GLIDE 游戏事件）。不参与 NBT（运行时计时器，重载从 0 起）。
 *
 * 仅 LivingEntity attach（LivingEntity 构造期 emplace，普通 Entity 不 attach）。纯 POD
 * 可移动，低频写（每帧 1 次）、高频读（hurt/isInvulnerableTo/updateFallFlying 等处 getter），
 * 按约定不进 m_builtIn 缓存，走 tryGetComponent 查询（与 EnvironmentState/Portal/Fire/Freeze
 * 同范式，见 ecs/README 坑9）。
 *
 * ## 留 OOP 未迁入本组件的 LivingEntity 计时器（见 ecs/README 与计划阶段 D）
 * - swing 动画三件套（m_swingInProgress/m_swingProgressInt/m_swingProgress/m_prevSwingProgress）：
 *   本地玩家渲染路径（ClientApplicationSession/EntityRendererManager/PlayerArmPoseResolver）
 *   直读 common 层 mc::Player getter，且 prev/cur 同帧插值时序耦合——暂留 OOP（TODO）。
 * - swimAmount/swimAmountO：updateSwimAmount 依赖 isVisuallySwimming() 虚函数（DrownedEntity
 *   override），ECS system 无多态分发能力，强迁引入双写——暂留 OOP（TODO）。
 */
struct LivingTimerComponent {
    i32 hurtResistantTime{0};   // 受伤无敌帧计时器（0..MAX_HURT_RESISTANT_TIME=20）
    bool inCombat{false};       // 是否处于战斗状态
    i32 lastDamageTimestamp{0}; // 最后受击时间戳（ticksExisted），参与 NBT
    i32 fallFlyTicks{0};        // 鞘翅飞行持续 tick 数，updateFallFlying 读 +1 周期触发
};

} // namespace mc::ecs
