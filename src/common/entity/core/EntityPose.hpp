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
#include <string>

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::u8;

/**
 * @brief 实体姿态枚举
 *
 * 不同的姿态影响实体的尺寸（高度）和眼睛高度。
 * 例如：蹲下时玩家变矮，游泳时玩家变得更扁平。
 *
 * 枚举值（= wire id）与原版 1.21.11 `net.minecraft.world.entity.Pose` 的 `id()`
 * 逐一对齐（非 enum ordinal——vanilla Pose.STREAM_CODEC = idMapper(BY_ID, Pose::id)，
 * wire 上传的是构造函数传入的显式 id）。Pose 字段（Entity DATA_POSE）经 Pose serializer
 * （EntityDataSerializers id=20）以 VarInt(id) 传输，故本枚举值必须与 vanilla id 严格一致。
 */
enum class EntityPose : u8 {
    Standing = 0,    // 站立 - 默认姿态
    FallFlying = 1,  // 鞘翅飞行
    Sleeping = 2,    // 睡眠
    Swimming = 3,    // 游泳
    SpinAttack = 4,  // 三叉戟激流攻击
    Crouching = 5,   // 蹲下/潜行
    LongJumping = 6, // 长跳中
    Dying = 7,       // 死亡动画
    Croaking = 8,    // 青蛙鸣叫
    UsingTongue = 9, // 舌头伸出（青蛙）
    Sitting = 10,    // 坐下
    Roaring = 11,    // 怒吼（监守者）
    Sniffing = 12,   // 嗅探（监守者）
    Emerging = 13,   // 钻出（嗅探兽）
    Digging = 14,    // 挖掘（嗅探兽）
    Sliding = 15,    // 滑行 - 旋风人专用
    Shooting = 16,   // 射击 - 旋风人专用
    Inhaling = 17,   // 吸气蓄力 - 旋风人专用
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
        case EntityPose::LongJumping:
            return "long_jumping";
        case EntityPose::Dying:
            return "dying";
        case EntityPose::Croaking:
            return "croaking";
        case EntityPose::UsingTongue:
            return "using_tongue";
        case EntityPose::Sitting:
            return "sitting";
        case EntityPose::Roaring:
            return "roaring";
        case EntityPose::Sniffing:
            return "sniffing";
        case EntityPose::Emerging:
            return "emerging";
        case EntityPose::Digging:
            return "digging";
        case EntityPose::Sliding:
            return "sliding";
        case EntityPose::Shooting:
            return "shooting";
        case EntityPose::Inhaling:
            return "inhaling";
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
    if (name == "long_jumping") return EntityPose::LongJumping;
    if (name == "dying") return EntityPose::Dying;
    if (name == "croaking") return EntityPose::Croaking;
    if (name == "using_tongue") return EntityPose::UsingTongue;
    if (name == "sitting") return EntityPose::Sitting;
    if (name == "roaring") return EntityPose::Roaring;
    if (name == "sniffing") return EntityPose::Sniffing;
    if (name == "emerging") return EntityPose::Emerging;
    if (name == "digging") return EntityPose::Digging;
    if (name == "sliding") return EntityPose::Sliding;
    if (name == "shooting") return EntityPose::Shooting;
    if (name == "inhaling") return EntityPose::Inhaling;
    return EntityPose::Standing;
}

} // namespace mc::entity
