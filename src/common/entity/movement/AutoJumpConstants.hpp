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

#include "../../core/Types.hpp"

namespace mc {
namespace entity {
namespace movement {

/**
 * @brief 自动跳跃系统常量
 *
 * 所有常量基于 MC 1.16.5 ClientPlayerEntity 实现。
 * 参考: net.minecraft.client.entity.player.ClientPlayerEntity
 */
namespace AutoJumpConstants {

// MC 源码: f7 = 1.2F
constexpr f32 BASE_JUMP_HEIGHT = 1.2f;

// MC 源码: f7 += (this.getActivePotionEffect(Effects.JUMP_BOOST).getAmplifier() + 1) * 0.75F
constexpr f32 JUMP_BOOST_PER_LEVEL = 0.75f;

// MC 逻辑: if (f14 <= 0.5F) return;
constexpr f32 MIN_JUMP_HEIGHT = 0.5f;

// MC 源码: if (f13 < -0.15F) return;
constexpr f32 FORWARD_THRESHOLD = -0.15f;

// MC 源码: f8 = Math.max(f * 7.0F, 1.0F / f12)
constexpr f32 DETECTION_DISTANCE_MULTIPLIER = 7.0f;

// MC 使用 player.getPosY() + 0.51D 作为检测线起点
constexpr f32 DETECTION_HEIGHT_OFFSET = 0.51f;

// MC 源码: this.autoJumpTime = 1;
constexpr i32 AUTO_JUMP_COOLDOWN = 1;

// MC 源码: f1 <= 0.001F
constexpr f32 MOVEMENT_THRESHOLD_SQ = 0.001f;

// MC 源码: (double)this.getJumpFactor() >= 1.0D
constexpr f64 JUMP_FACTOR_THRESHOLD = 1.0;

// MC 源码: vector3d6 = vector3d5.scale((double)(f9 * 0.5F))
constexpr f32 LINE_OFFSET_RATIO = 0.5f;

// MC 检查玩家眼睛上方一格和两格位置
constexpr i32 HEAD_SPACE_CHECK_HEIGHT = 2;

} // namespace AutoJumpConstants

} // namespace movement
} // namespace entity
} // namespace mc
