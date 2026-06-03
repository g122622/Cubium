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
 */

#pragma once

#include "common/core/Types.hpp"

namespace mc {
namespace entity {
namespace movement {

/**
 * @brief 自动跳跃系统常量
 *
 * 定义自动跳跃检测和执行所需的各种阈值和参数。
 */
namespace AutoJumpConstants {

/// 基础跳跃高度
constexpr f32 BASE_JUMP_HEIGHT = 1.2f;

/// 每级跳跃药水效果增加的跳跃高度
constexpr f32 JUMP_BOOST_PER_LEVEL = 0.75f;

/// 最小跳跃高度阈值
constexpr f32 MIN_JUMP_HEIGHT = 0.5f;

/// 前向移动阈值
constexpr f32 FORWARD_THRESHOLD = -0.15f;

/// 检测距离乘数
constexpr f32 DETECTION_DISTANCE_MULTIPLIER = 7.0f;

/// 检测高度偏移量
constexpr f32 DETECTION_HEIGHT_OFFSET = 0.51f;

/// 自动跳跃冷却时间（ticks）
constexpr i32 AUTO_JUMP_COOLDOWN = 1;

/// 移动距离平方阈值
constexpr f32 MOVEMENT_THRESHOLD_SQ = 0.001f;

/// 跳跃因子阈值
constexpr f64 JUMP_FACTOR_THRESHOLD = 1.0;

/// 检测线偏移比例
constexpr f32 LINE_OFFSET_RATIO = 0.5f;

/// 头部空间检测高度
constexpr i32 HEAD_SPACE_CHECK_HEIGHT = 2;

} // namespace AutoJumpConstants

} // namespace movement
} // namespace entity
} // namespace mc
