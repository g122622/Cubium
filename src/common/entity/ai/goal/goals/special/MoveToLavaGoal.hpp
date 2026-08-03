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

#include "MoveToBlockGoal.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {

class CreatureEntity;

namespace entity::ai::goal {

// ============================================================================
// MoveToBlockGoal 常量（熔岩相关）
// ============================================================================

namespace MoveToLavaGoalConstants {

/// 熔岩目标重新导航间隔（tick）
constexpr i32 LAVA_MOVE_INTERVAL = 20;

/// 熔岩目标水平搜索半径
constexpr i32 LAVA_SEARCH_LENGTH = 8;

/// 熔岩目标垂直搜索范围
constexpr i32 LAVA_VERTICAL_SEARCH_RANGE = 2;

} // namespace MoveToLavaGoalConstants

// ============================================================================
// MoveToLavaGoal
// ============================================================================

/**
 * @brief 移动到熔岩目标
 *
 * 炽足兽寻找熔岩的 AI 目标。当炽足兽离开熔岩时，
 * 会自动寻找附近的熔岩并移动过去。
 */
class MoveToLavaGoal : public MoveToBlockGoal {
public:
    /**
     * @brief 构造函数
     * @param creature 炽足兽实体（或其他需要寻找熔岩的生物）
     * @param speed 移动速度倍率
     */
    MoveToLavaGoal(CreatureEntity* creature, f64 speed);

    ~MoveToLavaGoal() override = default;

    /**
     * @brief 获取目标方块位置
     * 直接返回 destinationBlock（不是上方方块），这是与父类的重要区别
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
     * 每 20 tick 检查一次（父类默认是 40 tick）
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
