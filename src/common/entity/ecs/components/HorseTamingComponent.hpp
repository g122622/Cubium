#pragma once

#include <string>

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 马类驯服进度组件（A 类本地字段）
 *
 * 承载 AbstractHorseEntity 的驯服进度与主人归属。仅 AbstractHorseEntity 子树 attach。
 *
 * 字段分类：A 类纯本地无同步——temper/maxTemper/ownerUuid 均无 DataParameter，仅存盘。
 *
 * 持久化：Temper（i32）+ OwnerUUIDMost/OwnerUUIDLeast（i64 双 long，UUID 字符串编码）。
 * ownerUuid load 时调 setOwnerUuid 触发 setTame(true) 联动写 HorseStatusComponent +
 * STATUS_PARAM（见 HorseComponentSerialization priority=0，先于 HorseStatusComponent load）。
 *
 * 对齐 vanilla 1.21.11 AbstractHorse：Temper 字段 + EntityReference owner（项目用 UUID
 * 字符串 + 双 long 持久化，与项目既有 EvokerFangs/ projectile owner 一致）。
 */
struct HorseTamingComponent {
    i32 m_temper{0};      ///< 当前驯服进度
    i32 m_maxTemper{100}; ///< 驯服阈值，达到则驯服成功
    std::string m_ownerUuid; ///< 主人 UUID（空串表示无主人）
};

} // namespace mc::ecs
