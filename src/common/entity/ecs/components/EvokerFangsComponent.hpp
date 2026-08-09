#pragma once

#include "common/core/Types.hpp"
#include <string>

namespace mc {
class LivingEntity;
} // namespace mc

namespace mc::ecs {

/**
 * @brief 唤魔者尖牙状态组件
 *
 * 承载 EvokerFangsEntity 的 6 字段。EvokerFangsEntity 直接继承 Entity（不经
 * ProjectileEntity），有独立 owner 机制（owner 是唤魔者 LivingEntity，非弹射物
 * 发射者），故不挂 ProjectileOwnerComponent，本组件独立承载 owner。
 *
 * 仅 EvokerFangsEntity attach。对齐 vanilla EvokerFangs 的预热-攻击-消亡逻辑与
 * owner 持久化（owner UUID 双 long）。
 *
 * 字段语义：
 * - m_owner：所有者缓存指针（唤魔者，运行时用，非持久；每次 tick 校验有效性）。
 * - m_ownerUuid：所有者 UUID（持久化，跨 tick 重新查找用）。
 * - m_warmupDelay：预热延迟（ticks，从出现到开始攻击的等待）。
 * - m_sentAttackEvent：是否已发送攻击事件（防重复攻击）。
 * - m_lifeTicks：生命时长（ticks，默认 22，耗尽即消亡）。
 * - m_clientSideAttackStarted：客户端攻击开始标志（同步用）。
 *
 * 注意：m_owner 是裸 LivingEntity 指针，生命周期由唤魔者实体控制；tick 中读取前
 * 须校验有效性（参照 goal 持裸指针 UAF 历史教训）。
 */
struct EvokerFangsComponent {
    LivingEntity* m_owner{nullptr};
    std::string m_ownerUuid;
    i32 m_warmupDelay{0};
    bool m_sentAttackEvent{false};
    i32 m_lifeTicks{22};
    bool m_clientSideAttackStarted{false};
};

} // namespace mc::ecs
