#pragma once

#include "common/core/Types.hpp"
#include <string>

namespace mc::ecs {

/**
 * @brief 弹射物发射者组件
 *
 * 承载 ProjectileEntity 的发射者追踪字段：shooterUuid / shooterEntityId / leftShooter /
 * lastDeflectedById / hasBeenShot。对齐基岩版 ProjectileComponent 中 owner 相关子字段
 * 与 vanilla Projectile 持久化键（Owner/LeftOwner/HasBeenShot）。
 *
 * 仅 ProjectileEntity 子树 attach（含 FishingBobber/EvokerFangs/EyeOfEnder 三支系？
 * 否——这三者直接继承 Entity 且各有独立 owner 机制，不挂本组件，详见各自组件）。
 *
 * 字段语义：
 * - m_shooterUuid：发射者持久 UUID（跨 tick/world 重新查找用），vanilla "Owner" 键。
 * - m_shooterEntityId：发射者当前 tick 的运行时实体 id（世界内查找用），非持久。
 * - m_leftShooter：投掷物是否已离开发射者碰撞箱（命中检测门控，离开前不命中发射者自身）。
 *   vanilla "LeftOwner" 键。
 * - m_lastDeflectedById：上一个偏转此弹射物的实体 id（防同一实体连续偏转）。
 * - m_hasBeenShot：是否已发射。vanilla "HasBeenShot" 键；项目当前在构造即视为已发射，
 *   持久化时按 vanilla 写出，反序列化时回填。
 */
struct ProjectileOwnerComponent {
    std::string m_shooterUuid;
    EntityInstanceId m_shooterEntityId{INVALID_ENTITY_ID};
    bool m_leftShooter{false};
    EntityInstanceId m_lastDeflectedById{INVALID_ENTITY_ID};
    bool m_hasBeenShot{false};
};

} // namespace mc::ecs
