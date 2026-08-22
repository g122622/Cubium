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

    /// 移动主体：travel/aiStep/moveWithCollision 强时序内聚链的调度入口。当前无消费方。
    /// 阶段 C+F 计划曾拟在此落 MobAiTickSystem + AiStepSystem，但勘察发现计划前提错误（见下文
    /// AiStep 注释与 ecs/README 坑24）：当前 OOP 本就是跨帧 1-tick（AI 链在 aiStep 之后，对齐
    /// vanilla），且 aiStep 是 sync_point 串行整块抽成独立 system 调度收益为零却引入 fallFlyTicks
    /// 时序脆弱化。故 C+F 形态 A 决策：仅 AI 链抽 system 落 PostEntityTick，aiStep 留 LivingEntity::tick。
    Travel,

    /// 移动后处理：预留（如移动完成的实体相关收尾）。阶段 E 计划曾拟把乘客位置同步（RideTickSystem）
    /// 落本阶段，但勘察发现 PostTravel/PostMovement 均在 EntityTick 之前（载具本帧 OOP tick 内的
    /// aiStep/travel/move 移动尚未发生），落此处读到的仍是上一帧位置（无效搬迁）。故 RideTickSystem
    /// 实际注册到 PostEntityTick（EntityTick 之后），见该阶段注释与 ecs/README 坑23。本阶段当前无消费方。
    PostTravel,

    /// 常规 tick：玩家专用计时器等与移动无强时序耦合的递减。预留——阶段 G 落地 PlayerTimerSystem。
    NormalTick,

    /// AI 步进：当前无消费方。阶段 C+F 计划曾拟在此落 MobAiTickSystem（AI 控制器链）+ AiStepSystem
    /// （travel 消费）双 system 同帧，假设"AI 控制器输出 moveForward，AiStep 的 travel 同帧消费"。
    /// 勘察证明此假设是错的——当前 OOP 本就是跨帧 1-tick（MobEntity::tick 先 LivingEntity::tick 含
    /// aiStep/travel 消费上一帧输入，后 AI 链写下一帧输入），与 vanilla MobEntity.tick（先 super.tick
    /// 后 serverAiStep）同构；若改同帧消费会消除 1-tick 延迟偏离 vanilla。且本阶段在 EntityTick 之前，
    /// 误注册会破坏 baseTick 时序。故 C+F 形态 A 决策：AI 链抽到 mobAiTick 落 PostEntityTick（保持
    /// 跨帧语义），aiStep 留 LivingEntity::tick 不抽。详见 ecs/README 坑24、ecs-wiggly-cat.md 阶段 C+F。
    AiStep,

    /// 移动应用：预留（与 Travel/AiStep 重叠语义，首批无消费方）。
    Move,

    /// 移动后收尾计时器：预留（LivingEntity 独立计时器递减曾计划落本阶段，但 fallFlyTicks
    /// 时序约束要求递增晚于 EntityTick 内 updateFallFlying 的读取，故 LivingTimerSystem 实际
    /// 注册到 PostEntityTick，见该阶段注释）。当前无消费方，留待未来移动链后置逻辑接入。
    PostMovement,

    /// 移动状态复位：预留（如清速度标志、resetMovement）。
    ResetMovement,

    /// 实体主 tick：承载桥接壳（委托 EntityManager 遍历调每个实体的 Entity::tick() 虚函数链）。
    /// 首批注册的 system 之一。
    EntityTick,

    /// 实体主 tick 之后：承载状态递减/环境交互类 system（PortalTick / FireTick / BrainTick）。
    /// 此阶段在 EntityTick 之后执行，可读到本帧 baseTick 产出的环境状态。
    /// 首批注册的 system 之一。抽 system 到本阶段引入跨帧延迟 1 tick（递减结果下帧才读到）。
    /// 阶段 D 落地 LivingTimerSystem（livingTimerTick free function，递减 hurtResistantTime /
    /// combatTimeout 超时检查 / fallFlyTicks 递增），fallFlyTicks 时序硬约束详见
    /// LivingTimer.hpp 注释——递增须晚于 EntityTick 内 updateFallFlying 的读取，故落本阶段而非
    /// PostMovement，1-tick 延迟恰好复刻原 OOP 末尾递增语义。
    /// 阶段 E 落地 RideTickSystem（rideTick free function，载具主动同步乘客位置，经虚
    /// updatePassengerPosition 派发到 boat/horse 子类 override）。落本阶段是消除乘客位置 1-tick
    /// 滞后的唯一正确选择（PostMovement/PostTravel 在 EntityTick 前，载具本帧未移动，无效搬迁），
    /// 见 RideTick.hpp 注释与 ecs/README 坑23。
    /// 阶段 C+F 落地 mobAiTick（mobAiTick free function 桥接壳，sync_point 串行，承载原
    /// MobEntity::tick 中 AI 链 + 前置 UAF 防护，委托 _tickMobAi 调 MobEntity::tickAiChain）。
    /// 落本阶段（EntityTick 之后）是保持 1-tick 跨帧语义的关键：aiStep→travel 在 EntityTick 内
    /// 消费上一帧 AI 输入，mobAiTick 在其后写下一帧输入——与 vanilla 等价。aiStep 不抽 system
    /// （见 AiStep 注释与 ecs/README 坑24）。注册在 brainTick 之后、livingTimerTick 之前。
    PostEntityTick,

    /// 阶段数（须放末尾，作桶数组上界）
    Count
};

} // namespace mc::ecs
