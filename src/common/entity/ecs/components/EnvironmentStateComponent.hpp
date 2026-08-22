#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 环境状态组件
 *
 * 承载原 Entity::m_inWater / m_inLava / m_eyesInWater / m_eyesInLava /
 * m_waterHeight / m_lavaHeight / m_fluidHeight 七字段，描述实体当前所在流体浸入状态。
 *
 * 由 ecs::sys::environmentSensing（SystemPhase::EnvironmentSense 阶段，在 EntityTick 之前）
 * 每帧重写：遍历实体碰撞箱内方块流体，计算浸入高度与眼睛浸入判定。同帧产出同帧消费——
 * EntityTick 阶段 OOP Entity::tick()（含 baseTick）与 PostEntityTick 阶段的 fireTick
 * 经 isInWater()/isInLava() 等 getter 读本组件，无跨帧延迟。
 *
 * 字段语义（与原 Entity 成员逐字一致）：
 * - inWater/eyesInWater：实体/眼睛位置浸入水（water 或 flowing_water）。
 * - inLava/eyesInLava：实体/眼睛位置浸入岩浆（lava 或 flowing_lava）。
 * - waterHeight/lavaHeight：水/岩浆浸入高度（相对碰撞箱底，0.0 起）。
 * - fluidHeight：max(waterHeight, lavaHeight)，兼容旧代码的派生字段。
 *
 * 纯 POD 可移动，所有实体 attach（Entity 构造期 emplace）。低频写（每帧 1 次）、
 * 高频读（fireTick/各处 getter），按约定不进 m_builtIn 缓存，走 tryGetComponent 查询
 * （与 Portal/Fire/Freeze 同范式，见 ecs/README 坑9）。
 */
struct EnvironmentStateComponent {
    bool inWater{false};
    bool inLava{false};
    bool eyesInWater{false}; // 眼睛位置是否在水下
    bool eyesInLava{false};  // 眼睛位置是否在岩浆中
    f32 waterHeight{0.0f};   // 水浸入高度（0.0 起）
    f32 lavaHeight{0.0f};    // 岩浆浸入高度（0.0 起）
    f32 fluidHeight{0.0f};   // max(waterHeight, lavaHeight)，兼容旧代码派生字段
};

} // namespace mc::ecs
