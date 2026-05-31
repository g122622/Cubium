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

namespace mc::client::renderer {

/**
 * @brief 手臂姿态
 *
 * 定义手臂在不同使用状态下的姿态。
 * 用于动画系统决定手臂如何摆放。
 */
enum class ArmPose : u8 {
    /// 空手（无物品）
    Empty = 0,

    /// 持有普通物品
    Item = 1,

    /// 格挡（盾牌）
    Block = 2,

    /// 拉弓
    BowAndArrow = 3,

    /// 投掷三叉戟
    ThrowSpear = 4,

    /// 装填弩
    CrossbowCharge = 5,

    /// 持有已装填的弩
    CrossbowHold = 6,

    /// 吃食物/喝药水
    EatOrDrink = 7,

    /// 使用地图
    Map = 8,

    /// 鞘翅飞行
    FallFlying = 9,

    /// 游泳
    Swimming = 10,

    /// 睡眠
    Sleeping = 11,

    /// 潜行
    Sneaking = 12
};

/**
 * @brief 判断手臂姿态是否需要两手持握
 *
 * 某些物品（如弓、三叉戟）需要双手操作，
 * 当主手使用这些物品时，副手不应渲染。
 *
 * @param pose 手臂姿态
 * @return true 如果需要两手持握
 */
[[nodiscard]] inline bool isTwoHanded(ArmPose pose)
{
    return pose == ArmPose::BowAndArrow || pose == ArmPose::ThrowSpear || pose == ArmPose::CrossbowCharge;
}

/**
 * @brief 判断手臂姿态是否阻止副手渲染
 *
 * 当主手使用某些物品时，副手应该隐藏或不渲染。
 *
 * @param pose 手臂姿态
 * @return true 如果副手不应渲染
 */
[[nodiscard]] inline bool blocksOffHand(ArmPose pose)
{
    return pose == ArmPose::BowAndArrow || pose == ArmPose::ThrowSpear || pose == ArmPose::CrossbowCharge ||
        pose == ArmPose::CrossbowHold;
}

} // namespace mc::client::renderer
