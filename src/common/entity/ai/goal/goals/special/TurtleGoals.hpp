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

#include "../../../../../core/Types.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include "../BreedGoal.hpp"
#include "../PanicGoal.hpp"
#include "../RandomWalkingGoal.hpp"
#include "../TemptGoal.hpp"
#include <functional>
#include <string>

namespace mc {

// 前向声明
class TurtleEntity;
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 海龟返回出生地目标
 *
 * 当海龟有蛋时，会返回出生地产卵。
 * 或者有 1/700 概率在距离出生地超过 64 格时触发。
 */
class TurtleGoHomeGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 移动速度倍率
     */
    TurtleGoHomeGoal(TurtleEntity* turtle, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "TurtleGoHomeGoal"; }

private:
    /**
     * @brief 尝试找到通往出生地的路径
     * @return 是否找到路径
     */
    [[nodiscard]] bool _tryFindPathToHome();

    TurtleEntity* m_turtle;
    f64 m_speed;
    bool m_gaveUp = false;      // 是否放弃（找不到路径）
    i32 m_closeToHomeTimer = 0; // 接近出生地的计时器

    static constexpr f64 HOME_DISTANCE_TRIGGER = 64.0; // 触发回家的距离
    static constexpr f64 HOME_DISTANCE_ARRIVE = 7.0;   // 到达出生地的距离
    static constexpr i32 MAX_TRAVEL_TIME = 600;        // 最大旅行时间 (ticks)
    static constexpr i32 PATH_RECALC_DELAY = 10;       // 路径重算延迟
    static constexpr i32 RANDOM_TRIGGER_CHANCE = 700;  // 随机触发概率倒数
};

/**
 * @brief 海龟产卵目标
 *
 * 当海龟有蛋且靠近出生地时，寻找沙地并产卵。
 */
class TurtleLayEggGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 移动速度倍率
     */
    TurtleLayEggGoal(TurtleEntity* turtle, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "TurtleLayEggGoal"; }

private:
    /**
     * @brief 检查位置是否适合产卵
     * @param pos 要检查的位置
     * @return 是否适合产卵
     */
    [[nodiscard]] bool _shouldMoveTo(const BlockPos& pos);

    /**
     * @brief 搜索附近的产卵位置
     * @return 是否找到合适的位置
     */
    [[nodiscard]] bool _findLayEggPosition();

    TurtleEntity* m_turtle;
    f64 m_speed;
    BlockPos m_targetPos;
    bool m_foundTarget = false;
    i32 m_timeoutCounter = 0;

    static constexpr i32 SEARCH_RANGE = 16;       // 搜索范围
    static constexpr f64 HOME_DISTANCE_MAX = 9.0; // 距离出生地最大距离
    static constexpr i32 MAX_TIMEOUT = 1200;      // 最大超时 (ticks)
};

/**
 * @brief 海龟旅行目标
 *
 * 海龟在水中随机游泳，探索周围环境。
 */
class TurtleTravelGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 游泳速度倍率
     */
    TurtleTravelGoal(TurtleEntity* turtle, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "TurtleTravelGoal"; }

private:
    /**
     * @brief 设置随机旅行目标
     */
    void _setRandomTravelPos();

    /**
     * @brief 尝试找到通往旅行目标的路径
     * @return 是否找到路径
     */
    [[nodiscard]] bool _tryFindPathToTravelPos();

    TurtleEntity* m_turtle;
    f64 m_speed;
    BlockPos m_travelPos;
    bool m_gaveUp = false;

    static constexpr i32 TRAVEL_RANGE = 512;        // 旅行范围 (格)
    static constexpr i32 TRAVEL_VERTICAL_RANGE = 4; // 垂直范围
};

/**
 * @brief 海龟前往水中目标
 *
 * 当海龟在陆地上时，尝试寻找水源。
 */
class TurtleGoToWaterGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 移动速度倍率
     */
    TurtleGoToWaterGoal(TurtleEntity* turtle, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "TurtleGoToWaterGoal"; }

private:
    /**
     * @brief 搜索附近的水源
     * @return 是否找到水源
     */
    [[nodiscard]] bool _findWater();

    TurtleEntity* m_turtle;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    bool m_foundWater = false;
    i32 m_timeoutCounter = 0;

    static constexpr i32 SEARCH_RANGE_HORIZONTAL = 24; // 水平搜索范围
    static constexpr i32 SEARCH_RANGE_VERTICAL = 1;    // 垂直搜索范围
    static constexpr i32 MAX_TIMEOUT = 1200;           // 最大超时 (ticks)
};

/**
 * @brief 海龟繁殖目标
 *
 * 继承自 BreedGoal，繁殖后设置 hasEgg 状态。
 */
class TurtleMateGoal : public BreedGoal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 移动速度倍率
     */
    TurtleMateGoal(TurtleEntity* turtle, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] std::string getTypeName() const override { return "TurtleMateGoal"; }

private:
    TurtleEntity* m_turtle;
};

/**
 * @brief 海龟恐慌目标
 *
 * 当海龟受到攻击或着火时逃跑，优先寻找水源。
 */
class TurtlePanicGoal : public PanicGoal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 逃跑速度倍率
     */
    TurtlePanicGoal(TurtleEntity* turtle, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] std::string getTypeName() const override { return "TurtlePanicGoal"; }

private:
    TurtleEntity* m_turtle;
};

/**
 * @brief 海龟海草诱惑目标
 *
 * 当玩家手持海草时，海龟会被诱惑跟随。
 */
class TurtleTemptGoal : public TemptGoal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 移动速度倍率
     */
    TurtleTemptGoal(TurtleEntity* turtle, f64 speed);

    /**
     * @brief 检查物品是否为海草
     * @param stack 物品堆
     * @return 是否为海草
     */
    static bool isSeagrass(const ItemStack& stack);

    [[nodiscard]] std::string getTypeName() const override { return "TurtleTemptGoal"; }
};

/**
 * @brief 海龟随机游荡目标
 *
 * 只在陆地上且不在回家、有蛋状态时触发。
 */
class TurtleWanderGoal : public RandomWalkingGoal {
public:
    /**
     * @brief 构造函数
     * @param turtle 海龟实体
     * @param speed 移动速度倍率
     * @param chance 执行概率倒数
     */
    TurtleWanderGoal(TurtleEntity* turtle, f64 speed, i32 chance);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] std::string getTypeName() const noexcept override { return "TurtleWanderGoal"; }

private:
    TurtleEntity* m_turtle;
};

} // namespace entity::ai::goal
} // namespace mc
