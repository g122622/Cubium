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

#include "core/Types.hpp"
#include "entity/ai/goal/Goal.hpp"
#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "util/Direction.hpp"
#include <functional>
#include <string>

namespace mc {

// Forward declarations
class SilverfishEntity;
class LivingEntity;
class BlockPos;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 蠹虫藏入石头目标
 *
 * 蠹虫在没有攻击目标时，有概率进入附近的虫蚀方块。
 * 这个目标继承自 RandomWalkingGoal，但增加了藏入石头的特殊行为。
 *
 * 行为：
 * - 只有当蠹虫没有攻击目标且导航器空闲时才执行
 * - 需要 mobGriefing 游戏规则为 true
 * - 执行概率：1/10 (每tick)
 * - 检查蠹虫周围的虫蚀方块，如果找到就藏入其中
 * - 藏入时将虫蚀方块转换为对应的原版方块
 * - 藏入时蠹虫消失并播放消散粒子效果
 *
 * 互斥标志：MOVE
 */
class SilverfishHideInStoneGoal : public RandomWalkingGoal {
public:
    /**
     * @brief 构造函数
     * @param silverfish 拥有此目标的蠹虫实体
     */
    explicit SilverfishHideInStoneGoal(SilverfishEntity* silverfish);

    ~SilverfishHideInStoneGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const noexcept override { return "SilverfishHideInStoneGoal"; }

    // 常量（公开用于测试）
    static constexpr i32 MERGE_CHANCE = 10;

private:
    /**
     * @brief 检查并选择一个虫蚀方块方向
     * @return 如果找到虫蚀方块返回 true，并设置 m_facing 和 m_doMerge
     */
    [[nodiscard]] bool _checkForInfestedBlock();

    SilverfishEntity* m_silverfish;
    Direction m_facing = Direction::None; // 选中的方向
    bool m_doMerge = false;               // 是否执行藏入操作
};

/**
 * @brief 蠹虫召唤同伴目标
 *
 * 当蠹虫受到伤害时，召唤周围虫蚀方块中的其他蠹虫。
 *
 * 行为：
 * - 受伤时触发 notifyHurt()，设置 20 tick 的召唤计时器
 * - 计时器归零时，搜索周围虫蚀方块并破坏它们
 * - 破坏虫蚀方块时会生成新的蠹虫
 * - 破坏后有 50% 概率停止（避免召唤过多）
 *
 * 互斥标志：无
 */
class SilverfishSummonOthersGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param silverfish 拥有此目标的蠹虫实体
     */
    explicit SilverfishSummonOthersGoal(SilverfishEntity* silverfish);

    ~SilverfishSummonOthersGoal() override = default;

    /**
     * @brief 通知蠹虫受伤
     *
     * 当蠹虫受到伤害时调用此方法，触发召唤计时器。
     */
    void notifyHurt();

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SilverfishSummonOthersGoal"; }

    // 常量（公开用于测试）
    static constexpr i32 SUMMON_DURATION = 20;

private:
    SilverfishEntity* m_silverfish;
    i32 m_lookForFriends = 0; // 召唤计时器
};

} // namespace entity::ai::goal
} // namespace mc
