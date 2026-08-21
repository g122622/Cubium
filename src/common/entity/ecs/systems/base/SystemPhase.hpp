#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 系统 tick 阶段枚举
 *
 * 阶段间按固定顺序执行，编码业务时序。阶段名即时序意图，读注册代码可理解粗时序，
 * 避免 ECS 时序 bug 难查的问题。
 *
 * 阶段命名采用业务语义（环境感知 / 移动 / 实体主 tick / 主 tick 之后），首批实际注册
 * system 的只有 EntityTick 与 PostEntityTick 两个，其余阶段为预留空桶——后续批次按
 * 业务时序逐步把 OOP tick 链拆成对应阶段的真实 system（见 ECS 改造路线图）。
 *
 * 执行顺序按枚举值递增。新增阶段插在 Count 之前，所有阶段消费方（Collection 的桶数组、
 * tick() 遍历）以 Count 为界，新增不破坏既有。
 */
enum class SystemPhase : u8 {
    /// 环境感知：刷新实体所在流体状态（水中/岩浆中/眼中浸入等），供后续阶段消费。
    /// 在 EntityTick 之前，同帧产出同帧消费（无跨帧延迟）。预留——阶段 B 落地 EnvironmentSensingSystem。
    EnvironmentSense,

    /// 移动前处理：预留（如骑乘载具位置同步前置、leash 物理约束）。
    PreTravel,

    /// 移动主体：travel/aiStep/moveWithCollision 强时序内聚链的调度入口。预留——
    /// 阶段 C+F 落地 MobAiTickSystem + AiStepSystem（回调桥接壳，整块逻辑留 OOP）。
    Travel,

    /// 移动后处理：移动完成的实体相关收尾（如乘客位置同步）。预留——阶段 E 落地 RideTickSystem。
    PostTravel,

    /// 常规 tick：玩家专用计时器等与移动无强时序耦合的递减。预留——阶段 G 落地 PlayerTimerSystem。
    NormalTick,

    /// AI 步进：MobAiTickSystem（AI 控制器链）+ AiStepSystem（travel 消费）。
    /// 预留——阶段 C+F 落地，在 Travel 之后（AI 控制器输出 moveForward，AiStep 的 travel 消费）。
    AiStep,

    /// 移动应用：预留（与 Travel/AiStep 重叠语义，首批无消费方）。
    Move,

    /// 移动后收尾计时器：LivingEntity 独立计时器递减（hurtResistantTime/swing/fallFly 等）。
    /// 预留——阶段 D 落地 LivingTimerSystem。
    PostMovement,

    /// 移动状态复位：预留（如清速度标志、resetMovement）。
    ResetMovement,

    /// 实体主 tick：承载桥接壳（委托 EntityManager 遍历调每个实体的 Entity::tick() 虚函数链）。
    /// 首批注册的 system 之一。
    EntityTick,

    /// 实体主 tick 之后：承载状态递减/环境交互类 system（PortalTick / FireTick / BrainTick）。
    /// 此阶段在 EntityTick 之后执行，可读到本帧 baseTick 产出的环境状态。
    /// 首批注册的 system 之一。抽 system 到本阶段引入跨帧延迟 1 tick（递减结果下帧才读到）。
    PostEntityTick,

    /// 阶段数（须放末尾，作桶数组上界）
    Count
};

} // namespace mc::ecs
