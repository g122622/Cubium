#pragma once

#include "common/core/Types.hpp"
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
 * - m_target：目标实体缓存指针（运行时查找用，非持久；每次 tick 校验有效性）。
 * - m_targetUuid：目标 UUID（持久化/跨 tick 重新查找用）。
 * - m_direction：当前移动方向（默认 Up）。
 * - m_flightSteps：剩余飞行步数（耗尽后重新选向）。
 * - m_targetDelta：目标速度增量（朝目标的移动向量）。
 *
 * 注意：m_target 是裸 Entity 指针，生命周期由目标实体的销毁控制；tick 中读取前须
 * 校验（参照 goal 持裸指针 UAF 历史教训，本组件不持有目标所有权，仅缓存引用）。
 */
struct ShulkerBulletComponent {
    Entity* m_target{nullptr};
    std::string m_targetUuid;
    Direction m_direction{Direction::Up};
    i32 m_flightSteps{0};
    Vector3d m_targetDelta{0.0, 0.0, 0.0};
};

} // namespace mc::ecs
