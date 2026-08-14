#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::ecs {

/**
 * @brief 末影之眼状态组件
 *
 * 承载 EyeOfEnderEntity 的 4 字段：目标 X/Z / 存活时间 / 是否碎裂。对齐 vanilla
 * EyeOfEnder 朝要塞飞行并碎裂的逻辑。EyeOfEnderEntity 直接继承 Entity（不经
 * ProjectileEntity），故不挂 ProjectileOwnerComponent。
 *
 * 仅 EyeOfEnderEntity attach。末影之眼被使用后朝最近要塞飞行，到达后碎裂消失。
 *
 * 字段语义：
 * - m_targetX / m_targetZ：目标要塞的区块坐标（飞行终点）。
 * - m_lifetime：存活时间（达阈值碎裂）。
 * - m_break：是否已碎裂（防重复处理）。
 */
struct EyeOfEnderComponent {
    BlockCoord m_targetX{0};
    BlockCoord m_targetZ{0};
    i32 m_lifetime{0};
    bool m_break{false};
};

} // namespace mc::ecs
