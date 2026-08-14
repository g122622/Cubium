#pragma once

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/ecs/systems/ITickingSystem.hpp"

#include <functional>

namespace mc::ecs {

/**
 * @brief 旧版 OOP tick 桥接系统
 *
 * 将现有 OOP Entity::tick() 虚函数链包装为一个 ECS system，注册到
 * SystemPhase::LegacyTick 阶段。对齐基岩版 ActorLegacyTickSystem
 * （mc/entity/systems/ActorLegacyTickSystem.h）——基岩版用它把旧 Actor::normalTick
 * 接入新 ECS tick 流水线。
 *
 * 首批设计：本 system 不自行遍历 registry 重新实现 tick 逻辑，而是通过回调
 * 委托 EntityManager 已有的逐实体 tick（含模拟距离门控、ServerPlayer 永远 tick 等
 * 成熟逻辑）。这样：
 *   - 保持 EntityManager::tick 三步编排语义不变（步骤1委托本system→步骤2清graveyard→步骤3收死亡）
 *   - 避免 EntityManager ↔ Scheduler 循环依赖
 *   - 后续批次把 OOP tick 逐步拆成真正遍历组件的 ECS system 时，替换回调实现即可
 *
 * 强时序内聚逻辑（hurt 链/AI 决策）暂留 OOP tick 壳内，不拆成多个 system，
 * 保留调用栈可读性（混合架构不引入双重时序轴的关键，见 ecs/README 坑位）。
 */
class EntityLegacyTickSystem final : public ITickingSystem {
public:
    using TickCallback = std::function<void(EntityRegistry&)>;

    explicit EntityLegacyTickSystem(TickCallback callback) : m_callback(std::move(callback)) {}

    [[nodiscard]] const char* name() const noexcept override { return "EntityLegacyTickSystem"; }

    void tick(EntityRegistry& registry) override { m_callback(registry); }

private:
    TickCallback m_callback;
};

} // namespace mc::ecs
