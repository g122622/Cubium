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
#include "../../Goal.hpp"

namespace mc {

// Forward declarations
class TameableEntity;
class Player;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 跟随主人目标
 *
 * 使驯服动物跟随主人。
 * 当距离主人太远时会自动移动靠近。
 *
 * 参考 MC 1.16.5 FollowOwnerGoal
 */
class FollowOwnerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param entity 驯服动物
     * @param speed 移动速度倍率
     * @param minDistance 最小跟随距离
     * @param maxDistance 最大跟随距离（超过此距离开始跟随）
     * @param teleportDistance 传送距离（超过此距离传送）
     */
    FollowOwnerGoal(TameableEntity* entity, f64 speed, f32 minDistance, f32 maxDistance, f32 teleportDistance);

    ~FollowOwnerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

private:
    /**
     * @brief 检查主人是否存在且可跟随
     * @return 如果可以跟随返回true
     */
    [[nodiscard]] bool canFollowOwner() const;

    /**
     * @brief 传送到主人身边
     * @return 如果传送成功返回true
     */
    bool teleportToOwner();

    TameableEntity* m_entity;
    Player* m_owner = nullptr;
    f64 m_speed;
    f32 m_minDistance;
    f32 m_maxDistance;
    f32 m_teleportDistance;
    i32 m_timeToRecalcPath = 0;

    static constexpr i32 PATH_RECALC_INTERVAL = 10; // 路径重新计算间隔
};

/**
 * @brief 坐下目标
 *
 * 使驯服动物保持坐下状态。
 * 坐下时不会移动或跟随主人。
 *
 * 参考 MC 1.16.5 SitGoal
 */
class SitGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param entity 驯服动物
     */
    explicit SitGoal(TameableEntity* entity);

    ~SitGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

private:
    TameableEntity* m_entity;
};

/**
 * @brief 乞求目标
 *
 * 当玩家手持食物或驯服物品时，动物会看向玩家并乞求。
 * 主要用于狼（狗）的行为。
 *
 * 行为逻辑（MC 1.16.5）：
 * - 已驯服的动物：对驯服物品（如骨头）和繁殖物品（如肉类）乞求
 * - 未驯服的动物：仅对繁殖物品乞求
 *
 * 参考 MC 1.16.5 BegGoal
 */
class BegGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param entity 驯服动物
     * @param maxDistance 最大乞求距离
     */
    BegGoal(TameableEntity* entity, f32 maxDistance);

    ~BegGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

private:
    /**
     * @brief 检查玩家是否手持食物
     * @param player 玩家
     * @return 如果手持食物返回true
     */
    [[nodiscard]] bool isPlayerHoldingFood(const Player* player) const;

    TameableEntity* m_entity;
    Player* m_targetPlayer = nullptr;
    f32 m_maxDistance;
    f32 m_begAngle = 0.0f;

    static constexpr f32 BEG_ANGLE_SPEED = 0.15f;
};

} // namespace entity::ai::goal
} // namespace mc
