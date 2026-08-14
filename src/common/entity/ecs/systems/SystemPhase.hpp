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
 * 阶段命名采用业务语义（实体主 tick / 主 tick 之后），不沿用 System 类名里的
 * "Legacy"措辞——后者描述实现来源（桥接旧 OOP），不适合当阶段名。
 */
enum class SystemPhase : u8 {
    /// 实体主 tick：承载 EntityLegacyTickSystem（调每个实体的 Entity::tick() 虚函数链）
    EntityTick,

    /// 实体主 tick 之后：承载状态递减/环境交互类 System（PortalTickSystem / FireTickSystem）。
    /// 此阶段在 EntityTick 之后执行，可读到本帧 baseTick::updateEnvironmentState 产出的环境状态。
    PostEntityTick,

    /// 阶段数（须放末尾）
    Count
};

} // namespace mc::ecs
