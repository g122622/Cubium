#pragma once

#include <cstdint>

namespace mc::ecs {

/**
 * @brief 系统 tick 阶段枚举
 *
 * 阶段间按固定顺序执行，编码业务时序（参考基岩版 VanillaSystemsRegistration
 * 的 registerActorPreTravelSystems/registerActorTravelSystems/... 分阶段注册）。
 * 阶段名即时序意图，读注册代码可理解粗时序，避免 ECS 时序 bug 难查的问题。
 *
 * 首批仅 LegacyTick 一个阶段承载 EntityLegacyTickSystem（包装现有 OOP tick），
 * 后续批次按需在合适阶段注入 MovementSystem/PostMovementSystem 等。
 */
enum class SystemPhase : u8 {
    // 阶段前：系统注册/缓存预热等（首批未用）
    // PreMovement,
    // Movement,
    // PostMovement,
    // AiStep,
    // Reset,

    /// 旧版 OOP tick 桥接：调用每个实体的 Entity::tick() 虚函数链
    LegacyTick,

    /// 阶段数（须放末尾）
    Count
};

} // namespace mc::ecs
