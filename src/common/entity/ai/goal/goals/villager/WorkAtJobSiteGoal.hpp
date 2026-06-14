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
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace entity {
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 村民工作目标
 *
 * 村民在工作时间前往工作站点工作。
 * 包括补货逻辑。
 */
class WorkAtJobSiteGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit WorkAtJobSiteGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "WorkAtJobSiteGoal"; }

protected:
    /**
     * @brief 检查是否是工作时间
     */
    [[nodiscard]] bool _isWorkTime() const;

    /**
     * @brief 检查是否有工作站点
     */
    [[nodiscard]] bool _hasJobSite() const;

    /**
     * @brief 移动到工作站点
     */
    void _moveToJobSite();

    /**
     * @brief 执行工作
     */
    void _doWork();

    /**
     * @brief 检查是否需要补货
     */
    [[nodiscard]] bool _needsRestock() const;

    /**
     * @brief 执行补货
     */
    void _restock();

protected:
    VillagerEntity* m_villager;

private:
    i32 m_workTicks = 0;
    bool m_atJobSite = false;
    i32 m_lastRestockDay = -1;                 // 上次补货的游戏日
    static constexpr i32 WORK_TICKS_MIN = 100; // 最小工作时间
    static constexpr i32 WORK_TICKS_MAX = 600; // 最大工作时间
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
