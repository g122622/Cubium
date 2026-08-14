#pragma once

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/ecs/systems/ISystem.hpp"
#include "common/entity/ecs/systems/ITickingSystem.hpp"
#include "common/entity/ecs/systems/SystemPhase.hpp"

#include <memory>
#include <vector>

namespace mc::ecs {

/**
 * @brief 系统 tick 调度器——阶段化编排入口
 *
 * 持有按 SystemPhase 分组的 system 列表，tick() 时按阶段固定顺序遍历，
 * 阶段内首批顺序执行。
 *
 * 阶段化是 ECS 时序可查的关键：业务时序（如坠伤必须在移动后）由阶段名编码，
 * 不交给隐式 DAG。阶段内的组件读写冲突检测留待后续批次接入 entt::organizer
 * （首批仅一个 system，拓扑排序无意义）。
 *
 * EntityManager::tick() 委托本调度器，按阶段顺序执行：
 *   - EntityTick 阶段：EntityLegacyTickSystem 包装 OOP Entity::tick() 虚函数链；
 *   - PostEntityTick 阶段：状态递减/环境交互类 System（PortalTickSystem / FireTickSystem）。
 * EntityManager 自行处理 graveyard 延迟析构（见 EntityManager.cpp 三步编排）。
 */
class EntitySystemScheduler {
public:
    /**
     * @brief 注册 system 到指定阶段
     *
     * 同一阶段可注册多个 system，按注册顺序执行。注册顺序即阶段内时序，
     * 改动需谨慎并在 README 记录意图。
     */
    void registerSystem(SystemPhase phase, std::shared_ptr<ITickingSystem> system);

    /** 执行所有阶段的 system tick */
    void tick(EntityRegistry& registry);

private:
    // 每阶段的 system 列表（按 SystemPhase 枚举值索引）
    std::vector<std::shared_ptr<ITickingSystem>> m_systems[static_cast<u8>(SystemPhase::Count)];
};

} // namespace mc::ecs
