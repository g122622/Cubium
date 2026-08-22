#pragma once

#include "common/entity/ecs/systems/scheduler/OrganizerGraph.hpp"

namespace mc::ecs::sys {

/**
 * @brief Mob AI 链 tick 桥接系统（free function · sync_point）
 *
 * 将 MobEntity::tick() 中的 AI 链调用块（前置 m_attackTarget/m_lastHurtBy 的 isRemoved
 * UAF 防护 + senses/targetSelector/goalSelector/navigator/updateAITasks/
 * updateMovementGoalFlags/moveController/lookController/jumpController）抽成独立 system，
 * 注册到 SystemPhase::PostEntityTick 阶段。通过 payload（指向 EntityManager）委托
 * EntityManager::_tickMobAi()，复用 _tickEntities 的遍历+门控框架（playerChunks 快照/
 * isRemoved 跳过/ServerPlayer 短路/模拟距离门控），对 dynamic_cast<MobEntity*> 成功的
 * 实体调 MobEntity::tickAiChain()（承载原 AI 链逻辑）。
 *
 * 与 brainTick 同属回调委托型范式（区别在于：
 * - brainTick 仅桥接持有 Brain 的实体（当前仅 VillagerEntity）的 m_brain->tick() 调用；
 * - mobAiTick 桥接所有 MobEntity（怪物/动物等）的 AI 控制器链 tickAiChain() 调用）。
 *
 * ## 时序：保持 1-tick 跨帧语义（阶段 C+F 形态 A）
 * aiStep→travel 仍在 LivingEntity::tick()（EntityTick 阶段，早于 PostEntityTick）内执行，
 * 消费上一帧 AI 链写入的 m_moveForward/moveStrafing；本 mobAiTick 在 EntityTick 之后
 * （PostEntityTick）执行，AI 链写入的输入供下一帧 aiStep 消费。这与 vanilla
 * MobEntity.tick()（先 super.tick() 含 aiStep/travel，后 serverAiStep）时序等价——
 * aiStep 读上一帧 AI 输出、AI 链写下一帧输入，天然 1-tick 跨帧延迟。
 *
 * 关键：aiStep **不**抽成独立 system（不抽 AiStepSystem）。勘察证明：
 * - 计划原假设"MobAiTickSystem 在前、AiStepSystem 在后同帧消费"是错的——当前 OOP 本就是
 *   跨帧（AI 链在 aiStep 之后），vanilla 同构；
 * - SystemPhase::AiStep 阶段在 EntityTick **之前**（非之后），误注册会破坏 baseTick 时序；
 * - aiStep 是 sync_point 串行整块（travel/moveWithCollision 留 OOP），organizer 不会并行它，
 *   抽成独立 system 调度收益为零，却引入 fallFlyTicks 跨阶段时序脆弱化（updateFallFlying 在
 *   aiStep 内读 fallFlyTicks+1，LivingTimerSystem 在 PostEntityTick 递增，aiStep 抽出后
 *   二者同阶段顺序依赖注册序）+ 4 类级联时序变化。故 aiStep 留 LivingEntity::tick 不动，
 *   仅 AI 链抽 system。详见 ecs/README 坑24、ecs-wiggly-cat.md 阶段 C+F 计划前提修正。
 *
 * ## sync_point 语义
 * 同 legacyTick/brainTick：签名持 Registry& 参数触发 sync_point=true，永远串行、无依赖推导。
 *
 * ## 友元
 * 本函数需访问 EntityManager 私有 _tickMobAi()，故 EntityManager 友元本函数。
 *
 * ## 门控语义保持
 * MobEntity::tickAiChain() 内部保留 m_aiEnabled 门控（仅门控 targetSelector/goalSelector/
 * navigator/updateAITasks/updateMovementGoalFlags；senses 与 move/look/jump controller 在
 * 门控外永远执行，对齐 vanilla noAI 时仍跑感知与控制器）。
 */
void mobAiTick(const void* payload, OrganizerGraph::Registry& registry);

} // namespace mc::ecs::sys
