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

#include "Attribute.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 标准属性定义
 *
 * 定义游戏中的标准属性类型。
 * 这些属性可以通过修改器进行修改。
 *
 * 参考 MC 1.16.5 Attributes
 */
namespace Attributes {

/**
 * @brief 最大生命值
 *
 * 决定实体可以承受的伤害量。
 * 默认值: 20.0 (玩家)
 * 范围: 1.0 ~ 1024.0 (MC 1.16.5 最小值为 1.0，不能为 0)
 */
inline std::unique_ptr<Attribute> maxHealth()
{
    return std::make_unique<Attribute>("minecraft:max_health", 20.0, 1.0, 1024.0);
}

/**
 * @brief 跟随范围
 *
 * 决定实体追踪目标的距离。
 * 默认值: 32.0 (大多数生物)
 * 范围: 0.0 ~ 2048.0
 */
inline std::unique_ptr<Attribute> followRange()
{
    return std::make_unique<Attribute>("minecraft:follow_range", 32.0, 0.0, 2048.0);
}

/**
 * @brief 击退抗性
 *
 * 决定实体被击退的概率。
 * 默认值: 0.0
 * 范围: 0.0 ~ 1.0
 * 1.0 表示完全免疫击退
 */
inline std::unique_ptr<Attribute> knockbackResistance()
{
    return std::make_unique<Attribute>("minecraft:knockback_resistance", 0.0, 0.0, 1.0);
}

/**
 * @brief 移动速度
 *
 * 决定实体的移动速度。
 * 默认值: 0.7 (生物), 0.1 (玩家)
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> movementSpeed()
{
    return std::make_unique<Attribute>("minecraft:movement_speed", 0.7, 0.0, 1024.0);
}

/**
 * @brief 飞行速度
 *
 * 决定实体的飞行速度。
 * 默认值: 0.4 (飞行玩家)
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> flyingSpeed()
{
    return std::make_unique<Attribute>("minecraft:flying_speed", 0.4, 0.0, 1024.0);
}

/**
 * @brief 攻击伤害
 *
 * 决定实体的近战攻击伤害。
 * 默认值: 2.0 (玩家)
 * 范围: 0.0 ~ 2048.0
 */
inline std::unique_ptr<Attribute> attackDamage()
{
    return std::make_unique<Attribute>("minecraft:attack_damage", 2.0, 0.0, 2048.0);
}

/**
 * @brief 攻击击退
 *
 * 决定实体攻击时的击退力度。
 * 默认值: 0.0
 * 范围: 0.0 ~ 5.0
 */
inline std::unique_ptr<Attribute> attackKnockback()
{
    return std::make_unique<Attribute>("minecraft:attack_knockback", 0.0, 0.0, 5.0);
}

/**
 * @brief 攻击速度
 *
 * 决定实体的攻击速度（每秒攻击次数）。
 * 默认值: 4.0 (玩家)
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> attackSpeed()
{
    return std::make_unique<Attribute>("minecraft:attack_speed", 4.0, 0.0, 1024.0);
}

/**
 * @brief 护甲值
 *
 * 决定实体的护甲防御值。
 * 默认值: 0.0
 * 范围: 0.0 ~ 30.0
 */
inline std::unique_ptr<Attribute> armor()
{
    return std::make_unique<Attribute>("minecraft:armor", 0.0, 0.0, 30.0);
}

/**
 * @brief 护甲韧性
 *
 * 决定护甲减少伤害的效果。
 * 默认值: 0.0
 * 范围: 0.0 ~ 20.0
 */
inline std::unique_ptr<Attribute> armorToughness()
{
    return std::make_unique<Attribute>("minecraft:armor_toughness", 0.0, 0.0, 20.0);
}

/**
 * @brief 幸运
 *
 * 影响各种随机事件的结果。
 * 默认值: 0.0
 * 范围: -1024.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> luck()
{
    return std::make_unique<Attribute>("minecraft:luck", 0.0, -1024.0, 1024.0);
}

/**
 * @brief 最大吸收值
 *
 * 决定实体可以拥有的吸收生命值上限。
 * 默认值: 0.0
 * 范围: 0.0 ~ 2048.0
 */
inline std::unique_ptr<Attribute> maxAbsorption()
{
    // 1.21 起原版注册名是 `minecraft:max_absorption`；沿用 `generic.` 会让按名查找与
    // 数据组件、命令里的 `minecraft:max_absorption` 全部落空。
    return std::make_unique<Attribute>("minecraft:max_absorption", 0.0, 0.0, 2048.0);
}

/**
 * @brief 水下呼吸时间
 *
 * 决定实体可以在水下呼吸的时间（ticks）。
 * 默认值: 300 (15秒)
 * 范围: 0 ~ 6000
 */
inline std::unique_ptr<Attribute> breathMax()
{
    // 注意：使用整数属性，这里用 f64 表示
    return std::make_unique<Attribute>("generic.breath_max", 300.0, 0.0, 6000.0);
}

/**
 * @brief 跳跃力
 *
 * 决定实体起跳时的垂直初速度，由 getJumpPower() 消费。
 * 默认值: 0.42f（原版是 RangedAttribute(..., 0.42F, 0.0, 32.0)，0.42 先过 f32 再拓宽成
 *   double；直接写 0.42 会被当成 double 字面量，末位与原版差 1ulp）
 * 范围: 0.0 ~ 32.0
 */
inline std::unique_ptr<Attribute> jumpStrength()
{
    return std::make_unique<Attribute>("minecraft:jump_strength", static_cast<f64>(0.42f), 0.0, 32.0);
}

/**
 * @brief 马匹跳跃强度
 *
 * 决定马类实体的跳跃能力。
 * 默认值: 0.7
 * 范围: 0.0 ~ 2.0
 *
 * TODO: vanilla 1.21.11 马族统一用 generic.jump_strength（JUMP_STRENGTH）承载跳跃力
 *（createBaseHorseAttributes 基值 0.7 + 随机化 + 遗传），不使用独立的 horse.jump_strength。
 * 项目当前马匹用 HorseJumpComponent 组件作真相源、本属性作镜像，且本属性零 getValue 消费。
 * 待马匹体系重构统一到 JUMP_STRENGTH 后，本属性应移除。
 */
inline std::unique_ptr<Attribute> horseJumpStrength()
{
    return std::make_unique<Attribute>("horse.jump_strength", 0.7, 0.0, 2.0);
}

/**
 * @brief 僵尸增援概率
 *
 * 决定僵尸受伤时召唤增援的概率。
 * 默认值: 0.0
 * 范围: 0.0 ~ 1.0
 */
inline std::unique_ptr<Attribute> zombieSpawnReinforcements()
{
    return std::make_unique<Attribute>("minecraft:spawn_reinforcements", 0.0, 0.0, 1.0);
}

// ============================================================================
// Forge 扩展属性
// 注意：以下属性不是 MC 1.16.5 原版属性，而是 Forge 模组加载器扩展的属性
// ============================================================================

/**
 * @brief 实体重力
 *
 * 决定实体受到的重力加速度（blocks/tick²）。
 * 1.21 起原版自带该属性（Attributes.java:45-47，
 * RangedAttribute("attribute.name.gravity", 0.08, -1.0, 1.0)），注册名 `minecraft:gravity`；
 * 旧实现沿用 Forge 的 `forge.entity_gravity` 并把范围写成 ±8，与 Java 客户端下发的
 * 属性 id 对不上，按名查找 `minecraft:gravity` 也会落空。
 * 默认值: 0.08，范围: -1.0 ~ 1.0
 */
inline std::unique_ptr<Attribute> entityGravity()
{
    return std::make_unique<Attribute>("minecraft:gravity", 0.08, -1.0, 1.0);
}

/**
 * @brief 移动效率
 *
 * 决定实体在减速方块（灵魂沙、蜂蜜块等）上的移动效率。
 * generic.movement_efficiency 属性。
 * 默认值: 0.0（不抵消减速效果）
 * 范围: 0.0 ~ 1.0
 *
 * 在 LivingEntity.getBlockSpeedFactor() 中的使用方式：
 *   finalSpeedFactor = lerp(movementEfficiency, blockSpeedFactor, 1.0)
 * 当 movementEfficiency=0.0 时，使用方块原始 speedFactor（灵魂沙=0.4，正常=1.0）
 * 当 movementEfficiency=1.0 时，完全忽略方块减速效果（speedFactor 插值到 1.0）
 *
 * 灵魂疾行附魔通过 Addition 操作为此属性添加 +1.0 修饰符，
 * 从而完全抵消灵魂沙/灵魂土的减速效果。
 */
inline std::unique_ptr<Attribute> movementEfficiency()
{
    return std::make_unique<Attribute>("minecraft:movement_efficiency", 0.0, 0.0, 1.0);
}

/**
 * @brief 安全摔落距离
 *
 * 决定实体摔落多少格才开始受到摔落伤害。
 * 摔落伤害 = floor((摔落距离 + 1e-6 - 安全摔落距离) * 伤害倍率 * 摔落伤害倍率)。
 * 默认值: 3.0（多数生物），Fox=5.0。
 * 范围: -1024.0 ~ 1024.0
 * JumpBoost 药水通过 Addition 修饰符每级 +1（effect.minecraft.jump_boost）。
 */
inline std::unique_ptr<Attribute> safeFallDistance()
{
    return std::make_unique<Attribute>("minecraft:safe_fall_distance", 3.0, -1024.0, 1024.0);
}

/**
 * @brief 摔落伤害倍率
 *
 * 作为摔落伤害公式的最终乘数，与方块传入的 damageMultiplier 相乘。
 * 默认值: 1.0（多数生物），马类=0.5（AbstractHorse）。
 * 范围: 0.0 ~ 100.0
 * 注意：干草块/蜂蜜块的 0.2 减伤是通过 causeFallDamage 的 damageMultiplier 参数传入，
 *   不修改此属性。
 */
inline std::unique_ptr<Attribute> fallDamageMultiplier()
{
    return std::make_unique<Attribute>("minecraft:fall_damage_multiplier", 1.0, 0.0, 100.0);
}

/**
 * @brief 额外氧气
 *
 * 控制生物在水下氧气值的消耗概率，对齐 vanilla 1.21.11 的 oxygen_bonus 属性
 *（Attributes.java:70-72，RangedAttribute("attribute.name.oxygen_bonus", 0.0, 0.0, 1024.0)）。
 *
 * 消费点：LivingEntity::decreaseAirSupply（对齐 vanilla LivingEntity.java:571-582）。
 * 设属性计算值为 e：
 *   - e == 0：每 tick 必定消耗 1 点氧气（默认行为）。
 *   - e > 0：仅有 1/(e+1) 的概率消耗，即平均延长 (e+1) 倍的水下停留时间。
 * 水下呼吸魔咒通过 enchantment.respiration 修饰符（Op0 ADD_VALUE，每级 +1.0，HEAD 槽位）
 * 加到本属性，使每级水下呼吸降低氧气消耗概率（I级 50%、II级 66.7%、III级 75% 不消耗）。
 *
 * 默认值: 0.0（所有生物，无水下呼吸时不延长）
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> oxygenBonus()
{
    return std::make_unique<Attribute>("minecraft:oxygen_bonus", 0.0, 0.0, 1024.0);
}

/**
 * @brief 燃烧时间倍率
 *
 * 作为实体被点燃时燃烧持续 tick 数的乘数。默认值 1.0（满额燃烧时间）。
 * 属性 generic.burning_time（范围 [0.0, 1024.0]，默认 1.0）。
 *
 * 消费点：LivingEntity::igniteForTicks override：Mth.ceil(ticks * getAttributeValue(BURNING_TIME))。
 * 实体被点燃时，传入 tick 数乘以本属性值后向上取整，再委托基类设置火焰计时器。
 * 默认 1.0 时燃烧时间不变；火焰保护魔咒通过修饰符缩减此值从而缩短燃烧时间。
 *
 * 火焰保护魔咒通过 enchantment.fire_protection 修饰符（Op1 MULTIPLY_BASE，每级 -0.15，
 * 4 个盔甲槽位）加到本属性：1 级 → 0.85、4 级 → 0.4（单件）。
 *
 * 默认值: 1.0（所有生物，满额燃烧时间）
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> burningTime()
{
    return std::make_unique<Attribute>("minecraft:burning_time", 1.0, 0.0, 1024.0);
}

/**
 * @brief 爆炸击退抗性
 *
 * 决定实体被爆炸推开的力度衰减比例。属性 generic.explosion_knockback_resistance
 *（范围 [0.0, 1.0]，默认 0.0）。
 *
 * 消费点：爆炸击退计算（Explosion.cpp / WindChargeEntity.cpp / Player.cpp 风爆）：
 *   finalKnockback = baseKnockback * (1.0 - getAttributeValue(EXPLOSION_KNOCKBACK_RESISTANCE))。
 * 即抗性 0.0 时满击退，1.0 时完全免疫爆炸击退。
 *
 * 爆炸保护魔咒通过 enchantment.blast_protection 修饰符（Op0 ADD_VALUE，每级 +0.15，
 * 4 个盔甲槽位）注入：单件 IV 级 → 0.6，击退缩减为 40%。
 *
 * 默认值: 0.0（所有生物，满额爆炸击退）
 * 范围: 0.0 ~ 1.0
 */
inline std::unique_ptr<Attribute> explosionKnockbackResistance()
{
    return std::make_unique<Attribute>("minecraft:explosion_knockback_resistance", 0.0, 0.0, 1.0);
}

/**
 * @brief 横扫伤害比例
 *
 * 对齐 vanilla 1.21.11 的 sweeping_damage_ratio 属性
 *（Attributes.java:91-92，RangedAttribute("attribute.name.sweeping_damage_ratio", 0.0, 0.0, 1.0)）。
 *
 * 消费点：玩家横扫攻击伤害（Player::doSweepAttack，对齐 vanilla Player.doSweepAttack）：
 *   sweepBase = 1.0 + SWEEPING_DAMAGE_RATIO 属性值 × 冷却后的基础伤害。
 * 横扫之刃附魔通过 enchantment.sweeping_edge 修饰符（Op0 ADD_VALUE，amount = n/(n+1)）注入。
 *
 * 默认值: 0.0（无附魔时横扫只传递固定 1.0 基础伤害项）
 * 范围: 0.0 ~ 1.0
 */
inline std::unique_ptr<Attribute> sweepingDamageRatio()
{
    return std::make_unique<Attribute>("minecraft:sweeping_damage_ratio", 0.0, 0.0, 1.0);
}

/**
 * @brief 挖掘效率加成
 *
 * 对齐 vanilla 1.21.11 的 mining_efficiency 属性
 *（Attributes.java:61-62，RangedAttribute("attribute.name.mining_efficiency", 0.0, 0.0, 1024.0)）。
 *
 * 消费点：挖掘速度——工具对目标方块有效时叠加到基础速度上（对齐 vanilla
 * Player.getDestroySpeed 的 `if (f > 1.0F) f += getAttributeValue(MINING_EFFICIENCY)`）。
 * 效率附魔通过 enchantment.efficiency 修饰符（Op0 ADD_VALUE，level² + 1）注入。
 *
 * 默认值: 0.0
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> miningEfficiency()
{
    return std::make_unique<Attribute>("minecraft:mining_efficiency", 0.0, 0.0, 1024.0);
}

/**
 * @brief 水下挖掘速度乘数
 *
 * 对齐 vanilla 1.21.11 的 submerged_mining_speed 属性
 *（Attributes.java:88-89，RangedAttribute("attribute.name.submerged_mining_speed", 0.2, 0.0, 20.0)）。
 *
 * 消费点：眼睛在水中时挖掘速度乘以本属性值。默认 0.2 即水下只有陆地的 1/5；
 * 水下速掘附魔通过 enchantment.aqua_affinity 修饰符（Op2 MULTIPLY_TOTAL +4.0）把它抬到 1.0。
 *
 * 默认值: 0.2
 * 范围: 0.0 ~ 20.0
 */
inline std::unique_ptr<Attribute> submergedMiningSpeed()
{
    return std::make_unique<Attribute>("minecraft:submerged_mining_speed", 0.2, 0.0, 20.0);
}

/**
 * @brief 水中移动效率
 *
 * 对齐 vanilla 1.21.11 的 water_movement_efficiency 属性
 *（Attributes.java:95-96，RangedAttribute("attribute.name.water_movement_efficiency", 0.0, 0.0, 1.0)）。
 *
 * 消费点：水中移动的阻力与输入速度插值（LivingEntity::travelInWater / Player::_handleWaterMovement）。
 * 深海探索者附魔通过 enchantment.depth_strider 修饰符（Op0 ADD_VALUE，每级 0.33333334）注入。
 *
 * 默认值: 0.0
 * 范围: 0.0 ~ 1.0
 */
inline std::unique_ptr<Attribute> waterMovementEfficiency()
{
    return std::make_unique<Attribute>("minecraft:water_movement_efficiency", 0.0, 0.0, 1.0);
}

/**
 * @brief 潜行速度
 *
 * 对齐 vanilla 1.21.11 的 sneaking_speed 属性
 *（Attributes.java:79-80，RangedAttribute("attribute.name.sneaking_speed", 0.3, 0.0, 1.0)）。
 *
 * 消费点：玩家潜行时的移动速度乘数（Player::_applyMovementSpeed / travel 的潜行分支）。
 * 迅捷潜行附魔通过 enchantment.swift_sneak 修饰符（Op0 ADD_VALUE，每级 +0.15）注入。
 *
 * 默认值: 0.3
 * 范围: 0.0 ~ 1.0
 */
inline std::unique_ptr<Attribute> sneakingSpeed()
{
    return std::make_unique<Attribute>("minecraft:sneaking_speed", 0.3, 0.0, 1.0);
}

/**
 * @brief 方块破坏速度倍率
 *
 * 对齐 vanilla 1.21.11 的 block_break_speed 属性
 *（Attributes.java:19-21，RangedAttribute("attribute.name.block_break_speed", 1.0, 0.0, 1024.0)）。
 *
 * 消费点：挖掘速度——最终速度乘以本属性值（对齐 vanilla Player.getDestroySpeed 的
 * `f *= getAttributeValue(BLOCK_BREAK_SPEED)`）。默认 1.0 不改变速度；目前原版没有附魔或
 * 效果修饰它，属于「给数据包/命令预留」的属性。
 *
 * TODO: 接通挖掘速度的消费点（Player::getDigSpeed / MiningManager::_calculateDigSpeedMultiplier），
 *       当前只注册了定义，还没有任何代码读取它。
 *
 * 默认值: 1.0
 * 范围: 0.0 ~ 1024.0
 */
inline std::unique_ptr<Attribute> blockBreakSpeed()
{
    return std::make_unique<Attribute>("minecraft:block_break_speed", 1.0, 0.0, 1024.0);
}

/**
 * @brief 台阶高度
 *
 * 对齐 vanilla 1.21.11 的 step_height 属性
 *（Attributes.java:85-87，RangedAttribute("attribute.name.step_height", 0.6, 0.0, 10.0)）。
 *
 * 决定实体不跳跃就能跨过的最大高度。马类与部分生物会把它调高。
 *
 * TODO: 接通消费点——移动与寻路里的台阶判定目前仍用固定 0.6，没有读这个属性。
 *
 * 默认值: 0.6
 * 范围: 0.0 ~ 10.0
 */
inline std::unique_ptr<Attribute> stepHeight()
{
    return std::make_unique<Attribute>("minecraft:step_height", 0.6, 0.0, 10.0);
}

/**
 * @brief 实体缩放
 *
 * 对齐 vanilla 1.21.11 的 scale 属性
 *（Attributes.java:76-78，RangedAttribute("attribute.name.scale", 1.0, 0.0625, 16.0)）。
 *
 * 决定碰撞箱、渲染与视高的整体缩放比例（幼年生物、缩小/放大效果）。
 *
 * TODO: 接通消费点——实体尺寸目前由 EntitySize 直接给出，没有乘以本属性。
 *
 * 默认值: 1.0
 * 范围: 0.0625 ~ 16.0
 */
inline std::unique_ptr<Attribute> scale()
{
    return std::make_unique<Attribute>("minecraft:scale", 1.0, 0.0625, 16.0);
}

/**
 * @brief 视角距离
 *
 * 对齐 vanilla 1.21.11 的 camera_distance 属性
 *（Attributes.java:28-30，RangedAttribute("attribute.name.camera_distance", 4.0, 0.0, 32.0)）。
 *
 * 决定第三人称视角相机与实体的距离（狼坐下、监听者等会调整）。
 *
 * TODO: 接通消费点——相机距离目前是客户端写死的常量，没有读这个属性。
 *
 * 默认值: 4.0
 * 范围: 0.0 ~ 32.0
 */
inline std::unique_ptr<Attribute> cameraDistance()
{
    return std::make_unique<Attribute>("minecraft:camera_distance", 4.0, 0.0, 32.0);
}

/**
 * @brief 吸引范围
 *
 * 对齐 vanilla 1.21.11 的 tempt_range 属性
 *（Attributes.java:94，RangedAttribute("attribute.name.tempt_range", 10.0, 0.0, 2048.0)）。
 *
 * 决定手持诱饵物品时生物开始跟随的最大距离（猪/牛/羊/鸡等）。
 *
 * TODO: 接通消费点——TemptGoal 目前用的是写死的 10.0。
 *
 * 默认值: 10.0
 * 范围: 0.0 ~ 2048.0
 */
inline std::unique_ptr<Attribute> temptRange()
{
    return std::make_unique<Attribute>("minecraft:tempt_range", 10.0, 0.0, 2048.0);
}

/**
 * @brief 方块交互距离
 *
 * 决定玩家与方块交互（破坏/使用/放置）的最大距离。
 * 默认值: 4.5（生存/冒险模式）
 * 范围: 0.0 ~ 64.0
 * 创造模式由 ServerPlayer 添加 +0.5 的 Addition 修饰符，达到 5.0。
 */
inline std::unique_ptr<Attribute> blockInteractionRange()
{
    return std::make_unique<Attribute>("minecraft:block_interaction_range", 4.5, 0.0, 64.0);
}

/**
 * @brief 实体交互距离
 *
 * 决定玩家与实体交互（攻击/右键/捡选）的最大距离。
 * 默认值: 3.0（生存/冒险模式）
 * 范围: 0.0 ~ 64.0
 * 创造模式由 ServerPlayer 添加 +2.0 的 Addition 修饰符，达到 5.0。
 */
inline std::unique_ptr<Attribute> entityInteractionRange()
{
    return std::make_unique<Attribute>("minecraft:entity_interaction_range", 3.0, 0.0, 64.0);
}

/** @brief 导航点发送范围，所有生物默认不发送。 */
inline std::unique_ptr<Attribute> waypointTransmitRange()
{
    return std::make_unique<Attribute>("minecraft:waypoint_transmit_range", 0.0, 0.0, 60000000.0);
}

/** @brief 导航点接收范围。 */
inline std::unique_ptr<Attribute> waypointReceiveRange()
{
    return std::make_unique<Attribute>("minecraft:waypoint_receive_range", 0.0, 0.0, 60000000.0);
}

// ============================================================================
// 属性名称常量
// ============================================================================

// 原版属性名一律使用 1.21.11 的注册名（`minecraft:<path>`）：`ItemAttributeModifiers` 等
// 数据组件按这个名字与 Java 侧的 `BuiltInRegistries.ATTRIBUTE` 对应，沿用 1.16.5 的
// `generic.` 前缀会让所有按名查找失效（同一件装备既查不到属性，也写不进组件）。
constexpr const char* MAX_HEALTH = "minecraft:max_health";
constexpr const char* FOLLOW_RANGE = "minecraft:follow_range";
constexpr const char* KNOCKBACK_RESISTANCE = "minecraft:knockback_resistance";
constexpr const char* MOVEMENT_SPEED = "minecraft:movement_speed";
constexpr const char* FLYING_SPEED = "minecraft:flying_speed";
constexpr const char* ATTACK_DAMAGE = "minecraft:attack_damage";
constexpr const char* ATTACK_KNOCKBACK = "minecraft:attack_knockback";
constexpr const char* ATTACK_SPEED = "minecraft:attack_speed";
constexpr const char* ARMOR = "minecraft:armor";
constexpr const char* ARMOR_TOUGHNESS = "minecraft:armor_toughness";
constexpr const char* LUCK = "minecraft:luck";
constexpr const char* ZOMBIE_SPAWN_REINFORCEMENTS = "minecraft:spawn_reinforcements";
constexpr const char* HORSE_JUMP_STRENGTH = "horse.jump_strength";

// MC 1.21+ 新增属性
constexpr const char* WAYPOINT_TRANSMIT_RANGE = "minecraft:waypoint_transmit_range";
constexpr const char* WAYPOINT_RECEIVE_RANGE = "minecraft:waypoint_receive_range";
constexpr const char* MOVEMENT_EFFICIENCY = "minecraft:movement_efficiency";
constexpr const char* BLOCK_INTERACTION_RANGE = "minecraft:block_interaction_range";
constexpr const char* ENTITY_INTERACTION_RANGE = "minecraft:entity_interaction_range";
constexpr const char* SAFE_FALL_DISTANCE = "minecraft:safe_fall_distance";
constexpr const char* FALL_DAMAGE_MULTIPLIER = "minecraft:fall_damage_multiplier";
constexpr const char* JUMP_STRENGTH = "minecraft:jump_strength";
constexpr const char* MINING_EFFICIENCY = "minecraft:mining_efficiency";
constexpr const char* SUBMERGED_MINING_SPEED = "minecraft:submerged_mining_speed";
constexpr const char* WATER_MOVEMENT_EFFICIENCY = "minecraft:water_movement_efficiency";
constexpr const char* SNEAKING_SPEED = "minecraft:sneaking_speed";
constexpr const char* SWEEPING_DAMAGE_RATIO = "minecraft:sweeping_damage_ratio";

// 非原版属性（项目自定义或Forge扩展）
constexpr const char* MAX_ABSORPTION = "minecraft:max_absorption";
constexpr const char* BREATH_MAX = "generic.breath_max"; // TODO: 基岩版属性（控制最大氧气值），Java
                                                         // 主线无此属性；当前零消费，待基岩兼容层接通或确认移除
constexpr const char* OXYGEN_BONUS = "minecraft:oxygen_bonus";
constexpr const char* BURNING_TIME = "minecraft:burning_time";
constexpr const char* EXPLOSION_KNOCKBACK_RESISTANCE = "minecraft:explosion_knockback_resistance";
constexpr const char* ENTITY_GRAVITY = "minecraft:gravity";
constexpr const char* BLOCK_BREAK_SPEED = "minecraft:block_break_speed";
constexpr const char* STEP_HEIGHT = "minecraft:step_height";
constexpr const char* SCALE = "minecraft:scale";
constexpr const char* TEMPT_RANGE = "minecraft:tempt_range";
constexpr const char* CAMERA_DISTANCE = "minecraft:camera_distance";

} // namespace Attributes

} // namespace attribute
} // namespace entity
} // namespace mc
