#pragma once

namespace mc::ecs {

class EntityRegistry;

/**
 * @brief 系统基类接口
 *
 * 对齐基岩版 ISystem（mc/deps/ecs/systems/ISystem.h）。系统是 ECS 的行为单元，
 * 按组件组合查询实体并施加逻辑。首批仅 ITickingSystem 一种系统形态。
 *
 * 系统注册到 EntitySystemScheduler 的某个命名阶段（SystemPhase），阶段间按
 * 固定顺序执行，阶段内首批顺序执行（后续批次接入 entt::organizer 做冲突检测）。
 */
class ISystem {
public:
    virtual ~ISystem() = default;

    /** 系统名（用于 profiling 与日志） */
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

} // namespace mc::ecs
