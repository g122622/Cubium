#pragma once

#include "common/core/Types.hpp"

namespace mc::entity {

/**
 * @brief 钓鱼浮标状态
 *
 * 对齐 vanilla FishingHook.SynchedEntityData 状态枚举。本枚举原内联于
 * FishingBobberEntity 类内，批次6 子目标2 将钓鱼浮标字段迁入 FishingBobberComponent
 * 时提取为独立头，使组件能以值类型承载而不循环依赖 OtherProjectiles.hpp
 * （参照 PickupStatus / EntityFlags 提取先例）。
 */
enum class FishingBobberState : u8 {
    Flying,  // 飞行中
    Hooked,  // 钩住实体
    Bobbing, // 浮在水面
    Fishing  // 钓鱼中（咬钩状态）
};

/**
 * @brief 钓鱼水域类型（用于开放水域检测）
 */
enum class FishingWaterType : u8 {
    AboveWater,  // 水上方块（空气或睡莲）
    InsideWater, // 水内部（完整水源方块）
    Invalid      // 无效
};

} // namespace mc::entity
