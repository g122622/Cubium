#pragma once

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/ecs/systems/ITickingSystem.hpp"

#include <functional>

namespace mc::ecs {

/**
 * @brief Brain tick 桥接系统
 *
 * 将 VillagerEntity::tick() 中 m_brain->tick() 调用块抽成独立 System，注册到
 * SystemPhase::PostEntityTick 阶段。通过回调委托 EntityManager::_tickBrains()，
 * 复用 _tickEntities 的遍历+门控框架（playerChunks 快照/isRemoved 跳过/
 * ServerPlayer 短路/模拟距离门控），避免 EntityManager ↔ Scheduler 循环依赖。
 *
 * 与 EntityLegacyTickSystem 同属回调委托型范式（区别在于：
 * - EntityLegacyTickSystem 桥接所有实体的 OOP Entity::tick() 虚函数链；
 * - BrainTickSystem 仅桥接持有 Brain 的实体（当前仅 VillagerEntity）的
 *   m_brain->tick() 调用）。
 *
 * 时序：注册在 PortalTickSystem/FireTickSystem 之后，故 Brain tick 在所有实体
 * OOP tick（含 goalSelector.tick/navigator.tick）+ portal/fire 递减之后执行。
 * 跨实体传感器（NearestPlayersSensor 等）读到本帧最终状态，行为更正确。
 *
 * 设计依据：第19行决策"AI 系统保留 OOP（Goal/Brain/Navigator/Controller 不 ECS 化），
 * 仅用 System 做 tick 调度"。Brain 仍是 OOP 成员（VillagerEntity::m_brain），
 * 本 System 只搬"何时调 tick()"调度决策，不 ECS 化 Brain 数据。
 */
class BrainTickSystem final : public ITickingSystem {
public:
    using TickCallback = std::function<void(EntityRegistry&)>;

    explicit BrainTickSystem(TickCallback callback)
        : m_callback(std::move(callback))
    {}

    [[nodiscard]] const char* name() const noexcept override { return "BrainTickSystem"; }

    void tick(EntityRegistry& registry) override { m_callback(registry); }

private:
    TickCallback m_callback;
};

} // namespace mc::ecs
