#pragma once

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/ecs/systems/ITickingSystem.hpp"

namespace mc::ecs {

/**
 * @brief 传送门 tick 系统
 *
 * 承载原 Entity::tick() 中 baseTick 之后的 tickPortal() 逻辑，以及原 baseTick 内的
 * m_portalCooldown 递减。注册到 SystemPhase::PostEntityTick 阶段——在 EntityTick
 * （逐实体 OOP Entity::tick()）之后执行，可读到本帧刚由 updateEnvironmentState()
 * 产出的环境状态。
 *
 * 抽取自 OOP 的两段逻辑（逐字搬迁，行为等价）：
 *   1. baseTick 的 m_portalCooldown 递减（if >0 then --）；
 *   2. Entity::tickPortal()：读 inPortal → false 则 portalTime 衰减 4 → true 则重置
 *      inPortal → canTeleport 检查 → portalTime++ → 达 getMaxInPortalTime() 阈值则
 *      调 onPortalTriggered()（含 triggerPortalCooldown）。
 *
 * 多态保留：getMaxInPortalTime()/canTeleport()/onPortalTriggered() 经 EntityOwnerComponent
 * 反查 OOP Entity& 调虚函数，正确派发到 Player 等子类（Player::getMaxInPortalTime 返回
 * 1/80，基类返回 0）。原 Player::tickPortal() override 与基类逐字相同（仅注释不同），
 * 逻辑搬入 System 后该 override 删除，行为等价。
 *
 * 跨帧延迟（用户已接受）：portal 计时递进结果下帧 baseTick 才读到，单帧 50ms 玩家无感。
 */
class PortalTickSystem final : public ITickingSystem {
public:
    PortalTickSystem() = default;

    [[nodiscard]] const char* name() const noexcept override { return "PortalTickSystem"; }

    void tick(EntityRegistry& registry) override;
};

} // namespace mc::ecs
