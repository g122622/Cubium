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

namespace mc {

// 前向声明
class Entity;

namespace entity {
class ProjectileEntity;
} // namespace entity

/**
 * @brief 弹射物偏转类型
 *
 * 对应 MC Java 的 ProjectileDeflection。
 * 定义弹射物被实体偏转时的行为方式。
 */
enum class ProjectileDeflection : u8 {
    /// 无偏转，弹射物正常命中实体
    None,

    /// 反向偏转：速度乘以 -0.5，并添加随机 170~190 度偏航旋转
    /// 用于潜影贝和旋风人的默认偏转行为
    Reverse,

    /// 瞄准偏转：将弹射物速度设置为偏转者的视线方向
    /// 用于玩家攻击可偏转弹射物（如火球）时的重定向
    AimDeflect,

    /// 动量偏转：将弹射物速度设置为偏转者的移动方向（归一化）
    /// 用于特定场景的动量偏转
    MomentumDeflect,
};

/**
 * @brief 应用弹射物偏转
 *
 * 根据偏转类型修改弹射物的速度和旋转，并将偏转者设为新的发射者。
 *
 * @param deflection 偏转类型
 * @param projectile 被偏转的弹射物
 * @param deflector 偏转者（通常是命中到的实体）
 * @return 如果偏转成功返回 true
 */
bool applyProjectileDeflection(
    ProjectileDeflection deflection, entity::ProjectileEntity& projectile, Entity& deflector);

} // namespace mc
