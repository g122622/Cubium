/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
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

// Explosion 与 ExplosionContext 同属本命名空间，前向声明以便钩子方法引用
class Explosion;

/**
 * @brief 爆炸上下文基类
 *
 * 用于自定义爆炸行为，例如：
 * - 凋灵之首可以破坏更高抗性的方块
 * - TNT 矿车不破坏铁轨
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

    /**
     * @brief 判断爆炸是否应当对该实体造成伤害
     *
     * 返回 false 时，该实体不会被 hurt，但仍可能受击退（取决于 getKnockbackMultiplier）。
     * 默认实现返回 true（所有实体都可被爆炸伤害）。
     *
     * @param explosion 爆炸对象
     * @param entity 受检实体
     * @return true 表示应当造成伤害
     */
    [[nodiscard]] virtual bool shouldDamageEntity(const Explosion& explosion, const Entity& entity) const;

    /**
     * @brief 获取该实体的爆炸击退倍率
     *
     * 返回 0.0f 表示该实体完全不受击退。默认实现返回 1.0f（正常击退）。
     *
     * @param explosion 爆炸对象
     * @param entity 受检实体
     * @return 击退倍率
     */
    [[nodiscard]] virtual f32 getKnockbackMultiplier(const Explosion& explosion, const Entity& entity) const;

    /**
     * @brief 计算该实体受到的爆炸伤害量
     *
     * 默认实现复刻 vanilla ExplosionDamageCalculator：
     *   d0 = 距离 / (radius*2)；d1 = (1-d0) * seenPercent
     *   damage = floor((d1²+d1)/2 * 7 * (radius*2) + 1)
     *
     * @param explosion 爆炸对象
     * @param entity 受检实体
     * @param seenPercent 视线遮挡密度（getSeenPercent，0.0-1.0）
     * @return 伤害值
     */
    [[nodiscard]] virtual f32 getEntityDamageAmount(
        const Explosion& explosion, const Entity& entity, f32 seenPercent) const;
};

/**
 * @brief 实体相关的爆炸上下文
 *
 * 允许实体自定义爆炸行为。
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

/**
 * @brief 凋灵之首爆炸上下文
 *
 * 蓝色凋灵之首（dangerous skull）具有特殊的爆炸抗性穿透能力：
 * 对于不在 WITHER_IMMUNE 标签中的非空方块，将爆炸抗性限制在 min(0.8, 原始抗性)，
 * 使得蓝色凋灵之首可以破坏黑曜石等普通爆炸无法破坏的方块。
 * 普通凋灵之首使用基类行为，不做任何修改。
 */
class WitherSkullExplosionContext : public EntityExplosionContext {
public:
    /**
     * @brief 构造凋灵之首爆炸上下文
     * @param source 爆炸源实体（凋灵之首的发射者，可能为空）
     * @param isDangerous 是否为蓝色凋灵之首（dangerous skull）
     */
    WitherSkullExplosionContext(const Entity* source, bool isDangerous);

    [[nodiscard]] std::optional<f32> getExplosionResistance(
        const BlockState& blockState, const fluid::FluidState* fluidState) const override;

private:
    bool m_isDangerous;
};

} // namespace explosion
} // namespace world
} // namespace mc
