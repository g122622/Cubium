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
 * The above copyright notice and this permission notice shall included in all
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

#include "../../../../../core/Types.hpp"
#include "../../../../../util/math/Vector3.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"

namespace mc {

// 前向声明
class CreatureEntity;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 移动到方块目标基类
 *
 * 抽象基类，提供在范围内搜索特定方块并导航移动的功能。
 * 子类只需实现 shouldMoveTo() 方法来定义目标方块条件。
 *
 * 参考 MC 1.16.5 net.minecraft.entity.ai.goal.MoveToBlockGoal
 */
class MoveToBlockGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param searchLength 水平搜索半径
     */
    MoveToBlockGoal(CreatureEntity* creature, f64 speed, i32 searchLength);

    /**
     * @brief 构造函数（完整版）
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param searchLength 水平搜索半径
     * @param verticalSearchRange 垂直搜索范围
     */
    MoveToBlockGoal(CreatureEntity* creature, f64 speed, i32 searchLength, i32 verticalSearchRange);

    ~MoveToBlockGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 是否应该重新导航
     * MC 1.16.5 默认每 40 tick 检查一次
     */
    [[nodiscard]] virtual bool shouldMove() const;

    /**
     * @brief 获取目标位置（默认为方块上方）
     */
    [[nodiscard]] virtual BlockPos getTargetPosition() const;

    /**
     * @brief 获取到达目标的距离阈值（平方）
     */
    [[nodiscard]] virtual f64 getTargetDistanceSq() const;

protected:
    CreatureEntity* m_creature;
    f64 m_movementSpeed;
    i32 m_runDelay = 0;          // 执行延迟 (tick)
    i32 m_timeoutCounter = 0;    // 超时计数器
    i32 m_maxStayTicks = 0;      // 最大停留时间
    BlockPos m_destinationBlock; // 目标方块位置
    bool m_isAboveDestination = false;
    i32 m_searchLength;            // 水平搜索半径
    i32 m_verticalSearchRange;     // 垂直搜索范围
    i32 m_verticalSearchStart = 0; // Y轴搜索起始偏移

    /**
     * @brief 获取随机执行延迟
     * MC 1.16.5: 200 + random(200) = 200-400 tick
     */
    [[nodiscard]] i32 getRunDelay();

    /**
     * @brief 移动到目标位置
     */
    void moveToTarget();

    /**
     * @brief 检查是否在目标距离内
     */
    [[nodiscard]] bool isWithinDistance(const BlockPos& pos, f64 distSq) const;

    /**
     * @brief 搜索目标方块
     * MC 1.16.5 螺旋搜索算法
     */
    [[nodiscard]] bool searchForDestination();

    /**
     * @brief 检查目标方块是否符合条件（子类实现）
     * @param world 世界引用
     * @param pos 方块位置
     * @return 如果该方块是有效目标返回 true
     */
    [[nodiscard]] virtual bool shouldMoveTo(IWorld* world, const BlockPos& pos) = 0;
};

/**
 * @brief 移动到熔岩目标
 *
 * 炽足兽寻找熔岩的 AI 目标。当炽足兽离开熔岩时，
 * 会自动寻找附近的熔岩并移动过去。
 *
 * 参考 MC 1.16.5 net.minecraft.entity.passive.StriderEntity.MoveToLavaGoal
 */
class MoveToLavaGoal : public MoveToBlockGoal {
public:
    /**
     * @brief 构造函数
     * @param creature 炽足兽实体（或其他需要寻找熔岩的生物）
     * @param speed 移动速度倍率
     *
     * MC 1.16.5 调用参数:
     * - searchLength = 8 (水平搜索半径)
     * - verticalSearchRange = 2 (垂直搜索范围)
     */
    MoveToLavaGoal(CreatureEntity* creature, f64 speed);

    ~MoveToLavaGoal() override = default;

    /**
     * @brief 获取目标方块位置
     *
     * MC 1.16.5: 直接返回 destinationBlock（不是上方方块）
     * 这是与父类的重要区别！
     */
    [[nodiscard]] BlockPos getTargetPosition() const override;

    /**
     * @brief 是否应该开始执行
     *
     * 条件:
     * 1. 生物当前不在熔岩中
     * 2. 父类 shouldExecute() 返回 true（找到了目标熔岩）
     */
    [[nodiscard]] bool shouldExecute() override;

    /**
     * @brief 是否应该继续执行
     *
     * 条件:
     * 1. 生物仍然不在熔岩中
     * 2. 目标熔岩仍然有效
     */
    [[nodiscard]] bool shouldContinueExecuting() override;

    /**
     * @brief 是否应该重新导航
     *
     * MC 1.16.5: 每 20 tick 检查一次（父类默认是 40 tick）
     */
    [[nodiscard]] bool shouldMove() const override;

    [[nodiscard]] std::string getTypeName() const override { return "MoveToLavaGoal"; }

protected:
    /**
     * @brief 检查目标方块是否符合条件
     *
     * 条件:
     * 1. 目标方块是熔岩（使用 FluidTags::LAVA 检测）
     * 2. 上方方块允许通行
     */
    [[nodiscard]] bool shouldMoveTo(IWorld* world, const BlockPos& pos) override;
};

} // namespace entity::ai::goal
} // namespace mc
