/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "common/entity/ai/goal/Goal.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {

// 前向声明
class CatEntity;
class CreatureEntity;
class Player;
class IWorld;

namespace entity::ai::goal {

// ============================================================================
// 常量定义
// ============================================================================

namespace CatGoalConstants {

/// 猫躺在床上的搜索半径
constexpr i32 LIE_ON_BED_SEARCH_RANGE = 6;

/// 猫躺在床上的搜索起始Y偏移（向下搜索）
constexpr i32 LIE_ON_BED_VERTICAL_START = -2;

/// 猫靠近主人后的放松判定延迟（tick）
constexpr i32 RELAX_ON_OWNER_DELAY = 16;

/// 猫醒来后赠送礼物概率
constexpr f32 MORNING_GIFT_CHANCE = 0.7f;

/// 猫在床上的判定距离（平方）
constexpr f64 RELAX_ON_OWNER_NEAR_DIST_SQ = 2.5;

/// 猫靠近主人的搜索距离（平方）
constexpr f64 RELAX_ON_OWNER_SEARCH_DIST_SQ = 10000.0;

/// 猫在床上/主人附近的空间占用检测距离（方块）
constexpr f32 SPACE_OCCUPIED_CHECK_DIST = 2.0f;

/// 猫躺在床上的重新导航间隔（tick）
constexpr i32 LIE_ON_BED_MOVE_INTERVAL = 40;

} // namespace CatGoalConstants

// ============================================================================
// CatLieOnBedGoal
// ============================================================================

/**
 * @brief 猫躺在床上的 AI 目标
 *
 * 驯服的猫会寻找附近的床并躺在上面。
 * 继承自 MoveToBlockGoal，搜索床方块并导航移动过去。
 *
 * 状态转换：
 * - 寻找床 → 导航到床 → 到达床 → 设置躺下状态
 * - 离开床 → 取消躺下状态
 */
class CatLieOnBedGoal : public MoveToBlockGoal {
public:
    /**
     * @brief 构造函数
     * @param cat 猫实体
     * @param speed 移动速度倍率
     */
    CatLieOnBedGoal(CatEntity* cat, f64 speed);

    ~CatLieOnBedGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void tick() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "CatLieOnBedGoal"; }

protected:
    [[nodiscard]] bool shouldMoveTo(IWorld* world, const BlockPos& pos) override;
    [[nodiscard]] i32 nextStartTick() const;

private:
    CatEntity* m_cat;
};

// ============================================================================
// CatRelaxOnOwnerGoal
// ============================================================================

/**
 * @brief 猫在睡觉主人身边放松的 AI 目标
 *
 * 当驯服的猫的主人正在睡觉时，猫会走到主人身边，
 * 先看向主人（放松状态），然后躺下。
 * 目标结束时如果主人已充分睡眠，猫会赠送礼物。
 */
class CatRelaxOnOwnerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param cat 猫实体
     * @param speed 移动速度倍率
     */
    CatRelaxOnOwnerGoal(CatEntity* cat, f64 speed);

    ~CatRelaxOnOwnerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void tick() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "CatRelaxOnOwnerGoal"; }

private:
    /**
     * @brief 检查目标位置附近是否有其他猫占据
     */
    [[nodiscard]] bool _isSpaceOccupied() const;

    /**
     * @brief 赠送晨间礼物
     *
     * 当猫在主人身边躺下后主人醒来时，猫有一定概率在主人身旁掉落礼物。
     * 礼物来自猫的晨间礼物战利品表。
     */
    void _giveMorningGift();

    CatEntity* m_cat;
    f64 m_speed;
    Player* m_owner = nullptr;
    BlockPos m_goalPos;
    i32 m_onBedTicks = 0;
};

} // namespace entity::ai::goal
} // namespace mc
