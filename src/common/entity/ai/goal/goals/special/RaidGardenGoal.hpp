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

// 前向声明
class RabbitEntity;
class BlockState;
class IWorld;

namespace entity::ai::goal {

// ============================================================================
// 常量定义
// ============================================================================

namespace RaidGardenGoalConstants {

/// 移动速度倍率（对应 MC 1.21.11 Rabbit.RaidGardenGoal 构造函数 super(rabbit, 0.7F, 16)）
constexpr f64 MOVE_SPEED = 0.7;

/// 水平搜索半径（对应 MC 1.21.11 Rabbit.RaidGardenGoal 构造函数 searchLength=16）
constexpr i32 SEARCH_LENGTH = 16;

/// 垂直搜索范围（耕地与胡萝卜在同一层，只需检查脚下和上方）
constexpr i32 VERTICAL_SEARCH_RANGE = 1;

/// 到达目标后再次可触发前的冷却 tick（对应 MC nextStartTick = 10）
constexpr i32 NEXT_START_TICK = 10;

/// 啃食胡萝卜后 moreCarrotTicks 的重置值（对应 MC rabbit.moreCarrotTicks = 40）
constexpr i32 MORE_CARROTS_DELAY = 40;

/// 视线控制的最大偏航角变化速度（对应 MC setLookAt 的 yHeadRotSpeed=10.0F）
constexpr f32 LOOK_DELTA_YAW = 10.0f;

} // namespace RaidGardenGoalConstants

// ============================================================================
// 类定义
// ============================================================================

/**
 * @brief 偷胡萝卜目标
 *
 * 对应 MC 1.21.11 `Rabbit.RaidGardenGoal`（Rabbit.java 内部静态类，继承 `MoveToBlockGoal`）。
 *
 * 行为：
 * - 兔子饥饿（`wantsMoreFood()` 即 `moreCarrotTicks <= 0`）时，在 16 格范围内
 *   搜索下方为耕地、上方为成熟胡萝卜（`CarrotBlock` 且 `isMaxAge`）的目标。
 * - 到达目标后：
 *   - AGE==0：将胡萝卜方块设为 AIR 并播放破坏粒子/游戏事件。
 *   - AGE>=1：将胡萝卜 AGE 减 1，播放 `BLOCK_CHANGE` 游戏事件和 `levelEvent(2001)` 破坏粒子。
 * - 啃食后设置 `moreCarrotTicks = 40`，冷却 10 tick 后才能再次触发。
 *
 * 受 `MOB_GRIEFING` 游戏规则限制：规则为 false 时目标不会执行。
 */
class RaidGardenGoal : public MoveToBlockGoal {
public:
    /**
     * @brief 构造函数
     * @param rabbit 拥有此目标的兔子
     */
    explicit RaidGardenGoal(RabbitEntity* rabbit);

    ~RaidGardenGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "RaidGardenGoal"; }

protected:
    /**
     * @brief 检查目标位置是否为有效掠夺目标
     *
     * 对应 MC 1.21.11 `Rabbit.RaidGardenGoal.isValidTarget()`：
     * - 位置必须是 FARMLAND
     * - `wantsToRaid` 为 true 且 `canRaid` 为 false 时，检查上方是否为成熟胡萝卜
     * - 上方为成熟胡萝卜时设置 `canRaid = true` 并返回 true
     *
     * @param world 世界
     * @param pos 待检查位置（耕地位置）
     * @return 是否为有效目标
     */
    [[nodiscard]] bool shouldMoveTo(IWorld* world, const BlockPos& pos) override;

private:
    /**
     * @brief 执行掠夺胡萝卜动作
     *
     * 对应 MC 1.21.11 `Rabbit.RaidGardenGoal.tick()` 中 `isReachedTarget()` 分支：
     * - 获取目标上方方块（胡萝卜）
     * - AGE==0：设为 AIR + 播放破坏效果 + BLOCK_DESTROY 游戏事件
     * - AGE>=1：设为 AGE-1 + BLOCK_CHANGE 游戏事件 + levelEvent(2001) 破坏粒子
     * - 设置 `moreCarrotTicks = 40`、`canRaid = false`、`nextStartTick = 10`
     *
     * @param carrotPos 胡萝卜方块位置（目标方块上方）
     * @param carrotState 胡萝卜方块状态
     */
    void _raidCarrot(const BlockPos& carrotPos, const BlockState* carrotState);

    RabbitEntity* m_rabbit;
    bool m_wantsToRaid = false; // 兔子是否饥饿（wantsMoreFood），每次搜索开始时设置
    bool m_canRaid = false;     // 当前目标是否可掠夺（上方为成熟胡萝卜）
};

} // namespace entity::ai::goal
} // namespace mc
