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

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::u8;

/**
 * @brief 实体姿态枚举
 *
 * 不同的姿态影响实体的尺寸（高度）和眼睛高度。
 * 例如：蹲下时玩家变矮，游泳时玩家变得更扁平。
 *
 * 编号与原版 Pose 序号保持一致，以便网络同步与存档兼容。
 */
enum class EntityPose : u8 {
    Standing = 0,    // 站立 - 默认姿态
    FallFlying = 1,  // 鞘翅飞行
    Sleeping = 2,    // 睡眠
    Swimming = 3,    // 游泳
    SpinAttack = 4,  // 三叉戟激流攻击
    Crouching = 5,   // 蹲下/潜行
    Dying = 6,       // 死亡动画
    Sliding = 7,     // 滑行 - 旋风人专用
    Shooting = 8,    // 射击 - 旋风人专用
    Inhaling = 9,    // 吸气蓄力 - 旋风人专用
    LongJumping = 10 // 长跳中 - 旋风人专用
};

/**
 * @brief 获取姿态名称（用于序列化和调试）
 * @param pose 姿态
 * @return 姿态名称字符串
 */
inline const char* getPoseName(EntityPose pose)
{
    switch (pose) {
        case EntityPose::Standing:
            return "standing";
        case EntityPose::FallFlying:
            return "fall_flying";
        case EntityPose::Sleeping:
            return "sleeping";
        case EntityPose::Swimming:
            return "swimming";
        case EntityPose::SpinAttack:
            return "spin_attack";
        case EntityPose::Crouching:
            return "crouching";
        case EntityPose::Dying:
            return "dying";
        case EntityPose::Sliding:
            return "sliding";
        case EntityPose::Shooting:
            return "shooting";
        case EntityPose::Inhaling:
            return "inhaling";
        case EntityPose::LongJumping:
            return "long_jumping";
    }
    return "unknown";
}

/**
 * @brief 从名称获取姿态
 * @param name 姿态名称
 * @return 姿态枚举，未知名称返回 Standing
 */
inline EntityPose getPoseByName(const std::string& name)
{
    if (name == "standing") return EntityPose::Standing;
    if (name == "fall_flying") return EntityPose::FallFlying;
    if (name == "sleeping") return EntityPose::Sleeping;
    if (name == "swimming") return EntityPose::Swimming;
    if (name == "spin_attack") return EntityPose::SpinAttack;
    if (name == "crouching") return EntityPose::Crouching;
    if (name == "dying") return EntityPose::Dying;
    if (name == "sliding") return EntityPose::Sliding;
    if (name == "shooting") return EntityPose::Shooting;
    if (name == "inhaling") return EntityPose::Inhaling;
    if (name == "long_jumping") return EntityPose::LongJumping;
    return EntityPose::Standing;
}

} // namespace mc::entity
