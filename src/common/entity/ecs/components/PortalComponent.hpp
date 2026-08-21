#pragma once

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::ecs {

/**
 * @brief 传送门状态组件
 *
 * 承载 Entity::m_portalCooldown / m_portalTime / m_inPortal / m_portalPos 四字段。
 * 把传送门全套状态并入同一组件，因四字段同属传送门时序且总是相伴读写
 * （onEntityCollision 设 inPortal/portalPos，tickPortal 读之递进，
 * triggerPortalCooldown 设 cooldown）。
 *
 * 字段语义：
 * - m_portalCooldown：传送冷却计数。baseTick 每帧 -1（移入 ecs::sys::portalTick 后由系统递减），
 *   >0 期间 canTeleport() 返回 false，防止频繁传送。
 * - m_portalTime：实体当前在传送门中的累计 tick。达 getMaxInPortalTime() 阈值触发传送。
 *   离开门时每帧 -4（快速衰减），在门中时每帧 +1。
 * - m_inPortal：本帧是否处于门内（由 NetherPortalBlock::onEntityCollision 设置）。
 * - m_portalPos：所在传送门方块位置。
 */
struct PortalComponent {
    i32 m_portalCooldown{0}; // 传送冷却（防止频繁传送，单位：tick）
    i32 m_portalTime{0};     // 在传送门中的累计时间（单位：tick）
    bool m_inPortal{false};  // 是否在传送门中
    BlockPos m_portalPos{};  // 所在传送门方块的位置

    // portalPos() getter 在组件未 attach 时的防御性回退值（全实体 attach 后正常不触发）。
    // inline static 避免 ODR 重复定义，header-only 组件可直接使用。
    inline static const BlockPos s_defaultPos{};
};

} // namespace mc::ecs
