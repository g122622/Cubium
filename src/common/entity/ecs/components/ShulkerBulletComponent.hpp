#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/EntityReference.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include <string>

namespace mc {
class Entity;
} // namespace mc

namespace mc::ecs {

/**
 * @brief 潜影贝子弹状态组件
 *
 * 承载 ShulkerBulletEntity 的 5 字段：目标实体指针 / 目标 UUID / 移动方向 / 飞行步数 /
 * 目标速度增量。对齐 vanilla ShulkerBullet 的追踪移动逻辑。
 *
 * 仅 ShulkerBulletEntity attach。潜影贝子弹追踪目标实体，按方向飞行若干步后重新选取
 * 方向朝向目标。vanilla ShulkerBullet 无同步字段（行为全服务端算）。
 *
 * 字段语义：
 * - m_target：目标实体生命周期引用（运行时查找用，不拥有实体）。
 * - m_targetUuid：目标 UUID（持久化/跨 tick 重新查找用）。
 * - m_direction：当前移动方向（默认 Up）。
 * - m_flightSteps：剩余飞行步数（耗尽后重新选向）。
 * - m_targetDelta：目标速度增量（朝目标的移动向量）。
 *
 * 目标逻辑移除或析构后引用自动失效，不能解引用裸指针来判断其是否仍存活。
 */
struct ShulkerBulletComponent {
    EntityReference<Entity> m_target;
    std::string m_targetUuid;
    Direction m_direction{Direction::Up};
    i32 m_flightSteps{0};
    Vector3d m_targetDelta{0.0, 0.0, 0.0};
};

} // namespace mc::ecs
