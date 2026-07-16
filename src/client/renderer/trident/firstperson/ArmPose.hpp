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

} // namespace mc::client::renderer
