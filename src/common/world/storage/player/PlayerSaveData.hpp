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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/GlobalPos.hpp"
#include <optional>
#include <string>
#include <vector>

namespace mc {
namespace world::storage {

/**
 * @brief 玩家保存数据
 *
 * 用于持久化的玩家数据结构，包含所有需要保存的玩家状态。
 *
 * 包含的数据：
 * - 基本信息：用户名、UUID
 * - 位置和旋转：x, y, z, yaw, pitch
 * - 维度信息：当前维度、重生点
 * - 游戏模式：生存/创造/冒险/观察者
 * - 生命值和饥饿值
 * - 经验系统：等级、进度、总经验
 * - 玩家能力：飞行、无敌等
 * - 背包物品：快捷栏、主背包、护甲、副手
 * - 药水效果：当前生效的效果列表
 * - 睡眠状态
 */
struct PlayerSaveData {
    // ========== 基本信息 ==========

    /// 玩家唯一标识符（UUID字符串）
    std::string uuid;

    /// 用户名
    std::string username;

    // ========== 位置和旋转 ==========

    /// 世界坐标 X
    f64 posX = 0.0;

    /// 世界坐标 Y
    f64 posY = 64.0;

    /// 世界坐标 Z
    f64 posZ = 0.0;

    /// 偏航角（水平旋转）
    f32 yaw = 0.0f;

    /// 俯仰角（垂直旋转）
    f32 pitch = 0.0f;

    // ========== 维度信息 ==========

    /// 当前维度ID (0=主世界, -1=下界, 1=末地)
    DimensionId dimension = 0;

    /// 重生点位置（可选）
    std::optional<GlobalPos> spawnPoint;

    /// 是否强制重生点
    bool spawnForced = false;

    /// 进入下界时的位置（用于进度追踪）
    std::optional<Vector3d> enteredNetherPosition;

    /// 上次死亡位置（维度+方块坐标，用于追溯指南针和存档持久化）
    std::optional<GlobalPos> lastDeathLocation;

    // ========== 游戏模式 ==========

    /// 游戏模式
    GameMode gameMode = GameMode::Survival;

    /// 是否在飞行
    bool flying = false;

    // ========== 生命值和饥饿值 ==========

    /// 当前生命值
    f32 health = 20.0f;

    /// 最大生命值
    f32 maxHealth = 20.0f;

    /// 饥饿值 (0-20)
    i32 foodLevel = 20;

    /// 饱食度 (0-20)
    f32 saturationLevel = 5.0f;

    /// 消耗值
    f32 exhaustionLevel = 0.0f;

    /// 食物计时器
    i32 foodTickTimer = 0;

    // ========== 经验系统 ==========

    /// 经验等级
    i32 experienceLevel = 0;

    /// 经验进度 (0.0-1.0)
    f32 experienceProgress = 0.0f;

    /// 总经验值
    i32 totalExperience = 0;

    /// XP 种子（用于附魔随机）
    i32 xpSeed = 0;

    // ========== 玩家能力 ==========

    /// 无敌
    bool invulnerable = false;

    /// 可以飞行
    bool canFly = false;

    /// 飞行速度
    f32 flySpeed = 0.05f;

    /// 行走速度
    f32 walkSpeed = 0.1f;

    // ========== 背包物品 ==========

    /// 背包物品 (索引 0-40)
    /// 0-8: 快捷栏
    /// 9-35: 主背包
    /// 36-39: 护甲 (头盔、胸甲、护腿、靴子)
    /// 40: 副手
    /// 使用 optional<ItemStack> 表示空槽位
    std::vector<std::optional<ItemStack>> inventoryItems;

    /// 选中的快捷栏槽位 (0-8)
    i32 selectedSlot = 0;

    /// 鼠标持有的物品
    std::optional<ItemStack> carriedItem;

    // ========== 药水效果 ==========

    /// 当前生效的药水效果列表
    std::vector<entity::effect::EffectInstance> effects;

    // ========== 空气供应 ==========

    /// 空气供应量
    i32 airSupply = 300;

    /// 最大空气供应量
    i32 maxAirSupply = 300;

    // ========== 睡眠状态 ==========

    /// 是否在睡觉
    bool sleeping = false;

    /// 睡眠位置
    std::optional<BlockPos> sleepingPosition;

    /// 睡眠计时器
    i32 sleepTimer = 0;

    // ========== 其他状态 ==========

    /// 是否在地面上
    bool onGround = true;

    /// 疾跑状态
    bool sprinting = false;

    /// 潜行状态
    bool sneaking = false;

    // ========== 冲量上下文（坠落伤害减免） ==========

    /// 冲量冲击位置（砸地/爆炸位置），可选
    std::optional<Vector3> currentImpulseImpactPos;

    /// 是否忽略当前冲量的坠落伤害
    bool ignoreFallDamageFromCurrentImpulse = false;

    /// 冲量上下文重置宽限期（tick）
    i32 currentImpulseContextResetGraceTime = 0;

    // ========== 构造函数 ==========

    PlayerSaveData() = default;

    /**
     * @brief 构造玩家数据
     * @param playerUuid 玩家UUID
     * @param name 用户名
     */
    explicit PlayerSaveData(const std::string& playerUuid, const std::string& name)
        : uuid(playerUuid)
        , username(name)
    {}

    // ========== 序列化 ==========

    /**
     * @brief 序列化为 NBT
     * @return NBT 复合标签
     */
    [[nodiscard]] nbt::tags::compound_tag toNbt() const;

    /**
     * @brief 从 NBT 反序列化
     * @param tag NBT 复合标签
     * @return 成功或错误
     */
    static Result<PlayerSaveData> fromNbt(const nbt::tags::compound_tag& tag);

    /**
     * @brief 序列化为二进制数据
     * @return 压缩的二进制数据
     */
    [[nodiscard]] Result<std::vector<u8>> serialize() const;

    /**
     * @brief 从二进制数据反序列化
     * @param data 二进制数据
     * @return 玩家数据或错误
     */
    static Result<PlayerSaveData> deserialize(const std::vector<u8>& data);
};

} // namespace world::storage
} // namespace mc
