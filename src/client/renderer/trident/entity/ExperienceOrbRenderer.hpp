#pragma once

#include "EntityRenderer.hpp"
#include "../../../../common/core/Types.hpp"
#include <memory>

namespace mc {

class ExperienceOrbEntity;

namespace client::renderer {

/**
 * @brief 经验球渲染器
 *
 * 渲染世界中的经验球实体。经验球使用 billboard 方式渲染，
 * 始终面向摄像机，带有绿色发光效果和浮动动画。
 *
 * 特性：
 * - 11 种大小等级（对应不同经验值）
 * - 绿色主色调颜色动画
 * - 上下浮动动画
 * - 无阴影
 *
 * 参考 MC 1.16.5 ExperienceOrbRenderer
 */
class ExperienceOrbRenderer : public EntityRenderer {
public:
    ExperienceOrbRenderer();
    ~ExperienceOrbRenderer() override = default;

    // 禁止拷贝
    ExperienceOrbRenderer(const ExperienceOrbRenderer&) = delete;
    ExperienceOrbRenderer& operator=(const ExperienceOrbRenderer&) = delete;

    /**
     * @brief 渲染经验球
     * @param entity 实体（必须是 ExperienceOrbEntity）
     * @param partialTicks 部分 tick
     */
    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染阴影（经验球没有阴影）
     * @param entity 实体
     * @param partialTicks 部分 tick
     */
    void renderShadow(Entity& entity, f64 partialTicks) override;

private:
    /**
     * @brief 计算浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f64 calculateBobOffset(u32 ticksExisted, f64 partialTick) const;

    /**
     * @brief 计算颜色动画相位
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return 颜色相位 (0.0 - 1.0)
     */
    [[nodiscard]] f64 calculateColorPhase(u32 ticksExisted, f64 partialTick) const;

    // 动画常量（参考 MC 1.16.5）
    static constexpr f64 BOB_AMPLITUDE = 0.1f;       // 浮动高度
    static constexpr f64 BOB_FREQUENCY = 0.05f;      // 浮动速度（弧度/tick）
    static constexpr f64 COLOR_SPEED = 0.1f;         // 颜色变化速度
    static constexpr f64 BASE_SIZE = 0.25f;          // 基础大小
    static constexpr f64 SIZE_INCREMENT = 0.015f;    // 每级大小增量
};

} // namespace client::renderer
} // namespace mc
