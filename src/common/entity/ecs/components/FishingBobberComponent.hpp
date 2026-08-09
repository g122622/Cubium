#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/FishingBobberState.hpp"

namespace mc {
class Player;
class Entity;
} // namespace mc

namespace mc::ecs {

/**
 * @brief 钓鱼浮标状态组件
 *
 * 承载 FishingBobberEntity 的 13 字段。对齐 vanilla FishingHook 的同步字段与
 * 钓鱼逻辑状态。FishingBobberEntity 直接继承 Entity（不经 ProjectileEntity），
 * 故不挂 ProjectileOwnerComponent，本组件独立承载 angler（钓鱼者）引用。
 *
 * 仅 FishingBobberEntity attach。原 FishingBobberEntity::State/WaterType 内联枚举
 * 已提取到 FishingBobberState.hpp（FishingBobberState / FishingWaterType）。
 *
 * 字段语义：
 * - m_angler：钓鱼者缓存指针（运行时用，非持久；FishingBobber 由玩家持有）。
 * - m_caughtEntity：被钩住的实体缓存指针（运行时用）。
 * - m_caughtEntityId：被钩住实体 id（网络同步用，存储时+1，0 表示无）。
 * - m_state：当前钓鱼状态（Flying/Hooked/Bobbing/Fishing，已同步字段）。
 * - m_ticksCaughtDelay：咬钩等待计时器。
 * - m_ticksCatchableDelay：鱼接近计时器。
 * - m_ticksCatchable：可捕获窗口期。
 * - m_fishAngle：鱼的角度（动画用）。
 * - m_inOpenWater：是否在开放水域（影响宝藏钓获）。
 * - m_luckBonus：海之眷顾附魔等级。
 * - m_speedBonus：饵钓附魔等级。
 * - m_outOfWaterTime：离开水的时间计数器。
 * - m_lifetime：存在时间。
 *
 * 注意：m_angler / m_caughtEntity 是裸指针，生命周期由对应实体控制；tick 中读取前
 * 须校验有效性（参照 goal 持裸指针 UAF 历史教训）。
 */
struct FishingBobberComponent {
    Player* m_angler{nullptr};
    Entity* m_caughtEntity{nullptr};
    EntityInstanceId m_caughtEntityId{0};
    entity::FishingBobberState m_state{entity::FishingBobberState::Flying};
    i32 m_ticksCaughtDelay{0};
    i32 m_ticksCatchableDelay{0};
    i32 m_ticksCatchable{0};
    f32 m_fishAngle{0.0f};
    bool m_inOpenWater{false};
    i32 m_luckBonus{0};
    i32 m_speedBonus{0};
    i32 m_outOfWaterTime{0};
    i32 m_lifetime{0};
};

} // namespace mc::ecs
