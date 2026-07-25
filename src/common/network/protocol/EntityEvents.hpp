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
#include "common/util/assert/AssertAll.hpp"

namespace mc::network {

/// @brief 实体动画类型（对应 ir::play::Animate.action 字段值）
///
/// 1.21.11 Animate 的 action 值：0=SwingMainHand 1=TakeDamage 2=LeaveBed
/// 3=SwingOffHand 4=CriticalEffect 5=MagicCriticalEffect。TakeDamage 对应独立的
/// ClientboundHurtAnimationPacket（携带 hurtDir），其余对应 ClientboundAnimatePacket。
enum class EntityAnimation : u8 {
    SwingMainHand = 0,
    TakeDamage = 1,
    LeaveBed = 2,
    SwingOffHand = 3,
    CriticalEffect = 4,
    MagicCriticalEffect = 5
};

/// @brief 实体状态字节（对应 ir::play::EntityEvent.event 字段值）
///
/// 数值与 MC 原版 EntityStatus byte 对应。服务端经 EntityEvent 广播，客户端
/// ClientPlayVisitor 按 byte 分流到各实体的 handleEntityStatus。
enum class EntityStatus : u8 {
    // 兔子跳跃（MC 1.21.11 Rabbit.jumpFromGround() 中 broadcastEntityState(this, (byte)1)）
    // 客户端收到后启动 jumpDuration 计时器，用于 RabbitModel 计算 jumpRotation
    RabbitJump = 1,

    // 通用状态
    Hurt = 2,            // 受击反馈（红色闪烁）
    Death = 3,           // 死亡效果
    TamingFailed = 6,    // 驯服失败（烟雾粒子）
    TamingSucceeded = 7, // 驯服成功（爱心粒子）
    ShakeOffWater = 8,   // 抖落水分（狼）- 开始甩水动画

    // 狼甩水取消（MC 1.21.11 Wolf.tick() 中再次入水时广播 byte 56）
    // 客户端收到后立即取消甩水动画，重置 shakeAnim
    WolfStopShaking = 56,

    // 实体特定状态
    EatBlock = 10, // 吃草/方块动画（羊、 TNT 矿车引燃）

    // 村民状态
    VillagerHeart = 12,  // 村民爱心粒子（繁殖中/幼年村民出生）
    VillagerAngry = 13,  // 村民愤怒粒子（无床位/被玩家攻击）
    VillagerHappy = 14,  // 村民开心粒子（交易成功/获取职业/找到床位/找到集会点）
    VillagerSplash = 42, // 村民水花粒子（突袭中恐慌）

    // 动物状态
    LoveHeart = 18, // 繁殖爱心效果

    // 玩家状态
    // 玩家权限等级变更（status byte = 24 + permissionLevel，permissionLevel 范围 0-4）
    PermissionLevel0 = 24, // 普通玩家（非 OP）
    PermissionLevel1 = 25, // 版主（可绕过重生点保护）
    PermissionLevel2 = 26, // 游戏管理员（可使用命令方块等）
    PermissionLevel3 = 27, // 服务器管理员（可使用 /op、/deop 等）
    PermissionLevel4 = 28, // 服务器所有者（控制台级别权限）

    // 特殊状态
    FireworkExplosion = 17,
    GuardianAttack = 21,       // 守卫者攻击音效
    ArrowHit = 30,             // 箭矢命中音效
    TotemActivate = 35,        // 不死图腾激活
    Dolphin = 38,              // 海豚寻宝粒子
    OcelotTrustFailed = 40,    // 豹猫信任失败（烟雾粒子）
    OcelotTrustSucceeded = 41, // 豹猫信任成功（心形粒子）
    TeleportParticles = 46,    // 传送粒子效果

    // 铁傀儡状态
    IronGolemAttack = 4,    // 铁傀儡攻击动画（举臂）+ 播放攻击音效
    IronGolemHoldRose = 11, // 铁儡开始手持罂粟花
    IronGolemStopRose = 34, // 铁傀儡停止手持罂粟花

    // 疣猪兽/僵尸疣兽状态（MC 原版中铁傀儡和疣猪兽共用状态码 4，客户端按实体类型区分）
    HoglinAttack = 4, // 疣猪兽/僵尸疣兽攻击动画（甩头）+ 播放攻击音效

    // 装备破损状态
    EquipmentBreakMainHand = 47, // 主手装备破损动画 + 音效
    EquipmentBreakOffHand = 48,  // 副手装备破损动画 + 音效
    EquipmentBreakHead = 49,     // 头盔破损动画 + 音效
    EquipmentBreakChest = 50,    // 胸甲破损动画 + 音效
    EquipmentBreakLegs = 51,     // 护腿破损动画 + 音效
    EquipmentBreakFeet = 52,     // 靴子破损动画 + 音效

    // Mob 特定状态
    MobPoof = 60, // 生物变形/消失烟雾粒子
};

/// @brief 根据权限等级生成对应的状态字节
/// @param level 权限等级 (0-4)
/// @return 状态枚举值 (PermissionLevel0 ~ PermissionLevel4)
[[nodiscard]] inline EntityStatus permissionLevel(i32 level)
{
    MC_ASSERT_RELEASE(level >= 0 && level <= 4);
    return static_cast<EntityStatus>(24 + level);
}

/// @brief 从状态字节解析权限等级
/// @param status 状态字节
/// @return 权限等级 (0-4)，如果不是权限等级状态则返回 -1
[[nodiscard]] inline i32 toPermissionLevel(u8 status)
{
    if (status >= 24 && status <= 28) {
        return static_cast<i32>(status - 24);
    }
    return -1;
}

/// @brief 根据装备槽位索引获取对应的破损状态码
///
/// 对应 MC 原版 LivingEntity.entityEventForEquipmentBreak()
/// 槽位索引与 EquipmentSlot 枚举值对应：
/// 0=MainHand→47, 1=OffHand→48, 2=Feet→52, 3=Legs→51, 4=Chest→50, 5=Head→49
///
/// @param slotIndex 装备槽位索引（EquipmentSlot 枚举值）
/// @return 破损状态码
[[nodiscard]] inline EntityStatus equipmentBreakStatus(u8 slotIndex)
{
    // 槽位索引与 MC 原版 entityEventForEquipmentBreak 映射
    // EquipmentSlot: MainHand=0, OffHand=1, Feet=2, Legs=3, Chest=4, Head=5
    // EntityStatus:  47=MainHand, 48=OffHand, 49=Head, 50=Chest, 51=Legs, 52=Feet
    switch (slotIndex) {
        case 0:
            return EntityStatus::EquipmentBreakMainHand;
        case 1:
            return EntityStatus::EquipmentBreakOffHand;
        case 5:
            return EntityStatus::EquipmentBreakHead;
        case 4:
            return EntityStatus::EquipmentBreakChest;
        case 3:
            return EntityStatus::EquipmentBreakLegs;
        case 2:
            return EntityStatus::EquipmentBreakFeet;
        default:
            return EntityStatus::EquipmentBreakMainHand;
    }
}

} // namespace mc::network
