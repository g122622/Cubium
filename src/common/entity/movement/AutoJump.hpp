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

#include "AutoJumpConstants.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <algorithm>
#include <vector>

namespace mc {

// 前向声明
class Player;
class PhysicsEngine;
class AxisAlignedBB;

namespace entity {
namespace movement {

/**
 * @brief 自动跳跃检测结果
 *
 * 由 AutoJump::check() 返回，包含检测结果和相关信息。
 */
struct AutoJumpResult {
    /// 是否应该触发自动跳跃
    bool shouldJump = false;

    /// 检测到的障碍物高度（方块顶部Y坐标相对于玩家脚部）
    f32 obstacleHeight = 0.0f;
};

/**
 * @brief 自动跳跃系统
 *
 * 实现自动跳跃功能。
 * 当玩家走近可跳上的障碍物时自动触发跳跃。
 *
 * ## 工作原理
 *
 * 自动跳跃在玩家每 tick 移动后检测：
 * 1. 检查触发条件（启用、地面、非潜行、非骑乘等）
 * 2. 计算玩家移动方向
 * 3. 检查是否在向前移动（向后不触发）
 * 4. 检查头部空间（能站在障碍物上）
 * 5. 计算最大跳跃高度（含跳跃药水效果）
 * 6. 沿玩家左右边缘的两条检测线查找障碍物
 * 7. 如果找到合适高度的障碍物，触发跳跃
 *
 * ## 使用方法
 *
 * @code
 * // 在 Player 类中
 * AutoJump m_autoJump;
 *
 * // 同步设置
 * m_autoJump.setEnabled(settings.autoJump.get());
 *
 * // 每帧更新
 * m_autoJump.tick();
 *
 * // 移动后检测
 * auto result = m_autoJump.check(player, physicsEngine, movementInput);
 * if (result.shouldJump) {
 *     player.jump();
 * }
 * @endcode
 */
class AutoJump {
public:
    AutoJump() = default;
    ~AutoJump() = default;

    // 禁止拷贝，允许移动
    AutoJump(const AutoJump&) = delete;
    AutoJump& operator=(const AutoJump&) = delete;
    AutoJump(AutoJump&&) noexcept = default;
    AutoJump& operator=(AutoJump&&) noexcept = default;

    // ========== 配置 ==========

    /**
     * @brief 启用或禁用自动跳跃
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 检查自动跳跃是否启用
     * @return 是否启用
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * @brief 设置跳跃提升效果等级
     *
     * 跳跃提升效果会增加最大跳跃高度。
     * 等级 0 = 无效果，等级 1 = +0.75 格，等等。
     *
     * @param level 跳跃提升效果等级（0 = 无效果）
     */
    void setJumpBoostLevel(i32 level) { m_jumpBoostLevel = std::max(0, level); }

    /**
     * @brief 获取跳跃提升效果等级
     * @return 跳跃提升效果等级
     */
    [[nodiscard]] i32 jumpBoostLevel() const { return m_jumpBoostLevel; }

    // ========== 状态管理 ==========

    /**
     * @brief 获取自动跳跃冷却计时器
     * @return 剩余冷却 ticks
     */
    [[nodiscard]] i32 autoJumpTime() const { return m_autoJumpTime; }

    /**
     * @brief 每 tick 更新
     *
     * 递减冷却计时器。应在每帧开始时调用。
     */
    void tick();

    /**
     * @brief 重置冷却计时器
     *
     * 在触发自动跳跃后调用。
     */
    void resetCooldown();

    // ========== 核心检测 ==========

    /**
     * @brief 检测是否应该自动跳跃
     *
     * 这是自动跳跃的主入口点。应在玩家移动后调用。
     *
     * @param player 玩家实体
     * @param physicsEngine 物理引擎（用于碰撞检测）
     * @param movementInput 移动输入（forward, strafe）
     * @return 检测结果
     */
    [[nodiscard]] AutoJumpResult check(
        const Player& player, PhysicsEngine& physicsEngine, const Vector2& movementInput);

    // ========== 静态工具方法（供测试使用） ==========

    /**
     * @brief 计算移动方向
     *
     * 使用玩家当前速度或移动输入+yaw计算移动方向。
     * 如果速度太小，则使用移动输入计算。
     *
     * @param player 玩家实体
     * @param movementInput 移动输入
     * @return 归一化的移动方向向量（XZ平面）
     */
    [[nodiscard]] static Vector3 calculateMovementDirection(const Player& player, const Vector2& movementInput);

    /**
     * @brief 检查玩家是否在向前移动
     *
     * 使用移动方向和朝向的点积判断。
     * 点积 < FORWARD_THRESHOLD 时认为在向后移动，不触发自动跳跃。
     *
     * @param movementDir 移动方向（归一化）
     * @param forwardDir 玩家朝向（归一化）
     * @return 是否在向前移动
     */
    [[nodiscard]] static bool isMovingForward(const Vector3& movementDir, const Vector3& forwardDir);

    /**
     * @brief 计算最大跳跃高度
     *
     * 公式：BASE_JUMP_HEIGHT + JUMP_BOOST_PER_LEVEL * level
     *
     * @return 最大跳跃高度（方块数）
     */
    [[nodiscard]] f32 calculateMaxJumpHeight() const;

private:
    /// 是否启用自动跳跃
    bool m_enabled = true;

    /// 跳跃提升效果等级
    i32 m_jumpBoostLevel = 0;

    /// 自动跳跃冷却计时器（ticks）
    i32 m_autoJumpTime = 0;

    // ========== 内部检测方法 ==========

    /**
     * @brief 检查是否应该进行自动跳跃检测
     *
     * 检查所有触发条件：
     * - 自动跳跃已启用
     * - 冷却已过
     * - 玩家在地面
     * - 玩家未潜行
     * - 玩家未骑乘
     * - 有移动输入
     * - 跳跃因子正常（不在蜂蜜块上）
     *
     * @param player 玩家实体
     * @param hasMovementInput 是否有移动输入
     * @return 是否应该检测
     */
    [[nodiscard]] bool _shouldCheckForAutoJump(const Player& player, bool hasMovementInput) const;

    /**
     * @brief 检查指定位置上方是否有头部空间
     *
     * 检查玩家能否站在障碍物上（上方两格无碰撞）。
     *
     * @param player 玩家实体
     * @param physicsEngine 物理引擎
     * @param testPos 测试位置（X, Y=玩家脚部Y, Z）
     * @return 是否有足够的头部空间
     */
    [[nodiscard]] static bool _hasHeadSpace(const Player& player, PhysicsEngine& physicsEngine, const Vector3& testPos);

    /**
     * @brief 沿检测线查找障碍物高度
     *
     * 在玩家前方沿检测线查找第一个可跳上的障碍物。
     *
     * @param player 玩家实体
     * @param physicsEngine 物理引擎
     * @param origin 检测线起点
     * @param direction 检测方向（归一化）
     * @param distance 检测距离
     * @param maxJumpHeight 最大跳跃高度
     * @param collisionBoxes 碰撞箱列表（输出）
     * @return 障碍物顶部高度，如果没有找到返回 -1.0f
     */
    [[nodiscard]] static f32 _detectObstacleHeight(const Player& player,
        PhysicsEngine& physicsEngine,
        const Vector3& origin,
        const Vector3& direction,
        f32 distance,
        f32 maxJumpHeight,
        std::vector<AxisAlignedBB>& collisionBoxes);
};

} // namespace movement
} // namespace entity
} // namespace mc
