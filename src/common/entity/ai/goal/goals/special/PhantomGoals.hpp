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
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {
class PhantomEntity;
class Player;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 幻翼目标选择器
 *
 * 寻找 64 格内的玩家作为攻击目标。
 */
class PhantomAttackPlayerTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param phantom 幻翼实体指针
     */
    explicit PhantomAttackPlayerTargetGoal(PhantomEntity* phantom);

    /**
     * @brief 检查是否应该执行
     * @return 有可攻击的玩家时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     * @return 攻击目标仍然有效时返回true
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 重置时调用
     * 清除攻击目标
     */
    void resetTask() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "PhantomAttackPlayerTargetGoal"; }

private:
    /**
     * @brief 搜索可攻击的玩家
     * @return 找到的玩家，如果没有返回nullptr
     */
    [[nodiscard]] Player* _findAttackablePlayer();

    PhantomEntity* m_phantom;                 ///< 幻翼实体
    i32 m_tickDelay = 20;                     ///< 搜索延迟（初始20，成功后60）
    static constexpr f64 SEARCH_RANGE = 64.0; ///< 搜索范围
};

/**
 * @brief 幻翼移动目标基类
 *
 * 提供检查目标点是否接近的辅助方法。
 */
class PhantomMoveGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param phantom 幻翼实体指针
     */
    explicit PhantomMoveGoal(PhantomEntity* phantom);

protected:
    /**
     * @brief 检查幻翼是否接近环绕偏移点
     * @return 距离平方 < 4.0 时返回true
     */
    [[nodiscard]] bool isNearOrbitOffset() const;

    PhantomEntity* m_phantom; ///< 幻翼实体
};

/**
 * @brief 幻翼环绕飞行目标
 *
 * 在目标上方环绕飞行，等待攻击机会。
 */
class PhantomOrbitPointGoal : public PhantomMoveGoal {
public:
    /**
     * @brief 构造函数
     * @param phantom 幻翼实体指针
     */
    explicit PhantomOrbitPointGoal(PhantomEntity* phantom);

    /**
     * @brief 检查是否应该执行
     * @return 无攻击目标或处于环绕阶段时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 开始执行时调用
     * 初始化环绕参数
     */
    void startExecuting() override;

    /**
     * @brief 每tick执行
     * 更新环绕参数和目标位置
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "PhantomOrbitPointGoal"; }

private:
    /**
     * @brief 更新环绕偏移位置
     */
    void _updateOrbitOffset();

    f32 m_orbitAngle = 0.0f;        ///< 当前环绕角度
    f32 m_orbitRadius = 5.0f;       ///< 环绕半径（5.0 + random(10.0)）
    f32 m_orbitHeightOffset = 0.0f; ///< 高度偏移（-4.0 + random(9.0)）
    f32 m_orbitDirection = 1.0f;    ///< 环绕方向（1.0或-1.0）
};

/**
 * @brief 幻翼攻击阶段选择目标
 *
 * 在环绕和俯冲阶段之间切换。
 */
class PhantomPickAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param phantom 幻翼实体指针
     */
    explicit PhantomPickAttackGoal(PhantomEntity* phantom);

    /**
     * @brief 检查是否应该执行
     * @return 有攻击目标且可攻击时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 开始执行时调用
     * 设置初始环绕阶段
     */
    void startExecuting() override;

    /**
     * @brief 重置时调用
     * 更新环绕位置
     */
    void resetTask() override;

    /**
     * @brief 每tick执行
     * 管理攻击阶段切换
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "PhantomPickAttackGoal"; }

private:
    /**
     * @brief 设置环绕位置为目标上方
     */
    void _setOrbitPositionAboveTarget();

    PhantomEntity* m_phantom; ///< 幻翼实体
    i32 m_tickDelay = 0;      ///< 攻击阶段切换延迟
};

/**
 * @brief 幻翼俯冲攻击目标
 *
 * 执行俯冲攻击，撞击目标造成伤害。
 */
class PhantomSweepAttackGoal : public PhantomMoveGoal {
public:
    /**
     * @brief 构造函数
     * @param phantom 幻翼实体指针
     */
    explicit PhantomSweepAttackGoal(PhantomEntity* phantom);

    /**
     * @brief 检查是否应该执行
     * @return 有攻击目标且处于俯冲阶段时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     * @return 攻击目标仍然有效时返回true
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 重置时调用
     * 切换回环绕阶段
     */
    void resetTask() override;

    /**
     * @brief 每tick执行
     * 执行俯冲攻击
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "PhantomSweepAttackGoal"; }

private:
    /**
     * @brief 检查附近是否有猫（猫会驱赶幻翼）
     * @return 如果附近有猫返回false，应该停止攻击
     */
    bool _checkForCats();

    i32 m_catCheckTimer = 20;     ///< 猫检测计时器（初始化为20使首次调用立即检测）
    bool m_isScaredOfCat = false; ///< 是否害怕猫（检测到猫时设为true）
};

} // namespace entity::ai::goal
} // namespace mc
