/**
 * @file PhantomGoals.hpp
 * @brief 幻翼专用的AI目标类
 *
 * 参考 MC 1.16.5: net.minecraft.entity.monster.PhantomEntity
 *
 * 幻翼有四个特有AI目标：
 * - PhantomAttackPlayerTargetGoal: 目标选择器，寻找玩家作为攻击目标
 * - PhantomOrbitPointGoal: 环绕飞行，在目标周围盘旋
 * - PhantomPickAttackGoal: 选择攻击阶段，在环绕和俯冲之间切换
 * - PhantomSweepAttackGoal: 俯冲攻击执行
 *
 * 还有 PhantomMoveGoal 作为移动目标的抽象基类。
 */

#pragma once

#include "../../../../../core/Types.hpp"
#include "../../../../../util/math/Vector3.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"

namespace mc {
class PhantomEntity;
class Player;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 幻翼目标选择器
 *
 * 寻找 64 格内的玩家作为攻击目标。
 *
 * 参考 MC 1.16.5: PhantomEntity.AttackPlayerGoal
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
    Player* findAttackablePlayer();

    PhantomEntity* m_phantom;                 ///< 幻翼实体
    i32 m_tickDelay = 20;                     ///< 搜索延迟（MC 1.16.5: 初始20，成功后60）
    static constexpr f64 SEARCH_RANGE = 64.0; ///< 搜索范围（MC 1.16.5）
};

/**
 * @brief 幻翼移动目标基类
 *
 * 提供检查目标点是否接近的辅助方法。
 *
 * 参考 MC 1.16.5: PhantomEntity.MoveGoal
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
 *
 * 参考 MC 1.16.5: PhantomEntity.OrbitPointGoal
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
    void updateOrbitOffset();

    f32 m_orbitAngle = 0.0f;        ///< 当前环绕角度
    f32 m_orbitRadius = 5.0f;       ///< 环绕半径（MC 1.16.5: 5.0 + random(10.0)）
    f32 m_orbitHeightOffset = 0.0f; ///< 高度偏移（MC 1.16.5: -4.0 + random(9.0)）
    f32 m_orbitDirection = 1.0f;    ///< 环绕方向（1.0或-1.0）
};

/**
 * @brief 幻翼攻击阶段选择目标
 *
 * 在环绕和俯冲阶段之间切换。
 *
 * 参考 MC 1.16.5: PhantomEntity.PickAttackGoal
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
    void setOrbitPositionAboveTarget();

    PhantomEntity* m_phantom; ///< 幻翼实体
    i32 m_tickDelay = 0;      ///< 攻击阶段切换延迟
};

/**
 * @brief 幻翼俯冲攻击目标
 *
 * 执行俯冲攻击，撞击目标造成伤害。
 *
 * 参考 MC 1.16.5: PhantomEntity.SweepAttackGoal
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
    bool checkForCats();

    i32 m_catCheckTimer = 0; ///< 猫检测计时器（每20tick检测一次）
};

} // namespace entity::ai::goal
} // namespace mc
