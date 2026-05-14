#pragma once

#include "../../core/Types.hpp"
#include <optional>

namespace mc {

// 前向声明
class BlockState;
class Entity;
class BlockPos;
class IBlockReader;

namespace fluid {
class FluidState;
}

namespace world {
namespace explosion {

/**
 * @brief 爆炸上下文基类
 *
 * 用于自定义爆炸行为，例如：
 * - 凋灵之首可以破坏更高抗性的方块
 * - TNT 矿车不破坏铁轨
 *
 * 对应 Minecraft 1.16.5 的 ExplosionContext 类。
 */
class ExplosionContext {
public:
    virtual ~ExplosionContext() = default;

    /**
     * @brief 获取方块的爆炸抗性
     *
     * 可以被子类覆盖以修改特定方块的爆炸抗性。
     * 默认实现返回方块自身的抗性。
     *
     * @param blockState 方块状态
     * @param fluidState 流体状态（可能为空）
     * @return 爆炸抗性值，如果为空表示不消耗爆炸强度（如空气）
     */
    [[nodiscard]] virtual std::optional<f32> getExplosionResistance(
        const BlockState& blockState, const fluid::FluidState* fluidState) const;

    /**
     * @brief 判断方块是否可被爆炸破坏
     *
     * 可以被子类覆盖以阻止特定方块被破坏。
     *
     * @param blockState 方块状态
     * @param explosionPower 在该位置的爆炸强度
     * @return true 表示可以破坏
     */
    [[nodiscard]] virtual bool canDestroyBlock(const BlockState& blockState, f32 explosionPower) const;
};

/**
 * @brief 实体相关的爆炸上下文
 *
 * 允许实体自定义爆炸行为。
 * 例如凋灵之首可以破坏基岩以外的所有方块。
 */
class EntityExplosionContext : public ExplosionContext {
public:
    /**
     * @brief 构造实体爆炸上下文
     * @param source 爆炸源实体（可能为空）
     */
    explicit EntityExplosionContext(const Entity* source);

    [[nodiscard]] std::optional<f32> getExplosionResistance(
        const BlockState& blockState, const fluid::FluidState* fluidState) const override;

    [[nodiscard]] bool canDestroyBlock(const BlockState& blockState, f32 explosionPower) const override;

private:
    const Entity* m_source;
};

} // namespace explosion
} // namespace world
} // namespace mc
