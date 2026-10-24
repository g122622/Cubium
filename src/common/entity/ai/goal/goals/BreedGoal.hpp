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

#include "common/entity/core/EntityReference.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include <string>

namespace mc {

// 前向声明
class AnimalEntity;
class Player;

namespace entity::ai::goal {

/**
 * @brief 繁殖目标
 *
 * 使两只动物靠近并繁殖。
 */
class BreedGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param animal 动物实体
     * @param speed 移动速度倍率
     */
    BreedGoal(AnimalEntity* animal, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "BreedGoal";
    }

    /**
     * @brief 执行一次繁殖产出与繁殖后处理
     *
     * 对齐 vanilla BreedGoal.tick 在 loveTime 达到阈值后的繁殖分支：先经
     * AnimalEntity::spawnChildFromBreeding 产出（幼体或物品），再统一执行发情重置、
     * 繁殖冷却、繁殖事件、爱心粒子与经验球。
     *
     * @note 要求 m_targetMate 非空（由 shouldExecute 定位）；tick() 在满足倒计时与
     *       距离条件后调用。
     */
    void spawnBaby();

protected:
    /**
     * @brief 寻找附近的配偶
     * @return 附近的配偶，如果没有则返回 nullptr
     */
    [[nodiscard]] AnimalEntity* findNearbyMate();

    AnimalEntity* m_animal;
    f64 m_speed;
    EntityReference<AnimalEntity> m_targetMate;
    i32 m_spawnBabyDelay = 0;
};

} // namespace entity::ai::goal
} // namespace mc
